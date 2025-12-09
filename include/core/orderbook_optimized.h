#pragma once

#include "orderbook.h"
#include <unordered_map>
#include <vector>
#include <algorithm>

namespace perpetual {

// Optimized order book with cache-friendly data structures
class OrderBookOptimized : public OrderBook {
public:
    OrderBookOptimized(InstrumentID instrument_id);
    
    // Optimized best order retrieval with caching
    Order* get_best_order_optimized(bool is_buy) const;
    
    // Batch operations for better cache locality
    void batch_remove_orders(const std::vector<Order*>& orders);
    
    // Pre-allocate price levels
    void reserve_price_levels(size_t capacity);
    
private:
    // Cache best orders to avoid tree traversal
    mutable Order* cached_best_bid_;
    mutable Order* cached_best_ask_;
    mutable bool bid_cache_valid_;
    mutable bool ask_cache_valid_;
    
    // Price level cache
    mutable std::vector<PriceLevel*> price_level_cache_;
};

} // namespace perpetual




#include "orderbook.h"
#include <unordered_map>
#include <vector>
#include <algorithm>

namespace perpetual {

// Optimized order book with cache-friendly data structures
class OrderBookOptimized : public OrderBook {
public:
    OrderBookOptimized(InstrumentID instrument_id);
    
    // Optimized best order retrieval with caching
    Order* get_best_order_optimized(bool is_buy) const;
    
    // Batch operations for better cache locality
    void batch_remove_orders(const std::vector<Order*>& orders);
    
    // Pre-allocate price levels
    void reserve_price_levels(size_t capacity);
    
private:
    // Cache best orders to avoid tree traversal
    mutable Order* cached_best_bid_;
    mutable Order* cached_best_ask_;
    mutable bool bid_cache_valid_;
    mutable bool ask_cache_valid_;
    
    // Price level cache
    mutable std::vector<PriceLevel*> price_level_cache_;
};

} // namespace perpetual



