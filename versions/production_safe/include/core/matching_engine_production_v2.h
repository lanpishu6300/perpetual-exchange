#pragma once

#include "matching_engine_art_simd.h"  // Use ART+SIMD as base
#include "logger.h"
#include "config.h"
#include "metrics.h"
#include "rate_limiter.h"
#include "error_handler.h"
#include "persistence_optimized.h"
#include "health_check.h"
#include "order_validator.h"
#include "account_manager.h"
#include "position_manager.h"
#include "lockfree_queue.h"
#include <memory>
#include <atomic>
#include <thread>

namespace perpetual {

// Production V2: High-performance production engine with ART+SIMD
// Goal: Achieve near ART+SIMD performance while maintaining all production features
class ProductionMatchingEngineV2 : public MatchingEngineARTSIMD {
public:
    ProductionMatchingEngineV2(InstrumentID instrument_id);
    ~ProductionMatchingEngineV2();
    
    // Initialize with configuration
    bool initialize(const std::string& config_file);
    
    // Process order with optimized validation and async persistence
    std::vector<Trade> process_order_production_v2(Order* order);
    
    // Enhanced cancel with validation
    bool cancel_order_production_v2(OrderID order_id, UserID user_id);
    
    // Health check
    HealthInfo getHealth() const;
    
    // Metrics endpoint
    std::string getMetrics() const;
    
    // Graceful shutdown
    void shutdown();
    
    // Disable rate limiting (for benchmarks)
    void disable_rate_limiting() { rate_limiting_enabled_ = false; }
    void enable_rate_limiting() { rate_limiting_enabled_ = true; }
    
private:
    // Fast path validation (minimal overhead)
    bool validateOrderFastPath(const Order* order) const;
    
    // Rate limit check with caching
    bool checkRateLimitCached(UserID user_id);
    
    // Balance check with caching
    bool checkBalanceCached(UserID user_id, Price price, Quantity quantity);
    
    // Async persistence
    void persistenceWorker();
    void enqueuePersistence(const Order& order, const std::vector<Trade>& trades);
    
    // Lock-free metrics
    void updateMetricsLockFree(uint64_t orders, uint64_t trades);
    
private:
    std::unique_ptr<RateLimiter> global_rate_limiter_;
    std::unique_ptr<RateLimiter> user_rate_limiter_;
    std::unique_ptr<OptimizedPersistenceManager> optimized_persistence_;
    std::unique_ptr<OrderValidator> order_validator_;
    std::unique_ptr<AccountBalanceManager> account_manager_;
    std::unique_ptr<PositionManager> position_manager_;
    
    // Async persistence queue
    LockFreeSPSCQueue<std::pair<Order, std::vector<Trade>>> persistence_queue_;
    std::thread persistence_thread_;
    std::atomic<bool> persistence_running_{false};
    
    // Performance optimizations
    bool enable_async_persistence_ = true;
    bool enable_validation_cache_ = true;
    bool enable_fast_path_ = true;
    
    // Cache for rate limiting and balance checks
    mutable std::unordered_map<UserID, std::pair<std::chrono::steady_clock::time_point, bool>> rate_limit_cache_;
    mutable std::unordered_map<UserID, std::pair<std::chrono::steady_clock::time_point, bool>> balance_cache_;
    struct ValidationCacheEntry {
        std::atomic<uint64_t> last_access_time{0};
        std::atomic<bool> has_balance{false};
    };
    mutable std::unordered_map<UserID, ValidationCacheEntry> validation_cache_;
    mutable std::mutex cache_mutex_;
    
    bool initialized_ = false;
    bool rate_limiting_enabled_ = true;
    std::atomic<bool> shutting_down_{false};
    
    // Lock-free metrics counters
    std::atomic<uint64_t> orders_received_{0};
    std::atomic<uint64_t> orders_rejected_{0};
    std::atomic<uint64_t> orders_processed_{0};
    std::atomic<uint64_t> trades_executed_{0};
};

} // namespace perpetual
