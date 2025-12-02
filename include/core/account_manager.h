#pragma once

#include "account.h"
#include "types.h"
#include <unordered_map>
#include <mutex>
#include <atomic>

namespace perpetual {

// Account balance and margin management (enhanced version)
class AccountBalanceManager {
public:
    AccountBalanceManager();
    
    // Get account balance
    double getBalance(UserID user_id) const;
    
    // Get available balance (balance - frozen)
    double getAvailableBalance(UserID user_id) const;
    
    // Get margin used
    double getUsedMargin(UserID user_id) const;
    
    // Check if user has sufficient balance
    bool hasSufficientBalance(UserID user_id, double required_amount) const;
    
    // Check if user has sufficient margin
    bool hasSufficientMargin(UserID user_id, double required_margin) const;
    
    // Freeze balance for order
    bool freezeBalance(UserID user_id, double amount);
    
    // Unfreeze balance
    void unfreezeBalance(UserID user_id, double amount);
    
    // Update balance (for deposits/withdrawals)
    bool updateBalance(UserID user_id, double delta);
    
    // Calculate required margin for order
    double calculateRequiredMargin(Price price, Quantity quantity, double leverage) const;
    
    // Set account balance (for initialization)
    void setBalance(UserID user_id, double balance);
    
    // Get account statistics
    struct AccountStats {
        double balance = 0.0;
        double frozen = 0.0;
        double used_margin = 0.0;
        double available = 0.0;
    };
    
    AccountStats getAccountStats(UserID user_id) const;
    
private:
    struct AccountData {
        std::atomic<double> balance{0.0};
        std::atomic<double> frozen{0.0};
        std::atomic<double> used_margin{0.0};
        mutable std::mutex mutex;
    };
    
    mutable std::mutex accounts_mutex_;
    std::unordered_map<UserID, AccountData> accounts_;
    
    AccountData& getOrCreateAccount(UserID user_id);
    const AccountData& getAccount(UserID user_id) const;
};

} // namespace perpetual
