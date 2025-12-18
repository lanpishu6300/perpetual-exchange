#pragma once

#include "core/types.h"
#include <memory>

namespace perpetual {

// Order structure optimized for cache performance
// Aligned to cache line boundaries for better performance
struct alignas(64) Order {
    OrderID order_id;
    UserID user_id;
    InstrumentID instrument_id;
    
    OrderSide side;
    OrderType order_type;
    OffsetFlag offset_flag;
    OrderStatus status;
    
    Price price;
    Quantity quantity;
    Quantity filled_quantity;
    Quantity remaining_quantity;
    
    Timestamp timestamp;
    SequenceID sequence_id;
    
    // Position tracking
    PositionSide position_side;
    
    // Linked list pointers for price level aggregation
    Order* prev_same_price;
    Order* next_same_price;
    
    // For order book tree structure
    Order* parent;
    Order* left;
    Order* right;
    
    // Color for red-black tree (0 = black, 1 = red)
    uint8_t color;
    
    // Price level aggregation
    Quantity total_quantity;  // Total quantity at this price level
    
    Order() 
        : order_id(0), user_id(0), instrument_id(0)
        , side(OrderSide::BUY), order_type(OrderType::LIMIT)
        , offset_flag(OffsetFlag::OPEN), status(OrderStatus::PENDING)
        , price(0), quantity(0), filled_quantity(0), remaining_quantity(0)
        , timestamp(0), sequence_id(0), position_side(PositionSide::NET)
        , prev_same_price(nullptr), next_same_price(nullptr)
        , parent(nullptr), left(nullptr), right(nullptr)
        , color(0), total_quantity(0) {}
    
    Order(OrderID oid, UserID uid, InstrumentID iid, OrderSide s, 
          Price p, Quantity q, OrderType ot = OrderType::LIMIT)
        : order_id(oid), user_id(uid), instrument_id(iid)
        , side(s), order_type(ot), offset_flag(OffsetFlag::OPEN)
        , status(OrderStatus::PENDING), price(p), quantity(q)
        , filled_quantity(0), remaining_quantity(q)
        , timestamp(get_current_timestamp()), sequence_id(0)
        , position_side(PositionSide::NET)
        , prev_same_price(nullptr), next_same_price(nullptr)
        , parent(nullptr), left(nullptr), right(nullptr)
        , color(1), total_quantity(q) {}
    
    bool is_buy() const { return side == OrderSide::BUY; }
    bool is_sell() const { return side == OrderSide::SELL; }
    
    bool is_filled() const { 
        return remaining_quantity == 0 || status == OrderStatus::FILLED; 
    }
    
    bool is_active() const {
        return status == OrderStatus::PENDING || status == OrderStatus::PARTIAL_FILLED;
    }
};

// Trade result
struct Trade {
    OrderID buy_order_id;
    OrderID sell_order_id;
    UserID buy_user_id;
    UserID sell_user_id;
    InstrumentID instrument_id;
    Price price;
    Quantity quantity;
    Timestamp timestamp;
    SequenceID sequence_id;
    bool is_taker_buy;  // True if buy order is taker
};

} // namespace perpetual
