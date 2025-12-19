#pragma once

#include "core/types.h"
#include "core/order.h"
#include "core/account_manager.h"
#include "core/position_manager.h"
#include "core/liquidation_engine.h"
#include <unordered_map>
#include <mutex>
#include <memory>

namespace perpetual {

// Trading Shard: Handles account, order management, and liquidation checks
// Sharded by user_id for parallel processing
class TradingShard {
public:
    TradingShard(size_t shard_id);
    ~TradingShard();
    
    // Initialize shard
    bool initialize();
    
    // Validate and prepare order (account checks, balance checks, liquidation checks)
    bool validate_and_prepare_order(Order* order);
    
    // Update account and position after trade
    void update_after_trade(Order* order, const std::vector<Trade>& trades);
    
    // Check liquidation for user
    bool check_liquidation(UserID user_id, InstrumentID instrument_id, Price current_price);
    
    // Get account balance
    double get_balance(UserID user_id) const;
    
    // Get position size
    Quantity get_position_size(UserID user_id, InstrumentID instrument_id) const;
    
    // Get user orders
    std::vector<OrderID> get_user_orders(UserID user_id) const;
    
    // Add order to user's order list
    void add_user_order(UserID user_id, OrderID order_id);
    
    // Remove order from user's order list
    void remove_user_order(UserID user_id, OrderID order_id);
    
    // Get shard ID
    size_t get_shard_id() const { return shard_id_; }
    
private:
    size_t shard_id_;
    
    // Account management
    std::unique_ptr<AccountBalanceManager> account_manager_;
    
    // Position management
    std::unique_ptr<PositionManager> position_manager_;
    
    // Liquidation engine
    std::unique_ptr<LiquidationEngine> liquidation_engine_;
    
    // User orders index: user_id -> order_ids
    std::unordered_map<UserID, std::vector<OrderID>> user_orders_;
    mutable std::mutex user_orders_mutex_;
};

} // namespace perpetual

