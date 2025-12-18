#pragma once

#include "core/types.h"
#include <atomic>
#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>

namespace perpetual {

// Account balance and margin information
struct Account {
    UserID user_id;
    std::string currency;  // Settlement currency (e.g., "USDT")
    
    // Balance
    int64_t balance;           // Total balance
    int64_t available_balance; // Available balance (balance - used_margin - frozen)
    int64_t frozen_balance;    // Frozen balance (for open orders)
    
    // Margin
    int64_t total_margin;      // Total margin used across all positions
    int64_t available_margin;  // Available margin for new positions
    
    // PnL
    int64_t realized_pnl;      // Realized profit and loss
    int64_t unrealized_pnl;    // Unrealized profit and loss from positions
    
    // Risk metrics
    double margin_ratio;       // Margin ratio (used_margin / balance)
    bool is_liquidating;       // Flag indicating liquidation status
    
    Account()
        : user_id(0), currency("USDT")
        , balance(0), available_balance(0), frozen_balance(0)
        , total_margin(0), available_margin(0)
        , realized_pnl(0), unrealized_pnl(0)
        , margin_ratio(0.0), is_liquidating(false) {}
    
    // Update balance after deposit/withdrawal
    void deposit(int64_t amount);
    void withdraw(int64_t amount);
    
    // Freeze/unfreeze balance for orders
    void freeze(int64_t amount);
    void unfreeze(int64_t amount);
    
    // Update margin
    void update_margin(int64_t margin_change);
    
    // Update PnL
    void update_realized_pnl(int64_t pnl);
    void update_unrealized_pnl(int64_t pnl);
    
    // Check if has sufficient balance
    bool has_sufficient_balance(int64_t required) const;
    
    // Check if has sufficient margin
    bool has_sufficient_margin(int64_t required) const;
    
    // Calculate margin ratio
    void update_margin_ratio();
    
    // Check if should be liquidated
    bool should_liquidate(int64_t maintenance_margin_rate) const;
};

// Account manager
class AccountManager {
public:
    AccountManager();
    ~AccountManager();
    
    // Get or create account
    Account* get_account(UserID user_id, const std::string& currency);
    
    // Get account (returns nullptr if not exists)
    Account* find_account(UserID user_id, const std::string& currency) const;
    
    // Check sufficient balance
    bool check_balance(UserID user_id, const std::string& currency, int64_t required);
    
    // Check sufficient margin
    bool check_margin(UserID user_id, const std::string& currency, int64_t required);
    
    // Update account after trade
    void update_account_after_trade(UserID user_id, const std::string& currency,
                                    int64_t pnl, int64_t fee, int64_t margin_change);
    
private:
    std::unordered_map<UserID, std::unordered_map<std::string, std::unique_ptr<Account>>> accounts_;
    mutable std::mutex mutex_;
};

} // namespace perpetual
