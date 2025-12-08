#include "core/persistence_optimized.h"
#include "core/logger.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <numeric>

namespace perpetual {

thread_local std::string OptimizedPersistenceManager::serialization_buffer_;

OptimizedPersistenceManager::OptimizedPersistenceManager()
    : buffer_size_(10000), flush_interval_ms_(100) {
}

OptimizedPersistenceManager::~OptimizedPersistenceManager() {
    shutdown();
}

bool OptimizedPersistenceManager::initialize(const std::string& data_dir,
                                            size_t buffer_size,
                                            size_t flush_interval_ms) {
    if (initialized_.load()) {
        LOG_WARN("Persistence manager already initialized");
        return true;
    }
    
    try {
        // Create directories
        #if __has_include(<filesystem>)
        namespace fs = std::filesystem;
        #else
        namespace fs = std::experimental::filesystem;
        #endif
        fs::create_directories(data_dir);
        
        data_dir_ = data_dir;
        buffer_size_ = buffer_size;
        flush_interval_ms_ = flush_interval_ms;
        
        // Initialize lock-free queues (power of 2 for better performance)
        size_t queue_size = 1;
        while (queue_size < buffer_size * 2) {
            queue_size <<= 1;
        }
        
        trade_queue_ = std::make_unique<LockFreeSPSCQueue<OptimizedLogEntry>>(queue_size);
        order_queue_ = std::make_unique<LockFreeSPSCQueue<OptimizedLogEntry>>(queue_size);
        
        // Reserve buffer capacity
        trade_buffer_.reserve(buffer_size);
        order_buffer_.reserve(buffer_size);
        
        // Generate initial log file names
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
        std::string timestamp = ss.str();
        
        current_trade_file_ = data_dir + "/trades_" + timestamp + ".log";
        current_order_file_ = data_dir + "/orders_" + timestamp + ".log";
        
        // Open initial log files
        trade_log_ = std::make_unique<std::ofstream>(current_trade_file_, std::ios::app | std::ios::binary);
        order_log_ = std::make_unique<std::ofstream>(current_order_file_, std::ios::app | std::ios::binary);
        
        if (!trade_log_->is_open() || !order_log_->is_open()) {
            LOG_ERROR("Failed to open log files for persistence");
            return false;
        }
        
        // Start writer thread
        shutdown_requested_ = false;
        writer_thread_ = std::thread(&OptimizedPersistenceManager::writerThread, this);
        
        initialized_ = true;
        LOG_INFO("Optimized persistence manager initialized: " + data_dir);
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to initialize optimized persistence: " + std::string(e.what()));
        return false;
    }
}

void OptimizedPersistenceManager::logTrade(const Trade& trade) {
    if (!initialized_.load() || shutdown_requested_.load()) {
        return;
    }
    
    // Serialize trade (fast, no allocation if buffer reused)
    std::string data = serializeTrade(trade);
    Timestamp timestamp = get_current_timestamp();
    
    // Push to lock-free queue (non-blocking)
    OptimizedLogEntry entry(data, timestamp, true);
    
    // Try to push, if queue is full, log warning but don't block
    if (!trade_queue_->push(entry)) {
        // Queue full - increment error counter
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.write_errors++;
        LOG_WARN("Trade queue full, dropping entry");
    } else {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.trades_logged++;
    }
    
    // Notify writer thread
    writer_cv_.notify_one();
}

void OptimizedPersistenceManager::logOrder(const Order& order, const std::string& event_type) {
    if (!initialized_.load() || shutdown_requested_.load()) {
        return;
    }
    
    // Serialize order
    std::string data = serializeOrder(order, event_type);
    Timestamp timestamp = get_current_timestamp();
    
    // Push to lock-free queue (non-blocking)
    OptimizedLogEntry entry(data, timestamp, false);
    
    if (!order_queue_->push(entry)) {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.write_errors++;
        LOG_WARN("Order queue full, dropping entry");
    } else {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.orders_logged++;
    }
    
    // Notify writer thread
    writer_cv_.notify_one();
}

std::string OptimizedPersistenceManager::serializeTrade(const Trade& trade) {
    // Use thread-local buffer to avoid allocation
    serialization_buffer_.clear();
    serialization_buffer_.reserve(128); // Pre-allocate for typical trade size
    
    // Fast integer-to-string conversion
    serialization_buffer_ += std::to_string(trade.sequence_id);
    serialization_buffer_ += ',';
    serialization_buffer_ += std::to_string(trade.buy_order_id);
    serialization_buffer_ += ',';
    serialization_buffer_ += std::to_string(trade.sell_order_id);
    serialization_buffer_ += ',';
    serialization_buffer_ += std::to_string(trade.buy_user_id);
    serialization_buffer_ += ',';
    serialization_buffer_ += std::to_string(trade.sell_user_id);
    serialization_buffer_ += ',';
    serialization_buffer_ += std::to_string(trade.instrument_id);
    serialization_buffer_ += ',';
    
    // Price and quantity (use fixed precision for performance)
    char price_buf[32];
    snprintf(price_buf, sizeof(price_buf), "%.8f", price_to_double(trade.price));
    serialization_buffer_ += price_buf;
    serialization_buffer_ += ',';
    
    char qty_buf[32];
    snprintf(qty_buf, sizeof(qty_buf), "%.8f", quantity_to_double(trade.quantity));
    serialization_buffer_ += qty_buf;
    serialization_buffer_ += ',';
    
    serialization_buffer_ += std::to_string(trade.timestamp);
    serialization_buffer_ += ',';
    serialization_buffer_ += (trade.is_taker_buy ? "1" : "0");
    
    return serialization_buffer_;
}

std::string OptimizedPersistenceManager::serializeOrder(const Order& order, const std::string& event_type) {
    serialization_buffer_.clear();
    serialization_buffer_.reserve(128);
    
    serialization_buffer_ += std::to_string(order.order_id);
    serialization_buffer_ += ',';
    serialization_buffer_ += std::to_string(order.user_id);
    serialization_buffer_ += ',';
    serialization_buffer_ += std::to_string(order.instrument_id);
    serialization_buffer_ += ',';
    serialization_buffer_ += (order.side == OrderSide::BUY ? "BUY" : "SELL");
    serialization_buffer_ += ',';
    
    char price_buf[32];
    snprintf(price_buf, sizeof(price_buf), "%.8f", price_to_double(order.price));
    serialization_buffer_ += price_buf;
    serialization_buffer_ += ',';
    
    char qty_buf[32];
    snprintf(qty_buf, sizeof(qty_buf), "%.8f", quantity_to_double(order.quantity));
    serialization_buffer_ += qty_buf;
    serialization_buffer_ += ',';
    
    serialization_buffer_ += std::to_string(static_cast<int>(order.status));
    serialization_buffer_ += ',';
    serialization_buffer_ += std::to_string(order.timestamp);
    serialization_buffer_ += ',';
    serialization_buffer_ += event_type;
    
    return serialization_buffer_;
}

void OptimizedPersistenceManager::writerThread() {
    auto last_flush = std::chrono::steady_clock::now();
    
    while (!shutdown_requested_.load() || 
           !trade_queue_->empty() || 
           !order_queue_->empty()) {
        
        bool has_data = false;
        
        // Drain trade queue into buffer
        OptimizedLogEntry entry;
        while (trade_queue_->pop(entry)) {
            trade_buffer_.push_back(std::move(entry));
            has_data = true;
            
            // Batch size limit
            if (trade_buffer_.size() >= buffer_size_) {
                break;
            }
        }
        
        // Drain order queue into buffer
        while (order_queue_->pop(entry)) {
            order_buffer_.push_back(std::move(entry));
            has_data = true;
            
            if (order_buffer_.size() >= buffer_size_) {
                break;
            }
        }
        
        // Write batches if we have data
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_flush).count();
        
        if (has_data && (!trade_buffer_.empty() || !order_buffer_.empty())) {
            // Check if we should flush based on time or buffer size
            if (elapsed >= flush_interval_ms_ || 
                trade_buffer_.size() >= buffer_size_ || 
                order_buffer_.size() >= buffer_size_) {
                
                auto write_start = std::chrono::high_resolution_clock::now();
                
                if (!trade_buffer_.empty()) {
                    writeBatch(trade_buffer_);
                    trade_buffer_.clear();
                }
                
                if (!order_buffer_.empty()) {
                    writeBatch(order_buffer_);
                    order_buffer_.clear();
                }
                
                auto write_end = std::chrono::high_resolution_clock::now();
                auto write_latency = std::chrono::duration_cast<std::chrono::microseconds>(
                    write_end - write_start).count();
                
                // Update statistics
                {
                    std::lock_guard<std::mutex> lock(stats_mutex_);
                    stats_.batches_written++;
                    // Exponential moving average for latency
                    stats_.avg_write_latency_us = 
                        (stats_.avg_write_latency_us * 0.9) + (write_latency * 0.1);
                }
                
                last_flush = now;
            }
        }
        
        // Wait for more data or timeout
        if (!has_data && !shutdown_requested_.load()) {
            std::unique_lock<std::mutex> lock(writer_mutex_);
            writer_cv_.wait_for(lock, std::chrono::milliseconds(flush_interval_ms_));
        }
    }
    
