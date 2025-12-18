#include "core/position_manager.h"
#include <algorithm>

namespace perpetual {

PositionManager::PositionManager() {
}

PositionManager::~PositionManager() {
}

Quantity PositionManager::getPositionSize(UserID user_id, InstrumentID instrument_id) const {
    std::lock_guard<std::mutex> lock(positions_mutex_);
    uint64_t key = makeKey(user_id, instrument_id);
    auto it = current_positions_.find(key);
    if (it != current_positions_.end()) {
        return it->second;
    }
    return 0;
}

bool PositionManager::checkPositionLimit(UserID user_id, InstrumentID instrument_id,
                                        Quantity quantity, OrderSide side) const {
    std::lock_guard<std::mutex> lock_limits(limits_mutex_);
    uint64_t key = makeKey(user_id, instrument_id);
    auto limit_it = position_limits_.find(key);
    
    // If no limit set, allow
    if (limit_it == position_limits_.end()) {
        return true;
    }
    
    Quantity limit = limit_it->second;
    
    // Get current position
    std::lock_guard<std::mutex> lock_positions(positions_mutex_);
    auto pos_it = current_positions_.find(key);
    Quantity current_size = (pos_it != current_positions_.end()) ? pos_it->second : 0;
    
    // Calculate new position size
    Quantity new_size = calculateNewPositionSize(user_id, instrument_id, current_size, quantity, side);
    
    // Check if exceeds limit
    return std::abs(new_size) <= limit;
}

void PositionManager::setPositionLimit(UserID user_id, InstrumentID instrument_id, Quantity limit) {
    std::lock_guard<std::mutex> lock(limits_mutex_);
    uint64_t key = makeKey(user_id, instrument_id);
    position_limits_[key] = limit;
}

Quantity PositionManager::getPositionLimit(UserID user_id, InstrumentID instrument_id) const {
    std::lock_guard<std::mutex> lock(limits_mutex_);
    uint64_t key = makeKey(user_id, instrument_id);
    auto it = position_limits_.find(key);
    if (it != position_limits_.end()) {
        return it->second;
    }
    return 0; // No limit
}

Quantity PositionManager::calculateNewPositionSize(UserID user_id, InstrumentID instrument_id,
                                                   Quantity current_size, Quantity trade_size,
                                                   OrderSide side) const {
    if (side == OrderSide::BUY) {
        // Buying increases long position or decreases short position
        return current_size + trade_size;
    } else {
        // Selling increases short position or decreases long position
        return current_size - trade_size;
    }
}

void PositionManager::updatePosition(UserID user_id, InstrumentID instrument_id,
                                    Quantity delta, OrderSide side) {
    std::lock_guard<std::mutex> lock(positions_mutex_);
    uint64_t key = makeKey(user_id, instrument_id);
    
    if (side == OrderSide::BUY) {
        current_positions_[key] += delta;
    } else {
        current_positions_[key] -= delta;
    }
}

} // namespace perpetual

