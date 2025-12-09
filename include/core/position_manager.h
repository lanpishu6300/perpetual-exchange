#pragma once

#include "position.h"
#include "types.h"
#include "config.h"
#include <unordered_map>
#include <mutex>
#include <atomic>

namespace perpetual {

// Position limit management
class PositionManager {
public:
    PositionManager();
    
    // Check if position limit allows new position
    bool checkPositionLimit(UserID user_id, InstrumentID instrument_id, 
                           Quantity quantity, OrderSide side) const;
    
    // Get current position size
    Quantity getPositionSize(UserID user_id, InstrumentID instrument_id) const;
    
    // Get position limit for user/instrument
    Quantity getPositionLimit(UserID user_id, InstrumentID instrument_id) const;
    
    // Set position limit
    void setPositionLimit(UserID user_id, InstrumentID instrument_id, Quantity limit);
    
    // Set default position limit
    void setDefaultPositionLimit(Quantity limit) { default_limit_ = limit; }
    
    // Calculate new position size after order
    Quantity calculateNewPositionSize(UserID user_id, InstrumentID instrument_id,
                                      Quantity current_size, Quantity order_qty,
                                      OrderSide order_side) const;
    
private:
    struct PositionKey {
        UserID user_id;
        InstrumentID instrument_id;
        
        bool operator==(const PositionKey& other) const {
            return user_id == other.user_id && instrument_id == other.instrument_id;
        }
    };
    
    struct PositionKeyHash {
        size_t operator()(const PositionKey& key) const {
            return std::hash<UserID>()(key.user_id) ^ 
                   (std::hash<InstrumentID>()(key.instrument_id) << 1);
        }
    };
    
    mutable std::mutex limits_mutex_;
    std::unordered_map<PositionKey, Quantity, PositionKeyHash> position_limits_;
    Quantity default_limit_ = 1000000; // Default limit
};

} // namespace perpetual



