#include <gtest/gtest.h>
#include "core/position_manager.h"
#include "core/types.h"
#include "core/order.h"
#include <thread>
#include <vector>

using namespace perpetual;

class PositionManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        position_manager_ = std::make_unique<PositionManager>();
        user_id_ = 1000000;
        instrument_id_ = 1;
    }
    
    std::unique_ptr<PositionManager> position_manager_;
    UserID user_id_;
    InstrumentID instrument_id_;
};

// Test basic position operations
TEST_F(PositionManagerTest, BasicPositionOperations) {
    // Initially, position should be 0
    EXPECT_EQ(position_manager_->getPositionSize(user_id_, instrument_id_), 0);
    
    // Update position (buy)
    Quantity buy_quantity = double_to_quantity(0.5);
    position_manager_->updatePosition(user_id_, instrument_id_, buy_quantity, OrderSide::BUY);
    EXPECT_EQ(position_manager_->getPositionSize(user_id_, instrument_id_), buy_quantity);
    
    // Update position (buy more)
    position_manager_->updatePosition(user_id_, instrument_id_, buy_quantity, OrderSide::BUY);
    EXPECT_EQ(position_manager_->getPositionSize(user_id_, instrument_id_), buy_quantity * 2);
    
    // Update position (sell - reduces long position)
    Quantity sell_quantity = double_to_quantity(0.3);
    position_manager_->updatePosition(user_id_, instrument_id_, sell_quantity, OrderSide::SELL);
    EXPECT_EQ(position_manager_->getPositionSize(user_id_, instrument_id_), buy_quantity * 2 - sell_quantity);
}

// Test long and short positions
TEST_F(PositionManagerTest, LongAndShortPositions) {
    // Create long position
    Quantity buy_qty = double_to_quantity(1.0);
    position_manager_->updatePosition(user_id_, instrument_id_, buy_qty, OrderSide::BUY);
    Quantity pos = position_manager_->getPositionSize(user_id_, instrument_id_);
    EXPECT_GT(pos, 0); // Long position is positive
    
    // Sell more than long position (creates short position)
    Quantity sell_qty = double_to_quantity(2.0);
    position_manager_->updatePosition(user_id_, instrument_id_, sell_qty, OrderSide::SELL);
    pos = position_manager_->getPositionSize(user_id_, instrument_id_);
    EXPECT_LT(pos, 0); // Short position is negative
    
    // Buy to close short position
    position_manager_->updatePosition(user_id_, instrument_id_, buy_qty, OrderSide::BUY);
    pos = position_manager_->getPositionSize(user_id_, instrument_id_);
    EXPECT_EQ(pos, 0); // Position closed
}

// Test position limit checking
TEST_F(PositionManagerTest, PositionLimitChecking) {
    Quantity limit = double_to_quantity(10.0);
    position_manager_->setPositionLimit(user_id_, instrument_id_, limit);
    
    // Check limit retrieval
    EXPECT_EQ(position_manager_->getPositionLimit(user_id_, instrument_id_), limit);
    
    // Test limit check with no position
    Quantity order_qty = double_to_quantity(5.0);
    EXPECT_TRUE(position_manager_->checkPositionLimit(user_id_, instrument_id_, order_qty, OrderSide::BUY));
    EXPECT_TRUE(position_manager_->checkPositionLimit(user_id_, instrument_id_, order_qty, OrderSide::SELL));
    
    // Test limit check with existing position
    position_manager_->updatePosition(user_id_, instrument_id_, double_to_quantity(3.0), OrderSide::BUY);
    EXPECT_TRUE(position_manager_->checkPositionLimit(user_id_, instrument_id_, double_to_quantity(5.0), OrderSide::BUY));
    EXPECT_FALSE(position_manager_->checkPositionLimit(user_id_, instrument_id_, double_to_quantity(10.0), OrderSide::BUY)); // Would exceed limit
    
    // Test limit check with short position
    // First reset position to 0
    position_manager_->updatePosition(user_id_, instrument_id_, double_to_quantity(3.0), OrderSide::SELL); // -3.0
    position_manager_->updatePosition(user_id_, instrument_id_, double_to_quantity(7.0), OrderSide::SELL); // -10.0 total
    // Now position is -10.0, limit is 10.0, so abs(-10.0) = 10.0, which equals limit
    // Selling more would make it -15.0, abs(-15.0) = 15.0 > 10.0 limit, so should fail
    EXPECT_FALSE(position_manager_->checkPositionLimit(user_id_, instrument_id_, double_to_quantity(5.0), OrderSide::SELL)); // Would exceed limit (15 > 10)
    // But selling less to stay within limit should work
    // Reset to smaller position first
    position_manager_->updatePosition(user_id_, instrument_id_, double_to_quantity(5.0), OrderSide::BUY); // -5.0
    EXPECT_TRUE(position_manager_->checkPositionLimit(user_id_, instrument_id_, double_to_quantity(5.0), OrderSide::SELL)); // -10.0, within limit
}

