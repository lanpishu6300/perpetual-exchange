#include "core/orderbook_optimized.h"

namespace perpetual {

OrderBookOptimized::OrderBookOptimized(InstrumentID instrument_id)
    : OrderBook(instrument_id),
      cached_best_bid_(nullptr),
      cached_best_ask_(nullptr),
      bid_cache_valid_(false),
      ask_cache_valid_(false) {
}

Order* OrderBookOptimized::get_best_order_optimized(bool is_buy) const {
    if (is_buy) {
        if (bid_cache_valid_ && cached_best_bid_) {
            return cached_best_bid_;
        }
        cached_best_bid_ = bids().best_order();
        bid_cache_valid_ = true;
        return cached_best_bid_;
    } else {
        if (ask_cache_valid_ && cached_best_ask_) {
            return cached_best_ask_;
        }
        cached_best_ask_ = asks().best_order();
        ask_cache_valid_ = true;
        return cached_best_ask_;
    }
}

void OrderBookOptimized::batch_remove_orders(const std::vector<Order*>& orders) {
    // Batch removal for better cache locality
    for (Order* order : orders) {
        remove_order(order);
    }
    
    // Invalidate cache
    bid_cache_valid_ = false;
    ask_cache_valid_ = false;
}

void OrderBookOptimized::reserve_price_levels(size_t capacity) {
    price_level_cache_.reserve(capacity);
}

} // namespace perpetual




namespace perpetual {

OrderBookOptimized::OrderBookOptimized(InstrumentID instrument_id)
    : OrderBook(instrument_id),
      cached_best_bid_(nullptr),
      cached_best_ask_(nullptr),
      bid_cache_valid_(false),
      ask_cache_valid_(false) {
}

Order* OrderBookOptimized::get_best_order_optimized(bool is_buy) const {
    if (is_buy) {
        if (bid_cache_valid_ && cached_best_bid_) {
            return cached_best_bid_;
        }
        cached_best_bid_ = bids().best_order();
        bid_cache_valid_ = true;
        return cached_best_bid_;
    } else {
        if (ask_cache_valid_ && cached_best_ask_) {
            return cached_best_ask_;
        }
        cached_best_ask_ = asks().best_order();
        ask_cache_valid_ = true;
        return cached_best_ask_;
    }
}

void OrderBookOptimized::batch_remove_orders(const std::vector<Order*>& orders) {
    // Batch removal for better cache locality
    for (Order* order : orders) {
        remove_order(order);
    }
    
    // Invalidate cache
    bid_cache_valid_ = false;
    ask_cache_valid_ = false;
}

void OrderBookOptimized::reserve_price_levels(size_t capacity) {
    price_level_cache_.reserve(capacity);
}

} // namespace perpetual



