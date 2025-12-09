#include "core/persistence_async.h"
#include "core/types.h"
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <chrono>
#include <stdexcept>
#include <iostream>

namespace perpetual {

AsyncPersistenceManager::AsyncPersistenceManager() {
    last_flush_time_ = std::chrono::steady_clock::now();
}

AsyncPersistenceManager::~AsyncPersistenceManager() {
    stop();
}

bool AsyncPersistenceManager::initialize(const std::string& data_dir) {
    data_dir_ = data_dir;
    std::filesystem::create_directories(data_dir_);
    
    orders_log_path_ = data_dir_ + "/orders.log";
    trades_log_path_ = data_dir_ + "/trades.log";
    wal_path_ = data_dir_ + "/wal.log";
    
    // 打开WAL文件（追加模式）
    wal_file_ = std::make_unique<std::ofstream>(wal_path_, std::ios::app | std::ios::binary);
    if (!wal_file_->is_open()) {
        return false;
    }
    
    return true;
}

void AsyncPersistenceManager::start() {
    if (running_.load()) {
        return;
    }
    
    running_ = true;
    persistence_thread_ = std::thread(&AsyncPersistenceManager::persistenceWorker, this);
}

void AsyncPersistenceManager::stop() {
    if (!running_.load()) {
        return;
    }
    
    // 设置停止标志
    running_ = false;
    
    // 等待worker线程退出（最多等待1秒）
    if (persistence_thread_.joinable()) {
        // 使用超时join，避免无限等待
        auto start = std::chrono::steady_clock::now();
        while (persistence_thread_.joinable()) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - start).count();
            
            if (elapsed > 1000) {  // 1秒超时
                std::cerr << "Warning: Persistence thread did not stop within timeout" << std::endl;
                break;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        if (persistence_thread_.joinable()) {
            persistence_thread_.join();
        }
    }
    
    // 最终刷新
    flush();
    
    if (wal_file_) {
        wal_file_->close();
    }
}

void AsyncPersistenceManager::persistOrderAsync(const Order& order) {
    PersistItem item;
    item.type = PersistType::ORDER;
    item.order = order;
    
    // 非阻塞推送
    while (!persist_queue_.push(item)) {
        // 队列满，等待一小段时间
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
}

void AsyncPersistenceManager::persistTradeAsync(const Trade& trade) {
    PersistItem item;
    item.type = PersistType::TRADE;
    item.trade = trade;
    
    // 非阻塞推送
    while (!persist_queue_.push(item)) {
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
}

void AsyncPersistenceManager::persistBatchAsync(const std::vector<Trade>& trades) {
    PersistItem item;
    item.type = PersistType::BATCH;
    item.batch = trades;
    
    // 非阻塞推送
    while (!persist_queue_.push(item)) {
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
}

void AsyncPersistenceManager::persistenceWorker() {
    std::vector<PersistItem> batch;
    auto last_batch_time = std::chrono::steady_clock::now();
    int empty_polls = 0;
    const int MAX_EMPTY_POLLS = 100;  // 连续空轮询100次后检查退出
    
    // LockFreeMPMCQueue doesn't have empty(), use pop with timeout
    while (running_ || !batch.empty()) {
        PersistItem item;
        bool has_item = false;
        
        // 尝试pop一个item
        has_item = persist_queue_.pop(item);
        
        if (has_item) {
            empty_polls = 0;
            batch.push_back(item);
            
            // 批量达到阈值或超时
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - last_batch_time).count();
            
            if (batch.size() >= BATCH_SIZE || elapsed >= BATCH_TIMEOUT_MS) {
                persistBatch(batch);
                batch.clear();
                last_batch_time = now;
            }
        } else {
            empty_polls++;
            
            // 如果没有item且running_为false，检查是否应该退出
            if (!running_ && batch.empty()) {
                // 再尝试一次pop，确保队列真的为空
                if (!persist_queue_.pop(item)) {
                    break;
                } else {
                    // 还有item，继续处理
                    batch.push_back(item);
                    empty_polls = 0;
                }
            }
            
            // 如果连续空轮询太多次，稍微sleep避免CPU占用
            if (empty_polls > MAX_EMPTY_POLLS) {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                empty_polls = 0;
            }
        }
        
        // 尝试pop更多items（批量处理）
        while (persist_queue_.pop(item)) {
            empty_polls = 0;
            batch.push_back(item);
            
            // 批量达到阈值或超时
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - last_batch_time).count();
            
            if (batch.size() >= BATCH_SIZE || elapsed >= BATCH_TIMEOUT_MS) {
                persistBatch(batch);
                batch.clear();
                last_batch_time = now;
            }
        }
        
        // 处理剩余批次（超时或停止时）
        if (!batch.empty()) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - last_batch_time).count();
            
            if (!running_ || elapsed >= BATCH_TIMEOUT_MS) {
                persistBatch(batch);
                batch.clear();
                last_batch_time = now;
            }
        }
        
        // 定期刷新
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_flush_time_).count() >= 100) {  // 每100ms刷新一次
            flushBatch();
            last_flush_time_ = now;
        }
    }
    
    // 处理最后一批
    if (!batch.empty()) {
        persistBatch(batch);
    }
    
    // 最终刷新
    flushBatch();
}