    // Final flush on shutdown
    if (!trade_buffer_.empty()) {
        writeBatch(trade_buffer_);
    }
    if (!order_buffer_.empty()) {
        writeBatch(order_buffer_);
    }
    
    LOG_INFO("Persistence writer thread stopped");
}

void OptimizedPersistenceManager::writeBatch(const std::vector<OptimizedLogEntry>& batch) {
    if (batch.empty()) return;
    
    // Check file rotation
    rotateLogFiles();
    
    std::ofstream* log_file = nullptr;
    std::atomic<uint64_t>* file_size = nullptr;
    
    if (batch[0].is_trade) {
        log_file = trade_log_.get();
        file_size = &trade_file_size_;
    } else {
        log_file = order_log_.get();
        file_size = &order_file_size_;
    }
    
    if (!log_file || !log_file->is_open()) {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.write_errors += batch.size();
        return;
    }
    
    // Batch write (single system call for better performance)
    for (const auto& entry : batch) {
        *log_file << entry.data << '\n';
        file_size->fetch_add(entry.data.size() + 1); // +1 for newline
    }
    
    // Flush to disk
    log_file->flush();
    
    // Update statistics
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        size_t bytes = 0;
        for (const auto& entry : batch) {
            bytes += entry.data.size() + 1;
        }
        stats_.bytes_written += bytes;
    }
}

