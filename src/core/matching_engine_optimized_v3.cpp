#include "core/matching_engine_optimized_v3.h"
#include "core/matching_engine_event_sourcing.h"
#include <algorithm>
#include <chrono>

namespace perpetual {

MatchingEngineOptimizedV3::MatchingEngineOptimizedV3(InstrumentID instrument_id,
                                                     EventStore* event_store)
    : MatchingEngineEventSourcing(instrument_id, event_store),
      order_pool_(1000, 500) {  // 初始1000，每次增长500
}

MatchingEngineOptimizedV3::~MatchingEngineOptimizedV3() {
    stop();
}

bool MatchingEngineOptimizedV3::initialize(const std::string& event_store_dir,
                                           const std::string& persistence_dir) {
    // 初始化父类
    if (!MatchingEngineEventSourcing::initialize(event_store_dir)) {
        return false;
    }
    
    // 初始化异步持久化
    if (!persistence_dir.empty()) {
        async_persistence_ = std::make_unique<AsyncPersistenceManager>();
        if (!async_persistence_->initialize(persistence_dir)) {
            return false;
        }
    }
    
    return true;
}

void MatchingEngineOptimizedV3::start() {
    if (running_.load()) {
        return;
    }
    
    running_ = true;
    
    if (async_persistence_) {
        async_persistence_->start();
    }
}

void MatchingEngineOptimizedV3::stop() {
    if (!running_.load()) {
        return;
    }
    
    running_ = false;
    
    // 刷新所有待持久化数据
    {
        std::lock_guard<std::mutex> lock(batch_mutex_);
        if (!trade_batch_.empty() && async_persistence_) {
            async_persistence_->persistBatchAsync(trade_batch_);
            trade_batch_.clear();
        }
    }
    
    if (async_persistence_) {
        async_persistence_->flush();
        async_persistence_->stop();
    }
}

std::vector<Trade> MatchingEngineOptimizedV3::process_order_es(Order* order) {
    if (!order) {
        return {};
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 使用父类方法处理订单
    std::vector<Trade> trades = MatchingEngineEventSourcing::process_order_es(order);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(
        end_time - start_time).count();
    
    // 更新统计
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.orders_processed++;
        stats_.trades_executed += trades.size();
        
        if (stats_.orders_processed > 0) {
            stats_.avg_matching_latency_ns = 
                (stats_.avg_matching_latency_ns * (stats_.orders_processed - 1) + latency) /
                stats_.orders_processed;
        }
    }
    
    // 异步持久化交易
    if (!trades.empty() && async_persistence_) {
        persistTradesBatch(trades);
    }
    
    return trades;
}

std::vector<Trade> MatchingEngineOptimizedV3::processOrderBatch(const std::vector<Order*>& orders) {
    std::vector<Trade> all_trades;
    all_trades.reserve(orders.size());
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 批量处理
    for (Order* order : orders) {
        auto trades = process_order_es(order);
        all_trades.insert(all_trades.end(), trades.begin(), trades.end());
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(
        end_time - start_time).count();
    
    // 批量持久化
    if (!all_trades.empty() && async_persistence_) {
        async_persistence_->persistBatchAsync(all_trades);
    }
    
    return all_trades;
}

void MatchingEngineOptimizedV3::persistTradesBatch(const std::vector<Trade>& trades) {
    if (!async_persistence_) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(batch_mutex_);
    
    trade_batch_.insert(trade_batch_.end(), trades.begin(), trades.end());
    
    // 批量达到阈值，立即持久化
    if (trade_batch_.size() >= BATCH_SIZE) {
        async_persistence_->persistBatchAsync(trade_batch_);
        trade_batch_.clear();
    }
}

Order* MatchingEngineOptimizedV3::allocateOrder() {
    return order_pool_.allocate();
}

void MatchingEngineOptimizedV3::deallocateOrder(Order* order) {
    order_pool_.deallocate(order);
}

MatchingEngineOptimizedV3::Statistics MatchingEngineOptimizedV3::getStatistics() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    Statistics stats = stats_;
    
    if (async_persistence_) {
        stats.persistence_stats = async_persistence_->getStatistics();
    }
    
    return stats;
}

} // namespace perpetual

