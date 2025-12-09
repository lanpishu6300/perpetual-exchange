#pragma once

#include "types.h"
#include "order.h"
#include "lockfree_queue.h"
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <fstream>
#include <string>

namespace perpetual {

// 异步持久化管理器
// 完全异步，不阻塞撮合引擎
class AsyncPersistenceManager {
public:
    AsyncPersistenceManager();
    ~AsyncPersistenceManager();
    
    // 初始化
    bool initialize(const std::string& data_dir);
    
    // 异步持久化订单（非阻塞）
    void persistOrderAsync(const Order& order);
    
    // 异步持久化交易（非阻塞）
    void persistTradeAsync(const Trade& trade);
    
    // 批量持久化（非阻塞）
    void persistBatchAsync(const std::vector<Trade>& trades);
    
    // 启动后台线程
    void start();
    
    // 停止后台线程
    void stop();
    
    // 刷新所有待写入数据
    void flush();
    
    // 获取统计信息
    struct Statistics {
        uint64_t orders_persisted = 0;
        uint64_t trades_persisted = 0;
        uint64_t batches_persisted = 0;
        uint64_t queue_size = 0;
        double avg_persist_latency_ns = 0;
    };
    Statistics getStatistics() const;
    
private:
    // 持久化项类型
    enum class PersistType {
        ORDER = 0,
        TRADE = 1,
        BATCH = 2
    };
    
    struct PersistItem {
        PersistType type;
        Order order;
        Trade trade;
        std::vector<Trade> batch;
        
        PersistItem() : type(PersistType::TRADE) {}
        PersistItem(const PersistItem& other) 
            : type(other.type), order(other.order), trade(other.trade), batch(other.batch) {}
        PersistItem& operator=(const PersistItem& other) {
            if (this != &other) {
                type = other.type;
                order = other.order;
                trade = other.trade;
                batch = other.batch;
            }
            return *this;
        }
    };
    
    // 后台持久化线程
    void persistenceWorker();
    
    // 持久化单个订单
    void persistOrder(const Order& order);
    
    // 持久化单个交易
    void persistTrade(const Trade& trade);
    
    // 持久化批量交易
    void persistBatch(const std::vector<PersistItem>& batch);
    
    // WAL写入
    void writeToWAL(const std::string& data);
    
    // 批量刷新
    void flushBatch();
    
    std::string data_dir_;
    std::string orders_log_path_;
    std::string trades_log_path_;
    std::string wal_path_;
    
    // Lock-Free MPMC队列 (容量: 1M，必须是2的幂)
    LockFreeMPMCQueue<PersistItem> persist_queue_{1048576};  // 2^20
    
    // 后台线程
    std::thread persistence_thread_;
    std::atomic<bool> running_{false};
    
    // 批量缓冲区
    std::vector<PersistItem> batch_buffer_;
    std::mutex batch_mutex_;
    static constexpr size_t BATCH_SIZE = 1000;
    static constexpr int BATCH_TIMEOUT_MS = 10;  // 10ms超时
    
    // WAL文件
    std::unique_ptr<std::ofstream> wal_file_;
    std::mutex wal_mutex_;
    
    // 统计信息
    mutable std::mutex stats_mutex_;
    Statistics stats_;
    
    // 最后刷新时间
    std::chrono::steady_clock::time_point last_flush_time_;
};

} // namespace perpetual

