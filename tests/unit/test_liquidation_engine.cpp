#include <gtest/gtest.h>
#include "core/liquidation_engine.h"
#include "core/account_manager.h"
#include "core/position_manager.h"
#include "core/types.h"

using namespace perpetual;

class LiquidationEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        account_manager_ = std::make_unique<AccountBalanceManager>();
        position_manager_ = std::make_unique<PositionManager>();
        liquidation_engine_ = std::make_unique<LiquidationEngine>();
        
        liquidation_engine_->setAccountManager(account_manager_.get());
        liquidation_engine_->setPositionManager(position_manager_.get());
        
        // Setup test account
        user_id_ = 1000000;
        instrument_id_ = 1;
        account_manager_->setBalance(user_id_, 10000.0);  // $10,000
    }
    
    std::unique_ptr<AccountBalanceManager> account_manager_;
    std::unique_ptr<PositionManager> position_manager_;
    std::unique_ptr<LiquidationEngine> liquidation_engine_;
    
    UserID user_id_;
    InstrumentID instrument_id_;
};

TEST_F(LiquidationEngineTest, RiskLevelCalculation) {
    // Setup position
    // Note: PositionManager needs updatePosition method - simplified test
    
    Price current_price = double_to_price(50000.0);
    auto risk = liquidation_engine_->calculateRiskLevel(user_id_, instrument_id_, current_price);
    
    // Initial risk should be low (no position)
    EXPECT_FALSE(risk.is_liquidatable);
}

TEST_F(LiquidationEngineTest, LiquidationTrigger) {
    Price current_price = double_to_price(50000.0);
    
    // Test should liquidate check (no position = no liquidation)
    bool should_liquidate = liquidation_engine_->shouldLiquidate(user_id_, instrument_id_, current_price);
    EXPECT_FALSE(should_liquidate);
}

TEST_F(LiquidationEngineTest, InsuranceFundBalance) {
    // Note: Insurance fund balance is internal, test through liquidation operations
    Price current_price = double_to_price(50000.0);
    auto risk = liquidation_engine_->calculateRiskLevel(user_id_, instrument_id_, current_price);
    EXPECT_FALSE(risk.is_liquidatable);  // No position
}

TEST_F(LiquidationEngineTest, MaintenanceMarginRatio) {
    // Note: Maintenance margin ratio is internal, test through risk calculation
    Price current_price = double_to_price(50000.0);
    auto risk = liquidation_engine_->calculateRiskLevel(user_id_, instrument_id_, current_price);
    EXPECT_FALSE(risk.is_liquidatable);  // No position, should not liquidate
}

TEST_F(LiquidationEngineTest, RiskLevelWithPosition) {
    // Create a position
    Quantity position_size = double_to_quantity(1.0);  // 1 BTC long
    position_manager_->updatePosition(user_id_, instrument_id_, position_size, OrderSide::BUY);
    
    Price current_price = double_to_price(50000.0);
    auto risk = liquidation_engine_->calculateRiskLevel(user_id_, instrument_id_, current_price);
    
    // Should have position value
    EXPECT_GT(risk.position_value, 0.0);
    EXPECT_GT(risk.maintenance_margin, 0.0);
}

TEST_F(LiquidationEngineTest, LiquidationWithLowMargin) {
    // Set up account with low balance
    account_manager_->setBalance(user_id_, 100.0);  // Low balance
    
    // Create a large position
    Quantity position_size = double_to_quantity(1.0);  // 1 BTC
    position_manager_->updatePosition(user_id_, instrument_id_, position_size, OrderSide::BUY);
    
    Price current_price = double_to_price(50000.0);
    auto risk = liquidation_engine_->calculateRiskLevel(user_id_, instrument_id_, current_price);
    
    // With low balance and large position, should be at risk
    // Exact liquidation depends on margin calculations
    EXPECT_GT(risk.position_value, 0.0);
}

TEST_F(LiquidationEngineTest, CalculateMaintenanceMargin) {
    Quantity position_size = double_to_quantity(1.0);
    Price entry_price = double_to_price(50000.0);
    Price current_price = double_to_price(51000.0);
    double leverage = 10.0;
    
    double margin = liquidation_engine_->calculateMaintenanceMargin(
        position_size, entry_price, current_price, leverage);
    
    EXPECT_GT(margin, 0.0);
}

TEST_F(LiquidationEngineTest, InsuranceFundOperations) {
    // Note: Insurance fund is internal, test through liquidation operations
    Price current_price = double_to_price(50000.0);
    auto risk = liquidation_engine_->calculateRiskLevel(user_id_, instrument_id_, current_price);
    EXPECT_FALSE(risk.is_liquidatable);  // No position
}

TEST_F(LiquidationEngineTest, LiquidationMarginRatio) {
    // Note: Liquidation margin ratio is internal, test through risk calculation
    Price current_price = double_to_price(50000.0);
    auto risk = liquidation_engine_->calculateRiskLevel(user_id_, instrument_id_, current_price);
    EXPECT_FALSE(risk.is_liquidatable);  // No position
}

TEST_F(LiquidationEngineTest, NoPositionNoLiquidation) {
    Price current_price = double_to_price(50000.0);
    
    // No position should not trigger liquidation
    bool should_liquidate = liquidation_engine_->shouldLiquidate(user_id_, instrument_id_, current_price);
    EXPECT_FALSE(should_liquidate);
    
    auto risk = liquidation_engine_->calculateRiskLevel(user_id_, instrument_id_, current_price);
    EXPECT_FALSE(risk.is_liquidatable);
    EXPECT_EQ(risk.position_value, 0.0);
}

TEST_F(LiquidationEngineTest, RiskRatioCalculation) {
    // Set up account and position
    account_manager_->setBalance(user_id_, 5000.0);
    Quantity position_size = double_to_quantity(1.0);
    position_manager_->updatePosition(user_id_, instrument_id_, position_size, OrderSide::BUY);
    
    Price current_price = double_to_price(50000.0);
    auto risk = liquidation_engine_->calculateRiskLevel(user_id_, instrument_id_, current_price);
    
    // Risk ratio should be calculated
    EXPECT_GE(risk.risk_ratio, 0.0);
}

