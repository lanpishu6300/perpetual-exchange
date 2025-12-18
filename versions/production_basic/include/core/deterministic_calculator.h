#pragma once

#include "core/types.h"
#include "core/order.h"
#include <cstdint>
#include <limits>

namespace perpetual {

// Deterministic Calculator for Event Sourcing
// All calculations are deterministic and reproducible
// Uses fixed-point arithmetic to avoid floating-point non-determinism
class DeterministicCalculator {
public:
    DeterministicCalculator() = default;
    ~DeterministicCalculator() = default;
    
    // Deterministic price comparison
    // Returns: -1 if a < b, 0 if a == b, 1 if a > b
    static int compare_prices(Price a, Price b);
    
    // Deterministic quantity comparison
    static int compare_quantities(Quantity a, Quantity b);
    
    // Check if orders can match (deterministic)
    // For buy orders: taker_price >= maker_price
    // For sell orders: taker_price <= maker_price
    static bool can_match(Price taker_price, Price maker_price, bool is_buy_order);
    
    // Calculate match price (price-time priority, deterministic)
    // Returns the maker's price (resting order price)
    static Price calculate_match_price(Price taker_price, Price maker_price);
    
    // Calculate trade quantity (min of remaining quantities, deterministic)
    static Quantity calculate_trade_quantity(Quantity taker_remaining, 
                                            Quantity maker_remaining);
    
    // Calculate PnL (deterministic, using fixed-point arithmetic)
    // PnL = (current_price - entry_price) * position_size
    static Price calculate_pnl(Price entry_price, Price current_price, 
                              Quantity position_size, bool is_long);
    
    // Calculate margin requirement (deterministic)
    // margin = position_value * margin_rate
    // position_value = price * quantity
    static Price calculate_margin(Price price, Quantity quantity, 
                                 uint32_t margin_rate_bps);  // margin_rate in basis points (1/10000)
    
    // Calculate funding rate payment (deterministic)
    // funding_payment = position_size * funding_rate * price
    static Price calculate_funding_payment(Quantity position_size, 
                                          Price price, 
                                          int32_t funding_rate_bps);  // can be negative
    
    // Calculate liquidation price (deterministic)
    // For long: liquidation_price = entry_price * (1 - maintenance_margin_rate)
    // For short: liquidation_price = entry_price * (1 + maintenance_margin_rate)
    static Price calculate_liquidation_price(Price entry_price, 
                                            bool is_long,
                                            uint32_t maintenance_margin_bps);
    
    // Deterministic timestamp from sequence (for Event Sourcing)
    // Uses sequence_id as deterministic timestamp source
    static Timestamp sequence_to_timestamp(SequenceID sequence_id, 
                                          Timestamp base_timestamp = 0);
    
    // Deterministic sequence from timestamp (reverse mapping)
    static SequenceID timestamp_to_sequence(Timestamp timestamp, 
                                          Timestamp base_timestamp = 0);
    
    // Deterministic sorting key for price-time priority
    // Returns a combined key: (price << 64) | sequence_id
    // Higher price (for bids) or lower price (for asks) comes first
    // Then earlier sequence_id comes first
    static __uint128_t make_sort_key(Price price, SequenceID sequence_id, bool is_buy_side);
    
    // Compare sort keys (deterministic)
    // Returns: -1 if a < b, 0 if a == b, 1 if a > b
    static int compare_sort_keys(__uint128_t a, __uint128_t b);
    
    // Fixed-point multiplication (deterministic)
    // result = (a * b) / scale
    template<typename T>
    static T fixed_multiply(T a, T b, T scale) {
        // Use 128-bit intermediate to avoid overflow
        __int128_t result = static_cast<__int128_t>(a) * static_cast<__int128_t>(b);
        return static_cast<T>(result / static_cast<__int128_t>(scale));
    }
    
    // Fixed-point division (deterministic)
    // result = (a * scale) / b
    template<typename T>
    static T fixed_divide(T a, T b, T scale) {
        if (b == 0) return 0;
        __int128_t result = static_cast<__int128_t>(a) * static_cast<__int128_t>(scale);
        return static_cast<T>(result / static_cast<__int128_t>(b));
    }
    
    // Validate calculation inputs (prevent overflow/underflow)
    static bool validate_price(Price price);
    static bool validate_quantity(Quantity quantity);
    static bool validate_calculation(Price price, Quantity quantity);
    
private:
    // Helper for price comparison with overflow protection
    static bool price_overflow_check(Price a, Price b);
};

} // namespace perpetual
