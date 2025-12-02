#pragma once

#include "order.h"
#include "types.h"
#include "lockfree_queue.h"
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <fstream>
#include <chrono>

namespace perpetual {

// Optimized log entry with pre-serialized data
struct OptimizedLogEntry {
    std::string data;           // Pre-serialized CSV data
    Timestamp timestamp;        // Entry timestamp
    bool is_trade;              // true for trade, false for order
    
    OptimizedLogEntry() = default;
    OptimizedLogEntry(const std::string& d, Timestamp ts, bool trade)
        : data(d), timestamp(ts), is_trade(trade) {}
};

// High-performance persistence manager with async writing
class OptimizedPersistenceManager {
public:
    OptimizedPersistenceManager();
    ~OptimizedPersistenceManager();
    
    bool initialize(const std::string& data_dir, 
                    size_t buffer_size = 10000,
                    size_t flush_interval_ms = 100);
    
    // High-performance logging (non-blocking)
    void logTrade(const Trade& trade);
    void logOrder(const Order& order, const std::string& event_type);
    
    // Force flush (blocking)
    void flush();
    
    // Statistics
    struct Stats {
        uint64_t trades_logged = 0;
        uint64_t orders_logged = 0;
        uint64_t batches_written = 0;
        uint64_t bytes_written = 0;
        uint64_t write_errors = 0;
        double avg_write_latency_us = 0.0;
    };
    
    Stats getStats() const;
    
    // Shutdown gracefully
    void shutdown();
    
private:
    // Background writer thread
    void writerThread();
    
    // Serialize trade to string (optimized)
    std::string serializeTrade(const Trade& trade);
    
    // Serialize order to string (optimized)
    std::string serializeOrder(const Order& order, const std::string& event_type);
    
    // Write batch to file
    void writeBatch(const std::vector<OptimizedLogEntry>& batch);
    
    // Rotate log files if needed
    void rotateLogFiles();
    
    // Configuration
    std::string data_dir_;
    size_t buffer_size_;
    size_t flush_interval_ms_;
    std::atomic<bool> initialized_{false};
    std::atomic<bool> shutdown_requested_{false};
    
    // Lock-free queues for async writing
    std::unique_ptr<LockFreeSPSCQueue<OptimizedLogEntry>> trade_queue_;
    std::unique_ptr<LockFreeSPSCQueue<OptimizedLogEntry>> order_queue_;
    
    // Writer thread
    std::thread writer_thread_;
    std::condition_variable writer_cv_;
    std::mutex writer_mutex_;
    
    // Buffers for batching
    std::vector<OptimizedLogEntry> trade_buffer_;
    std::vector<OptimizedLogEntry> order_buffer_;
    
    // File streams (with rotation)
    std::unique_ptr<std::ofstream> trade_log_;
    std::unique_ptr<std::ofstream> order_log_;
    std::string current_trade_file_;
    std::string current_order_file_;
    std::atomic<uint64_t> trade_file_size_{0};
    std::atomic<uint64_t> order_file_size_{0};
    static constexpr uint64_t MAX_FILE_SIZE = 100 * 1024 * 1024; // 100MB
    
    // Statistics
    mutable std::mutex stats_mutex_;
    Stats stats_;
    
    // Pre-allocated string buffers for serialization
    thread_local static std::string serialization_buffer_;
};

} // namespace perpetual
