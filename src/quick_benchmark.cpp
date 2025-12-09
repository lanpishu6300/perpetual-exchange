#include "core/matching_engine.h"
#include "core/types.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <iomanip>

using namespace perpetual;
using namespace std::chrono;

int main() {
    std::cout << "Quick Benchmark - 10K Orders\n";
    std::cout << "============================\n\n";
    
    InstrumentID instrument_id = 1;
    MatchingEngine engine(instrument_id);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> price_dist(40000.0, 60000.0);
    std::uniform_real_distribution<double> qty_dist(0.01, 1.0);
    std::uniform_real_distribution<double> side_dist(0.0, 1.0);
    
    const size_t num_orders = 10000;
    std::vector<std::unique_ptr<Order>> orders;
    orders.reserve(num_orders);
    
    uint64_t total_trades = 0;
    uint64_t total_volume = 0;
    nanoseconds total_latency{0};
    nanoseconds min_latency{nanoseconds::max()};
    nanoseconds max_latency{nanoseconds::zero()};
    
    auto start = high_resolution_clock::now();
    
    for (size_t i = 0; i < num_orders; ++i) {
        UserID user_id = (i % 1000) + 1;
        OrderSide side = (side_dist(gen) < 0.5) ? OrderSide::BUY : OrderSide::SELL;
        Price price = double_to_price(price_dist(gen));
        Quantity quantity = double_to_quantity(qty_dist(gen));
        
        auto order = std::make_unique<Order>(
            i + 1, user_id, instrument_id,
            side, price, quantity, OrderType::LIMIT
        );
        
        auto order_start = high_resolution_clock::now();
        
        try {
            auto trades = engine.process_order(order.get());
            total_trades += trades.size();
            for (const auto& trade : trades) {
                total_volume += trade.quantity;
            }
            
            auto order_end = high_resolution_clock::now();
            auto latency = duration_cast<nanoseconds>(order_end - order_start);
            
            total_latency += latency;
            if (latency < min_latency) min_latency = latency;
            if (latency > max_latency) max_latency = latency;
            
            if (order->is_active()) {
                orders.push_back(std::move(order));
            }
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
        }
        
        if ((i + 1) % 1000 == 0) {
            std::cout << "Processed " << (i + 1) << " orders, "
                      << total_trades << " trades\n";
        }
    }
    
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    
    std::cout << "\n" << std::string(50, '=') << "\n";
    std::cout << "Benchmark Results\n";
    std::cout << std::string(50, '=') << "\n";
    std::cout << std::fixed << std::setprecision(2);
    
    std::cout << "Total Orders:      " << num_orders << "\n";
    std::cout << "Total Trades:      " << total_trades << "\n";
    std::cout << "Total Volume:      " << quantity_to_double(total_volume) << "\n";
    std::cout << "Total Time:        " << duration.count() << " ms\n";
    std::cout << "Throughput:        " << (num_orders * 1000.0 / duration.count()) 
              << " K orders/sec\n";
    std::cout << "\n";
    
    std::cout << "Latency Statistics:\n";
    std::cout << "  Average:        " << (total_latency.count() / num_orders) << " ns\n";
    std::cout << "  Average:        " << (total_latency.count() / num_orders / 1000.0) << " μs\n";
    if (min_latency != nanoseconds::max()) {
        std::cout << "  Min:            " << min_latency.count() << " ns\n";
        std::cout << "  Min:            " << (min_latency.count() / 1000.0) << " μs\n";
    }
    if (max_latency != nanoseconds::zero()) {
        std::cout << "  Max:            " << max_latency.count() << " ns\n";
        std::cout << "  Max:            " << (max_latency.count() / 1000.0) << " μs\n";
    }
    
    // Get order book stats
    const OrderBook& ob = engine.get_orderbook();
    std::vector<PriceLevel> bids, asks;
    ob.get_depth(5, bids, asks);
    
    std::cout << "\nOrder Book:\n";
    std::cout << "  Bids: " << bids.size() << " levels\n";
    std::cout << "  Asks: " << asks.size() << " levels\n";
    
    return 0;
}