void OptimizedPersistenceManager::rotateLogFiles() {
    // Rotate trade log if needed
    if (trade_file_size_.load() >= MAX_FILE_SIZE && trade_log_ && trade_log_->is_open()) {
        trade_log_->close();
        
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
        current_trade_file_ = data_dir_ + "/trades_" + ss.str() + ".log";
        
        trade_log_ = std::make_unique<std::ofstream>(current_trade_file_, std::ios::app | std::ios::binary);
        trade_file_size_ = 0;
        
        LOG_INFO("Rotated trade log to: " + current_trade_file_);
    }
    
    // Rotate order log if needed
    if (order_file_size_.load() >= MAX_FILE_SIZE && order_log_ && order_log_->is_open()) {
        order_log_->close();
        
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
        current_order_file_ = data_dir_ + "/orders_" + ss.str() + ".log";
        
        order_log_ = std::make_unique<std::ofstream>(current_order_file_, std::ios::app | std::ios::binary);
        order_file_size_ = 0;
        
        LOG_INFO("Rotated order log to: " + current_order_file_);
    }
}

void OptimizedPersistenceManager::flush() {
    if (!initialized_.load()) return;
    
    // Wait for queues to be empty
    while (!trade_queue_->empty() || !order_queue_->empty()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    // Notify writer to flush immediately
    writer_cv_.notify_one();
    
    // Wait a bit for writer thread to process
    std::this_thread::sleep_for(std::chrono::milliseconds(flush_interval_ms_ + 10));
    
    // Final flush of file streams
    if (trade_log_ && trade_log_->is_open()) {
        trade_log_->flush();
    }
    if (order_log_ && order_log_->is_open()) {
        order_log_->flush();
    }
}

OptimizedPersistenceManager::Stats OptimizedPersistenceManager::getStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

void OptimizedPersistenceManager::shutdown() {
    if (!initialized_.load()) {
        return;
    }
    
    shutdown_requested_ = true;
    writer_cv_.notify_all();
    
    if (writer_thread_.joinable()) {
        writer_thread_.join();
    }
    
    // Final flush
    flush();
    
    if (trade_log_ && trade_log_->is_open()) {
        trade_log_->close();
    }
    if (order_log_ && order_log_->is_open()) {
        order_log_->close();
    }
    
    initialized_ = false;
    LOG_INFO("Optimized persistence manager shut down");
}

} // namespace perpetual


