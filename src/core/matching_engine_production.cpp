#include "core/matching_engine_production.h"
#include "core/config.h"
#include <algorithm>

namespace perpetual {

ProductionMatchingEngine::ProductionMatchingEngine(InstrumentID instrument_id)
    : OptimizedMatchingEngine(instrument_id) {
    HealthChecker::getInstance().start();
}

ProductionMatchingEngine::~ProductionMatchingEngine() {
    shutdown();
}

bool ProductionMatchingEngine::initialize(const std::string& config_file) {
    auto& config = Config::getInstance();
    auto& logger = Logger::getInstance();
    auto& metrics = Metrics::getInstance();
    
    // Load configuration
    if (!config_file.empty()) {
        if (!config.loadFromFile(config_file)) {
            LOG_WARN("Config file not found, using defaults: " + config_file);
        }
    }
    config.loadFromEnv();
    
    // Initialize logger
    std::string log_file = config.getString(ConfigKeys::LOG_FILE, "");
    std::string log_level_str = config.getString(ConfigKeys::LOG_LEVEL, "INFO");
    LogLevel log_level = LogLevel::INFO;
    if (log_level_str == "DEBUG") log_level = LogLevel::DEBUG;
    else if (log_level_str == "WARN") log_level = LogLevel::WARN;
    else if (log_level_str == "ERROR") log_level = LogLevel::ERROR;
    
    logger.initialize(log_file, log_level);
    LOG_INFO("Production Matching Engine initializing...");
    
    // Initialize rate limiters
    RateLimitConfig rate_config;
    rate_config.orders_per_second = config.getDouble("rate_limit.global_orders_per_second", 1000.0);
    rate_config.burst_size = config.getDouble("rate_limit.burst_size", 2000.0);
    rate_config.per_user_orders_per_second = config.getDouble("rate_limit.per_user_orders_per_second", 100.0);
    rate_config.per_user_burst_size = config.getDouble("rate_limit.per_user_burst_size", 200.0);
    
    global_rate_limiter_ = std::make_unique<RateLimiter>(
        rate_config.orders_per_second, rate_config.burst_size);
    user_rate_limiter_ = std::make_unique<RateLimiter>(
        rate_config.per_user_orders_per_second, rate_config.per_user_burst_size);
    
    // Initialize optimized persistence (preferred)
    if (config.getBool(ConfigKeys::ENABLE_PERSISTENCE, true)) {
        std::string db_path = config.getString(ConfigKeys::DB_PATH, "./data");
        size_t buffer_size = config.getInt("persistence.buffer_size", 10000);
        size_t flush_interval = config.getInt("persistence.flush_interval_ms", 100);
        
        optimized_persistence_ = std::make_unique<OptimizedPersistenceManager>();
        if (!optimized_persistence_->initialize(db_path, buffer_size, flush_interval)) {
            LOG_ERROR("Failed to initialize optimized persistence, falling back to legacy");
            // Fallback to legacy persistence
            persistence_ = std::make_unique<PersistenceManager>();
            if (!persistence_->initialize(db_path)) {
                LOG_ERROR("Failed to initialize legacy persistence");
                return false;
            }
        } else {
            LOG_INFO("Using optimized persistence manager");
        }
    }
    
    // Update health status
    HealthChecker::getInstance().setHealthy();
    
    initialized_ = true;
    LOG_INFO("Production Matching Engine initialized successfully");
    
    return true;
}

std::vector<Trade> ProductionMatchingEngine::process_order_production(Order* order) {
    if (shutting_down_.load()) {
        throw SystemException("System is shutting down");
    }
    
    if (!initialized_) {
        throw SystemException("Engine not initialized");
    }
    
    METRICS_TIMER("order_processing_latency");
    Metrics::getInstance().incrementCounter("orders_received");
    
    try {
        // Enhanced validation using OrderValidator
        auto validation_result = validateOrderEnhanced(order);
        if (!validation_result.valid) {
            Metrics::getInstance().incrementCounter("orders_rejected_invalid");
            throw InvalidOrderException(validation_result.reason);
        }
        
        // Check rate limit
        if (!checkRateLimit(order->user_id)) {
            Metrics::getInstance().incrementCounter("orders_rejected_rate_limit");
            throw OrderRejectedException("Rate limit exceeded");
        }
        
        // Check balance
        if (!checkBalance(order->user_id, order->price, order->quantity)) {
            Metrics::getInstance().incrementCounter("orders_rejected_insufficient_balance");
            throw InsufficientBalanceException();
        }
        
        // Check position limits
        if (!checkPositionLimit(order->user_id, order->instrument_id, order->quantity)) {
            Metrics::getInstance().incrementCounter("orders_rejected_position_limit");
            throw OrderRejectedException("Position limit exceeded");
        }
        
        // Process order
        auto trades = OptimizedMatchingEngine::process_order(order);
        
        // Log trades (use optimized persistence if available)
        if (optimized_persistence_) {
            for (const auto& trade : trades) {
                optimized_persistence_->logTrade(trade);
            }
            optimized_persistence_->logOrder(*order, "PROCESSED");
        } else if (persistence_) {
            // Fallback to legacy persistence
            for (const auto& trade : trades) {
                persistence_->logTrade(trade);
            }
            persistence_->logOrder(*order, "PROCESSED");
        }
        
        // Update metrics
        Metrics::getInstance().incrementCounter("orders_processed");
        Metrics::getInstance().incrementCounter("trades_executed", trades.size());
        
        // Update health metrics
        auto total_orders = Metrics::getInstance().getCounter("orders_processed");
        auto total_trades = Metrics::getInstance().getCounter("trades_executed");
        HealthChecker::getInstance().updateMetrics(total_orders, total_trades, 0.0);
        
        return trades;
        
    } catch (const ExchangeException& e) {
        LOG_ERROR("Order processing error: " + std::string(e.what()));
        Metrics::getInstance().incrementCounter("orders_errors");
        throw;
    } catch (const std::exception& e) {
        LOG_CRITICAL("Unexpected error: " + std::string(e.what()));
        Metrics::getInstance().incrementCounter("orders_errors");
        HealthChecker::getInstance().setDegraded("Unexpected errors occurred");
        throw SystemException(e.what());
    }
}

bool ProductionMatchingEngine::cancel_order_production(OrderID order_id, UserID user_id) {
    if (shutting_down_.load()) {
        return false;
    }
    
    Metrics::getInstance().incrementCounter("cancel_requests");
    
    bool result = MatchingEngine::cancel_order(order_id, user_id);
    
    if (result) {
        Metrics::getInstance().incrementCounter("orders_cancelled");
        if (persistence_) {
            // Log cancellation (would need to get order first)
        }
    } else {
        Metrics::getInstance().incrementCounter("cancel_failed");
    }
    
    return result;
}

bool ProductionMatchingEngine::validateOrder(const Order* order) const {
    if (!order) return false;
    if (order->quantity <= 0) return false;
    if (order->price <= 0 && order->order_type == OrderType::LIMIT) return false;
    if (order->instrument_id == 0) return false;
    if (order->user_id == 0) return false;
    return true;
}

bool ProductionMatchingEngine::checkRateLimit(UserID user_id) {
    // Check global rate limit
    if (!global_rate_limiter_->allow()) {
        return false;
    }
    
    // Check per-user rate limit
    std::string user_key = std::to_string(user_id);
    if (!user_rate_limiter_->allow(user_key)) {
        return false;
    }
    
    return true;
}

bool ProductionMatchingEngine::checkBalance(UserID user_id, Price price, Quantity quantity) {
    if (!account_manager_) {
        return true; // Fallback if not initialized
    }
    
    // Calculate required margin
    double required_margin = account_manager_->calculateRequiredMargin(
        price, quantity, default_leverage_);
    
    // Check if user has sufficient margin
    return account_manager_->hasSufficientMargin(user_id, required_margin);
}

bool ProductionMatchingEngine::checkPositionLimit(UserID user_id, InstrumentID instrument_id, Quantity quantity) {
    if (!position_manager_) {
        return true; // Fallback if not initialized
    }
    
    // Get current position size (simplified - would query actual position)
    Quantity current_size = position_manager_->getPositionSize(user_id, instrument_id);
    
    // Check if new position would exceed limit
    // For simplicity, we check absolute size
    // In production, you'd check based on order side and current position
    return position_manager_->checkPositionLimit(user_id, instrument_id, quantity, OrderSide::BUY);
}

OrderValidator::ValidationResult ProductionMatchingEngine::validateOrderEnhanced(const Order* order) const {
    if (!order_validator_) {
        // Fallback to basic validation
        OrderValidator::ValidationResult result;
        result.valid = validateOrder(order);
        if (!result.valid) {
            result.reason = "Basic validation failed";
            result.error_code = ErrorCode::INVALID_ORDER;
        }
        return result;
    }
    
    return order_validator_->validate(order);
}

HealthInfo ProductionMatchingEngine::getHealth() const {
    return HealthChecker::getInstance().getHealth();
}

std::string ProductionMatchingEngine::getMetrics() const {
    return Metrics::getInstance().getPrometheusFormat();
}

void ProductionMatchingEngine::shutdown() {
    if (shutting_down_.exchange(true)) {
        return; // Already shutting down
    }
    
    LOG_INFO("Shutting down Production Matching Engine...");
    
    HealthChecker::getInstance().stop();
    
    if (optimized_persistence_) {
        optimized_persistence_->flush();
        optimized_persistence_->shutdown();
    }
    if (persistence_) {
        persistence_->flush();
    }
    
    LOG_INFO("Shutdown complete");
}

} // namespace perpetual
