#pragma once

#include "matching_engine.h"
#include "orderbook_art.h"
#include "order.h"
#include "types.h"
#include <vector>
#include <unordered_map>
#include <memory>

namespace perpetual {

// Matching engine using ART-based order book
// This version uses Adaptive Radix Tree instead of Red-Black Tree
// Expected performance improvements:
// - Better cache locality
// - Lower memory overhead
// - Faster lookups for dense price ranges
class MatchingEngineART : public MatchingEngine {
public:
    MatchingEngineART(InstrumentID instrument_id);
    ~MatchingEngineART();
    
    // Process new order using ART order book
    std::vector<Trade> process_order_art(Order* order);
    
    // Get ART order book (for testing/comparison)
    OrderBookART& get_orderbook_art() { return orderbook_art_; }
    const OrderBookART& get_orderbook_art() const { return orderbook_art_; }
    
private:
    // Match order against opposite side using ART order book
    std::vector<Trade> match_order_art(Order* order);
    
    // Execute trade
    void execute_trade_art(Order* taker, Order* maker, Price price, Quantity quantity);
    
private:
    OrderBookART orderbook_art_;
    SequenceID trade_sequence_;
    std::unordered_map<OrderID, std::unique_ptr<Order>> orders_;
    std::unordered_map<UserID, std::vector<OrderID>> user_orders_;
    uint64_t total_trades_;
    double total_volume_;
};

} // namespace perpetual