void AsyncPersistenceManager::persistOrder(const Order& order) {
    // 序列化订单
    std::ostringstream oss;
    oss << order.order_id << ","
        << order.user_id << ","
        << order.instrument_id << ","
        << static_cast<int>(order.side) << ","
        << order.price << ","
        << order.quantity << ","
        << static_cast<int>(order.order_type) << ","
        << order.timestamp << "\n";
    
    std::string data = oss.str();
    writeToWAL(data);
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.orders_persisted++;
    }
}

void AsyncPersistenceManager::persistTrade(const Trade& trade) {
    // 序列化交易
    std::ostringstream oss;
    oss << trade.buy_order_id << ","
        << trade.sell_order_id << ","
        << trade.buy_user_id << ","
        << trade.sell_user_id << ","
        << trade.instrument_id << ","
        << trade.price << ","
        << trade.quantity << ","
        << trade.timestamp << ","
        << trade.sequence_id << "\n";
    
    std::string data = oss.str();
    writeToWAL(data);
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.trades_persisted++;
    }
}

void AsyncPersistenceManager::persistBatch(const std::vector<PersistItem>& batch) {
    if (batch.empty()) return;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::ostringstream wal_buffer;
    
    for (const auto& item : batch) {
        switch (item.type) {
            case PersistType::ORDER:
                persistOrder(item.order);
                break;
            case PersistType::TRADE: {
                std::ostringstream oss;
                oss << item.trade.buy_order_id << ","
                    << item.trade.sell_order_id << ","
                    << item.trade.buy_user_id << ","
                    << item.trade.sell_user_id << ","
                    << item.trade.instrument_id << ","
                    << item.trade.price << ","
                    << item.trade.quantity << ","
                    << item.trade.timestamp << ","
                    << item.trade.sequence_id << "\n";
                wal_buffer << oss.str();
                break;
            }
            case PersistType::BATCH:
                for (const auto& trade : item.batch) {
                    std::ostringstream oss;
                    oss << trade.buy_order_id << ","
                        << trade.sell_order_id << ","
                        << trade.buy_user_id << ","
                        << trade.sell_user_id << ","
                        << trade.instrument_id << ","
                        << trade.price << ","
                        << trade.quantity << ","
                        << trade.timestamp << ","
                        << trade.sequence_id << "\n";
                    wal_buffer << oss.str();
                }
                break;
        }
    }
    
    // 批量写入WAL
    if (wal_buffer.str().size() > 0) {
        writeToWAL(wal_buffer.str());
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(
        end_time - start_time).count();
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.batches_persisted++;
        stats_.trades_persisted += batch.size();
        if (stats_.batches_persisted > 0) {
            stats_.avg_persist_latency_ns = 
                (stats_.avg_persist_latency_ns * (stats_.batches_persisted - 1) + latency) / 
                stats_.batches_persisted;
        }
    }
}

void AsyncPersistenceManager::writeToWAL(const std::string& data) {
    std::lock_guard<std::mutex> lock(wal_mutex_);
    if (wal_file_ && wal_file_->is_open()) {
        *wal_file_ << data;
        // 不立即flush，由flushBatch统一处理
    }
}

void AsyncPersistenceManager::flushBatch() {
    std::lock_guard<std::mutex> lock(wal_mutex_);
    if (wal_file_ && wal_file_->is_open()) {
        wal_file_->flush();
    }
}

void AsyncPersistenceManager::flush() {
    // 等待队列清空
    int retries = 0;
    // LockFreeMPMCQueue doesn't have empty(), try to pop items
    PersistItem dummy;
    while (persist_queue_.pop(dummy) && retries < 1000) {
        retries++;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        retries++;
    }
    
    // 刷新WAL
    flushBatch();
}

AsyncPersistenceManager::Statistics AsyncPersistenceManager::getStatistics() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    Statistics stats = stats_;
    // Note: LockFreeMPMCQueue doesn't have size() method
    // We track queue size through push/pop operations if needed
    return stats;
}

} // namespace perpetual

