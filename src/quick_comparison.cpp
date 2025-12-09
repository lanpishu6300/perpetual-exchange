#include "core/matching_engine.h"
#include "core/matching_engine_optimized.h"
#include "core/types.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <iomanip>

using namespace perpetual;
using namespace std::chrono;

void run_comparison_test(size_t num_orders) {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "Performance Comparison Test (" << num_orders << " orders)\n";
    std::cout << std::string(70, '=') << "\n";
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> price_dist(40000.0, 60000.0);
    std::uniform_real_distribution<double> qty_dist(0.01, 1.0);
    std::uniform_real_distribution<double> side_dist(0.0, 1.0);
    
    InstrumentID instrument_id = 1;
    
    // Test original version
    std::cout << "\nTesting Original Version...\n";
    MatchingEngine engine_orig(instrument_id);
    
    std::vector<std::unique_ptr<Order>> orders_orig;
    orders_orig.reserve(num_orders);
    
    uint64_t trades_orig = 0;
    nanoseconds total_latency_orig{0};
    nanoseconds min_latency_orig{nanoseconds::max()};
    nanoseconds max_latency_orig{nanoseconds::zero()};
    
    auto start_orig = high_resolution_clock::now();
    
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
        auto trades = engine_orig.process_order(order.get());
        auto order_end = high_resolution_clock::now();
        
        auto latency = duration_cast<nanoseconds>(order_end - order_start);
        total_latency_orig += latency;
        if (latency < min_latency_orig) min_latency_orig = latency;
        if (latency > max_latency_orig) max_latency_orig = latency;
        
        trades_orig += trades.size();
        
        if (order->is_active()) {
            orders_orig.push_back(std::move(order));
        }
    }
    
    auto end_orig = high_resolution_clock::now();
    auto duration_orig = duration_cast<milliseconds>(end_orig - start_orig);
    
    // Test optimized version
    std::cout << "Testing Optimized Version...\n";
    OptimizedMatchingEngine engine_opt(instrument_id);
    
    std::vector<std::unique_ptr<Order>> orders_opt;
    orders_opt.reserve(num_orders);
    
    uint64_t trades_opt = 0;
    nanoseconds total_latency_opt{0};
    nanoseconds min_latency_opt{nanoseconds::max()};
    nanoseconds max_latency_opt{nanoseconds::zero()};
    
    gen.seed(rd()); // Reset RNG for fair comparison
    
    auto start_opt = high_resolution_clock::now();
    
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
        auto trades = engine_opt.process_order_optimized(order.get());
        auto order_end = high_resolution_clock::now();
        
        auto latency = duration_cast<nanoseconds>(order_end - order_start);
        total_latency_opt += latency;
        if (latency < min_latency_opt) min_latency_opt = latency;
        if (latency > max_latency_opt) max_latency_opt = latency;
        
        trades_opt += trades.size();
        
        if (order->is_active()) {
            orders_opt.push_back(std::move(order));
        }
    }
    
    auto end_opt = high_resolution_clock::now();
    auto duration_opt = duration_cast<milliseconds>(end_opt - start_opt);
    
    // Print comparison
    std::cout << "\n" << std::string(70, '-') << "\n";
    std::cout << "Results Comparison\n";
    std::cout << std::string(70, '-') << "\n";
    std::cout << std::fixed << std::setprecision(2);
    
    std::cout << "\nThroughput:\n";
    double throughput_orig = (num_orders * 1000.0) / duration_orig.count();
    double throughput_opt = (num_orders * 1000.0) / duration_opt.count();
    std::cout << "  Original:  " << throughput_orig / 1000.0 << " K orders/sec\n";
    std::cout << "  Optimized: " << throughput_opt / 1000.0 << " K orders/sec\n";
    double throughput_improvement = ((throughput_opt - throughput_orig) / throughput_orig) * 100.0;
    std::cout << "  Improvement: " << throughput_improvement << "%\n";
    
    std::cout << "\nLatency:\n";
    double avg_latency_orig = total_latency_orig.count() / static_cast<double>(num_orders);
    double avg_latency_opt = total_latency_opt.count() / static_cast<double>(num_orders);
    std::cout << "  Original Avg:  " << avg_latency_orig << " ns (" 
              << avg_latency_orig / 1000.0 << " μs)\n";
    std::cout << "  Optimized Avg: " << avg_latency_opt << " ns (" 
              << avg_latency_opt / 1000.0 << " μs)\n";
    double latency_improvement = ((avg_latency_orig - avg_latency_opt) / avg_latency_orig) * 100.0;
    std::cout << "  Improvement: " << latency_improvement << "%\n";
    
    std::cout << "\nMin Latency:\n";
    std::cout << "  Original:  " << min_latency_orig.count() << " ns\n";
    std::cout << "  Optimized: " << min_latency_opt.count() << " ns\n";
    
    std::cout << "\nMax Latency:\n";
    std::cout << "  Original:  " << max_latency_orig.count() << " ns\n";
    std::cout << "  Optimized: " << max_latency_opt.count() << " ns\n";
    
    std::cout << "\nTrades:\n";
    std::cout << "  Original:  " << trades_orig << "\n";
    std::cout << "  Optimized: " << trades_opt << "\n";
    
    std::cout << "\n";
}

int main() {
    std::cout << "Quick Performance Comparison\n";
    std::cout << "============================\n";
    
    // Run small scale test
    run_comparison_test(10000);
    
    std::cout << "\nComparison completed!\n";
    return 0;
}


