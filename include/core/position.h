#pragma once

#include "types.h"
#include <atomic>

namespace perpetual {

// Position for a user and instrument
struct Position {
    UserID user_id;
    InstrumentID instrument_id;
    PositionSide side;
    
    // Position size (positive for long, negative for short)
    Quantity size;  // Net position size
    
    // Position details
    Quantity long_size;      // Long position
    Quantity short_size;     // Short position
    Price avg_open_price;    // Average opening price
    
    // Unrealized PnL
    int64_t unrealized_pnl;  // In settlement currency (scaled)
    
    // Margin
    int64_t used_margin;     // Used margin
    int64_t available_margin; // Available margin
    
    // Leverage
    int32_t leverage;        // Position leverage (e.g., 10 for 10x)
    
    // Liquidation
    Price liquidation_price; // Estimated liquidation price
    bool is_liquidating;     // Flag for liquidation status
    
    Position()
        : user_id(0), instrument_id(0), side(PositionSide::NET)
        , size(0), long_size(0), short_size(0), avg_open_price(0)
        , unrealized_pnl(0), used_margin(0), available_margin(0)
        , leverage(1), liquidation_price(0), is_liquidating(false) {}
    
    // Update position after trade
    void update_position(OrderSide trade_side, Quantity trade_size, Price trade_price, 
                         OffsetFlag offset_flag, int32_t contract_multiplier);
    
    // Calculate unrealized PnL based on mark price
    void update_unrealized_pnl(Price mark_price, bool is_inverse);
    
    // Calculate required margin
    int64_t calculate_margin(Price mark_price, int32_t contract_multiplier) const;
    
    // Check if position should be liquidated
    bool should_liquidate(Price mark_price, int64_t account_balance, 
                          int64_t maintenance_margin_rate) const;
    
    // Get liquidation price
    Price calculate_liquidation_price(int64_t account_balance, 
                                      int64_t maintenance_margin_rate,
                                      int32_t contract_multiplier,
                                      bool is_inverse) const;
};

} // namespace perpetual
