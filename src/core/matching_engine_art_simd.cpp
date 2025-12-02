#include "core/matching_engine_art_simd.h"
#include <algorithm>

namespace perpetual {

MatchingEngineARTSIMD::MatchingEngineARTSIMD(InstrumentID instrument_id)
    : MatchingEngineART(instrument_id),
      orderbook_art_simd_(instrument_id) {
}

MatchingEngineARTSIMD::~MatchingEngineARTSIMD() {
}

std::vector<Trade> MatchingEngineARTSIMD::process_order_art_simd(Order* order) {
    if (!order || order->remaining_quantity <= 0) {
        return {};
    }
    
    // Store order
    orders_[order->order_id] = std::unique_ptr<Order>(order);
    user_orders_[order->user_id].push_back(order->order_id);
    
    // Match order using SIMD-optimized ART
    std::vector<Trade> trades = match_order_art_simd(order);
    
    // If order not fully filled, add to order book
    if (order->remaining_quantity > 0 && order->order_type == OrderType::LIMIT) {
        orderbook_art_simd_.insert_order(order);
    }
    
    return trades;
}

std::vector<Trade> MatchingEngineARTSIMD::match_order_art_simd(Order* order) {
    std::vector<Trade> trades;
    
    if (order->side == OrderSide::BUY) {
        // Match against asks using SIMD-optimized lookup
        OrderBookSideARTSIMD& asks = orderbook_art_simd_.asks();
        
        while (order->remaining_quantity > 0 && !asks.empty()) {
            // Use SIMD-optimized best price lookup
            Price best_ask = asks.best_price_simd();
            if (best_ask == 0 || order->price < best_ask) {
                break;  // Cannot match
            }
            
            PriceLevel* level = asks.best_level();
            if (!level || level->first_order == nullptr) {
                break;
            }
            
            Order* maker = level->first_order;
            Price trade_price = maker->price;  // Price-time priority
            Quantity trade_qty = std::min(order->remaining_quantity, maker->remaining_quantity);
            
            execute_trade_art(order, maker, trade_price, trade_qty);
            
            Trade trade;
            trade.buy_order_id = order->order_id;
            trade.sell_order_id = maker->order_id;
            trade.buy_user_id = order->user_id;
            trade.sell_user_id = maker->user_id;
            trade.instrument_id = order->instrument_id;
            trade.price = trade_price;
            trade.quantity = trade_qty;
            trade.timestamp = get_current_timestamp();
            trade.sequence_id = ++trade_sequence_;
            trade.is_taker_buy = true;
            trades.push_back(trade);
            
            total_trades_++;
            total_volume_ += quantity_to_double(trade_qty);
            
            // Remove maker if fully filled
            if (maker->remaining_quantity == 0) {
                orderbook_art_simd_.remove_order(maker);
            }
        }
    } else {
        // Match against bids using SIMD-optimized lookup
        OrderBookSideARTSIMD& bids = orderbook_art_simd_.bids();
        
        while (order->remaining_quantity > 0 && !bids.empty()) {
            // Use SIMD-optimized best price lookup
            Price best_bid = bids.best_price_simd();
            if (best_bid == 0 || order->price > best_bid) {
                break;  // Cannot match
            }
            
            PriceLevel* level = bids.best_level();
            if (!level || level->first_order == nullptr) {
                break;
            }
            
            Order* maker = level->first_order;
            Price trade_price = maker->price;  // Price-time priority
            Quantity trade_qty = std::min(order->remaining_quantity, maker->remaining_quantity);
            
            execute_trade_art(order, maker, trade_price, trade_qty);
            
            Trade trade;
            trade.buy_order_id = maker->order_id;
            trade.sell_order_id = order->order_id;
            trade.buy_user_id = maker->user_id;
            trade.sell_user_id = order->user_id;
            trade.instrument_id = order->instrument_id;
            trade.price = trade_price;
            trade.quantity = trade_qty;
            trade.timestamp = get_current_timestamp();
            trade.sequence_id = ++trade_sequence_;
            trade.is_taker_buy = false;
            trades.push_back(trade);
            
            total_trades_++;
            total_volume_ += quantity_to_double(trade_qty);
            
            // Remove maker if fully filled
            if (maker->remaining_quantity == 0) {
                orderbook_art_simd_.remove_order(maker);
            }
        }
    }
    
    return trades;
}

} // namespace perpetual