#include "core/matching_engine_optimized.h"
#include "core/types.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <iomanip>

using namespace perpetual;
using namespace std::chrono;

void run_comparison_test(size_t num_orders) {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "Performance Comparison Test (" << num_orders << " orders)\n";
    std::cout << std::string(70, '=') << "\n";
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> price_dist(40000.0, 60000.0);
    std::uniform_real_distribution<double> qty_dist(0.01, 1.0);
    std::uniform_real_distribution<double> side_dist(0.0, 1.0);
    
    InstrumentID instrument_id = 1;
    
    // Test original version
    std::cout << "\nTesting Original Version...\n";
    MatchingEngine engine_orig(instrument_id);
    
    std::vector<std::unique_ptr<Order>> orders_orig;
    orders_orig.reserve(num_orders);
    
    uint64_t trades_orig = 0;
    nanoseconds total_latency_orig{0};
    nanoseconds min_latency_orig{nanoseconds::max()};
    nanoseconds max_latency_orig{nanoseconds::zero()};
    
    auto start_orig = high_resolution_clock::now();
    
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
        auto trades = engine_orig.process_order(order.get());
        auto order_end = high_resolution_clock::now();
        
        auto latency = duration_cast<nanoseconds>(order_end - order_start);
        total_latency_orig += latency;
        if (latency < min_latency_orig) min_latency_orig = latency;
        if (latency > max_latency_orig) max_latency_orig = latency;
        
        trades_orig += trades.size();
        
        if (order->is_active()) {
            orders_orig.push_back(std::move(order));
        }
    }
    
    auto end_orig = high_resolution_clock::now();
    auto duration_orig = duration_cast<milliseconds>(end_orig - start_orig);
    
    // Test optimized version
    std::cout << "Testing Optimized Version...\n";
    OptimizedMatchingEngine engine_opt(instrument_id);
    
    std::vector<std::unique_ptr<Order>> orders_opt;
    orders_opt.reserve(num_orders);
    
    uint64_t trades_opt = 0;
    nanoseconds total_latency_opt{0};
    nanoseconds min_latency_opt{nanoseconds::max()};
    nanoseconds max_latency_opt{nanoseconds::zero()};
    
    gen.seed(rd()); // Reset RNG for fair comparison
    
    auto start_opt = high_resolution_clock::now();
    
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
        auto trades = engine_opt.process_order_optimized(order.get());
        auto order_end = high_resolution_clock::now();
        
        auto latency = duration_cast<nanoseconds>(order_end - order_start);
        total_latency_opt += latency;
        if (latency < min_latency_opt) min_latency_opt = latency;
        if (latency > max_latency_opt) max_latency_opt = latency;
        
        trades_opt += trades.size();
        
        if (order->is_active()) {
            orders_opt.push_back(std::move(order));
        }
    }
    
    auto end_opt = high_resolution_clock::now();
    auto duration_opt = duration_cast<milliseconds>(end_opt - start_opt);
    
    // Print comparison
    std::cout << "\n" << std::string(70, '-') << "\n";
    std::cout << "Results Comparison\n";
    std::cout << std::string(70, '-') << "\n";
    std::cout << std::fixed << std::setprecision(2);
    
    std::cout << "\nThroughput:\n";
    double throughput_orig = (num_orders * 1000.0) / duration_orig.count();
    double throughput_opt = (num_orders * 1000.0) / duration_opt.count();
    std::cout << "  Original:  " << throughput_orig / 1000.0 << " K orders/sec\n";
    std::cout << "  Optimized: " << throughput_opt / 1000.0 << " K orders/sec\n";
    double throughput_improvement = ((throughput_opt - throughput_orig) / throughput_orig) * 100.0;
    std::cout << "  Improvement: " << throughput_improvement << "%\n";
    
    std::cout << "\nLatency:\n";
    double avg_latency_orig = total_latency_orig.count() / static_cast<double>(num_orders);
    double avg_latency_opt = total_latency_opt.count() / static_cast<double>(num_orders);
    std::cout << "  Original Avg:  " << avg_latency_orig << " ns (" 
              << avg_latency_orig / 1000.0 << " μs)\n";
    std::cout << "  Optimized Avg: " << avg_latency_opt << " ns (" 
              << avg_latency_opt / 1000.0 << " μs)\n";
    double latency_improvement = ((avg_latency_orig - avg_latency_opt) / avg_latency_orig) * 100.0;
    std::cout << "  Improvement: " << latency_improvement << "%\n";
    
    std::cout << "\nMin Latency:\n";
    std::cout << "  Original:  " << min_latency_orig.count() << " ns\n";
    std::cout << "  Optimized: " << min_latency_opt.count() << " ns\n";
    
    std::cout << "\nMax Latency:\n";
    std::cout << "  Original:  " << max_latency_orig.count() << " ns\n";
    std::cout << "  Optimized: " << max_latency_opt.count() << " ns\n";
    
    std::cout << "\nTrades:\n";
    std::cout << "  Original:  " << trades_orig << "\n";
    std::cout << "  Optimized: " << trades_opt << "\n";
    
    std::cout << "\n";
}

int main() {
    std::cout << "Quick Performance Comparison\n";
    std::cout << "============================\n";
    
    // Run small scale test
    run_comparison_test(10000);
    
    std::cout << "\nComparison completed!\n";
    return 0;
}


