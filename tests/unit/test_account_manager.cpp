#include <gtest/gtest.h>
#include "core/account_manager.h"
#include "core/types.h"
#include <thread>
#include <vector>
#include <atomic>

using namespace perpetual;

class AccountManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        account_manager_ = std::make_unique<AccountBalanceManager>();
        user_id_ = 1000000;
    }
    
    std::unique_ptr<AccountBalanceManager> account_manager_;
    UserID user_id_;
};

// Test basic balance operations
TEST_F(AccountManagerTest, BasicBalanceOperations) {
    // Test initial balance (should be 0)
    EXPECT_DOUBLE_EQ(account_manager_->getBalance(user_id_), 0.0);
    
    // Set balance
    account_manager_->setBalance(user_id_, 10000.0);
    EXPECT_DOUBLE_EQ(account_manager_->getBalance(user_id_), 10000.0);
    
    // Update balance (add)
    bool result = account_manager_->updateBalance(user_id_, 5000.0);
    EXPECT_TRUE(result);
    EXPECT_DOUBLE_EQ(account_manager_->getBalance(user_id_), 15000.0);
    
    // Update balance (subtract)
    result = account_manager_->updateBalance(user_id_, -2000.0);
    EXPECT_TRUE(result);
    EXPECT_DOUBLE_EQ(account_manager_->getBalance(user_id_), 13000.0);
    
    // Try to subtract more than available (should fail)
    result = account_manager_->updateBalance(user_id_, -20000.0);
    EXPECT_FALSE(result);
    EXPECT_DOUBLE_EQ(account_manager_->getBalance(user_id_), 13000.0);
}

// Test available balance calculation
TEST_F(AccountManagerTest, AvailableBalance) {
    account_manager_->setBalance(user_id_, 10000.0);
    
    // Initially, available should equal balance
    EXPECT_DOUBLE_EQ(account_manager_->getAvailableBalance(user_id_), 10000.0);
    
    // Freeze some balance
    bool result = account_manager_->freezeBalance(user_id_, 3000.0);
    EXPECT_TRUE(result);
    EXPECT_DOUBLE_EQ(account_manager_->getAvailableBalance(user_id_), 7000.0);
    
    // Freeze more
    result = account_manager_->freezeBalance(user_id_, 2000.0);
    EXPECT_TRUE(result);
    EXPECT_DOUBLE_EQ(account_manager_->getAvailableBalance(user_id_), 5000.0);
    
    // Try to freeze more than available (should fail)
    result = account_manager_->freezeBalance(user_id_, 6000.0);
    EXPECT_FALSE(result);
    EXPECT_DOUBLE_EQ(account_manager_->getAvailableBalance(user_id_), 5000.0);
}

// Test freeze and unfreeze operations
TEST_F(AccountManagerTest, FreezeUnfreezeOperations) {
    account_manager_->setBalance(user_id_, 10000.0);
    
    // Freeze balance
    bool result = account_manager_->freezeBalance(user_id_, 5000.0);
    EXPECT_TRUE(result);
    EXPECT_DOUBLE_EQ(account_manager_->getAvailableBalance(user_id_), 5000.0);
    
    // Unfreeze balance
    account_manager_->unfreezeBalance(user_id_, 3000.0);
    EXPECT_DOUBLE_EQ(account_manager_->getAvailableBalance(user_id_), 8000.0);
    
    // Unfreeze remaining
    account_manager_->unfreezeBalance(user_id_, 2000.0);
    EXPECT_DOUBLE_EQ(account_manager_->getAvailableBalance(user_id_), 10000.0);
    
    // Unfreeze more than frozen (should not go negative)
    account_manager_->freezeBalance(user_id_, 1000.0);
    account_manager_->unfreezeBalance(user_id_, 2000.0);
    EXPECT_DOUBLE_EQ(account_manager_->getAvailableBalance(user_id_), 10000.0);
}

// Test sufficient balance check
TEST_F(AccountManagerTest, SufficientBalanceCheck) {
    account_manager_->setBalance(user_id_, 10000.0);
    
    // Should have sufficient balance
    EXPECT_TRUE(account_manager_->hasSufficientBalance(user_id_, 5000.0));
    EXPECT_TRUE(account_manager_->hasSufficientBalance(user_id_, 10000.0));
    EXPECT_FALSE(account_manager_->hasSufficientBalance(user_id_, 10001.0));
    
    // Freeze some balance
    account_manager_->freezeBalance(user_id_, 3000.0);
    EXPECT_TRUE(account_manager_->hasSufficientBalance(user_id_, 5000.0));
    EXPECT_FALSE(account_manager_->hasSufficientBalance(user_id_, 8000.0));
}

