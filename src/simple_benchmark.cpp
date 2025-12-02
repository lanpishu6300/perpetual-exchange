#include "core/matching_engine.h"
#include "core/types.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <random>

using namespace perpetual;
using namespace std::chrono;

int main() {
    std::cout << "Simple Benchmark Test\n";
    std::cout << "====================\n\n";
    
    InstrumentID instrument_id = 1;
    MatchingEngine engine(instrument_id);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> price_dist(40000.0, 60000.0);
    std::uniform_real_distribution<double> qty_dist(0.01, 1.0);
    std::uniform_real_distribution<double> side_dist(0.0, 1.0);
    
    const size_t num_orders = 1000;
    std::vector<std::unique_ptr<Order>> orders;
    orders.reserve(num_orders);
    
    uint64_t total_trades = 0;
    uint64_t total_volume = 0;
    
    auto start = high_resolution_clock::now();
    
    for (size_t i = 0; i < num_orders; ++i) {
        UserID user_id = (i % 100) + 1;
        OrderSide side = (side_dist(gen) < 0.5) ? OrderSide::BUY : OrderSide::SELL;
        Price price = double_to_price(price_dist(gen));
        Quantity quantity = double_to_quantity(qty_dist(gen));
        
        auto order = std::make_unique<Order>(
            i + 1, user_id, instrument_id,
            side, price, quantity, OrderType::LIMIT
        );
        
        try {
            auto trades = engine.process_order(order.get());
            total_trades += trades.size();
            for (const auto& trade : trades) {
                total_volume += trade.quantity;
            }
            
            if (order->is_active()) {
                orders.push_back(std::move(order));
            }
        } catch (const std::exception& e) {
            std::cerr << "Error processing order " << i << ": " << e.what() << "\n";
        } catch (...) {
            std::cerr << "Unknown error processing order " << i << "\n";
        }
        
        if ((i + 1) % 100 == 0) {
            std::cout << "Processed " << (i + 1) << " orders, "
                      << total_trades << " trades\n";
        }
    }
    
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    
    std::cout << "\nResults:\n";
    std::cout << "Total orders: " << num_orders << "\n";
    std::cout << "Total trades: " << total_trades << "\n";
    std::cout << "Total volume: " << quantity_to_double(total_volume) << "\n";
    std::cout << "Time: " << duration.count() << " ms\n";
    std::cout << "Throughput: " << (num_orders * 1000.0 / duration.count()) << " orders/sec\n";
    
    return 0;
}
