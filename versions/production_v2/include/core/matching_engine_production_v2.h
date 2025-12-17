#pragma once

#include "core/matching_engine_art_simd.h"  // Use ART+SIMD as base
#include "core/logger.h"
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
    void updateMetricsLockFree(const std::string& metric, uint64_t value = 1);
    
    struct PersistenceTask {
        Order order;
        std::vector<Trade> trades;
        Timestamp timestamp;
    };
    
    // Async persistence
    std::unique_ptr<OptimizedPersistenceManager> persistence_;
    LockFreeSPSCQueue<PersistenceTask> persistence_queue_;
    std::thread persistence_thread_;
    std::atomic<bool> persistence_running_{false};
    
    // Rate limiter with caching
    std::unique_ptr<RateLimiter> global_rate_limiter_;
    std::unique_ptr<RateLimiter> user_rate_limiter_;
    
    // Validation cache
    struct ValidationCache {
        std::atomic<uint64_t> last_access_time{0};
        std::atomic<bool> has_balance{false};
        std::atomic<int64_t> cached_balance{0};
    };
    std::unordered_map<UserID, ValidationCache> validation_cache_;
    
    // Validators
    std::unique_ptr<OrderValidator> order_validator_;
    std::unique_ptr<AccountBalanceManager> account_manager_;
    std::unique_ptr<PositionManager> position_manager_;
    
    // Configuration
    bool initialized_ = false;
    bool rate_limiting_enabled_ = true;
    bool enable_async_persistence_ = true;
    bool enable_validation_cache_ = true;
    bool enable_fast_path_ = true;
    double default_leverage_ = 10.0;
    
    // Statistics (lock-free)
    std::atomic<uint64_t> orders_received_{0};
    std::atomic<uint64_t> orders_processed_{0};
    std::atomic<uint64_t> orders_rejected_{0};
    std::atomic<uint64_t> trades_executed_{0};
    
    std::atomic<bool> shutting_down_{false};
};

} // namespace perpetual
