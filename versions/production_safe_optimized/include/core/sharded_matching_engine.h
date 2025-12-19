#pragma once

#include "core/matching_engine_production_safe_optimized.h"
#include "core/trading_shard.h"
#include "core/types.h"
#include "core/order.h"
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>

namespace perpetual {

// Sharded Matching Engine: Dual-sharding strategy
// 
// Key features:
// 1. Trading shards: Sharded by user_id (account/order management/liquidation)
// 2. Matching shards: Sharded by instrument_id (matching engine/WAL)
// 3. Zero data loss guarantee maintained per matching shard
// 4. Lock-free routing (no contention between shards)
//
// Performance target: 100-200 K orders/sec (2-5x improvement)
class ShardedMatchingEngine {
public:
    // Constructor
    // num_trading_shards: Number of trading shards (recommended: CPU cores)
    // num_matching_shards: Number of matching shards (recommended: CPU cores or instrument count)
    ShardedMatchingEngine(size_t num_trading_shards = 0, size_t num_matching_shards = 0);
    
    ~ShardedMatchingEngine();
    
    // Initialize all shards
    bool initialize(const std::string& config_file, bool enable_wal = true);
    
    // Process order (routes to trading shard, then matching shard)
    std::vector<Trade> process_order(Order* order);
    
    // Process order with zero data loss guarantee
    std::vector<Trade> process_order_zero_loss(Order* order);
    
    // Shutdown all shards gracefully
    void shutdown();
    
    // Get statistics from all shards
    struct ShardStats {
        uint64_t total_orders = 0;
        uint64_t total_trades = 0;
        uint64_t total_volume = 0;
        uint64_t async_writes = 0;
        uint64_t sync_writes = 0;
        uint64_t queue_size = 0;
        uint64_t wal_size = 0;
    };
    ShardStats get_stats() const;
    
    // Get shard counts
    size_t get_trading_shard_count() const { return trading_shards_.size(); }
    size_t get_matching_shard_count() const { return matching_shards_.size(); }
    
    // Public methods for testing
    size_t get_trading_shard_id(UserID user_id) const;
    size_t get_matching_shard_id(InstrumentID instrument_id) const;
    
private:
    
    // Number of shards
    size_t num_trading_shards_;
    size_t num_matching_shards_;
    
    // Trading shards (sharded by user_id)
    std::vector<std::unique_ptr<TradingShard>> trading_shards_;
    
    // Matching shards (sharded by instrument_id)
    std::vector<std::unique_ptr<ProductionMatchingEngineSafeOptimized>> matching_shards_;
    
    // Statistics
    mutable std::mutex stats_mutex_;
};

} // namespace perpetual

