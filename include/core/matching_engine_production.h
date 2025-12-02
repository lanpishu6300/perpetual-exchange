#pragma once

#include "matching_engine_optimized.h"
#include "logger.h"
#include "config.h"
#include "metrics.h"
#include "rate_limiter.h"
#include "error_handler.h"
#include "persistence.h"
#include "persistence_optimized.h"
#include "health_check.h"
#include <memory>

namespace perpetual {

// Production-ready matching engine with all enterprise features
class ProductionMatchingEngine : public OptimizedMatchingEngine {
public:
    ProductionMatchingEngine(InstrumentID instrument_id);
    ~ProductionMatchingEngine();
    
    // Initialize with configuration
    bool initialize(const std::string& config_file);
    
    // Process order with validation and rate limiting
    std::vector<Trade> process_order_production(Order* order);
    
    // Enhanced cancel with validation
    bool cancel_order_production(OrderID order_id, UserID user_id);
    
    // Health check
    HealthInfo getHealth() const;
    
    // Metrics endpoint
    std::string getMetrics() const;
    
    // Graceful shutdown
    void shutdown();
    
private:
    bool validateOrder(const Order* order) const;
    bool checkRateLimit(UserID user_id);
    bool checkBalance(UserID user_id, Price price, Quantity quantity);
    bool checkPositionLimit(UserID user_id, InstrumentID instrument_id, Quantity quantity);
    
    std::unique_ptr<RateLimiter> global_rate_limiter_;
    std::unique_ptr<RateLimiter> user_rate_limiter_;
    std::unique_ptr<PersistenceManager> persistence_;  // Legacy
    std::unique_ptr<OptimizedPersistenceManager> optimized_persistence_;  // Optimized version
    
    bool initialized_ = false;
    std::atomic<bool> shutting_down_{false};
};

} // namespace perpetual
