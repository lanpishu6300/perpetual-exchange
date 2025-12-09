#include "core/matching_engine.h"
#include "core/account.h"
#include "core/position.h"
#include "core/funding_rate.h"
#include "core/types.h"
#include <iostream>
#include <vector>

using namespace perpetual;

int main() {
    std::cout << "Perpetual Exchange - Nanosecond Latency Matching Engine\n";
    std::cout << "========================================================\n\n";
    
    // Initialize matching engine for BTC-USDT perpetual
    InstrumentID btc_usdt = 1;
    MatchingEngine engine(btc_usdt);
    
    // Setup callbacks
    engine.set_trade_callback([](const Trade& trade) {
        std::cout << "Trade executed: "
                  << "Price=" << price_to_double(trade.price)
                  << ", Quantity=" << quantity_to_double(trade.quantity)
                  << ", BuyOrder=" << trade.buy_order_id
                  << ", SellOrder=" << trade.sell_order_id << "\n";
    });
    
    engine.set_order_update_callback([](Order* order) {
        std::cout << "Order update: "
                  << "OrderID=" << order->order_id
                  << ", Status=" << static_cast<int>(order->status)
                  << ", Filled=" << quantity_to_double(order->filled_quantity)
                  << ", Remaining=" << quantity_to_double(order->remaining_quantity) << "\n";
    });
    
    // Create some test orders
    UserID user1 = 1001;
    UserID user2 = 1002;
    
    // User1 places a buy limit order at $50000
    Price buy_price = double_to_price(50000.0);
    Quantity buy_qty = double_to_quantity(0.1);
    auto buy_order = std::make_unique<Order>(1, user1, btc_usdt, OrderSide::BUY, 
                                              buy_price, buy_qty, OrderType::LIMIT);
    
    // User2 places a sell limit order at $50000
    Price sell_price = double_to_price(50000.0);
    Quantity sell_qty = double_to_quantity(0.1);
    auto sell_order = std::make_unique<Order>(2, user2, btc_usdt, OrderSide::SELL,
                                               sell_price, sell_qty, OrderType::LIMIT);
    
    // Process buy order first (will be added to book)
    std::cout << "\nProcessing buy order...\n";
    auto trades1 = engine.process_order(buy_order.get());
    std::cout << "Trades generated: " << trades1.size() << "\n";
    
    // Process sell order (will match with buy order)
    std::cout << "\nProcessing sell order...\n";
    auto trades2 = engine.process_order(sell_order.get());
    std::cout << "Trades generated: " << trades2.size() << "\n";
    
    // Print statistics
    std::cout << "\nEngine Statistics:\n";
    std::cout << "Total trades: " << engine.total_trades() << "\n";
    std::cout << "Total volume: " << quantity_to_double(engine.total_volume()) << "\n";
    
    // Get order book depth
    const OrderBook& ob = engine.get_orderbook();
    std::vector<PriceLevel> bids, asks;
    ob.get_depth(5, bids, asks);
    
    std::cout << "\nOrder Book Depth:\n";
    std::cout << "Bids: " << bids.size() << " levels\n";
    std::cout << "Asks: " << asks.size() << " levels\n";
    
    if (!bids.empty()) {
        std::cout << "Best bid: " << price_to_double(bids[0].price) 
                  << ", Qty: " << quantity_to_double(bids[0].total_quantity) << "\n";
    }
    if (!asks.empty()) {
        std::cout << "Best ask: " << price_to_double(asks[0].price)
                  << ", Qty: " << quantity_to_double(asks[0].total_quantity) << "\n";
    }
    
    std::cout << "\nDemo completed successfully!\n";
    return 0;
}


#include "core/account.h"
#include "core/position.h"
#include "core/funding_rate.h"
#include "core/types.h"
#include <iostream>
#include <vector>

using namespace perpetual;

int main() {
    std::cout << "Perpetual Exchange - Nanosecond Latency Matching Engine\n";
    std::cout << "========================================================\n\n";
    
    // Initialize matching engine for BTC-USDT perpetual
    InstrumentID btc_usdt = 1;
    MatchingEngine engine(btc_usdt);
    
    // Setup callbacks
    engine.set_trade_callback([](const Trade& trade) {
        std::cout << "Trade executed: "
                  << "Price=" << price_to_double(trade.price)
                  << ", Quantity=" << quantity_to_double(trade.quantity)
                  << ", BuyOrder=" << trade.buy_order_id
                  << ", SellOrder=" << trade.sell_order_id << "\n";
    });
    
    engine.set_order_update_callback([](Order* order) {
        std::cout << "Order update: "
                  << "OrderID=" << order->order_id
                  << ", Status=" << static_cast<int>(order->status)
                  << ", Filled=" << quantity_to_double(order->filled_quantity)
                  << ", Remaining=" << quantity_to_double(order->remaining_quantity) << "\n";
    });
    
    // Create some test orders
    UserID user1 = 1001;
    UserID user2 = 1002;
    
    // User1 places a buy limit order at $50000
    Price buy_price = double_to_price(50000.0);
    Quantity buy_qty = double_to_quantity(0.1);
    auto buy_order = std::make_unique<Order>(1, user1, btc_usdt, OrderSide::BUY, 
                                              buy_price, buy_qty, OrderType::LIMIT);
    
    // User2 places a sell limit order at $50000
    Price sell_price = double_to_price(50000.0);
    Quantity sell_qty = double_to_quantity(0.1);
    auto sell_order = std::make_unique<Order>(2, user2, btc_usdt, OrderSide::SELL,
                                               sell_price, sell_qty, OrderType::LIMIT);
    
    // Process buy order first (will be added to book)
    std::cout << "\nProcessing buy order...\n";
    auto trades1 = engine.process_order(buy_order.get());
    std::cout << "Trades generated: " << trades1.size() << "\n";
    
    // Process sell order (will match with buy order)
    std::cout << "\nProcessing sell order...\n";
    auto trades2 = engine.process_order(sell_order.get());
    std::cout << "Trades generated: " << trades2.size() << "\n";
    
    // Print statistics
    std::cout << "\nEngine Statistics:\n";
    std::cout << "Total trades: " << engine.total_trades() << "\n";
    std::cout << "Total volume: " << quantity_to_double(engine.total_volume()) << "\n";
    
    // Get order book depth
    const OrderBook& ob = engine.get_orderbook();
    std::vector<PriceLevel> bids, asks;
    ob.get_depth(5, bids, asks);
    
    std::cout << "\nOrder Book Depth:\n";
    std::cout << "Bids: " << bids.size() << " levels\n";
    std::cout << "Asks: " << asks.size() << " levels\n";
    
    if (!bids.empty()) {
        std::cout << "Best bid: " << price_to_double(bids[0].price) 
                  << ", Qty: " << quantity_to_double(bids[0].total_quantity) << "\n";
    }
    if (!asks.empty()) {
        std::cout << "Best ask: " << price_to_double(asks[0].price)
                  << ", Qty: " << quantity_to_double(asks[0].total_quantity) << "\n";
    }
    
    std::cout << "\nDemo completed successfully!\n";
    return 0;
}


