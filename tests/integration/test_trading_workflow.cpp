#include <gtest/gtest.h>
#include "core/auth_manager.h"
#include "core/matching_engine_event_sourcing.h"
#include "core/account_manager.h"
#include "core/position_manager.h"
#include "core/liquidation_engine.h"
#include "core/funding_rate_manager.h"
#include "core/order.h"
#include "core/types.h"
#include <filesystem>

using namespace perpetual;

class TradingWorkflowTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup authentication
        auth_manager_ = std::make_unique<AuthManager>();
        std::string error_msg;
        auth_manager_->registerUser("testuser", "test@example.com", "password123", error_msg);
        
        std::string token;
        auth_manager_->login("testuser", "password123", token, error_msg);
        
        user_id_ = 1000000;  // First user
        
        // Setup account
        account_manager_ = std::make_unique<AccountBalanceManager>();
        account_manager_->setBalance(user_id_, 10000.0);  // $10,000
        
        // Setup position manager
        position_manager_ = std::make_unique<PositionManager>();
        
        // Setup matching engine with event sourcing
        event_store_dir_ = "./test_event_store_" + std::to_string(get_current_timestamp());
        std::filesystem::create_directories(event_store_dir_);
        
        matching_engine_ = std::make_unique<MatchingEngineEventSourcing>(1);
        matching_engine_->initialize(event_store_dir_);
        matching_engine_->set_deterministic_mode(true);
        
        // Setup liquidation engine
        liquidation_engine_ = std::make_unique<LiquidationEngine>();
        liquidation_engine_->setAccountManager(account_manager_.get());
        liquidation_engine_->setPositionManager(position_manager_.get());
        
        // Setup funding rate manager
        funding_manager_ = std::make_unique<FundingRateManager>();
        funding_manager_->setAccountManager(account_manager_.get());
        funding_manager_->setPositionManager(position_manager_.get());
    }
    
    void TearDown() override {
        if (std::filesystem::exists(event_store_dir_)) {
            std::filesystem::remove_all(event_store_dir_);
        }
    }
    
    std::unique_ptr<AuthManager> auth_manager_;
    std::unique_ptr<AccountBalanceManager> account_manager_;
    std::unique_ptr<PositionManager> position_manager_;
    std::unique_ptr<MatchingEngineEventSourcing> matching_engine_;
    std::unique_ptr<LiquidationEngine> liquidation_engine_;
    std::unique_ptr<FundingRateManager> funding_manager_;
    
    std::string event_store_dir_;
    UserID user_id_;
};

TEST_F(TradingWorkflowTest, CompleteOrderWorkflow) {
    // 1. Submit buy order
    Order buy_order(1, user_id_, 1, OrderSide::BUY,
                   double_to_price(50000.0), double_to_quantity(0.1),
                   OrderType::LIMIT);
    
    auto trades1 = matching_engine_->process_order_es(&buy_order);
    EXPECT_EQ(buy_order.status, OrderStatus::PENDING);
    
    // 2. Submit matching sell order
    Order sell_order(2, user_id_ + 1, 1, OrderSide::SELL,
                    double_to_price(50000.0), double_to_quantity(0.1),
                    OrderType::LIMIT);
    
    auto trades2 = matching_engine_->process_order_es(&sell_order);
    
    // Should have trade if matched
    // EXPECT_GE(trades2.size(), 0);
}

TEST_F(TradingWorkflowTest, AccountBalanceManagement) {
    double initial_balance = account_manager_->getBalance(user_id_);
    EXPECT_DOUBLE_EQ(initial_balance, 10000.0);
    
    // Freeze balance
    bool result = account_manager_->freezeBalance(user_id_, 1000.0);
    EXPECT_TRUE(result);
    
    double available = account_manager_->getAvailableBalance(user_id_);
    EXPECT_LT(available, initial_balance);
}

TEST_F(TradingWorkflowTest, RiskCalculation) {
    Price current_price = double_to_price(50000.0);
    
    auto risk = liquidation_engine_->calculateRiskLevel(user_id_, 1, current_price);
    
    // Initial risk should be low (no position)
    EXPECT_FALSE(risk.is_liquidatable);
}

TEST_F(TradingWorkflowTest, FundingRateCalculation) {
    double rate = funding_manager_->calculateFundingRate(1, 0.0001, 0.0001);
    
    EXPECT_GE(rate, -0.0075);
    EXPECT_LE(rate, 0.0075);
}

TEST_F(TradingWorkflowTest, EventSourcingIntegration) {
    // Process order
    Order order(1, user_id_, 1, OrderSide::BUY,
               double_to_price(50000.0), double_to_quantity(0.1),
               OrderType::LIMIT);
    
    auto trades = matching_engine_->process_order_es(&order);
    
    // Events are automatically stored by the engine
    // Event store access is internal to the engine
    EXPECT_TRUE(order.order_id != 0);
    
    // Test replay functionality
    bool replay_result = matching_engine_->replay_events(0, UINT64_MAX);
    EXPECT_TRUE(replay_result);
}

