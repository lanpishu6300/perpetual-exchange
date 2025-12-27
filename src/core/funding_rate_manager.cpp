#include "core/funding_rate_manager.h"
#include "core/account_manager.h"
#include "core/position_manager.h"
#include "core/types.h"
#include <algorithm>
#include <cmath>

namespace perpetual {

FundingRateManager::FundingRateManager() {
}

FundingRateManager::~FundingRateManager() {
}

double FundingRateManager::calculateFundingRate(InstrumentID instrument_id, double premium_index, double interest_rate) {
    // Funding rate = premium_index + interest_rate
    // Clamp to -0.75% to +0.75%
    double rate = premium_index + interest_rate;
    return std::max(-0.0075, std::min(0.0075, rate));
}

void FundingRateManager::updatePremiumIndex(InstrumentID instrument_id, Price best_bid, Price best_ask, Price mark_price) {
    if (best_bid <= 0 || best_ask <= 0 || mark_price <= 0) {
        return;
    }
    
    double bid = price_to_double(best_bid);
    double ask = price_to_double(best_ask);
    double mark = price_to_double(mark_price);
    
    // Premium index = (best_bid + best_ask) / 2 - mark_price) / mark_price
    double mid_price = (bid + ask) / 2.0;
    double premium = (mid_price - mark) / mark;
    
    premium_indices_[instrument_id] = premium;
}

double FundingRateManager::getPremiumIndex(InstrumentID instrument_id) const {
    auto it = premium_indices_.find(instrument_id);
    if (it != premium_indices_.end()) {
        return it->second;
    }
    return 0.0;
}

bool FundingRateManager::shouldSettle(InstrumentID instrument_id) const {
    // Simple implementation: always return false for now
    // In production, this would check if current time >= next settlement time
    (void)instrument_id; // Suppress unused parameter warning
    return false;
}

void FundingRateManager::settleFunding(InstrumentID instrument_id, Price current_price) {
    // Placeholder implementation
    // In production, this would:
    // 1. Calculate funding payment for each position
    // 2. Update account balances
    // 3. Update next settlement time
    (void)instrument_id;
    (void)current_price;
}

} // namespace perpetual

