#include "core/account_manager.h"
#include "core/logger.h"
#include <algorithm>

namespace perpetual {

AccountBalanceManager::AccountBalanceManager() {
}

double AccountBalanceManager::getBalance(UserID user_id) const {
    const auto& account = getAccount(user_id);
    return account.balance.load();
}

double AccountBalanceManager::getAvailableBalance(UserID user_id) const {
    const auto& account = getAccount(user_id);
    double balance = account.balance.load();
    double frozen = account.frozen.load();
    return std::max(0.0, balance - frozen);
}

double AccountBalanceManager::getUsedMargin(UserID user_id) const {
    const auto& account = getAccount(user_id);
    return account.used_margin.load();
}

bool AccountBalanceManager::hasSufficientBalance(UserID user_id, double required_amount) const {
    return getAvailableBalance(user_id) >= required_amount;
}

bool AccountBalanceManager::hasSufficientMargin(UserID user_id, double required_margin) const {
    double available = getAvailableBalance(user_id);
    double used = getUsedMargin(user_id);
    return (available - used) >= required_margin;
}

bool AccountBalanceManager::freezeBalance(UserID user_id, double amount) {
    if (amount <= 0) {
        return false;
    }
    
    auto& account = getOrCreateAccount(user_id);
    
    // Check if sufficient balance
    double current_balance = account.balance.load();
    double current_frozen = account.frozen.load();
    
    if (current_balance - current_frozen < amount) {
        return false;
    }
    
    // Atomic update
    double expected = current_frozen;
    while (!account.frozen.compare_exchange_weak(expected, expected + amount)) {
        expected = account.frozen.load();
        if (current_balance - expected < amount) {
            return false;
        }
    }
    
    return true;
}

void AccountBalanceManager::unfreezeBalance(UserID user_id, double amount) {
    if (amount <= 0) {
        return;
    }
    
    auto& account = getOrCreateAccount(user_id);
    
    // Atomic update
    double expected = account.frozen.load();
    while (expected > 0 && !account.frozen.compare_exchange_weak(expected, std::max(0.0, expected - amount))) {
        expected = account.frozen.load();
    }
}

bool AccountBalanceManager::updateBalance(UserID user_id, double delta) {
    auto& account = getOrCreateAccount(user_id);
    
    double expected = account.balance.load();
    double new_balance;
    
    do {
        new_balance = expected + delta;
        if (new_balance < 0) {
            return false; // Insufficient balance
        }
    } while (!account.balance.compare_exchange_weak(expected, new_balance));
    
    return true;
}

double AccountBalanceManager::calculateRequiredMargin(Price price, Quantity quantity, double leverage) const {
    if (leverage <= 0) {
        leverage = 1.0;
    }
    
    double price_double = price_to_double(price);
    double quantity_double = quantity_to_double(quantity);
    double notional = price_double * quantity_double;
    
    return notional / leverage;
}

void AccountBalanceManager::setBalance(UserID user_id, double balance) {
    auto& account = getOrCreateAccount(user_id);
    account.balance.store(balance);
}

AccountBalanceManager::AccountStats AccountBalanceManager::getAccountStats(UserID user_id) const {
    const auto& account = getAccount(user_id);
    
    AccountStats stats;
    stats.balance = account.balance.load();
    stats.frozen = account.frozen.load();
    stats.used_margin = account.used_margin.load();
    stats.available = std::max(0.0, stats.balance - stats.frozen);
    
    return stats;
}

AccountBalanceManager::AccountData& AccountBalanceManager::getOrCreateAccount(UserID user_id) {
    std::lock_guard<std::mutex> lock(accounts_mutex_);
    return accounts_[user_id];
}

const AccountBalanceManager::AccountData& AccountBalanceManager::getAccount(UserID user_id) const {
    std::lock_guard<std::mutex> lock(accounts_mutex_);
    auto it = accounts_.find(user_id);
    if (it == accounts_.end()) {
        static AccountData default_account;
        return default_account;
    }
    return it->second;
}


} // namespace perpetual
