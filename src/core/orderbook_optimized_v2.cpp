#include "core/orderbook.h"
#include "core/hot_path_utils.h"
#include <algorithm>

namespace perpetual {

// Optimized best order retrieval with branch prediction hints
__attribute__((hot))
Order* OrderBookSide::best_order_optimized() const {
    if (empty()) {
        return nullptr;
    }
    
    // For buy side (bids), best is leftmost (highest price)
    // For sell side (asks), best is leftmost (lowest price)
    // Red-black tree: leftmost is always the best
    
    Order* current = root_;
    if (current == sentinel_) {
        return nullptr;
    }
    
    // Traverse to leftmost node (likely to be in cache)
    while (current->left != sentinel_) {
        // Prefetch next node
        HotPathUtils::prefetch_order(current->left);
        current = current->left;
    }
    
    return current;
}

// Optimized price level lookup with caching
__attribute__((hot))
PriceLevel* OrderBookSide::get_price_level_cached(Price price) const {
    // Check cache first (if implemented)
    auto it = price_levels_.find(price);
    if (it != price_levels_.end()) {
        return it->second.get();
    }
    
    // Create new price level
    return get_or_create_price_level(price);
}

// Batch insert optimization
void OrderBookSide::batch_insert_orders(const std::vector<Order*>& orders) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Sort orders by price for better cache locality
    std::vector<Order*> sorted_orders = orders;
    std::sort(sorted_orders.begin(), sorted_orders.end(), 
        [this](const Order* a, const Order* b) {
            if (price_better(a->price, b->price)) return true;
            if (price_better(b->price, a->price)) return false;
            return a->timestamp < b->timestamp;
        });
    
    // Insert in sorted order
    for (Order* order : sorted_orders) {
        insert(order);
    }
}

} // namespace perpetual

