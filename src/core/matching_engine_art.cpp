#include "core/matching_engine_art.h"
#include <algorithm>

namespace perpetual {

MatchingEngineART::MatchingEngineART(InstrumentID instrument_id)
    : MatchingEngine(instrument_id),
      orderbook_art_(instrument_id),
      trade_sequence_(0),
      total_trades_(0),
      total_volume_(0.0) {
}

MatchingEngineART::~MatchingEngineART() {
}

std::vector<Trade> MatchingEngineART::process_order_art(Order* order) {
    if (!order || order->quantity <= 0) {
        return {};
    }
    
    // Store order
    orders_[order->order_id] = std::unique_ptr<Order>(order);
    user_orders_[order->user_id].push_back(order->order_id);
    
    // Match order
    std::vector<Trade> trades = match_order_art(order);
    
    // If order not fully filled, add to order book
    if (order->quantity > 0 && order->type == OrderType::LIMIT) {
        orderbook_art_.insert_order(order);
    }
    
    return trades;
}

std::vector<Trade> MatchingEngineART::match_order_art(Order* order) {
    std::vector<Trade> trades;
    
    if (order->side == OrderSide::BUY) {
        // Match against asks
        OrderBookSideART& asks = orderbook_art_.asks();
        
        while (order->quantity > 0 && !asks.empty()) {
            Price best_ask = asks.best_price();
            if (best_ask == 0 || order->price < best_ask) {
                break;  // Cannot match
            }
            
            PriceLevel* level = asks.best_level();
            if (!level || level->first_order == nullptr) {
                break;
            }
            
            Order* maker = level->first_order;
            Price trade_price = maker->price;  // Price-time priority
            Quantity trade_qty = std::min(order->quantity, maker->quantity);
            
            execute_trade_art(order, maker, trade_price, trade_qty);
            
            Trade trade;
            trade.trade_id = ++trade_sequence_;
            trade.instrument_id = order->instrument_id;
            trade.price = trade_price;
            trade.quantity = trade_qty;
            trade.taker_order_id = order->order_id;
            trade.maker_order_id = maker->order_id;
            trade.taker_user_id = order->user_id;
            trade.maker_user_id = maker->user_id;
            trade.timestamp = get_current_timestamp();
            trades.push_back(trade);
            
            total_trades_++;
            total_volume_ += quantity_to_double(trade_qty);
            
            // Remove maker if fully filled
            if (maker->quantity == 0) {
                orderbook_art_.remove_order(maker);
            }
        }
    } else {
        // Match against bids
        OrderBookSideART& bids = orderbook_art_.bids();
        
        while (order->quantity > 0 && !bids.empty()) {
            Price best_bid = bids.best_price();
            if (best_bid == 0 || order->price > best_bid) {
                break;  // Cannot match
            }
            
            PriceLevel* level = bids.best_level();
            if (!level || level->first_order == nullptr) {
                break;
            }
            
            Order* maker = level->first_order;
            Price trade_price = maker->price;  // Price-time priority
            Quantity trade_qty = std::min(order->quantity, maker->quantity);
            
            execute_trade_art(order, maker, trade_price, trade_qty);
            
            Trade trade;
            trade.trade_id = ++trade_sequence_;
            trade.instrument_id = order->instrument_id;
            trade.price = trade_price;
            trade.quantity = trade_qty;
            trade.taker_order_id = order->order_id;
            trade.maker_order_id = maker->order_id;
            trade.taker_user_id = order->user_id;
            trade.maker_user_id = maker->user_id;
            trade.timestamp = get_current_timestamp();
            trades.push_back(trade);
            
            total_trades_++;
            total_volume_ += quantity_to_double(trade_qty);
            
            // Remove maker if fully filled
            if (maker->quantity == 0) {
                orderbook_art_.remove_order(maker);
            }
        }
    }
    
    return trades;
}

void MatchingEngineART::execute_trade_art(Order* taker, Order* maker, Price price, Quantity quantity) {
    taker->quantity -= quantity;
    maker->quantity -= quantity;
    
    if (taker->quantity == 0) {
        taker->status = OrderStatus::FILLED;
    } else {
        taker->status = OrderStatus::PARTIAL_FILLED;
    }
    
    if (maker->quantity == 0) {
        maker->status = OrderStatus::FILLED;
    } else {
        maker->status = OrderStatus::PARTIAL_FILLED;
    }
}

} // namespace perpetual

