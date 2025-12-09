#pragma once

#include "matching_engine_event_sourcing.h"
#include "persistence_async.h"
#include "thread_local_memory_pool.h"
#include "lockfree_queue.h"
#include <vector>
#include <memory>
#include <atomic>

namespace perpetual {

// 优化版本3: 集成异步持久化、内存池、批量处理
class MatchingEngineOptimizedV3 : public MatchingEngineEventSourcing {
public:
    MatchingEngineOptimizedV3(InstrumentID instrument_id, 
                              EventStore* event_store = nullptr);
    ~MatchingEngineOptimizedV3();
    
    // 初始化（包含异步持久化）
    bool initialize(const std::string& event_store_dir, 
                   const std::string& persistence_dir = "");
    
    // 批量处理订单（优化版本）
    std::vector<Trade> processOrderBatch(const std::vector<Order*>& orders);
    
    // 处理单个订单（使用内存池）
    std::vector<Trade> process_order_es(Order* order);
    
    // 启动后台线程
    void start();
    
    // 停止后台线程
    void stop();
    
    // 获取统计信息
    struct Statistics {
        uint64_t orders_processed = 0;
        uint64_t trades_executed = 0;
        double avg_matching_latency_ns = 0;
        double avg_persistence_latency_ns = 0;
        AsyncPersistenceManager::Statistics persistence_stats;
    };
    Statistics getStatistics() const;
    
private:
    // 使用内存池分配订单
    Order* allocateOrder();
    void deallocateOrder(Order* order);
    
    // 批量持久化交易
    void persistTradesBatch(const std::vector<Trade>& trades);
    
    // 异步持久化管理器
    std::unique_ptr<AsyncPersistenceManager> async_persistence_;
    
    // Thread-Local内存池
    ThreadLocalMemoryPool<Order> order_pool_;
    
    // 批量缓冲区
    std::vector<Trade> trade_batch_;
    std::mutex batch_mutex_;
    static constexpr size_t BATCH_SIZE = 100;
    
    // 统计信息
    mutable std::mutex stats_mutex_;
    Statistics stats_;
    
    // 运行状态
    std::atomic<bool> running_{false};
};

} // namespace perpetual


#include "matching_engine_event_sourcing.h"
#include "persistence_async.h"
#include "thread_local_memory_pool.h"
#include "lockfree_queue.h"
#include <vector>
#include <memory>
#include <atomic>

namespace perpetual {

// 优化版本3: 集成异步持久化、内存池、批量处理
class MatchingEngineOptimizedV3 : public MatchingEngineEventSourcing {
public:
    MatchingEngineOptimizedV3(InstrumentID instrument_id, 
                              EventStore* event_store = nullptr);
    ~MatchingEngineOptimizedV3();
    
    // 初始化（包含异步持久化）
    bool initialize(const std::string& event_store_dir, 
                   const std::string& persistence_dir = "");
    
    // 批量处理订单（优化版本）
    std::vector<Trade> processOrderBatch(const std::vector<Order*>& orders);
    
    // 处理单个订单（使用内存池）
    std::vector<Trade> process_order_es(Order* order);
    
    // 启动后台线程
    void start();
    
    // 停止后台线程
    void stop();
    
    // 获取统计信息
    struct Statistics {
        uint64_t orders_processed = 0;
        uint64_t trades_executed = 0;
        double avg_matching_latency_ns = 0;
        double avg_persistence_latency_ns = 0;
        AsyncPersistenceManager::Statistics persistence_stats;
    };
    Statistics getStatistics() const;
    
private:
    // 使用内存池分配订单
    Order* allocateOrder();
    void deallocateOrder(Order* order);
    
    // 批量持久化交易
    void persistTradesBatch(const std::vector<Trade>& trades);
    
    // 异步持久化管理器
    std::unique_ptr<AsyncPersistenceManager> async_persistence_;
    
    // Thread-Local内存池
    ThreadLocalMemoryPool<Order> order_pool_;
    
    // 批量缓冲区
    std::vector<Trade> trade_batch_;
    std::mutex batch_mutex_;
    static constexpr size_t BATCH_SIZE = 100;
    
    // 统计信息
    mutable std::mutex stats_mutex_;
    Statistics stats_;
    
    // 运行状态
    std::atomic<bool> running_{false};
};

} // namespace perpetual

