#pragma once

#include "types.h"
#include <unordered_map>
#include <memory>

namespace perpetual {

// Forward declarations
class AccountBalanceManager;
class PositionManager;

// Funding rate manager for perpetual contracts
class FundingRateManager {
public:
    FundingRateManager();
    ~FundingRateManager();
    
    // Set dependencies
    void setAccountManager(AccountBalanceManager* am) { account_manager_ = am; }
    void setPositionManager(PositionManager* pm) { position_manager_ = pm; }
    
    // Calculate funding rate
    double calculateFundingRate(InstrumentID instrument_id, double premium_index, double interest_rate);
    
    // Premium index management
    void updatePremiumIndex(InstrumentID instrument_id, Price best_bid, Price best_ask, Price mark_price);
    double getPremiumIndex(InstrumentID instrument_id) const;
    
    // Settlement
    bool shouldSettle(InstrumentID instrument_id) const;
    void settleFunding(InstrumentID instrument_id, Price current_price);
    
private:
    AccountBalanceManager* account_manager_ = nullptr;
    PositionManager* position_manager_ = nullptr;
    
    // Premium index storage
    mutable std::unordered_map<InstrumentID, double> premium_indices_;
};

} // namespace perpetual

