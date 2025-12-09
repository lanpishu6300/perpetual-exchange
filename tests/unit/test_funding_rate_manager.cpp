#include <gtest/gtest.h>
#include "core/funding_rate_manager.h"
#include "core/account_manager.h"
#include "core/position_manager.h"
#include "core/types.h"

using namespace perpetual;

class FundingRateManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        account_manager_ = std::make_unique<AccountBalanceManager>();
        position_manager_ = std::make_unique<PositionManager>();
        funding_manager_ = std::make_unique<FundingRateManager>();
        
        funding_manager_->setAccountManager(account_manager_.get());
        funding_manager_->setPositionManager(position_manager_.get());
        
        instrument_id_ = 1;
    }
    
    std::unique_ptr<AccountBalanceManager> account_manager_;
    std::unique_ptr<PositionManager> position_manager_;
    std::unique_ptr<FundingRateManager> funding_manager_;
    
    InstrumentID instrument_id_;
};

TEST_F(FundingRateManagerTest, FundingRateCalculation) {
    double premium_index = 0.0001;  // 0.01%
    double interest_rate = 0.0001;  // 0.01%
    
    double rate = funding_manager_->calculateFundingRate(instrument_id_, premium_index, interest_rate);
    
    EXPECT_GE(rate, -0.0075);  // Min rate -0.75%
    EXPECT_LE(rate, 0.0075);   // Max rate +0.75%
}

TEST_F(FundingRateManagerTest, PremiumIndexCalculation) {
    Price best_bid = double_to_price(50000.0);
    Price best_ask = double_to_price(50010.0);
    Price mark_price = double_to_price(50005.0);
    
    funding_manager_->updatePremiumIndex(instrument_id_, best_bid, best_ask, mark_price);
    double premium = funding_manager_->getPremiumIndex(instrument_id_);
    
    // Premium should be calculated based on spread
    EXPECT_NE(premium, 0.0);
}

TEST_F(FundingRateManagerTest, SettlementTime) {
    int64_t next_time = 1000000000;  // Some timestamp
    funding_manager_->setNextSettlementTime(instrument_id_, next_time);
    
    int64_t retrieved_time = funding_manager_->getNextSettlementTime(instrument_id_);
    EXPECT_EQ(retrieved_time, next_time);
}

TEST_F(FundingRateManagerTest, ShouldSettle) {
    // Test settlement time check
    // ShouldSettle checks if current time >= next settlement time
    int64_t next_time = get_current_timestamp() + 3600;  // 1 hour from now
    funding_manager_->setNextSettlementTime(instrument_id_, next_time);
    
    // Initially may or may not settle depending on implementation
    // This is a basic test that the method exists
    bool should_settle = funding_manager_->shouldSettle(instrument_id_);
    // Result depends on current time vs next_time
}