// Test position limit without limit set
TEST_F(PositionManagerTest, NoPositionLimit) {
    // When no limit is set, should allow any position
    Quantity large_qty = double_to_quantity(1000.0);
    EXPECT_TRUE(position_manager_->checkPositionLimit(user_id_, instrument_id_, large_qty, OrderSide::BUY));
    EXPECT_TRUE(position_manager_->checkPositionLimit(user_id_, instrument_id_, large_qty, OrderSide::SELL));
    
    // Get limit should return 0 (no limit)
    EXPECT_EQ(position_manager_->getPositionLimit(user_id_, instrument_id_), 0);
}

// Test calculate new position size
TEST_F(PositionManagerTest, CalculateNewPositionSize) {
    Quantity current_size = double_to_quantity(5.0);
    Quantity trade_size = double_to_quantity(2.0);
    
    // Buy increases long position
    Quantity new_size_buy = position_manager_->calculateNewPositionSize(
        user_id_, instrument_id_, current_size, trade_size, OrderSide::BUY);
    EXPECT_EQ(new_size_buy, current_size + trade_size);
    
    // Sell decreases long position
    Quantity new_size_sell = position_manager_->calculateNewPositionSize(
        user_id_, instrument_id_, current_size, trade_size, OrderSide::SELL);
    EXPECT_EQ(new_size_sell, current_size - trade_size);
    
    // Test with short position (negative)
    current_size = double_to_quantity(-3.0);
    new_size_buy = position_manager_->calculateNewPositionSize(
        user_id_, instrument_id_, current_size, trade_size, OrderSide::BUY);
    EXPECT_EQ(new_size_buy, current_size + trade_size); // -3 + 2 = -1
    
    new_size_sell = position_manager_->calculateNewPositionSize(
        user_id_, instrument_id_, current_size, trade_size, OrderSide::SELL);
    EXPECT_EQ(new_size_sell, current_size - trade_size); // -3 - 2 = -5
}

// Test multiple instruments
TEST_F(PositionManagerTest, MultipleInstruments) {
    InstrumentID inst1 = 1;
    InstrumentID inst2 = 2;
    InstrumentID inst3 = 3;
    
    // Update positions for different instruments
    position_manager_->updatePosition(user_id_, inst1, double_to_quantity(1.0), OrderSide::BUY);
    position_manager_->updatePosition(user_id_, inst2, double_to_quantity(2.0), OrderSide::BUY);
    position_manager_->updatePosition(user_id_, inst3, double_to_quantity(3.0), OrderSide::BUY);
    
    EXPECT_EQ(position_manager_->getPositionSize(user_id_, inst1), double_to_quantity(1.0));
    EXPECT_EQ(position_manager_->getPositionSize(user_id_, inst2), double_to_quantity(2.0));
    EXPECT_EQ(position_manager_->getPositionSize(user_id_, inst3), double_to_quantity(3.0));
    
    // Set different limits for each instrument
    position_manager_->setPositionLimit(user_id_, inst1, double_to_quantity(5.0));
    position_manager_->setPositionLimit(user_id_, inst2, double_to_quantity(10.0));
    position_manager_->setPositionLimit(user_id_, inst3, double_to_quantity(15.0));
    
    EXPECT_EQ(position_manager_->getPositionLimit(user_id_, inst1), double_to_quantity(5.0));
    EXPECT_EQ(position_manager_->getPositionLimit(user_id_, inst2), double_to_quantity(10.0));
    EXPECT_EQ(position_manager_->getPositionLimit(user_id_, inst3), double_to_quantity(15.0));
}

