#include "core/funding_rate_manager.h"
#include "core/position_manager.h"
#include "core/account_manager.h"
#include "core/types.h"
#include <algorithm>
#include <cmath>

namespace perpetual {

FundingRateManager::FundingRateManager() {
    settlement_interval_ = 8 * 3600;  // 8 hours
    interest_rate_ = 0.0001;  // 0.01%
}

double FundingRateManager::calculateFundingRate(InstrumentID instrument_id,
                                               double premium_index,
                                               double interest_rate) {
    // Funding Rate = Premium Index + Interest Rate
    // In production, use more sophisticated calculation
    
    double funding_rate = premium_index + interest_rate;
    
    // Clamp to reasonable range (-0.75% to +0.75%)
    funding_rate = std::max(-0.0075, std::min(0.0075, funding_rate));
    
    std::lock_guard<std::mutex> lock(mutex_);
    current_funding_rates_[instrument_id] = funding_rate;
    
    // Record in history
    FundingRateRecord record;
    record.instrument_id = instrument_id;
    record.rate = funding_rate;
    record.premium_index = premium_index;
    record.interest_rate = interest_rate;
    record.timestamp = get_current_timestamp() / 1000000000;  // Convert to seconds
    record.settlement_time = record.timestamp;
    
    funding_rate_history_[instrument_id].push_back(record);
    
    // Keep only last 1000 records
    if (funding_rate_history_[instrument_id].size() > 1000) {
        funding_rate_history_[instrument_id].erase(
            funding_rate_history_[instrument_id].begin(),
            funding_rate_history_[instrument_id].begin() + 
            (funding_rate_history_[instrument_id].size() - 1000)
        );
    }
    
    return funding_rate;
}

double FundingRateManager::getCurrentFundingRate(InstrumentID instrument_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = current_funding_rates_.find(instrument_id);
    if (it != current_funding_rates_.end()) {
        return it->second;
    }
    return interest_rate_;  // Default rate
}

std::vector<FundingRateManager::FundingRateRecord> 
FundingRateManager::getFundingRateHistory(InstrumentID instrument_id,
                                         int64_t start_time,
                                         int64_t end_time) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = funding_rate_history_.find(instrument_id);
    if (it == funding_rate_history_.end()) {
        return {};
    }
    
    std::vector<FundingRateRecord> result;
    for (const auto& record : it->second) {
        if (record.timestamp >= start_time && record.timestamp <= end_time) {
            result.push_back(record);
        }
    }
    
    return result;
}

std::vector<FundingRateManager::FundingSettlement> 
FundingRateManager::settleFunding(InstrumentID instrument_id, Price mark_price) {
    std::vector<FundingSettlement> settlements;
    
    if (!position_manager_ || !account_manager_) {
        return settlements;
    }
    
    double funding_rate = getCurrentFundingRate(instrument_id);
    
    // Get all users with positions in this instrument
    // In production, maintain an index of users with open positions
    // For now, this is a placeholder - would need position manager to support iteration
    
    // Settlement calculation:
    // Payment = Position Size * Mark Price * Funding Rate
    // Positive rate: Long pays Short
    // Negative rate: Short pays Long
    
    // Placeholder: would iterate through all positions
    // For each position:
    //   Quantity position_size = position_manager_->getPositionSize(user_id, instrument_id);
    //   if (position_size != 0) {
    //       double position_value = (position_size / QTY_SCALE) * (mark_price / PRICE_SCALE);
    //       double payment = position_value * funding_rate;
    //       if (position_size > 0) {
    //           payment = -payment;  // Long pays when rate is positive
    //       }
    //       
    //       // Update account balance
    //       account_manager_->updateBalance(user_id, payment);
    //       
    //       FundingSettlement settlement;
    //       settlement.user_id = user_id;
    //       settlement.instrument_id = instrument_id;
    //       settlement.position_size = position_size;
    //       settlement.funding_rate = funding_rate;
    //       settlement.payment = payment;
    //       settlement.timestamp = get_current_timestamp() / 1000000000;
    //       settlements.push_back(settlement);
    //   }
    
    // Update next settlement time
    setNextSettlementTime(instrument_id, 
                         (get_current_timestamp() / 1000000000) + settlement_interval_);
    
    return settlements;
}

void FundingRateManager::updatePremiumIndex(InstrumentID instrument_id,
                                           Price best_bid, Price best_ask,
                                           Price mark_price) {
    double premium_index = calculatePremiumIndex(best_bid, best_ask, mark_price);
    
    std::lock_guard<std::mutex> lock(mutex_);
    premium_indices_[instrument_id] = premium_index;
    
    // Recalculate funding rate with new premium index
    calculateFundingRate(instrument_id, premium_index, interest_rate_);
}

double FundingRateManager::getPremiumIndex(InstrumentID instrument_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = premium_indices_.find(instrument_id);
    if (it != premium_indices_.end()) {
        return it->second;
    }
    return 0.0;
}

bool FundingRateManager::shouldSettle(InstrumentID instrument_id) const {
    int64_t next_time = getNextSettlementTime(instrument_id);
    if (next_time == 0) {
        // First settlement
        return true;
    }
    
    int64_t current_time = get_current_timestamp() / 1000000000;
    return current_time >= next_time;
}

void FundingRateManager::setNextSettlementTime(InstrumentID instrument_id, int64_t time) {
    std::lock_guard<std::mutex> lock(mutex_);
    next_settlement_times_[instrument_id] = time;
}

int64_t FundingRateManager::getNextSettlementTime(InstrumentID instrument_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = next_settlement_times_.find(instrument_id);
    if (it != next_settlement_times_.end()) {
        return it->second;
    }
    return 0;
}

double FundingRateManager::calculatePremiumIndex(Price best_bid, Price best_ask, Price mark_price) const {
    if (best_bid == 0 || best_ask == 0 || mark_price == 0) {
        return 0.0;
    }
    
    // Premium Index = (Best Bid + Best Ask) / 2 - Mark Price) / Mark Price
    double mid_price = (static_cast<double>(best_bid) + static_cast<double>(best_ask)) / 2.0;
    double mark = static_cast<double>(mark_price);
    
    return (mid_price - mark) / mark;
}

} // namespace perpetual

