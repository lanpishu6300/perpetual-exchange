#pragma once

#include "types.h"
#include <memory>
#include <functional>
#include <vector>

namespace perpetual {

// Forward declarations
class PositionManager;
class AccountBalanceManager;
struct Order;
struct Trade;

// Risk level and Liquidation result should be in namespace scope, not class scope
// Moving them outside class to avoid scope issues

// Liquidation strategy
enum class LiquidationStrategy {
    FULL_LIQUIDATION,
    PARTIAL_LIQUIDATION
};

// Risk level information
struct RiskLevel {
    double position_value = 0.0;
    double maintenance_margin = 0.0;
    double margin_ratio = 0.0;
    double risk_ratio = 0.0;
    double available_balance = 0.0;
    bool is_liquidatable = false;
};

// Liquidation result
struct LiquidationResult {
    UserID user_id = 0;
    InstrumentID instrument_id = 0;
    Price liquidation_price = 0;
    Quantity liquidated_quantity = 0;
    std::vector<Trade> trades;
    std::string error_message;
    bool success = false;
    double insurance_fund_used = 0.0;
};

// Liquidation engine for managing margin calls and liquidations
class LiquidationEngine {
public:
    LiquidationEngine();
    
    // Set dependencies
    void setPositionManager(PositionManager* pm) { position_manager_ = pm; }
    void setAccountManager(AccountBalanceManager* am) { account_manager_ = am; }
    void setMatchCallback(std::function<std::vector<Trade>(Order*)> cb) { match_callback_ = cb; }
    
    // Calculate risk level for a position
    RiskLevel calculateRiskLevel(UserID user_id, InstrumentID instrument_id, Price current_price) const;
    
    // Check if position should be liquidated
    bool shouldLiquidate(UserID user_id, InstrumentID instrument_id, Price current_price) const;
    
    // Execute liquidation
    LiquidationResult liquidate(UserID user_id, InstrumentID instrument_id, Price current_price,
                               LiquidationStrategy strategy = LiquidationStrategy::FULL_LIQUIDATION);
    
    // Calculate maintenance margin
    double calculateMaintenanceMargin(Quantity position_size, Price entry_price,
                                     Price current_price, double leverage) const;
    
private:
    // Create liquidation order
    std::unique_ptr<Order> createLiquidationOrder(UserID user_id, InstrumentID instrument_id,
                                                  Quantity quantity, OrderSide side);
    
    // Calculate liquidation price
    Price calculateLiquidationPrice(UserID user_id, InstrumentID instrument_id, Quantity position_size) const;
    
    // Check all positions for liquidation
    std::vector<UserID> checkAllPositions(Price current_price);
    
    // Margin ratios
    double maintenance_margin_ratio_;
    double liquidation_margin_ratio_;
    double insurance_fund_balance_;
    
    // Dependencies
    PositionManager* position_manager_ = nullptr;
    AccountBalanceManager* account_manager_ = nullptr;
    std::function<std::vector<Trade>(Order*)> match_callback_;
};

} // namespace perpetual

