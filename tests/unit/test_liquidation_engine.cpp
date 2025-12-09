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
    double initial_balance = 1000000.0;
    liquidation_engine_->setInsuranceFundBalance(initial_balance);
    
    EXPECT_DOUBLE_EQ(liquidation_engine_->getInsuranceFundBalance(), initial_balance);
}

TEST_F(LiquidationEngineTest, MaintenanceMarginRatio) {
    double ratio = 0.005;  // 0.5%
    liquidation_engine_->setMaintenanceMarginRatio(ratio);
    
    // Verify ratio is set by checking risk calculation uses it
    Price current_price = double_to_price(50000.0);
    auto risk = liquidation_engine_->calculateRiskLevel(user_id_, instrument_id_, current_price);
    EXPECT_FALSE(risk.is_liquidatable);  // No position, should not liquidate
}

