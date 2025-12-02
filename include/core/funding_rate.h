#pragma once

#include "types.h"
#include <vector>

namespace perpetual {

// Funding rate calculation for perpetual contracts
class FundingRateCalculator {
public:
    FundingRateCalculator();
    ~FundingRateCalculator();
    
    // Calculate funding rate based on premium index
    // Premium index = (Mark price - Index price) / Index price
    // Funding rate = Premium index * funding_rate_factor
    int64_t calculate_funding_rate(Price mark_price, Price index_price, 
                                   int64_t funding_rate_factor) const;
    
    // Calculate funding fee for a position
    // Funding fee = Position size * Mark price * Funding rate
    int64_t calculate_funding_fee(Quantity position_size, Price mark_price,
                                  int64_t funding_rate) const;
    
    // Get funding interval (typically 8 hours)
    int32_t get_funding_interval_seconds() const { return funding_interval_seconds_; }
    void set_funding_interval_seconds(int32_t seconds) { funding_interval_seconds_ = seconds; }
    
private:
    int32_t funding_interval_seconds_;  // Funding interval in seconds (default 8 hours)
};

// Funding payment record
struct FundingPayment {
    UserID user_id;
    InstrumentID instrument_id;
    int64_t funding_rate;      // Funding rate (scaled)
    int64_t funding_fee;       // Funding fee paid/received (positive = paid, negative = received)
    Timestamp timestamp;
    SequenceID sequence_id;
};

} // namespace perpetual
