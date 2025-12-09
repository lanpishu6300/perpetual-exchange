#include "core/funding_rate.h"

namespace perpetual {

FundingRateCalculator::FundingRateCalculator() 
    : funding_interval_seconds_(8 * 3600) {  // Default 8 hours
}

FundingRateCalculator::~FundingRateCalculator() {
}

int64_t FundingRateCalculator::calculate_funding_rate(Price mark_price, Price index_price,
                                                      int64_t funding_rate_factor) const {
    if (index_price == 0) {
        return 0;
    }
    
    // Premium index = (Mark price - Index price) / Index price
    int64_t premium_index = ((mark_price - index_price) * PRICE_SCALE) / index_price;
    
    // Funding rate = Premium index * funding_rate_factor
    // funding_rate_factor is typically in basis points (e.g., 0.01% = 1 basis point)
    int64_t funding_rate = (premium_index * funding_rate_factor) / 10000;
    
    // Clamp funding rate to reasonable bounds (e.g., -1% to +1%)
    int64_t max_rate = PRICE_SCALE / 100;  // 1%
    int64_t min_rate = -max_rate;
    
    if (funding_rate > max_rate) {
        funding_rate = max_rate;
    } else if (funding_rate < min_rate) {
        funding_rate = min_rate;
    }
    
    return funding_rate;
}

int64_t FundingRateCalculator::calculate_funding_fee(Quantity position_size, Price mark_price,
                                                     int64_t funding_rate) const {
    if (position_size == 0 || funding_rate == 0) {
        return 0;
    }
    
    // Funding fee = Position size * Mark price * Funding rate
    // Result is scaled appropriately
    int64_t notional_value = (std::abs(position_size) * mark_price) / QTY_SCALE;
    int64_t fee = (notional_value * funding_rate) / PRICE_SCALE;
    
    // For long positions, positive funding rate means paying fee (positive)
    // For short positions, positive funding rate means receiving fee (negative)
    if (position_size < 0) {
        fee = -fee;
    }
    
    return fee;
}

} // namespace perpetual



namespace perpetual {

FundingRateCalculator::FundingRateCalculator() 
    : funding_interval_seconds_(8 * 3600) {  // Default 8 hours
}

FundingRateCalculator::~FundingRateCalculator() {
}

int64_t FundingRateCalculator::calculate_funding_rate(Price mark_price, Price index_price,
                                                      int64_t funding_rate_factor) const {
    if (index_price == 0) {
        return 0;
    }
    
    // Premium index = (Mark price - Index price) / Index price
    int64_t premium_index = ((mark_price - index_price) * PRICE_SCALE) / index_price;
    
    // Funding rate = Premium index * funding_rate_factor
    // funding_rate_factor is typically in basis points (e.g., 0.01% = 1 basis point)
    int64_t funding_rate = (premium_index * funding_rate_factor) / 10000;
    
    // Clamp funding rate to reasonable bounds (e.g., -1% to +1%)
    int64_t max_rate = PRICE_SCALE / 100;  // 1%
    int64_t min_rate = -max_rate;
    
    if (funding_rate > max_rate) {
        funding_rate = max_rate;
    } else if (funding_rate < min_rate) {
        funding_rate = min_rate;
    }
    
    return funding_rate;
}

int64_t FundingRateCalculator::calculate_funding_fee(Quantity position_size, Price mark_price,
                                                     int64_t funding_rate) const {
    if (position_size == 0 || funding_rate == 0) {
        return 0;
    }
    
    // Funding fee = Position size * Mark price * Funding rate
    // Result is scaled appropriately
    int64_t notional_value = (std::abs(position_size) * mark_price) / QTY_SCALE;
    int64_t fee = (notional_value * funding_rate) / PRICE_SCALE;
    
    // For long positions, positive funding rate means paying fee (positive)
    // For short positions, positive funding rate means receiving fee (negative)
    if (position_size < 0) {
        fee = -fee;
    }
    
    return fee;
}

} // namespace perpetual


