#include "core/matching_engine_optimized_v2.h"
#include "core/types.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <iomanip>

using namespace perpetual;
using namespace std::chrono;

void benchmarkOptimizedV2() {
    std::cout << "========================================\n";
    std::cout << "Performance Benchmark V2 (Hot Path Optimized)\n";
    std::cout << "========================================\n\n";
    
    InstrumentID instrument_id = 1;
    MatchingEngineOptimizedV2 engine(instrument_id);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> price_dist(40000.0, 60000.0);
    std::uniform_real_distribution<double> qty_dist(0.01, 1.0);
    std::uniform_real_distribution<double> side_dist(0.0, 1.0);
    
    const size_t num_orders = 100000;
    std::vector<std::unique_ptr<Order>> orders;
    orders.reserve(num_orders);
    
    // Create orders
    for (size_t i = 0; i < num_orders; ++i) {
        UserID user_id = (i % 1000) + 1;
        OrderSide side = (side_dist(gen) < 0.5) ? OrderSide::BUY : OrderSide::SELL;
        Price price = double_to_price(price_dist(gen));
        Quantity quantity = double_to_quantity(qty_dist(gen));
        
        orders.push_back(std::make_unique<Order>(
            i + 1, user_id, instrument_id,
            side, price, quantity, OrderType::LIMIT
        ));
    }
    
    // Warm up
    for (size_t i = 0; i < 1000; ++i) {
        engine.process_order(orders[i].get());
    }
    
    // Benchmark
    uint64_t total_trades = 0;
    nanoseconds total_latency{0};
    nanoseconds min_latency{nanoseconds::max()};
    nanoseconds max_latency{nanoseconds::zero()};
    
    auto start = high_resolution_clock::now();
    
    for (size_t i = 1000; i < num_orders; ++i) {
        auto order_start = high_resolution_clock::now();
        
        auto trades = engine.process_order_optimized_v2(orders[i].get());
        
        auto order_end = high_resolution_clock::now();
        auto latency = duration_cast<nanoseconds>(order_end - order_start);
        
        total_latency += latency;
        if (latency < min_latency) min_latency = latency;
        if (latency > max_latency) max_latency = latency;
        
        total_trades += trades.size();
    }
    
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    
    size_t processed_orders = num_orders - 1000;
    double throughput = (processed_orders * 1000.0) / duration.count();
    double avg_latency = total_latency.count() / (double)processed_orders;
    
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Results:\n";
    std::cout << "  Orders processed: " << processed_orders << "\n";
    std::cout << "  Total trades: " << total_trades << "\n";
    std::cout << "  Total time: " << duration.count() << " ms\n";
    std::cout << "  Throughput: " << throughput / 1000.0 << " K orders/sec\n";
    std::cout << "  Avg latency: " << avg_latency / 1000.0 << " μs\n";
    std::cout << "  Min latency: " << min_latency.count() / 1000.0 << " μs\n";
    std::cout << "  Max latency: " << max_latency.count() / 1000.0 << " μs\n";
    
    std::cout << "\n========================================\n";
    std::cout << "Benchmark completed!\n";
    std::cout << "========================================\n";
}

int main() {
    benchmarkOptimizedV2();
    return 0;
}


#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <iomanip>

using namespace perpetual;
using namespace std::chrono;

void benchmarkOptimizedV2() {
    std::cout << "========================================\n";
    std::cout << "Performance Benchmark V2 (Hot Path Optimized)\n";
    std::cout << "========================================\n\n";
    
    InstrumentID instrument_id = 1;
    MatchingEngineOptimizedV2 engine(instrument_id);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> price_dist(40000.0, 60000.0);
    std::uniform_real_distribution<double> qty_dist(0.01, 1.0);
    std::uniform_real_distribution<double> side_dist(0.0, 1.0);
    
    const size_t num_orders = 100000;
    std::vector<std::unique_ptr<Order>> orders;
    orders.reserve(num_orders);
    
    // Create orders
    for (size_t i = 0; i < num_orders; ++i) {
        UserID user_id = (i % 1000) + 1;
        OrderSide side = (side_dist(gen) < 0.5) ? OrderSide::BUY : OrderSide::SELL;
        Price price = double_to_price(price_dist(gen));
        Quantity quantity = double_to_quantity(qty_dist(gen));
        
        orders.push_back(std::make_unique<Order>(
            i + 1, user_id, instrument_id,
            side, price, quantity, OrderType::LIMIT
        ));
    }
    
    // Warm up
    for (size_t i = 0; i < 1000; ++i) {
        engine.process_order(orders[i].get());
    }
    
    // Benchmark
    uint64_t total_trades = 0;
    nanoseconds total_latency{0};
    nanoseconds min_latency{nanoseconds::max()};
    nanoseconds max_latency{nanoseconds::zero()};
    
    auto start = high_resolution_clock::now();
    
    for (size_t i = 1000; i < num_orders; ++i) {
        auto order_start = high_resolution_clock::now();
        
        auto trades = engine.process_order_optimized_v2(orders[i].get());
        
        auto order_end = high_resolution_clock::now();
        auto latency = duration_cast<nanoseconds>(order_end - order_start);
        
        total_latency += latency;
        if (latency < min_latency) min_latency = latency;
        if (latency > max_latency) max_latency = latency;
        
        total_trades += trades.size();
    }
    
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    
    size_t processed_orders = num_orders - 1000;
    double throughput = (processed_orders * 1000.0) / duration.count();
    double avg_latency = total_latency.count() / (double)processed_orders;
    
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Results:\n";
    std::cout << "  Orders processed: " << processed_orders << "\n";
    std::cout << "  Total trades: " << total_trades << "\n";
    std::cout << "  Total time: " << duration.count() << " ms\n";
    std::cout << "  Throughput: " << throughput / 1000.0 << " K orders/sec\n";
    std::cout << "  Avg latency: " << avg_latency / 1000.0 << " μs\n";
    std::cout << "  Min latency: " << min_latency.count() / 1000.0 << " μs\n";
    std::cout << "  Max latency: " << max_latency.count() / 1000.0 << " μs\n";
    
    std::cout << "\n========================================\n";
    std::cout << "Benchmark completed!\n";
    std::cout << "========================================\n";
}

int main() {
    benchmarkOptimizedV2();
    return 0;
}