// Test multiple users
TEST_F(PositionManagerTest, MultipleUsers) {
    UserID user1 = 1000000;
    UserID user2 = 1000001;
    UserID user3 = 1000002;
    
    // Update positions for different users
    position_manager_->updatePosition(user1, instrument_id_, double_to_quantity(1.0), OrderSide::BUY);
    position_manager_->updatePosition(user2, instrument_id_, double_to_quantity(2.0), OrderSide::BUY);
    position_manager_->updatePosition(user3, instrument_id_, double_to_quantity(3.0), OrderSide::BUY);
    
    EXPECT_EQ(position_manager_->getPositionSize(user1, instrument_id_), double_to_quantity(1.0));
    EXPECT_EQ(position_manager_->getPositionSize(user2, instrument_id_), double_to_quantity(2.0));
    EXPECT_EQ(position_manager_->getPositionSize(user3, instrument_id_), double_to_quantity(3.0));
    
    // Operations on one user should not affect others
    position_manager_->updatePosition(user1, instrument_id_, double_to_quantity(0.5), OrderSide::SELL);
    EXPECT_EQ(position_manager_->getPositionSize(user1, instrument_id_), double_to_quantity(0.5));
    EXPECT_EQ(position_manager_->getPositionSize(user2, instrument_id_), double_to_quantity(2.0));
    EXPECT_EQ(position_manager_->getPositionSize(user3, instrument_id_), double_to_quantity(3.0));
}

// Test position closing
TEST_F(PositionManagerTest, PositionClosing) {
    // Create long position
    Quantity initial_qty = double_to_quantity(5.0);
    position_manager_->updatePosition(user_id_, instrument_id_, initial_qty, OrderSide::BUY);
    EXPECT_EQ(position_manager_->getPositionSize(user_id_, instrument_id_), initial_qty);
    
    // Close position by selling same amount
    position_manager_->updatePosition(user_id_, instrument_id_, initial_qty, OrderSide::SELL);
    EXPECT_EQ(position_manager_->getPositionSize(user_id_, instrument_id_), 0);
    
    // Create short position
    position_manager_->updatePosition(user_id_, instrument_id_, initial_qty, OrderSide::SELL);
    EXPECT_LT(position_manager_->getPositionSize(user_id_, instrument_id_), 0);
    
    // Close short position by buying
    position_manager_->updatePosition(user_id_, instrument_id_, initial_qty, OrderSide::BUY);
    EXPECT_EQ(position_manager_->getPositionSize(user_id_, instrument_id_), 0);
}

// Test edge cases
TEST_F(PositionManagerTest, EdgeCases) {
    // Test zero quantity update
    position_manager_->updatePosition(user_id_, instrument_id_, 0, OrderSide::BUY);
    EXPECT_EQ(position_manager_->getPositionSize(user_id_, instrument_id_), 0);
    
    // Test very small quantity
    Quantity tiny_qty = 1; // Minimum quantity
    position_manager_->updatePosition(user_id_, instrument_id_, tiny_qty, OrderSide::BUY);
    EXPECT_EQ(position_manager_->getPositionSize(user_id_, instrument_id_), tiny_qty);
    
    // Test position limit with zero limit
    position_manager_->setPositionLimit(user_id_, instrument_id_, 0);
    EXPECT_FALSE(position_manager_->checkPositionLimit(user_id_, instrument_id_, double_to_quantity(1.0), OrderSide::BUY));
    
    // Test position limit with very large limit
    Quantity large_limit = double_to_quantity(1000000.0);
    position_manager_->setPositionLimit(user_id_, instrument_id_, large_limit);
    EXPECT_TRUE(position_manager_->checkPositionLimit(user_id_, instrument_id_, double_to_quantity(1000.0), OrderSide::BUY));
}

// Test position limit update
TEST_F(PositionManagerTest, PositionLimitUpdate) {
    // Set initial limit
    Quantity limit1 = double_to_quantity(10.0);
    position_manager_->setPositionLimit(user_id_, instrument_id_, limit1);
    EXPECT_EQ(position_manager_->getPositionLimit(user_id_, instrument_id_), limit1);
    
    // Update limit
    Quantity limit2 = double_to_quantity(20.0);
    position_manager_->setPositionLimit(user_id_, instrument_id_, limit2);
    EXPECT_EQ(position_manager_->getPositionLimit(user_id_, instrument_id_), limit2);
    
    // Verify limit check uses new limit
    position_manager_->updatePosition(user_id_, instrument_id_, double_to_quantity(15.0), OrderSide::BUY);
    EXPECT_TRUE(position_manager_->checkPositionLimit(user_id_, instrument_id_, double_to_quantity(5.0), OrderSide::BUY));
    EXPECT_FALSE(position_manager_->checkPositionLimit(user_id_, instrument_id_, double_to_quantity(10.0), OrderSide::BUY));
}

