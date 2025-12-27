#include "core/sharded_matching_engine.h"
#include "core/logger.h"
#include <thread>
#include <algorithm>

namespace perpetual {

ShardedMatchingEngine::ShardedMatchingEngine(size_t num_trading_shards, size_t num_matching_shards)
    : num_trading_shards_(num_trading_shards), num_matching_shards_(num_matching_shards) {
    
    // Auto-detect number of shards if not specified
    size_t num_cores = std::thread::hardware_concurrency();
    if (num_cores == 0) {
        num_cores = 4;  // Default to 4 if detection fails
    }
    
    if (num_trading_shards_ == 0) {
        num_trading_shards_ = num_cores;  // Trading shards = CPU cores
    }
    
    if (num_matching_shards_ == 0) {
        num_matching_shards_ = num_cores;  // Matching shards = CPU cores
    }
    
    LOG_INFO("ShardedMatchingEngine: " + std::to_string(num_trading_shards_) + 
             " trading shards, " + std::to_string(num_matching_shards_) + " matching shards");
    
    // Initialize trading shards
    trading_shards_.reserve(num_trading_shards_);
    for (size_t i = 0; i < num_trading_shards_; ++i) {
        trading_shards_.push_back(std::make_unique<TradingShard>(i));
    }
    
    // Initialize matching shards
    matching_shards_.reserve(num_matching_shards_);
    for (size_t i = 0; i < num_matching_shards_; ++i) {
        // Each matching shard handles multiple instruments (instrument_id % num_matching_shards == i)
        InstrumentID base_instrument_id = 1;
        matching_shards_.push_back(std::make_unique<ProductionMatchingEngineSafeOptimized>(base_instrument_id));
    }
}

ShardedMatchingEngine::~ShardedMatchingEngine() {
    shutdown();
}

bool ShardedMatchingEngine::initialize(const std::string& config_file, bool enable_wal) {
    bool all_initialized = true;
    
    // Initialize trading shards
    for (size_t i = 0; i < trading_shards_.size(); ++i) {
        if (!trading_shards_[i]->initialize()) {
            LOG_ERROR("Failed to initialize trading shard " + std::to_string(i));
            all_initialized = false;
        } else {
            LOG_INFO("Trading shard " + std::to_string(i) + " initialized");
        }
    }
    
    // Initialize matching shards with CPU core binding
    // Each matching shard binds to a dedicated CPU core (shares with its WAL threads)
    for (size_t i = 0; i < matching_shards_.size(); ++i) {
        // Each matching shard needs its own WAL directory to avoid conflicts
        std::string shard_wal_path = "./data/wal/matching_shard_" + std::to_string(i);
        // Bind matching shard i to CPU core i (0-indexed)
        // This allows WAL threads to share the same core with processing
        int cpu_core = static_cast<int>(i);
        if (!matching_shards_[i]->initialize(config_file, enable_wal, shard_wal_path, cpu_core)) {
            LOG_ERROR("Failed to initialize matching shard " + std::to_string(i));
            all_initialized = false;
        } else {
            LOG_INFO("Matching shard " + std::to_string(i) + " initialized with WAL: " + shard_wal_path + 
                     " (CPU core " + std::to_string(cpu_core) + ")");
        }
    }
    
    return all_initialized;
}

size_t ShardedMatchingEngine::get_trading_shard_id(UserID user_id) const {
    // Trading shard: shard by user_id
    return user_id % num_trading_shards_;
}

size_t ShardedMatchingEngine::get_matching_shard_id(InstrumentID instrument_id) const {
    // Matching shard: shard by instrument_id
    return instrument_id % num_matching_shards_;
}

std::vector<Trade> ShardedMatchingEngine::process_order(Order* order) {
    if (!order) {
        return {};
    }
    
    // 无锁优化：如果交易分片数为0，完全跳过交易分片（最快路径）
    if (num_trading_shards_ == 0) {
        // 最快路径：直接路由到撮合分片，无锁，无交易验证
        size_t matching_shard_id = order->instrument_id % num_matching_shards_;
        return matching_shards_[matching_shard_id]->process_order_optimized(order);
    }
    
    // 标准路径：包含交易分片验证（已优化为最快路径）
    // Step 1: Route to trading shard (by user_id) - 内联计算，无锁
    size_t trading_shard_id = order->user_id % num_trading_shards_;
    auto* trading_shard = trading_shards_[trading_shard_id].get();
    
    // Step 2: Validate and prepare order (已优化：直接返回true，无锁操作)
    if (!trading_shard->validate_and_prepare_order(order)) {
        return {};
    }
    
    // Step 3: Route to matching shard (by instrument_id) - 内联计算，无锁
    size_t matching_shard_id = order->instrument_id % num_matching_shards_;
    auto* matching_shard = matching_shards_[matching_shard_id].get();
    
    // Step 4: Process order in matching shard (matching, WAL write)
    auto trades = matching_shard->process_order_optimized(order);
    
    // Step 5: Update trading shard after trade (已优化：空操作，无锁)
    trading_shard->update_after_trade(order, trades);
    
    return trades;
}

std::vector<Trade> ShardedMatchingEngine::process_order_zero_loss(Order* order) {
    if (!order) {
        return {};
    }
    
    // Same flow as process_order, but use zero_loss method
    size_t trading_shard_id = get_trading_shard_id(order->user_id);
    auto* trading_shard = trading_shards_[trading_shard_id].get();
    
    if (!trading_shard->validate_and_prepare_order(order)) {
        return {};
    }
    
    size_t matching_shard_id = get_matching_shard_id(order->instrument_id);
    auto* matching_shard = matching_shards_[matching_shard_id].get();
    
    auto trades = matching_shard->process_order_zero_loss(order);
    
    trading_shard->update_after_trade(order, trades);
    
    return trades;
}

void ShardedMatchingEngine::shutdown() {
    // Shutdown matching shards
    for (size_t i = 0; i < matching_shards_.size(); ++i) {
        matching_shards_[i]->shutdown();
    }
    
    // Trading shards don't need explicit shutdown (no threads)
}

ShardedMatchingEngine::ShardStats ShardedMatchingEngine::get_stats() const {
    ShardStats total_stats;
    
    // Aggregate stats from matching shards
    for (const auto& shard : matching_shards_) {
        auto stats = shard->get_stats();
        total_stats.total_orders += stats.async_writes + stats.sync_writes;
        total_stats.total_trades += stats.sync_count;  // Approximate
        total_stats.async_writes += stats.async_writes;
        total_stats.sync_writes += stats.sync_writes;
        total_stats.queue_size += stats.queue_size;
        total_stats.wal_size += stats.wal_size;
    }
    
    return total_stats;
}

} // namespace perpetual
