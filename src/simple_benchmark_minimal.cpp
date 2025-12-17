#include "core/matching_engine.h"
#include "core/types.h"
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>

using namespace perpetual;
using namespace std::chrono;

int main() {
    std::cout << "Simple Benchmark - Basic Matching Engine\n";
    std::cout << "========================================\n\n";
    
    InstrumentID instrument_id = 1;
    MatchingEngine engine(instrument_id);
    
    const size_t num_orders = 10000;
    std::vector<std::unique_ptr<Order>> orders;
    orders.reserve(num_orders);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> price_dist(40000.0, 60000.0);
    std::uniform_real_distribution<double> qty_dist(0.01, 1.0);
    std::uniform_real_distribution<double> side_dist(0.0, 1.0);
    
    uint64_t total_orders = 0;
    uint64_t total_trades = 0;
    auto start = high_resolution_clock::now();
    
    for (size_t i = 0; i < num_orders; ++i) {
        UserID user_id = (i % 1000) + 1;
        OrderSide side = (side_dist(gen) < 0.5) ? OrderSide::BUY : OrderSide::SELL;
        Price price = double_to_price(price_dist(gen));
        Quantity quantity = double_to_quantity(qty_dist(gen));
        
        auto order = std::make_unique<Order>(
            i + 1, user_id, instrument_id, side, price, quantity, OrderType::LIMIT
        );
        
        try {
            auto trades = engine.process_order(order.get());
            total_orders++;
            total_trades += trades.size();
            
            if (order && order->is_active()) {
                orders.push_back(std::move(order));
            }
        } catch (...) {
            // Ignore errors
        }
    }
    
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Results:\n";
    std::cout << "  Orders processed: " << total_orders << "\n";
    std::cout << "  Trades executed: " << total_trades << "\n";
    std::cout << "  Total time: " << duration.count() << " ms\n";
    std::cout << "  Throughput: " << (total_orders * 1000.0 / duration.count()) / 1000.0 
              << " K orders/sec\n";
    
    return 0;
}
