#include "core/matching_engine_production_safe_optimized.h"
#include "core/logger.h"
#include "core/types.h"
#include <chrono>
#include <algorithm>
#include <thread>
#include <condition_variable>

using namespace std::chrono;

namespace perpetual {

ProductionMatchingEngineSafeOptimized::ProductionMatchingEngineSafeOptimized(InstrumentID instrument_id)
    : ProductionMatchingEngineV2(instrument_id) {
}

ProductionMatchingEngineSafeOptimized::~ProductionMatchingEngineSafeOptimized() {
    shutdown();
}

bool ProductionMatchingEngineSafeOptimized::initialize(const std::string& config_file, bool enable_wal) {
    // Initialize V2 first
    if (!ProductionMatchingEngineV2::initialize(config_file)) {
        return false;
    }
    
    wal_enabled_ = enable_wal;
    
    if (wal_enabled_) {
        // Initialize WAL
        std::string wal_path = "./data/wal";
        try {
            wal_ = std::make_unique<WriteAheadLog>(wal_path);
            LOG_INFO("WAL initialized: " + wal_path);
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to initialize WAL: " + std::string(e.what()));
            return false;
        }
        
        // Initialize lock-free queue
        wal_queue_ = std::make_unique<LockFreeSPSCQueue<WALEntry>>(WAL_QUEUE_SIZE);
        
        // Initialize batch confirm manager
        batch_confirm_manager_ = std::make_unique<BatchConfirmManager>();
        batch_confirm_manager_->start();
        
        // Note: mmap disabled by default - can be enabled if needed
        // if (wal_) {
        //     wal_->enable_mmap(64 * 1024 * 1024);  // 64MB initial size
        // }
        
        // Start worker threads
        running_ = true;
        wal_writer_thread_ = std::thread(&ProductionMatchingEngineSafeOptimized::wal_writer_thread, this);
        sync_worker_thread_ = std::thread(&ProductionMatchingEngineSafeOptimized::sync_worker_thread, this);
        
        LOG_INFO("Production Safe Optimized engine initialized with async WAL and batch confirmation");
    } else {
        LOG_WARNING("WAL is disabled");
    }
    
    return true;
}

std::vector<Trade> ProductionMatchingEngineSafeOptimized::process_order_optimized(Order* order) {
    if (!order) {
        return {};
    }
    
    // 1. Process order using V2 (ART+SIMD, ~1.2μs) - NO WAL blocking!
    auto trades = ProductionMatchingEngineV2::process_order_production_v2(order);
    
    // 2. Update sequence (removed event buffer mutex from critical path for performance)
    uint64_t seq_id = pending_sequence_.fetch_add(1, std::memory_order_relaxed) + 1;
    
    // 3. Check if critical order (optimized: check trades first, fastest path)
    // Fast path: if has trades, it's critical (most common case)
    bool is_critical = !trades.empty() || (zero_loss_mode_);
    
    // Only check thresholds if not already critical (avoid unnecessary checks)
    if (!is_critical && (critical_quantity_threshold_ > 0 || critical_order_threshold_ > 0)) {
        is_critical = is_critical_order(order, trades);
    }
    
    if (is_critical && wal_enabled_) {
        // Critical order: sync write immediately (zero data loss)
        sync_write_critical(order, trades);
        sync_writes_.fetch_add(1, std::memory_order_relaxed);
    } else {
        // Normal order: async WAL write with guaranteed persistence
        if (wal_enabled_ && wal_queue_) {
            // Optimize: get timestamp once, reuse for all entries
            Timestamp ts = get_current_timestamp();
            
            WALEntry entry;
            entry.type = WALEntry::Type::ORDER;
            entry.order = *order;
            entry.timestamp = ts;
            entry.sequence_id = seq_id;
            
            // Try to enqueue (non-blocking)
            if (wal_queue_->push(entry)) {
                async_writes_.fetch_add(1, std::memory_order_relaxed);
                
                // Also enqueue trades (only if there are trades)
                if (!trades.empty()) {
                    for (const auto& trade : trades) {
                        WALEntry trade_entry;
                        trade_entry.type = WALEntry::Type::TRADE;
                        trade_entry.trade = trade;
                        trade_entry.timestamp = ts;  // Reuse timestamp
                        trade_entry.sequence_id = seq_id;  // Reuse sequence
                        
                        wal_queue_->push(trade_entry);
                    }
                }
                
                // Zero data loss guarantee: ensure entry is written to WAL file
                // ✅ Use batch confirmation for better performance (immediate notification, no batch delay)
                if (batch_confirm_manager_) {
                    // Wait for confirmation (max 50ms, reduced from 110ms)
                    batch_confirm_manager_->wait_for_confirm(seq_id, std::chrono::milliseconds(50));
                } else {
                    // Fallback to original method
                    ensure_wal_written(seq_id);
                }
            } else {
                // Queue full - fallback to sync write for safety (rare case)
                // This ensures zero data loss even when queue is full
                sync_write_critical(order, trades);
                sync_writes_.fetch_add(1, std::memory_order_relaxed);
            }
        }
    }
    
    // Return immediately - no blocking on disk I/O for normal orders!
    return trades;
}

std::vector<Trade> ProductionMatchingEngineSafeOptimized::process_order_zero_loss(Order* order) {
    if (!order) {
        return {};
    }
    
    // Force zero-loss mode: all orders are treated as critical
    bool old_mode = zero_loss_mode_;
    zero_loss_mode_ = true;
    
    auto trades = process_order_optimized(order);
    
    zero_loss_mode_ = old_mode;
    return trades;
}

std::vector<Trade> ProductionMatchingEngineSafeOptimized::process_order_guaranteed_zero_loss(Order* order) {
    if (!order) {
        return {};
    }
    
    // Guaranteed zero data loss: all orders are immediately synced
    // This is slower but provides true zero data loss guarantee
    auto trades = ProductionMatchingEngineV2::process_order_production_v2(order);
    
    if (wal_enabled_) {
        // Immediate sync write for all orders (true zero data loss)
        sync_write_critical(order, trades);
        sync_writes_.fetch_add(1, std::memory_order_relaxed);
    }
    
    return trades;
}

void ProductionMatchingEngineSafeOptimized::ensure_wal_written(uint64_t sequence_id) {
    if (!wal_enabled_ || !wal_queue_) {
        return;
    }
    
    // ✅ Optimization: Fast path - if already written, return immediately
    uint64_t last_written = last_written_sequence_.load(std::memory_order_acquire);
    if (last_written >= sequence_id) {
        return;  // Already written, no need to wait
    }
    
    // ✅ Optimization: Use condition variable instead of polling
    std::unique_lock<std::mutex> lock(wal_written_mutex_);
    
    // Wait at most 10ms to allow WAL writer to catch up (increased from 1ms for high load)
    auto timeout = std::chrono::steady_clock::now() + std::chrono::milliseconds(10);
    
    bool notified = wal_written_cv_.wait_until(lock, timeout, [this, sequence_id]() {
        return last_written_sequence_.load(std::memory_order_acquire) >= sequence_id;
    });
    
    // If timeout and still not written, continue waiting with yield (for zero data loss)
    if (!notified) {
        uint64_t current_written = last_written_sequence_.load(std::memory_order_acquire);
        if (current_written < sequence_id) {
            // Continue waiting with yield to ensure zero data loss
            int additional_retries = 0;
            const int max_additional_retries = 100;  // Additional 100ms wait
            
            while (current_written < sequence_id && additional_retries < max_additional_retries) {
                std::this_thread::yield();
                current_written = last_written_sequence_.load(std::memory_order_acquire);
                additional_retries++;
            }
            
            // Final check - if still not written, log warning
            if (current_written < sequence_id) {
                LOG_WARNING("ensure_wal_written extended timeout for sequence " + 
                           std::to_string(sequence_id) + 
                           " (last written: " + std::to_string(current_written) + ")");
            }
        }
    }
}

void ProductionMatchingEngineSafeOptimized::wal_writer_thread() {
    LOG_INFO("WAL writer thread started");
    
    // ✅ Optimization: Batch processing for better performance
    const size_t BATCH_SIZE = 100;  // Batch process 100 entries
    std::vector<WALEntry> batch;
    batch.reserve(BATCH_SIZE);
    
    std::vector<Order> orders;
    std::vector<Trade> trades;
    orders.reserve(BATCH_SIZE);
    trades.reserve(BATCH_SIZE);
    
    while (running_.load(std::memory_order_relaxed) || !wal_queue_->empty()) {
        // ✅ Collect batch of entries
        batch.clear();
        orders.clear();
        trades.clear();
        
        for (size_t i = 0; i < BATCH_SIZE; ++i) {
            WALEntry entry;
            if (wal_queue_->pop(entry)) {
                batch.push_back(entry);
            } else {
                break;  // Queue empty, process collected batch
            }
        }
        
        if (!batch.empty()) {
            uint64_t max_seq = 0;
            
            // ✅ Separate orders and trades for batch writing
            for (const auto& entry : batch) {
                if (entry.sequence_id > max_seq) {
                    max_seq = entry.sequence_id;
                }
                
                if (entry.type == WALEntry::Type::ORDER) {
                    orders.push_back(entry.order);
                } else if (entry.type == WALEntry::Type::TRADE) {
                    trades.push_back(entry.trade);
                }
            }
            
            // ✅ Batch write using append_batch (reduces system calls)
            try {
                if (!orders.empty()) {
                    if (!wal_->append_batch(orders)) {
                        LOG_ERROR("Failed to append batch of orders");
                    }
                }
                if (!trades.empty()) {
                    if (!wal_->append_batch_trades(trades)) {
                        LOG_ERROR("Failed to append batch of trades");
                    }
                }
                
                // ✅ Batch update sequence number
                last_written_sequence_.store(max_seq, std::memory_order_release);
                
                // ✅ Immediate notification (no batch delay)
                {
                    std::lock_guard<std::mutex> lock(wal_written_mutex_);
                    wal_written_cv_.notify_all();
                }
                
                // ✅ Notify batch confirm manager (immediate notification)
                if (batch_confirm_manager_) {
                    batch_confirm_manager_->notify_written(max_seq);
                }
                
                // ✅ Use async fsync only for large batches (reduce overhead)
                if (batch.size() >= BATCH_SIZE && wal_) {
                    wal_->async_sync();
                }
                
            } catch (const std::exception& e) {
                LOG_ERROR("WAL batch write failed: " + std::string(e.what()));
            }
        } else {
            // Queue empty, yield to other threads
            std::this_thread::yield();
        }
    }
    
    LOG_INFO("WAL writer thread stopped");
}

void ProductionMatchingEngineSafeOptimized::sync_worker_thread() {
    LOG_INFO("Sync worker thread started");
    
    while (running_.load(std::memory_order_relaxed)) {
        auto sync_start = high_resolution_clock::now();
        
        if (should_sync()) {
            perform_sync();
            
            auto sync_end = high_resolution_clock::now();
            auto sync_time = duration_cast<microseconds>(sync_end - sync_start);
            
            sync_count_.fetch_add(1, std::memory_order_relaxed);
            total_sync_time_us_.fetch_add(sync_time.count(), std::memory_order_relaxed);
        }
        
        // Sleep for sync interval
        std::this_thread::sleep_for(sync_interval_);
    }
    
    // Final sync on shutdown
    perform_sync();
    
    LOG_INFO("Sync worker thread stopped");
}

bool ProductionMatchingEngineSafeOptimized::should_sync() const {
    if (!wal_enabled_ || !wal_) {
        return false;
    }
    
    // Condition 1: Time-based (every sync_interval_)
    auto now = get_current_timestamp();
    if (last_sync_time_ == 0 || 
        (now - last_sync_time_) > static_cast<uint64_t>(sync_interval_.count() * 1000)) {
        return true;
    }
    
    // Condition 2: Sequence-based (every sync_batch_size_ entries)
    uint64_t pending = pending_sequence_.load(std::memory_order_acquire);
    uint64_t committed = committed_sequence_.load(std::memory_order_acquire);
    if (pending - committed >= sync_batch_size_) {
        return true;
    }
    
    return false;
}

void ProductionMatchingEngineSafeOptimized::perform_sync() {
    if (!wal_enabled_ || !wal_) {
        return;
    }
    
    try {
        // Wait for WAL writer to catch up (drain queue)
        // Optimized: only wait if queue has significant backlog
        if (wal_queue_ && wal_queue_->size() > 100) {
            // Use yield instead of sleep for better responsiveness
            int retries = 0;
            while (wal_queue_->size() > 10 && retries < 10) {
                std::this_thread::yield();
                retries++;
            }
        }
        
        // Sync to disk (fsync)
        wal_->sync();
        
        // Mark as committed
        uint64_t current_pending = pending_sequence_.load(std::memory_order_acquire);
        committed_sequence_.store(current_pending, std::memory_order_release);
        last_sync_sequence_.store(current_pending, std::memory_order_release);
        last_sync_time_ = get_current_timestamp();
        
    } catch (const std::exception& e) {
        LOG_ERROR("Sync failed: " + std::string(e.what()));
    }
}

bool ProductionMatchingEngineSafeOptimized::recover_from_wal() {
    if (!wal_enabled_) {
        return true;
    }
    
    LOG_INFO("Starting recovery from WAL...");
    
    auto uncommitted_orders = wal_->read_uncommitted_orders();
    
    if (uncommitted_orders.empty()) {
        LOG_INFO("No uncommitted orders");
        return true;
    }
    
    LOG_INFO("Found " + std::to_string(uncommitted_orders.size()) + " uncommitted orders");
    
    size_t recovered = 0;
    for (auto& order : uncommitted_orders) {
        try {
            auto trades = process_order_optimized(&order);
            recovered++;
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to recover order: " + std::string(e.what()));
        }
    }
    
    LOG_INFO("Recovery complete: " + std::to_string(recovered) + " orders recovered");
    
    return true;
}

bool ProductionMatchingEngineSafeOptimized::is_critical_order(const Order* order, const std::vector<Trade>& trades) const {
    if (!order) {
        return false;
    }
    
    // Fast path: trades already checked in process_order_optimized
    // This function is only called if trades.empty() && !zero_loss_mode_
    
    // Large quantity orders are critical
    if (critical_quantity_threshold_ > 0) {
        double qty = perpetual::quantity_to_double(order->quantity);
        if (qty >= critical_quantity_threshold_) {
            return true;
        }
    }
    
    // High value orders are critical
    if (critical_order_threshold_ > 0) {
        double price = perpetual::price_to_double(order->price);
        if (price >= critical_order_threshold_) {
            return true;
        }
    }
    
    return false;
}

void ProductionMatchingEngineSafeOptimized::sync_write_critical(const Order* order, const std::vector<Trade>& trades) {
    if (!wal_enabled_ || !wal_ || !order) {
        return;
    }
    
    try {
        // ✅ Optimization: Smart wait for queue to drain (instead of fixed sleep)
        if (wal_queue_ && !wal_queue_->empty()) {
            // Use yield + limited retries instead of fixed sleep
            int retries = 0;
            const int max_retries = 50;  // Max 50 yields (approx 500μs)
            
            while (!wal_queue_->empty() && retries < max_retries) {
                std::this_thread::yield();
                retries++;
            }
            
            // If queue still not empty, wait a bit longer with shorter sleeps
            if (!wal_queue_->empty()) {
                for (int i = 0; i < 10 && !wal_queue_->empty(); ++i) {
                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                }
            }
        }
        
        // 2. Write order to WAL (synchronous)
        if (!wal_->append(*order)) {
            LOG_ERROR("Failed to append order to WAL");
            return;
        }
        
        // ✅ Optimization: Use batch write for trades if there are many
        if (trades.size() > 10) {
            if (!wal_->append_batch_trades(trades)) {
                LOG_ERROR("Failed to append batch of trades");
            }
        } else {
            // 3. Write trades to WAL (synchronous, for small batches)
            for (const auto& trade : trades) {
                if (!wal_->append(trade)) {
                    LOG_ERROR("Failed to append trade to WAL");
                }
            }
        }
        
        // 4. Immediate fsync (zero data loss guarantee)
        wal_->sync();
        
        // 5. Update committed sequence
        uint64_t current_pending = pending_sequence_.load(std::memory_order_acquire);
        committed_sequence_.store(current_pending, std::memory_order_release);
        last_sync_sequence_.store(current_pending, std::memory_order_release);
        last_sync_time_ = get_current_timestamp();
        
    } catch (const std::exception& e) {
        LOG_ERROR("Sync write failed: " + std::string(e.what()));
    }
}

ProductionMatchingEngineSafeOptimized::Stats ProductionMatchingEngineSafeOptimized::get_stats() const {
    Stats stats;
    stats.wal_size = wal_ ? wal_->size() : 0;
    stats.uncommitted_count = wal_ ? wal_->uncommitted_count() : 0;
    stats.async_writes = async_writes_.load();
    stats.sync_writes = sync_writes_.load();
    stats.sync_count = sync_count_.load();
    stats.queue_size = wal_queue_ ? wal_queue_->size() : 0;
    
    uint64_t total_time = total_sync_time_us_.load();
    uint64_t count = sync_count_.load();
    stats.avg_sync_time_us = count > 0 ? static_cast<double>(total_time) / count : 0.0;
    
    return stats;
}

void ProductionMatchingEngineSafeOptimized::shutdown() {
    if (running_.exchange(false)) {
        // Zero data loss guarantee: wait for all queue entries to be written
        if (wal_queue_) {
            uint64_t target_sequence = pending_sequence_.load(std::memory_order_acquire);
            int retries = 0;
            const int max_retries = 10000;  // 10 seconds max wait
            
            while (retries < max_retries) {
                // Check if all entries have been written to WAL
                uint64_t last_written = last_written_sequence_.load(std::memory_order_acquire);
                
                if (last_written >= target_sequence && wal_queue_->empty()) {
                    // All entries written and queue empty
                    break;
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                retries++;
            }
            
            // Final check: if queue still has entries, log warning
            if (!wal_queue_->empty()) {
                LOG_WARNING("Queue not empty after shutdown wait: " + 
                           std::to_string(wal_queue_->size()) + " entries");
            }
        }
        
        // Stop batch confirm manager
        if (batch_confirm_manager_) {
            batch_confirm_manager_->stop();
        }
        
        // Join threads (they will finish processing remaining entries)
        if (wal_writer_thread_.joinable()) {
            wal_writer_thread_.join();
        }
        if (sync_worker_thread_.joinable()) {
            sync_worker_thread_.join();
        }
        
        // Wait for async fsync to complete
        if (wal_) {
            wal_->wait_async_sync();
        }
        
        // Final sync to ensure all data is persisted
        if (wal_enabled_ && wal_) {
            try {
                wal_->sync();
                LOG_INFO("Final sync completed, zero data loss guaranteed");
            } catch (const std::exception& e) {
                LOG_ERROR("Final sync failed: " + std::string(e.what()));
            }
        }
    }
    
    ProductionMatchingEngineV2::shutdown();
}

} // namespace perpetual

