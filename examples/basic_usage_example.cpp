#include "core/auth_manager.h"
#include "core/matching_engine.h"
#include "core/account_manager.h"
#include "core/position_manager.h"
#include "core/liquidation_engine.h"
#include "core/funding_rate_manager.h"
#include "core/market_data_service.h"
#include "core/notification_service.h"
#include "core/monitoring_system.h"
#include "core/order.h"
#include "core/types.h"
#include <iostream>

using namespace perpetual;

int main() {
    std::cout << "=== High-Performance Matching Engine - Basic Usage Example ===" << std::endl;
    
    // 1. Initialize Authentication
    std::cout << "\n1. User Authentication" << std::endl;
    AuthManager auth;
    std::string error_msg;
    
    // Register a user
    if (auth.registerUser("alice", "alice@example.com", "password123", error_msg)) {
        std::cout << "  ✅ User registered: alice" << std::endl;
    } else {
        std::cout << "  ❌ Registration failed: " << error_msg << std::endl;
    }
    
    // Login
    std::string token;
    if (auth.login("alice", "password123", token, error_msg)) {
        std::cout << "  ✅ Login successful, token: " << token.substr(0, 20) << "..." << std::endl;
    } else {
        std::cout << "  ❌ Login failed: " << error_msg << std::endl;
    }
    
    // 2. Initialize Account
    std::cout << "\n2. Account Management" << std::endl;
    AccountBalanceManager account_manager;
    UserID user_id = 1000000;
    account_manager.setBalance(user_id, 10000.0);  // $10,000
    std::cout << "  ✅ Account balance: $" << account_manager.getBalance(user_id) << std::endl;
    
    // 3. Initialize Matching Engine
    std::cout << "\n3. Matching Engine" << std::endl;
    InstrumentID instrument_id = 1;
    MatchingEngine matching_engine(instrument_id);
    std::cout << "  ✅ Matching engine initialized for instrument " << instrument_id << std::endl;
    
    // 4. Submit Orders
    std::cout << "\n4. Order Submission" << std::endl;
    
    // Create buy order
    Order buy_order(1, user_id, instrument_id, OrderSide::BUY, 
                    double_to_price(50000.0), double_to_quantity(0.1), OrderType::LIMIT);
    auto trades = matching_engine.process_order(&buy_order);
    std::cout << "  ✅ Buy order submitted: " << quantity_to_double(buy_order.quantity) 
              << " @ $" << price_to_double(buy_order.price) << std::endl;
    
    // Create sell order (should match with buy order)
    Order sell_order(2, user_id + 1, instrument_id, OrderSide::SELL,
                     double_to_price(50000.0), double_to_quantity(0.1), OrderType::LIMIT);
    trades = matching_engine.process_order(&sell_order);
    std::cout << "  ✅ Sell order submitted: " << quantity_to_double(sell_order.quantity)
              << " @ $" << price_to_double(sell_order.price) << std::endl;
    
    if (!trades.empty()) {
        std::cout << "  ✅ Trade executed: " << trades.size() << " trade(s)" << std::endl;
        for (const auto& trade : trades) {
            std::cout << "     Price: $" << price_to_double(trade.price)
                      << ", Quantity: " << quantity_to_double(trade.quantity) << std::endl;
        }
    }
    
    // 5. Position Management
    std::cout << "\n5. Position Management" << std::endl;
    PositionManager position_manager;
    Quantity position_size = position_manager.getPositionSize(user_id, instrument_id);
    std::cout << "  ✅ Current position size: " << quantity_to_double(position_size) << std::endl;
    
    // 6. Liquidation Check
    std::cout << "\n6. Liquidation Engine" << std::endl;
    LiquidationEngine liquidation_engine;
    liquidation_engine.setPositionManager(&position_manager);
    liquidation_engine.setAccountManager(&account_manager);
    
    Price current_price = double_to_price(50000.0);
    auto risk = liquidation_engine.calculateRiskLevel(user_id, instrument_id, current_price);
    std::cout << "  ✅ Risk ratio: " << risk.risk_ratio << std::endl;
    std::cout << "  ✅ Liquidatable: " << (risk.is_liquidatable ? "Yes" : "No") << std::endl;
    
    // 7. Funding Rate
    std::cout << "\n7. Funding Rate" << std::endl;
    FundingRateManager funding_manager;
    funding_manager.setPositionManager(&position_manager);
    funding_manager.setAccountManager(&account_manager);
    
    double funding_rate = funding_manager.calculateFundingRate(instrument_id, 0.0001, 0.0001);
    std::cout << "  ✅ Current funding rate: " << (funding_rate * 100) << "%" << std::endl;
    
    // 8. Market Data
    std::cout << "\n8. Market Data Service" << std::endl;
    MarketDataService market_data;
    market_data.subscribe(user_id, instrument_id, MarketDataService::SUBSCRIBE_DEPTH);
    std::cout << "  ✅ Subscribed to market data" << std::endl;
    
    // 9. Monitoring
    std::cout << "\n9. Monitoring" << std::endl;
    MonitoringSystem monitoring;
    monitoring.recordOrderSubmitted(instrument_id);
    monitoring.recordTrade(instrument_id, double_to_quantity(0.1));
    monitoring.recordMatchingLatency(100000);  // 100 microseconds
    
    std::string metrics = monitoring.getPrometheusMetrics();
    std::cout << "  ✅ Metrics recorded" << std::endl;
    std::cout << "  Metrics (first 200 chars): " << metrics.substr(0, 200) << "..." << std::endl;
    
    // 10. Notifications
    std::cout << "\n10. Notification Service" << std::endl;
    NotificationService notification_service;
    notification_service.notifyOrderFilled(user_id, 1, instrument_id, 
                                          double_to_quantity(0.1), double_to_price(50000.0));
    std::cout << "  ✅ Notification sent" << std::endl;
    
    std::cout << "\n=== Example Complete ===" << std::endl;
    
    return 0;
}

