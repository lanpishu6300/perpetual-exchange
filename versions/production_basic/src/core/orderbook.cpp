#include "core/orderbook.h"
#include <algorithm>
#include <cassert>
#include <mutex>

namespace perpetual {

// OrderBookSide implementation (based on map, not red-black tree)
OrderBookSide::OrderBookSide(bool is_buy) : is_buy_(is_buy) {
}

OrderBookSide::~OrderBookSide() {
}

bool OrderBookSide::insert(Order* order) {
    if (!order || order->price <= 0 || order->quantity <= 0) {
        return false;
    }
    
    // Get appropriate price level map
    if (is_buy_) {
        auto& level = price_levels_buy_[order->price];
        level.price = order->price;
        level.total_quantity += order->remaining_quantity;
        if (!level.first_order) {
            level.first_order = order;
            level.last_order = order;
            order->prev_same_price = nullptr;
            order->next_same_price = nullptr;
        } else {
            level.last_order->next_same_price = order;
            order->prev_same_price = level.last_order;
            order->next_same_price = nullptr;
            level.last_order = order;
        }
    } else {
        auto& level = price_levels_sell_[order->price];
        level.price = order->price;
        level.total_quantity += order->remaining_quantity;
        if (!level.first_order) {
            level.first_order = order;
            level.last_order = order;
            order->prev_same_price = nullptr;
            order->next_same_price = nullptr;
        } else {
            level.last_order->next_same_price = order;
            order->prev_same_price = level.last_order;
            order->next_same_price = nullptr;
            level.last_order = order;
        }
    }
    
    orders_[order->order_id] = order;
    return true;
}

bool OrderBookSide::remove(Order* order) {
    if (!order) {
        return false;
    }
    
    auto it = orders_.find(order->order_id);
    if (it == orders_.end()) {
        return false;
    }
    
    // Remove from price level
    if (is_buy_) {
        auto level_it = price_levels_buy_.find(order->price);
        if (level_it != price_levels_buy_.end()) {
            PriceLevel& level = level_it->second;
            level.total_quantity -= order->remaining_quantity;
            
            // Update linked list
            if (order->prev_same_price) {
                order->prev_same_price->next_same_price = order->next_same_price;
            } else {
                level.first_order = order->next_same_price;
            }
            if (order->next_same_price) {
                order->next_same_price->prev_same_price = order->prev_same_price;
            } else {
                level.last_order = order->prev_same_price;
            }
            
            // Remove empty level
            if (level.total_quantity == 0) {
                price_levels_buy_.erase(level_it);
            }
        }
    } else {
        auto level_it = price_levels_sell_.find(order->price);
        if (level_it != price_levels_sell_.end()) {
            PriceLevel& level = level_it->second;
            level.total_quantity -= order->remaining_quantity;
            
            // Update linked list
            if (order->prev_same_price) {
                order->prev_same_price->next_same_price = order->next_same_price;
            } else {
                level.first_order = order->next_same_price;
            }
            if (order->next_same_price) {
                order->next_same_price->prev_same_price = order->prev_same_price;
            } else {
                level.last_order = order->prev_same_price;
            }
            
            // Remove empty level
            if (level.total_quantity == 0) {
                price_levels_sell_.erase(level_it);
            }
        }
    }
    
    orders_.erase(it);
    return true;
}

Price OrderBookSide::best_price() const {
    if (is_buy_) {
        if (price_levels_buy_.empty()) {
            return 0;
        }
        return price_levels_buy_.begin()->first;  // Highest bid
    } else {
        if (price_levels_sell_.empty()) {
            return 0;
        }
        return price_levels_sell_.begin()->first;  // Lowest ask
    }
}

Order* OrderBookSide::best_order() const {
    if (is_buy_) {
        if (price_levels_buy_.empty()) {
            return nullptr;
        }
        return price_levels_buy_.begin()->second.first_order;
    } else {
        if (price_levels_sell_.empty()) {
            return nullptr;
        }
        return price_levels_sell_.begin()->second.first_order;
    }
}

PriceLevel* OrderBookSide::get_price_level(Price price) {
    if (is_buy_) {
        auto it = price_levels_buy_.find(price);
        if (it != price_levels_buy_.end()) {
            return &it->second;
        }
    } else {
        auto it = price_levels_sell_.find(price);
        if (it != price_levels_sell_.end()) {
            return &it->second;
        }
    }
    return nullptr;
}

Order* OrderBookSide::find_order(OrderID order_id) const {
    auto it = orders_.find(order_id);
    if (it != orders_.end()) {
        return it->second;
    }
    return nullptr;
}

bool OrderBookSide::empty() const {
    return orders_.empty();
}

size_t OrderBookSide::size() const {
    return orders_.size();
}

// OrderBook implementation
OrderBook::OrderBook(InstrumentID instrument_id) 
    : instrument_id_(instrument_id), bids_(true), asks_(false) {
}

OrderBook::~OrderBook() {
}

bool OrderBook::insert_order(Order* order) {
    if (!order) {
        return false;
    }
    
    if (order->side == OrderSide::BUY) {
        return bids_.insert(order);
    } else {
        return asks_.insert(order);
    }
}

bool OrderBook::remove_order(Order* order) {
    if (!order) {
        return false;
    }
    
    if (order->side == OrderSide::BUY) {
        return bids_.remove(order);
    } else {
        return asks_.remove(order);
    }
}

Price OrderBook::best_bid() const {
    return bids_.best_price();
}

Price OrderBook::best_ask() const {
    return asks_.best_price();
}

Price OrderBook::spread() const {
    Price bid = best_bid();
    Price ask = best_ask();
    if (bid == 0 || ask == 0) {
        return 0;
    }
    return ask - bid;
}

bool OrderBook::empty() const {
    return bids_.empty() && asks_.empty();
}

bool OrderBook::can_match(Order* order) const {
    if (!order) return false;
    
    if (order->side == OrderSide::BUY) {
        Price ask = best_ask();
        return ask > 0 && (order->order_type == OrderType::MARKET || order->price >= ask);
    } else {
        Price bid = best_bid();
        return bid > 0 && (order->order_type == OrderType::MARKET || order->price <= bid);
    }
}

} // namespace perpetual
