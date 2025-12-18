#pragma once

#include "core/types.h"
#include "core/order.h"
#include <unordered_map>
#include <mutex>

namespace perpetual {

// Position manager for tracking and limiting positions
class PositionManager {
public:
    PositionManager();
    ~PositionManager();
    
    // Get position size for a user and instrument
    Quantity getPositionSize(UserID user_id, InstrumentID instrument_id) const;
    
    // Check if position limit allows new position
    bool checkPositionLimit(UserID user_id, InstrumentID instrument_id, 
                           Quantity quantity, OrderSide side) const;
    
    // Set position limit
    void setPositionLimit(UserID user_id, InstrumentID instrument_id, Quantity limit);
    
    // Get position limit
    Quantity getPositionLimit(UserID user_id, InstrumentID instrument_id) const;
    
    // Calculate new position size after trade
    Quantity calculateNewPositionSize(UserID user_id, InstrumentID instrument_id,
                                     Quantity current_size, Quantity trade_size, OrderSide side) const;
    
    // Update position size (called by matching engine)
    void updatePosition(UserID user_id, InstrumentID instrument_id, 
                       Quantity delta, OrderSide side);
    
private:
    // Position limits: (user_id, instrument_id) -> limit
    mutable std::mutex limits_mutex_;
    std::unordered_map<uint64_t, Quantity> position_limits_;
    
    // Current positions: (user_id, instrument_id) -> size
    mutable std::mutex positions_mutex_;
    std::unordered_map<uint64_t, Quantity> current_positions_;
    
    // Helper to create key
    uint64_t makeKey(UserID user_id, InstrumentID instrument_id) const {
        return (static_cast<uint64_t>(user_id) << 32) | static_cast<uint64_t>(instrument_id);
    }
};

} // namespace perpetual

