#include "core/account.h"
#include <algorithm>
#include <mutex>

namespace perpetual {

void Account::deposit(int64_t amount) {
    if (amount > 0) {
        balance += amount;
        available_balance += amount;
    }
}

void Account::withdraw(int64_t amount) {
    if (amount > 0 && available_balance >= amount) {
        balance -= amount;
        available_balance -= amount;
    }
}

void Account::freeze(int64_t amount) {
    if (amount > 0 && available_balance >= amount) {
        available_balance -= amount;
        frozen_balance += amount;
    }
}

void Account::unfreeze(int64_t amount) {
    if (amount > 0 && frozen_balance >= amount) {
        frozen_balance -= amount;
        available_balance += amount;
    }
}

void Account::update_margin(int64_t margin_change) {
    total_margin += margin_change;
    if (total_margin < 0) {
        total_margin = 0;
    }
    update_margin_ratio();
}

void Account::update_realized_pnl(int64_t pnl) {
    realized_pnl += pnl;
    balance += pnl;
    available_balance += pnl;
}

void Account::update_unrealized_pnl(int64_t pnl) {
    int64_t pnl_diff = pnl - unrealized_pnl;
    unrealized_pnl = pnl;
    available_balance += pnl_diff;
}

bool Account::has_sufficient_balance(int64_t required) const {
    return available_balance >= required;
}

bool Account::has_sufficient_margin(int64_t required) const {
    return (available_balance - total_margin) >= required;
}

void Account::update_margin_ratio() {
    if (balance > 0) {
        margin_ratio = static_cast<double>(total_margin) / static_cast<double>(balance);
    } else {
        margin_ratio = 0.0;
    }
}

bool Account::should_liquidate(int64_t maintenance_margin_rate) const {
    if (balance <= 0) {
        return true;
    }
    
    int64_t total_equity = balance + unrealized_pnl;
    int64_t required_margin = (total_margin * maintenance_margin_rate) / 10000;  // maintenance_margin_rate in basis points
    
    return total_equity < required_margin;
}

// AccountManager implementation
AccountManager::AccountManager() {
}

AccountManager::~AccountManager() {
}

Account* AccountManager::get_account(UserID user_id, const std::string& currency) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& user_accounts = accounts_[user_id];
    auto it = user_accounts.find(currency);
    if (it == user_accounts.end()) {
        auto account = std::make_unique<Account>();
        account->user_id = user_id;
        account->currency = currency;
        Account* ptr = account.get();
        user_accounts[currency] = std::move(account);
        return ptr;
    }
    return it->second.get();
}

Account* AccountManager::find_account(UserID user_id, const std::string& currency) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto user_it = accounts_.find(user_id);
    if (user_it == accounts_.end()) {
        return nullptr;
    }
    
    auto it = user_it->second.find(currency);
    if (it == user_it->second.end()) {
        return nullptr;
    }
    
    return it->second.get();
}

bool AccountManager::check_balance(UserID user_id, const std::string& currency, int64_t required) {
    Account* account = find_account(user_id, currency);
    if (!account) {
        return false;
    }
    return account->has_sufficient_balance(required);
}

bool AccountManager::check_margin(UserID user_id, const std::string& currency, int64_t required) {
    Account* account = find_account(user_id, currency);
    if (!account) {
        return false;
    }
    return account->has_sufficient_margin(required);
}

void AccountManager::update_account_after_trade(UserID user_id, const std::string& currency,
                                                int64_t pnl, int64_t fee, int64_t margin_change) {
    Account* account = get_account(user_id, currency);
    if (account) {
        account->update_realized_pnl(pnl);
        account->withdraw(fee);  // Deduct fee
        account->update_margin(margin_change);
    }
}

} // namespace perpetual
