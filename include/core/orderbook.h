#pragma once

#include "types.h"
#include "order.h"
#include <map>
#include <unordered_map>
#include <vector>

namespace perpetual {

// Forward declaration
struct Order;

// Price level in the order book
struct PriceLevel {
    Price price;
    Quantity total_quantity;
    Order* first_order;
    Order* last_order;
    
    PriceLevel() : price(0), total_quantity(0), first_order(nullptr), last_order(nullptr) {}
};

// Order book side (bids or asks)
class OrderBookSide {
public:
    OrderBookSide(bool is_buy);
    ~OrderBookSide();
    
    // Insert order
    bool insert(Order* order);
    
    // Remove order
    bool remove(Order* order);
    
    // Get best price
    Price best_price() const;
    
    // Get best order
    Order* best_order() const;
    
    // Get price level
    PriceLevel* get_price_level(Price price);
    
    // Find order by order_id
    Order* find_order(OrderID order_id) const;
    
    // Check if empty
    bool empty() const;
    
    // Get size
    size_t size() const;
    
private:
    bool is_buy_;
    std::map<Price, PriceLevel, std::greater<Price>> price_levels_buy_;  // For bids (descending)
    std::map<Price, PriceLevel, std::less<Price>> price_levels_sell_;     // For asks (ascending)
    std::unordered_map<OrderID, Order*> orders_;
};

// Order book - maintains buy and sell sides
class OrderBook {
public:
    OrderBook(InstrumentID instrument_id);
    ~OrderBook();
    
    // Insert order
    bool insert_order(Order* order);
    
    // Remove order
    bool remove_order(Order* order);
    
    // Get bids side
    OrderBookSide& bids() { return bids_; }
    const OrderBookSide& bids() const { return bids_; }
    
    // Get asks side
    OrderBookSide& asks() { return asks_; }
    const OrderBookSide& asks() const { return asks_; }
    
    // Get best bid
    Price best_bid() const;
    
    // Get best ask
    Price best_ask() const;
    
    // Get spread
    Price spread() const;
    
    // Check if empty
    bool empty() const;
    
    // Check if order can match
    bool can_match(Order* order) const;
    
private:
    InstrumentID instrument_id_;
    OrderBookSide bids_;
    OrderBookSide asks_;
};

} // namespace perpetual
