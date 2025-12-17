#pragma once

#include "matching_engine.h"
#include "orderbook_art.h"
#include "types.h"
#include <vector>

namespace perpetual {

// Matching engine using Adaptive Radix Tree (ART) for order book
// Provides better cache locality and O(k) complexity
class MatchingEngineART : public MatchingEngine {
public:
    MatchingEngineART(InstrumentID instrument_id);
    ~MatchingEngineART();
    
    // Process order using ART-based order book
    std::vector<Trade> process_order_art(Order* order);
    
    // Get ART order book
    OrderBookART& get_orderbook_art() { return orderbook_art_; }
    const OrderBookART& get_orderbook_art() const { return orderbook_art_; }
    
protected:
    // Match order using ART
    std::vector<Trade> match_order_art(Order* order);
    
    // Execute trade
    void execute_trade_art(Order* taker, Order* maker, Price price, Quantity quantity);
    
private:
    OrderBookART orderbook_art_;
    uint64_t trade_sequence_;
    uint64_t total_trades_;
    double total_volume_;
};

} // namespace perpetual

