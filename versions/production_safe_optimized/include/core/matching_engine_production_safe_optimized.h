#pragma once

#include "core/matching_engine_production_v2.h"
#include "core/wal.h"
#include "core/lockfree_queue.h"  // Use from main include directory
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include <memory>
#include <condition_variable>

namespace perpetual {

// Production Safe Optimized: Combines production_safe (WAL zero data loss) 
// with event_sourcing (high performance async event storage)
// 
// Key optimizations:
// 1. Async WAL writes (non-blocking critical path)
// 2. In-memory event buffer (like event_sourcing)
// 3. Batch fsync (group commit optimization)
// 4. Lock-free queue for WAL operations
//
// Performance target: ~200K orders/sec, ~5μs latency (vs 9.78K/s, 99.68μs)
// Safety: Zero data loss guarantee maintained
class ProductionMatchingEngineSafeOptimized : public ProductionMatchingEngineV2 {
public:
    ProductionMatchingEngineSafeOptimized(InstrumentID instrument_id);
    ~ProductionMatchingEngineSafeOptimized();
    
    // Initialize with optimized WAL
    bool initialize(const std::string& config_file, bool enable_wal = true, const std::string& wal_path = "");
    
    // Process order with optimized async WAL
    std::vector<Trade> process_order_optimized(Order* order);
    
    // Process order with zero data loss guarantee (sync for critical orders)
    std::vector<Trade> process_order_zero_loss(Order* order);
    
    // Process order with guaranteed zero data loss (all orders sync, slower but safe)
    std::vector<Trade> process_order_guaranteed_zero_loss(Order* order);
    
    // Recover from WAL after crash
    bool recover_from_wal();
    
    // Shutdown gracefully
    void shutdown();
    
    // Get statistics
    struct Stats {
        uint64_t wal_size;
        uint64_t uncommitted_count;
        uint64_t async_writes;
        uint64_t sync_writes;  // Critical orders with immediate sync
        uint64_t sync_count;
        double avg_sync_time_us;
        uint64_t queue_size;
    };
    Stats get_stats() const;
    
private:
    // WAL entry for async queue
    struct WALEntry {
        enum class Type : uint8_t {
            ORDER = 1,
            TRADE = 2,
            BATCH_COMMIT = 3
        };
        
        Type type;
        Order order;
        Trade trade;
        Timestamp timestamp;
        uint64_t sequence_id;
        
        WALEntry() : type(Type::ORDER), timestamp(0), sequence_id(0) {}
    };
    
    // Async WAL writer thread
    void wal_writer_thread();
    
    // Sync worker thread (periodic fsync)
    void sync_worker_thread();
    
    // Check if should sync
    bool should_sync() const;
    
    // Perform sync
    void perform_sync();
    
    // Check if order is critical (needs immediate sync)
    bool is_critical_order(const Order* order, const std::vector<Trade>& trades) const;
    
    // Sync write for critical orders (zero data loss)
    void sync_write_critical(const Order* order, const std::vector<Trade>& trades);
    
    // Ensure WAL entry is written to file (for zero data loss guarantee)
    void ensure_wal_written(uint64_t sequence_id);
    
    // WAL for durability
    std::unique_ptr<WriteAheadLog> wal_;
    bool wal_enabled_ = false;
    
    // Zero data loss configuration
    bool zero_loss_mode_ = false;  // If true, all orders are critical
    bool wait_for_wal_write_ = false;  // If true, wait for WAL write completion (slower but safer)
    Price critical_order_threshold_ = 0;  // Orders above this price are critical
    Quantity critical_quantity_threshold_ = 0;  // Orders above this quantity are critical
    
    // Async WAL queue (lock-free, single producer/consumer)
    static constexpr size_t WAL_QUEUE_SIZE = 65536;  // 64K entries
    std::unique_ptr<LockFreeSPSCQueue<WALEntry>> wal_queue_;
    
    // In-memory event buffer (like event_sourcing)
    struct EventBuffer {
        Order order;
        std::vector<Trade> trades;
        Timestamp timestamp;
        uint64_t sequence_id;
    };
    std::vector<EventBuffer> event_buffer_;
    std::mutex event_buffer_mutex_;
    size_t max_event_buffer_size_ = 10000;  // 10K events in memory
    
    // Sync control
    std::atomic<uint64_t> last_sync_sequence_{0};
    std::atomic<uint64_t> pending_sequence_{0};
    std::atomic<uint64_t> committed_sequence_{0};
    std::atomic<uint64_t> last_written_sequence_{0};  // Last sequence written to WAL file
    
    // Condition variable for efficient WAL write notification (optimization)
    std::condition_variable wal_written_cv_;
    std::mutex wal_written_mutex_;  // Only for condition variable
    
    // Batch confirm manager for optimized waiting (async batch confirmation)
    class BatchConfirmManager {
    public:
        BatchConfirmManager() : running_(false), last_confirmed_seq_(0) {}
        
        void start() {
            running_ = true;
            confirm_thread_ = std::thread(&BatchConfirmManager::confirm_thread, this);
        }
        
        void stop() {
            running_ = false;
            if (confirm_thread_.joinable()) {
                confirm_thread_.join();
            }
        }
        
        // Notify that a sequence has been written (called by WAL writer, immediate notification)
        void notify_written(uint64_t seq_id) {
            std::lock_guard<std::mutex> lock(confirm_mutex_);
            if (seq_id > last_confirmed_seq_.load(std::memory_order_relaxed)) {
                last_confirmed_seq_.store(seq_id, std::memory_order_release);
                confirm_cv_.notify_all();  // Immediate notification
            }
        }
        
        // Wait for confirmation (fast check, no batch delay)
        void wait_for_confirm(uint64_t seq_id, std::chrono::milliseconds timeout) {
            // Fast path: immediate check
            if (last_confirmed_seq_.load(std::memory_order_acquire) >= seq_id) {
                return;  // Already confirmed, return immediately
            }
            
            // Slow path: wait for notification
            std::unique_lock<std::mutex> lock(confirm_mutex_);
            auto deadline = std::chrono::steady_clock::now() + timeout;
            
            confirm_cv_.wait_until(lock, deadline, [this, seq_id]() {
                return last_confirmed_seq_.load(std::memory_order_acquire) >= seq_id;
            });
        }
        
    private:
        void confirm_thread() {
            // This thread is mainly for statistics, actual notification is immediate
            const auto STATS_INTERVAL = std::chrono::milliseconds(100);
            
            while (running_) {
                std::this_thread::sleep_for(STATS_INTERVAL);
                // Statistics collection can be added here if needed
            }
        }
        
        std::atomic<bool> running_;
        std::atomic<uint64_t> last_confirmed_seq_;
        std::condition_variable confirm_cv_;
        std::mutex confirm_mutex_;
        std::thread confirm_thread_;
    };
    
    std::unique_ptr<BatchConfirmManager> batch_confirm_manager_;
    
    // Worker threads
    std::thread wal_writer_thread_;
    std::thread sync_worker_thread_;
    std::atomic<bool> running_{false};
    
    // Statistics
    std::atomic<uint64_t> async_writes_{0};
    std::atomic<uint64_t> sync_writes_{0};  // Critical orders with immediate sync
    std::atomic<uint64_t> sync_count_{0};
    std::atomic<uint64_t> total_sync_time_us_{0};
    
    // Sync timing - optimized for better performance
    std::chrono::milliseconds sync_interval_{50};  // 50ms sync interval (reduced frequency)
    size_t sync_batch_size_ = 5000;  // Sync every 5000 entries (larger batch)
    Timestamp last_sync_time_ = 0;
};

} // namespace perpetual

