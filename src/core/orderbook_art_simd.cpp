#include "core/orderbook_art_simd.h"
#include <algorithm>

namespace perpetual {

OrderBookSideARTSIMD::OrderBookSideARTSIMD(bool is_buy) : is_buy_(is_buy) {
}

OrderBookSideARTSIMD::~OrderBookSideARTSIMD() {
}

bool OrderBookSideARTSIMD::price_better(Price a, Price b) const {
    if (is_buy_) {
        return a > b;  // For bids, higher is better
    } else {
        return a < b;  // For asks, lower is better
    }
}

bool OrderBookSideARTSIMD::insert(Order* order) {
    if (!order || order->price <= 0 || order->quantity <= 0) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Add to price level
    PriceLevel* level = get_or_create_price_level(order->price);
    add_order_to_price_level(level, order);
    
    // Insert into ART tree (if not already present)
    void* existing = art_tree_simd_.find_simd(order->price);
    if (existing == nullptr) {
        art_tree_simd_.insert(order->price, level);
    }
    
    // Add to order map
    order_map_[order->order_id] = order;
    
    return true;
}

bool OrderBookSideARTSIMD::remove(Order* order) {
    if (!order || order_map_.find(order->order_id) == order_map_.end()) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Remove from price level
    PriceLevel* level = get_or_create_price_level(order->price);
    remove_order_from_price_level(level, order);
    
    // Remove price level if empty
    remove_price_level_if_empty(order->price);
    
    // Remove from order map
    order_map_.erase(order->order_id);
    
    return true;
}

bool OrderBookSideARTSIMD::update_quantity(Order* order, Quantity new_quantity) {
    if (!order || order_map_.find(order->order_id) == order_map_.end()) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    PriceLevel* level = get_or_create_price_level(order->price);
    level->total_quantity -= order->quantity;
    order->quantity = new_quantity;
    level->total_quantity += new_quantity;
    
    return true;
}

Price OrderBookSideARTSIMD::best_price() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (art_tree_simd_.empty()) {
        return 0;
    }
    
    if (is_buy_) {
        return art_tree_simd_.max_key();  // Highest bid
    } else {
        return art_tree_simd_.min_key();  // Lowest ask
    }
}

Quantity OrderBookSideARTSIMD::best_quantity() const {
    Price price = best_price();
    if (price == 0) {
        return 0;
    }
    
    auto it = price_levels_.find(price);
    if (it != price_levels_.end()) {
        return it->second.total_quantity;
    }
    return 0;
}

Order* OrderBookSideARTSIMD::best_order() const {
    Price price = best_price_simd();
    if (price == 0) {
        return nullptr;
    }
    
    auto it = price_levels_.find(price);
    if (it != price_levels_.end() && it->second.first_order != nullptr) {
        return it->second.first_order;
    }
    return nullptr;
}

PriceLevel* OrderBookSideARTSIMD::best_level() const {
    Price price = best_price_simd();
    if (price == 0) {
        return nullptr;
    }
    
    auto it = price_levels_.find(price);
    if (it != price_levels_.end()) {
        return const_cast<PriceLevel*>(&it->second);
    }
    return nullptr;
}

Order* OrderBookSideARTSIMD::find_order(OrderID order_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = order_map_.find(order_id);
    if (it != order_map_.end()) {
        return it->second;
    }
    return nullptr;
}

void OrderBookSideARTSIMD::get_depth(size_t n, std::vector<PriceLevel>& levels) const {
    std::lock_guard<std::mutex> lock(mutex_);
    levels.clear();
    levels.reserve(n);
    
    // Collect prices
    std::vector<Price> prices;
    prices.reserve(price_levels_.size());
    
    for (const auto& pair : price_levels_) {
        prices.push_back(pair.first);
    }
    
    // Sort based on buy/sell
    if (is_buy_) {
        std::sort(prices.rbegin(), prices.rend());  // Descending for bids
    } else {
        std::sort(prices.begin(), prices.end());  // Ascending for asks
    }
    
    // Get top N
    size_t count = std::min(n, prices.size());
    for (size_t i = 0; i < count; ++i) {
        auto it = price_levels_.find(prices[i]);
        if (it != price_levels_.end()) {
            levels.push_back(it->second);
        }
    }
}

