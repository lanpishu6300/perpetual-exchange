#include "core/deterministic_calculator.h"
#include <limits>
#include <algorithm>

namespace perpetual {

int DeterministicCalculator::compare_prices(Price a, Price b) {
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

int DeterministicCalculator::compare_quantities(Quantity a, Quantity b) {
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

bool DeterministicCalculator::can_match(Price taker_price, Price maker_price, bool is_buy_order) {
    if (is_buy_order) {
        // For buy orders: taker_price >= maker_price
        return taker_price >= maker_price;
    } else {
        // For sell orders: taker_price <= maker_price
        return taker_price <= maker_price;
    }
}

Price DeterministicCalculator::calculate_match_price(Price taker_price, Price maker_price) {
    // Price-time priority: maker's price (resting order) takes precedence
    return maker_price;
}

Quantity DeterministicCalculator::calculate_trade_quantity(Quantity taker_remaining, 
                                                           Quantity maker_remaining) {
    // Deterministic min
    return (taker_remaining < maker_remaining) ? taker_remaining : maker_remaining;
}

Price DeterministicCalculator::calculate_pnl(Price entry_price, Price current_price,
                                             Quantity position_size, bool is_long) {
    if (position_size == 0) {
        return 0;
    }
    
    if (is_long) {
        // Long position: PnL = (current_price - entry_price) * position_size
        Price price_diff = current_price - entry_price;
        // Use fixed-point multiplication to avoid overflow
        return fixed_multiply(price_diff, position_size, QTY_SCALE);
    } else {
        // Short position: PnL = (entry_price - current_price) * position_size
        Price price_diff = entry_price - current_price;
        return fixed_multiply(price_diff, position_size, QTY_SCALE);
    }
}

Price DeterministicCalculator::calculate_margin(Price price, Quantity quantity,
                                                uint32_t margin_rate_bps) {
    // margin = (price * quantity * margin_rate) / (10000 * QTY_SCALE)
    // margin_rate_bps is in basis points (1/10000)
    
    // Calculate position value: price * quantity
    __int128_t position_value = static_cast<__int128_t>(price) * static_cast<__int128_t>(quantity);
    
    // Calculate margin: position_value * margin_rate_bps / 10000
    __int128_t margin = (position_value * margin_rate_bps) / 10000;
    
    // Scale down by QTY_SCALE
    margin = margin / static_cast<__int128_t>(QTY_SCALE);
    
    // Clamp to Price range
    if (margin > static_cast<__int128_t>(std::numeric_limits<Price>::max())) {
        return std::numeric_limits<Price>::max();
    }
    if (margin < static_cast<__int128_t>(std::numeric_limits<Price>::min())) {
        return std::numeric_limits<Price>::min();
    }
    
    return static_cast<Price>(margin);
}

Price DeterministicCalculator::calculate_funding_payment(Quantity position_size,
                                                         Price price,
                                                         int32_t funding_rate_bps) {
    // funding_payment = (position_size * price * funding_rate_bps) / (10000 * QTY_SCALE)
    // funding_rate_bps can be negative
    
    if (position_size == 0) {
        return 0;
    }
    
    __int128_t value = static_cast<__int128_t>(position_size) * static_cast<__int128_t>(price);
    __int128_t payment = (value * static_cast<__int128_t>(funding_rate_bps)) / 10000;
    payment = payment / static_cast<__int128_t>(QTY_SCALE);
    
    // Clamp to Price range
    if (payment > static_cast<__int128_t>(std::numeric_limits<Price>::max())) {
        return std::numeric_limits<Price>::max();
    }
    if (payment < static_cast<__int128_t>(std::numeric_limits<Price>::min())) {
        return std::numeric_limits<Price>::min();
    }
    
    return static_cast<Price>(payment);
}

Price DeterministicCalculator::calculate_liquidation_price(Price entry_price,
                                                           bool is_long,
                                                           uint32_t maintenance_margin_bps) {
    // For long: liquidation_price = entry_price * (1 - maintenance_margin_rate)
    // For short: liquidation_price = entry_price * (1 + maintenance_margin_rate)
    
    // maintenance_margin_rate = maintenance_margin_bps / 10000
    
    if (is_long) {
        // liquidation = entry * (10000 - maintenance_margin_bps) / 10000
        __int128_t liquidation = (static_cast<__int128_t>(entry_price) * 
                                  (10000 - maintenance_margin_bps)) / 10000;
        return static_cast<Price>(liquidation);
    } else {
        // liquidation = entry * (10000 + maintenance_margin_bps) / 10000
        __int128_t liquidation = (static_cast<__int128_t>(entry_price) * 
                                  (10000 + maintenance_margin_bps)) / 10000;
        return static_cast<Price>(liquidation);
    }
}

Timestamp DeterministicCalculator::sequence_to_timestamp(SequenceID sequence_id,
                                                         Timestamp base_timestamp) {
    // Deterministic timestamp: base + sequence * 1 nanosecond
    // This ensures deterministic ordering while maintaining timestamp semantics
    if (base_timestamp == 0) {
        // Use a fixed base timestamp (e.g., epoch start)
        base_timestamp = 1609459200000000000LL;  // 2021-01-01 00:00:00 UTC in nanoseconds
    }
    return base_timestamp + static_cast<Timestamp>(sequence_id);
}

SequenceID DeterministicCalculator::timestamp_to_sequence(Timestamp timestamp,
                                                           Timestamp base_timestamp) {
    if (base_timestamp == 0) {
        base_timestamp = 1609459200000000000LL;
    }
    if (timestamp < base_timestamp) {
        return 0;
    }
    return static_cast<SequenceID>(timestamp - base_timestamp);
}

__uint128_t DeterministicCalculator::make_sort_key(Price price, SequenceID sequence_id, bool is_buy_side) {
    // For bids (buy side): higher price first, then earlier sequence
    // For asks (sell side): lower price first, then earlier sequence
    
    __uint128_t key = 0;
    
    if (is_buy_side) {
        // For bids: invert price so higher prices sort first
        // Use: (MAX_PRICE - price) << 64 | sequence_id
        Price inverted_price = std::numeric_limits<Price>::max() - price;
        key = (static_cast<__uint128_t>(inverted_price) << 64) | static_cast<__uint128_t>(sequence_id);
    } else {
        // For asks: use price directly, lower prices sort first
        // Use: price << 64 | sequence_id
        key = (static_cast<__uint128_t>(price) << 64) | static_cast<__uint128_t>(sequence_id);
    }
    
    return key;
}

int DeterministicCalculator::compare_sort_keys(__uint128_t a, __uint128_t b) {
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

bool DeterministicCalculator::validate_price(Price price) {
    return price > 0 && price <= std::numeric_limits<Price>::max();
}

bool DeterministicCalculator::validate_quantity(Quantity quantity) {
    return quantity > 0 && quantity <= std::numeric_limits<Quantity>::max();
}

bool DeterministicCalculator::validate_calculation(Price price, Quantity quantity) {
    if (!validate_price(price) || !validate_quantity(quantity)) {
        return false;
    }
    
    // Check for overflow in price * quantity
    __int128_t product = static_cast<__int128_t>(price) * static_cast<__int128_t>(quantity);
    return product <= static_cast<__int128_t>(std::numeric_limits<int64_t>::max()) &&
           product >= static_cast<__int128_t>(std::numeric_limits<int64_t>::min());
}

bool DeterministicCalculator::price_overflow_check(Price a, Price b) {
    // Check if a + b would overflow
    if (a > 0 && b > std::numeric_limits<Price>::max() - a) {
        return false;  // Would overflow
    }
    if (a < 0 && b < std::numeric_limits<Price>::min() - a) {
        return false;  // Would underflow
    }
    return true;
}

} // namespace perpetual

