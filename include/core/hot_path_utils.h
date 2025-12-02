#pragma once

#include "types.h"
#include <algorithm>
#include <cstring>

namespace perpetual {

// Hot path utilities with aggressive optimizations
namespace HotPathUtils {

// Branchless min/max for better branch prediction
__attribute__((always_inline))
inline Quantity min_quantity(Quantity a, Quantity b) {
    return a < b ? a : b;
}

__attribute__((always_inline))
inline Price max_price(Price a, Price b) {
    return a > b ? a : b;
}

// Fast price comparison (avoid function call overhead)
__attribute__((always_inline))
inline bool price_greater_equal(Price a, Price b) {
    return a >= b;
}

// Fast quantity check (avoid function call overhead)
__attribute__((always_inline))
inline bool quantity_positive(Quantity qty) {
    return qty > 0;
}

// Memory prefetch hint for next order
__attribute__((always_inline))
inline void prefetch_order(const Order* order) {
    if (order) {
        __builtin_prefetch(order, 0, 3); // Read, high temporal locality
    }
}

// Branchless order side check
__attribute__((always_inline))
inline bool is_buy_side(OrderSide side) {
    return side == OrderSide::BUY;
}

// Fast order status check
__attribute__((always_inline))
inline bool is_order_active(OrderStatus status) {
    return status == OrderStatus::PENDING || status == OrderStatus::PARTIAL_FILLED;
}

// Optimized trade quantity calculation
__attribute__((hot))
inline Quantity calculate_trade_quantity(Quantity order_qty, Quantity resting_qty) {
    // Use branchless min
    return order_qty < resting_qty ? order_qty : resting_qty;
}

} // namespace HotPathUtils

} // namespace perpetual