// Test margin calculations
TEST_F(AccountManagerTest, MarginCalculations) {
    account_manager_->setBalance(user_id_, 10000.0);
    
    // Test required margin calculation
    Price price = double_to_price(50000.0);
    Quantity quantity = double_to_quantity(0.1);
    double leverage = 10.0;
    
    double required_margin = account_manager_->calculateRequiredMargin(price, quantity, leverage);
    EXPECT_GT(required_margin, 0.0);
    EXPECT_LT(required_margin, 1000.0); // Should be less than position value
    
    // Test with different leverage
    double margin_5x = account_manager_->calculateRequiredMargin(price, quantity, 5.0);
    double margin_10x = account_manager_->calculateRequiredMargin(price, quantity, 10.0);
    EXPECT_GT(margin_5x, margin_10x); // Higher leverage = lower margin requirement
    
    // Test margin check
    account_manager_->setBalance(user_id_, 10000.0);
    EXPECT_TRUE(account_manager_->hasSufficientMargin(user_id_, 500.0));
    EXPECT_FALSE(account_manager_->hasSufficientMargin(user_id_, 15000.0));
}

// Test used margin tracking
TEST_F(AccountManagerTest, UsedMarginTracking) {
    account_manager_->setBalance(user_id_, 10000.0);
    
    // Initially, used margin should be 0
    EXPECT_DOUBLE_EQ(account_manager_->getUsedMargin(user_id_), 0.0);
    
    // Note: Used margin is typically updated by the matching engine
    // This test verifies the getter works
    EXPECT_DOUBLE_EQ(account_manager_->getUsedMargin(user_id_), 0.0);
}

// Test account statistics
TEST_F(AccountManagerTest, AccountStatistics) {
    account_manager_->setBalance(user_id_, 10000.0);
    account_manager_->freezeBalance(user_id_, 2000.0);
    
    auto stats = account_manager_->getAccountStats(user_id_);
    EXPECT_DOUBLE_EQ(stats.balance, 10000.0);
    EXPECT_DOUBLE_EQ(stats.frozen, 2000.0);
    EXPECT_DOUBLE_EQ(stats.available, 8000.0);
    EXPECT_DOUBLE_EQ(stats.used_margin, 0.0);
}

// Test multiple users
TEST_F(AccountManagerTest, MultipleUsers) {
    UserID user1 = 1000000;
    UserID user2 = 1000001;
    UserID user3 = 1000002;
    
    account_manager_->setBalance(user1, 10000.0);
    account_manager_->setBalance(user2, 20000.0);
    account_manager_->setBalance(user3, 30000.0);
    
    EXPECT_DOUBLE_EQ(account_manager_->getBalance(user1), 10000.0);
    EXPECT_DOUBLE_EQ(account_manager_->getBalance(user2), 20000.0);
    EXPECT_DOUBLE_EQ(account_manager_->getBalance(user3), 30000.0);
    
    // Operations on one user should not affect others
    account_manager_->freezeBalance(user1, 5000.0);
    EXPECT_DOUBLE_EQ(account_manager_->getAvailableBalance(user1), 5000.0);
    EXPECT_DOUBLE_EQ(account_manager_->getAvailableBalance(user2), 20000.0);
    EXPECT_DOUBLE_EQ(account_manager_->getAvailableBalance(user3), 30000.0);
}

// Test edge cases
TEST_F(AccountManagerTest, EdgeCases) {
    // Test zero balance
    account_manager_->setBalance(user_id_, 0.0);
    EXPECT_DOUBLE_EQ(account_manager_->getBalance(user_id_), 0.0);
    EXPECT_FALSE(account_manager_->hasSufficientBalance(user_id_, 0.1));
    
    // Test negative delta (should fail)
    account_manager_->setBalance(user_id_, 1000.0);
    bool result = account_manager_->updateBalance(user_id_, -2000.0);
    EXPECT_FALSE(result);
    EXPECT_DOUBLE_EQ(account_manager_->getBalance(user_id_), 1000.0);
    
    // Test freeze with zero amount (should fail)
    result = account_manager_->freezeBalance(user_id_, 0.0);
    EXPECT_FALSE(result);
    
    // Test unfreeze with zero amount (should not crash)
    account_manager_->unfreezeBalance(user_id_, 0.0);
    
    // Test invalid leverage in margin calculation
    Price price = double_to_price(50000.0);
    Quantity quantity = double_to_quantity(0.1);
    double margin = account_manager_->calculateRequiredMargin(price, quantity, 0.0);
    EXPECT_GT(margin, 0.0); // Should default to 1.0 leverage
}

// Test thread safety (basic concurrent access)
TEST_F(AccountManagerTest, ThreadSafety) {
    account_manager_->setBalance(user_id_, 10000.0);
    
    const int num_threads = 4;
    const int operations_per_thread = 100;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    
    // Concurrent freeze/unfreeze operations
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                if (j % 2 == 0) {
                    if (account_manager_->freezeBalance(user_id_, 10.0)) {
                        success_count++;
                    }
                } else {
                    account_manager_->unfreezeBalance(user_id_, 10.0);
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Balance should still be valid
    double final_balance = account_manager_->getBalance(user_id_);
    EXPECT_EQ(final_balance, 10000.0);
    
    // Available balance should be reasonable
    double available = account_manager_->getAvailableBalance(user_id_);
    EXPECT_GE(available, 0.0);
    EXPECT_LE(available, 10000.0);
}