PriceLevel* OrderBookSideARTSIMD::get_or_create_price_level(Price price) {
    auto it = price_levels_.find(price);
    if (it != price_levels_.end()) {
        return &it->second;
    }
    
    PriceLevel level;
    level.price = price;
    level.total_quantity = 0;
    level.first_order = nullptr;
    level.last_order = nullptr;
    
    auto result = price_levels_.emplace(price, level);
    return &result.first->second;
}

void OrderBookSideARTSIMD::remove_price_level_if_empty(Price price) {
    auto it = price_levels_.find(price);
    if (it != price_levels_.end() && it->second.first_order == nullptr) {
        price_levels_.erase(it);
        art_tree_simd_.remove(price);
    }
}

void OrderBookSideARTSIMD::add_order_to_price_level(PriceLevel* level, Order* order) {
    if (level->first_order == nullptr) {
        level->first_order = order;
        level->last_order = order;
        order->next_same_price = nullptr;
        order->prev_same_price = nullptr;
    } else {
        order->prev_same_price = level->last_order;
        order->next_same_price = nullptr;
        level->last_order->next_same_price = order;
        level->last_order = order;
    }
    level->total_quantity += order->quantity;
}

void OrderBookSideARTSIMD::remove_order_from_price_level(PriceLevel* level, Order* order) {
    if (order->prev_same_price) {
        order->prev_same_price->next_same_price = order->next_same_price;
    } else {
        level->first_order = order->next_same_price;
    }
    
    if (order->next_same_price) {
        order->next_same_price->prev_same_price = order->prev_same_price;
    } else {
        level->last_order = order->prev_same_price;
    }
    
    level->total_quantity -= order->quantity;
    order->next_same_price = nullptr;
    order->prev_same_price = nullptr;
}

// OrderBookARTSIMD implementation
OrderBookARTSIMD::OrderBookARTSIMD(InstrumentID instrument_id) 
    : instrument_id_(instrument_id), bids_(true), asks_(false) {
}

OrderBookARTSIMD::~OrderBookARTSIMD() {
}

bool OrderBookARTSIMD::insert_order(Order* order) {
    if (!order) {
        return false;
    }
    
    if (order->side == OrderSide::BUY) {
        return bids_.insert(order);
    } else {
        return asks_.insert(order);
    }
}

bool OrderBookARTSIMD::remove_order(Order* order) {
    if (!order) {
        return false;
    }
    
    if (order->side == OrderSide::BUY) {
        return bids_.remove(order);
    } else {
        return asks_.remove(order);
    }
}

bool OrderBookARTSIMD::update_order(Order* order, Price new_price, Quantity new_quantity) {
    if (!order) {
        return false;
    }
    
    // Remove and reinsert with new price/quantity
    remove_order(order);
    order->price = new_price;
    order->quantity = new_quantity;
    return insert_order(order);
}

Price OrderBookARTSIMD::spread() const {
    Price best_bid_price = best_bid();
    Price best_ask_price = best_ask();
    
    if (best_bid_price == 0 || best_ask_price == 0) {
        return 0;
    }
    
    return best_ask_price - best_bid_price;
}

Price OrderBookARTSIMD::mid_price() const {
    Price best_bid_price = best_bid();
    Price best_ask_price = best_ask();
    
    if (best_bid_price == 0 || best_ask_price == 0) {
        return 0;
    }
    
    return (best_bid_price + best_ask_price) / 2;
}

bool OrderBookARTSIMD::can_match(Order* order) const {
    if (!order) {
        return false;
    }
    
    if (order->side == OrderSide::BUY) {
        Price best_ask_price = asks_.best_price();
        return best_ask_price > 0 && order->price >= best_ask_price;
    } else {
        Price best_bid_price = bids_.best_price();
        return best_bid_price > 0 && order->price <= best_bid_price;
    }
}

void OrderBookARTSIMD::get_depth(size_t n, std::vector<PriceLevel>& bids, 
                                 std::vector<PriceLevel>& asks) const {
    bids_.get_depth(n, bids);
    asks_.get_depth(n, asks);
}

} // namespace perpetual
