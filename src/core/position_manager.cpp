#include "core/position_manager.h"
#include "core/config.h"

namespace perpetual {

PositionManager::PositionManager() {
    auto& config = Config::getInstance();
    default_limit_ = double_to_quantity(config.getDouble(ConfigKeys::MAX_POSITION_SIZE, 1000000.0));
}

bool PositionManager::checkPositionLimit(UserID user_id, InstrumentID instrument_id,
                                        Quantity quantity, OrderSide side) const {
    PositionKey key{user_id, instrument_id};
    Quantity limit = getPositionLimit(user_id, instrument_id);
    
    // For now, we check absolute position size
    // In production, you'd get current position from PositionManager
    // This is a simplified check
    return quantity <= limit;
}

Quantity PositionManager::getPositionSize(UserID user_id, InstrumentID instrument_id) const {
    // In production, this would query the actual position
    // For now, return 0 as placeholder
    return 0;
}

Quantity PositionManager::getPositionLimit(UserID user_id, InstrumentID instrument_id) const {
    std::lock_guard<std::mutex> lock(limits_mutex_);
    
    PositionKey key{user_id, instrument_id};
    auto it = position_limits_.find(key);
    if (it != position_limits_.end()) {
        return it->second;
    }
    
    return default_limit_;
}

void PositionManager::setPositionLimit(UserID user_id, InstrumentID instrument_id, Quantity limit) {
    std::lock_guard<std::mutex> lock(limits_mutex_);
    PositionKey key{user_id, instrument_id};
    position_limits_[key] = limit;
}

Quantity PositionManager::calculateNewPositionSize(UserID user_id, InstrumentID instrument_id,
                                                  Quantity current_size, Quantity order_qty,
                                                  OrderSide order_side) const {
    // Calculate new position after order execution
    if (order_side == OrderSide::BUY) {
        return current_size + order_qty;
    } else {
        // SELL - reduce position (can go negative for short positions)
        return current_size - order_qty;
    }
}

} // namespace perpetual