#include "core/matching_engine.h"
#include "core/account_manager.h"
#include "core/position_manager.h"
#include "core/liquidation_engine.h"
#include "core/funding_rate_manager.h"
#include "core/market_data_service.h"
#include "core/notification_service.h"
#include "core/monitoring_system.h"
#include "core/order.h"
#include "core/types.h"
#include <iostream>

using namespace perpetual;

int main() {
    std::cout << "=== High-Performance Matching Engine - Basic Usage Example ===" << std::endl;
    
    // 1. Initialize Authentication
    std::cout << "\n1. User Authentication" << std::endl;
    AuthManager auth;
    std::string error_msg;
    
    // Register a user
    if (auth.registerUser("alice", "alice@example.com", "password123", error_msg)) {
        std::cout << "  ✅ User registered: alice" << std::endl;
    } else {
        std::cout << "  ❌ Registration failed: " << error_msg << std::endl;
    }
    
    // Login
    std::string token;
    if (auth.login("alice", "password123", token, error_msg)) {
        std::cout << "  ✅ Login successful, token: " << token.substr(0, 20) << "..." << std::endl;
    } else {
        std::cout << "  ❌ Login failed: " << error_msg << std::endl;
    }
    
    // 2. Initialize Account
    std::cout << "\n2. Account Management" << std::endl;
    AccountBalanceManager account_manager;
    UserID user_id = 1000000;
    account_manager.setBalance(user_id, 10000.0);  // $10,000
    std::cout << "  ✅ Account balance: $" << account_manager.getBalance(user_id) << std::endl;
    
    // 3. Initialize Matching Engine
    std::cout << "\n3. Matching Engine" << std::endl;
    InstrumentID instrument_id = 1;
    MatchingEngine matching_engine(instrument_id);
    std::cout << "  ✅ Matching engine initialized for instrument " << instrument_id << std::endl;
    
    // 4. Submit Orders
    std::cout << "\n4. Order Submission" << std::endl;
    
    // Create buy order
    Order buy_order(1, user_id, instrument_id, OrderSide::BUY, 
                    double_to_price(50000.0), double_to_quantity(0.1), OrderType::LIMIT);
    auto trades = matching_engine.process_order(&buy_order);
    std::cout << "  ✅ Buy order submitted: " << quantity_to_double(buy_order.quantity) 
              << " @ $" << price_to_double(buy_order.price) << std::endl;
    
    // Create sell order (should match with buy order)
    Order sell_order(2, user_id + 1, instrument_id, OrderSide::SELL,
                     double_to_price(50000.0), double_to_quantity(0.1), OrderType::LIMIT);
    trades = matching_engine.process_order(&sell_order);
    std::cout << "  ✅ Sell order submitted: " << quantity_to_double(sell_order.quantity)
              << " @ $" << price_to_double(sell_order.price) << std::endl;
    
    if (!trades.empty()) {
        std::cout << "  ✅ Trade executed: " << trades.size() << " trade(s)" << std::endl;
        for (const auto& trade : trades) {
            std::cout << "     Price: $" << price_to_double(trade.price)
                      << ", Quantity: " << quantity_to_double(trade.quantity) << std::endl;
        }
    }
    
    // 5. Position Management
    std::cout << "\n5. Position Management" << std::endl;
    PositionManager position_manager;
    Quantity position_size = position_manager.getPositionSize(user_id, instrument_id);
    std::cout << "  ✅ Current position size: " << quantity_to_double(position_size) << std::endl;
    
    // 6. Liquidation Check
    std::cout << "\n6. Liquidation Engine" << std::endl;
    LiquidationEngine liquidation_engine;
    liquidation_engine.setPositionManager(&position_manager);
    liquidation_engine.setAccountManager(&account_manager);
    
    Price current_price = double_to_price(50000.0);
    auto risk = liquidation_engine.calculateRiskLevel(user_id, instrument_id, current_price);
    std::cout << "  ✅ Risk ratio: " << risk.risk_ratio << std::endl;
    std::cout << "  ✅ Liquidatable: " << (risk.is_liquidatable ? "Yes" : "No") << std::endl;
    
    // 7. Funding Rate
    std::cout << "\n7. Funding Rate" << std::endl;
    FundingRateManager funding_manager;
    funding_manager.setPositionManager(&position_manager);
    funding_manager.setAccountManager(&account_manager);
    
    double funding_rate = funding_manager.calculateFundingRate(instrument_id, 0.0001, 0.0001);
    std::cout << "  ✅ Current funding rate: " << (funding_rate * 100) << "%" << std::endl;
    
    // 8. Market Data
    std::cout << "\n8. Market Data Service" << std::endl;
    MarketDataService market_data;
    market_data.subscribe(user_id, instrument_id, MarketDataService::SUBSCRIBE_DEPTH);
    std::cout << "  ✅ Subscribed to market data" << std::endl;
    
    // 9. Monitoring
    std::cout << "\n9. Monitoring" << std::endl;
    MonitoringSystem monitoring;
    monitoring.recordOrderSubmitted(instrument_id);
    monitoring.recordTrade(instrument_id, double_to_quantity(0.1));
    monitoring.recordMatchingLatency(100000);  // 100 microseconds
    
    std::string metrics = monitoring.getPrometheusMetrics();
    std::cout << "  ✅ Metrics recorded" << std::endl;
    std::cout << "  Metrics (first 200 chars): " << metrics.substr(0, 200) << "..." << std::endl;
    
    // 10. Notifications
    std::cout << "\n10. Notification Service" << std::endl;
    NotificationService notification_service;
    notification_service.notifyOrderFilled(user_id, 1, instrument_id, 
                                          double_to_quantity(0.1), double_to_price(50000.0));
    std::cout << "  ✅ Notification sent" << std::endl;
    
    std::cout << "\n=== Example Complete ===" << std::endl;
    
    return 0;
}