#include "core/types.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <iomanip>

using namespace perpetual;
using namespace std::chrono;

int main() {
    std::cout << "Quick Benchmark - 10K Orders\n";
    std::cout << "============================\n\n";
    
    InstrumentID instrument_id = 1;
    MatchingEngine engine(instrument_id);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> price_dist(40000.0, 60000.0);
    std::uniform_real_distribution<double> qty_dist(0.01, 1.0);
    std::uniform_real_distribution<double> side_dist(0.0, 1.0);
    
    const size_t num_orders = 10000;
    std::vector<std::unique_ptr<Order>> orders;
    orders.reserve(num_orders);
    
    uint64_t total_trades = 0;
    uint64_t total_volume = 0;
    nanoseconds total_latency{0};
    nanoseconds min_latency{nanoseconds::max()};
    nanoseconds max_latency{nanoseconds::zero()};
    
    auto start = high_resolution_clock::now();
    
    for (size_t i = 0; i < num_orders; ++i) {
        UserID user_id = (i % 1000) + 1;
        OrderSide side = (side_dist(gen) < 0.5) ? OrderSide::BUY : OrderSide::SELL;
        Price price = double_to_price(price_dist(gen));
        Quantity quantity = double_to_quantity(qty_dist(gen));
        
        auto order = std::make_unique<Order>(
            i + 1, user_id, instrument_id,
            side, price, quantity, OrderType::LIMIT
        );
        
        auto order_start = high_resolution_clock::now();
        
        try {
            auto trades = engine.process_order(order.get());
            total_trades += trades.size();
            for (const auto& trade : trades) {
                total_volume += trade.quantity;
            }
            
            auto order_end = high_resolution_clock::now();
            auto latency = duration_cast<nanoseconds>(order_end - order_start);
            
            total_latency += latency;
            if (latency < min_latency) min_latency = latency;
            if (latency > max_latency) max_latency = latency;
            
            if (order->is_active()) {
                orders.push_back(std::move(order));
            }
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
        }
        
        if ((i + 1) % 1000 == 0) {
            std::cout << "Processed " << (i + 1) << " orders, "
                      << total_trades << " trades\n";
        }
    }
    
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    
    std::cout << "\n" << std::string(50, '=') << "\n";
    std::cout << "Benchmark Results\n";
    std::cout << std::string(50, '=') << "\n";
    std::cout << std::fixed << std::setprecision(2);
    
    std::cout << "Total Orders:      " << num_orders << "\n";
    std::cout << "Total Trades:      " << total_trades << "\n";
    std::cout << "Total Volume:      " << quantity_to_double(total_volume) << "\n";
    std::cout << "Total Time:        " << duration.count() << " ms\n";
    std::cout << "Throughput:        " << (num_orders * 1000.0 / duration.count()) 
              << " K orders/sec\n";
    std::cout << "\n";
    
    std::cout << "Latency Statistics:\n";
    std::cout << "  Average:        " << (total_latency.count() / num_orders) << " ns\n";
    std::cout << "  Average:        " << (total_latency.count() / num_orders / 1000.0) << " μs\n";
    if (min_latency != nanoseconds::max()) {
        std::cout << "  Min:            " << min_latency.count() << " ns\n";
        std::cout << "  Min:            " << (min_latency.count() / 1000.0) << " μs\n";
    }
    if (max_latency != nanoseconds::zero()) {
        std::cout << "  Max:            " << max_latency.count() << " ns\n";
        std::cout << "  Max:            " << (max_latency.count() / 1000.0) << " μs\n";
    }
    
    // Get order book stats
    const OrderBook& ob = engine.get_orderbook();
    std::vector<PriceLevel> bids, asks;
    ob.get_depth(5, bids, asks);
    
    std::cout << "\nOrder Book:\n";
    std::cout << "  Bids: " << bids.size() << " levels\n";
    std::cout << "  Asks: " << asks.size() << " levels\n";
    
    return 0;
}


