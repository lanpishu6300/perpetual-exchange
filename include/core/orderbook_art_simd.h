#pragma once

#include "order.h"
#include "orderbook.h"  // For PriceLevel
#include "art_tree_simd.h"
#include <map>
#include <unordered_map>
#include <mutex>
#include <vector>

namespace perpetual {

// Order book side using ART with SIMD optimizations
class OrderBookSideARTSIMD : public OrderBookSideART {
public:
    OrderBookSideARTSIMD(bool is_buy);
    ~OrderBookSideARTSIMD();
    
    // SIMD-optimized best price lookup
    Price best_price_simd() const;
    
    // SIMD-optimized batch price level lookup
    void get_best_prices_simd(size_t n, std::vector<Price>& prices) const;
    
private:
    ARTTreeSIMD art_tree_simd_;
};

// Full order book using ART with SIMD
class OrderBookARTSIMD {
public:
    OrderBookARTSIMD(InstrumentID instrument_id);
    ~OrderBookARTSIMD();
    
    // Insert order
    bool insert_order(Order* order);
    
    // Remove order
    bool remove_order(Order* order);
    
    // Update order
    bool update_order(Order* order, Price new_price, Quantity new_quantity);
    
    // Get best bid and ask (SIMD optimized)
    Price best_bid() const { return bids_.best_price(); }
    Price best_ask() const { return asks_.best_price(); }
    
    // Get spread
    Price spread() const;
    
    // Get mid price
    Price mid_price() const;
    
    // Check if order can match
    bool can_match(Order* order) const;
    
    // Get instrument ID
    InstrumentID instrument_id() const { return instrument_id_; }
    
    // Get order book depth
    void get_depth(size_t n, std::vector<PriceLevel>& bids, 
                   std::vector<PriceLevel>& asks) const;
    
    // Access order book sides
    OrderBookSideARTSIMD& bids() { return bids_; }
    OrderBookSideARTSIMD& asks() { return asks_; }
    const OrderBookSideARTSIMD& bids() const { return bids_; }
    const OrderBookSideARTSIMD& asks() const { return asks_; }
    
private:
    InstrumentID instrument_id_;
    OrderBookSideARTSIMD bids_;
    OrderBookSideARTSIMD asks_;
};

} // namespace perpetual

