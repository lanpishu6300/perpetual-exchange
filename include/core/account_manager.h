#pragma once

#include "types.h"
#include "account.h"
#include <unordered_map>
#include <mutex>
#include <atomic>

namespace perpetual {

// Account balance manager - thread-safe balance operations
class AccountBalanceManager {
public:
    // Account data structure
    struct AccountData {
        std::atomic<double> balance{0.0};
        std::atomic<double> frozen{0.0};
        std::atomic<double> used_margin{0.0};
    };
    
    // Account statistics
    struct AccountStats {
        double balance = 0.0;
        double frozen = 0.0;
        double used_margin = 0.0;
        double available = 0.0;
    };
    
    AccountBalanceManager();
    
    // Get balance
    double getBalance(UserID user_id) const;
    
    // Get available balance (balance - frozen)
    double getAvailableBalance(UserID user_id) const;
    
    // Get used margin
    double getUsedMargin(UserID user_id) const;
    
    // Check if sufficient balance
    bool hasSufficientBalance(UserID user_id, double required_amount) const;
    
    // Check if sufficient margin
    bool hasSufficientMargin(UserID user_id, double required_margin) const;
    
    // Freeze balance
    bool freezeBalance(UserID user_id, double amount);
    
    // Unfreeze balance
    void unfreezeBalance(UserID user_id, double amount);
    
    // Update balance (add/subtract)
    bool updateBalance(UserID user_id, double delta);
    
    // Calculate required margin
    double calculateRequiredMargin(Price price, Quantity quantity, double leverage) const;
    
    // Set balance (direct set)
    void setBalance(UserID user_id, double balance);
    
    // Get account statistics
    AccountStats getAccountStats(UserID user_id) const;
    
private:
    // Get or create account
    AccountData& getOrCreateAccount(UserID user_id);
    
    // Get account (const)
    const AccountData& getAccount(UserID user_id) const;
    
    // Account storage
    std::unordered_map<UserID, AccountData> accounts_;
    mutable std::mutex accounts_mutex_;
};

} // namespace perpetual
