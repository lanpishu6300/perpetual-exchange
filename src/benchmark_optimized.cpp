#include "core/matching_engine_optimized.h"
#include "core/types.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <iomanip>
#include <fstream>
#include <algorithm>

using namespace perpetual;
using namespace std::chrono;

struct BenchmarkResult {
    std::string test_name;
    size_t num_orders;
    size_t num_trades;
    uint64_t total_volume;
    milliseconds total_time;
    nanoseconds avg_latency;
    nanoseconds min_latency;
    nanoseconds max_latency;
    double throughput_ops_per_sec;
    size_t memory_pool_allocations;
    size_t memory_pool_blocks;
};

BenchmarkResult run_test_optimized(const std::string& name, size_t num_orders, size_t num_users = 1000) {
    InstrumentID instrument_id = 1;
    OptimizedMatchingEngine engine(instrument_id);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> price_dist(40000.0, 60000.0);
    std::uniform_real_distribution<double> qty_dist(0.01, 1.0);
    std::uniform_real_distribution<double> side_dist(0.0, 1.0);
    
    std::vector<std::unique_ptr<Order>> orders;
    orders.reserve(num_orders);
    
    uint64_t total_trades = 0;
    uint64_t total_volume = 0;
    nanoseconds total_latency{0};
    nanoseconds min_latency{nanoseconds::max()};
    nanoseconds max_latency{nanoseconds::zero()};
    
    auto start = high_resolution_clock::now();
    
    for (size_t i = 0; i < num_orders; ++i) {
        UserID user_id = (i % num_users) + 1;
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
        } catch (...) {
            // Ignore errors for benchmark
        }
        
        if ((i + 1) % std::max<size_t>(num_orders / 10, 1000) == 0) {
            std::cout << "  Progress: " << ((i + 1) * 100 / num_orders) << "%\n";
            std::cout.flush();
        }
    }
    
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    
    BenchmarkResult result;
    result.test_name = name;
    result.num_orders = num_orders;
    result.num_trades = total_trades;
    result.total_volume = total_volume;
    result.total_time = duration;
    result.avg_latency = nanoseconds(total_latency.count() / num_orders);
    result.min_latency = min_latency;
    result.max_latency = max_latency;
    result.throughput_ops_per_sec = (num_orders * 1000.0) / duration.count();
    result.memory_pool_allocations = engine.memory_pool_allocations();
    result.memory_pool_blocks = engine.memory_pool_blocks();
    
    return result;
}

BenchmarkResult run_test_original(const std::string& name, size_t num_orders, size_t num_users = 1000) {
    InstrumentID instrument_id = 1;
    MatchingEngine engine(instrument_id);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> price_dist(40000.0, 60000.0);
    std::uniform_real_distribution<double> qty_dist(0.01, 1.0);
    std::uniform_real_distribution<double> side_dist(0.0, 1.0);
    
    std::vector<std::unique_ptr<Order>> orders;
    orders.reserve(num_orders);
    
    uint64_t total_trades = 0;
    uint64_t total_volume = 0;
    nanoseconds total_latency{0};
    nanoseconds min_latency{nanoseconds::max()};
    nanoseconds max_latency{nanoseconds::zero()};
    
    auto start = high_resolution_clock::now();
    
    for (size_t i = 0; i < num_orders; ++i) {
        UserID user_id = (i % num_users) + 1;
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
        } catch (...) {
            // Ignore errors for benchmark
        }
        
        if ((i + 1) % std::max<size_t>(num_orders / 10, 1000) == 0) {
            std::cout << "  Progress: " << ((i + 1) * 100 / num_orders) << "%\n";
            std::cout.flush();
        }
    }
    
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    
    BenchmarkResult result;
    result.test_name = name;
    result.num_orders = num_orders;
    result.num_trades = total_trades;
    result.total_volume = total_volume;
    result.total_time = duration;
    result.avg_latency = nanoseconds(total_latency.count() / num_orders);
    result.min_latency = min_latency;
    result.max_latency = max_latency;
    result.throughput_ops_per_sec = (num_orders * 1000.0) / duration.count();
    result.memory_pool_allocations = 0;
    result.memory_pool_blocks = 0;
    
    return result;
}

void print_comparison(const BenchmarkResult& original, const BenchmarkResult& optimized) {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "Performance Comparison: " << original.test_name << "\n";
    std::cout << std::string(70, '=') << "\n";
    std::cout << std::fixed << std::setprecision(2);
    
    std::cout << "\nThroughput:\n";
    std::cout << "  Original:  " << original.throughput_ops_per_sec / 1000.0 << " K orders/sec\n";
    std::cout << "  Optimized: " << optimized.throughput_ops_per_sec / 1000.0 << " K orders/sec\n";
    double throughput_improvement = ((optimized.throughput_ops_per_sec - original.throughput_ops_per_sec) 
                                     / original.throughput_ops_per_sec) * 100.0;
    std::cout << "  Improvement: " << throughput_improvement << "%\n";
    
    std::cout << "\nLatency:\n";
    std::cout << "  Original Avg:  " << original.avg_latency.count() << " ns (" 
              << original.avg_latency.count() / 1000.0 << " μs)\n";
    std::cout << "  Optimized Avg: " << optimized.avg_latency.count() << " ns (" 
              << optimized.avg_latency.count() / 1000.0 << " μs)\n";
    double latency_improvement = ((original.avg_latency.count() - optimized.avg_latency.count()) 
                                  / original.avg_latency.count()) * 100.0;
    std::cout << "  Improvement: " << latency_improvement << "%\n";
    
    std::cout << "\nMemory:\n";
    std::cout << "  Original:  Dynamic allocation\n";
    std::cout << "  Optimized: " << optimized.memory_pool_allocations << " allocations, "
              << optimized.memory_pool_blocks << " blocks\n";
    
    std::cout << "\n";
}

void generate_comparison_report(const std::vector<std::pair<BenchmarkResult, BenchmarkResult>>& comparisons) {
    std::ofstream report("benchmark_comparison_report.txt");
    
    report << "High-Performance Matching Engine - Optimized vs Original Performance Comparison\n";
    report << "==================================================================\n\n";
    
    auto now = system_clock::now();
    auto time_t = system_clock::to_time_t(now);
    report << "Generated: " << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "\n\n";
    
    report << std::fixed << std::setprecision(2);
    
    for (const auto& [original, optimized] : comparisons) {
        report << "Test: " << original.test_name << "\n";
        report << std::string(60, '-') << "\n";
        
        report << "Throughput:\n";
        report << "  Original:  " << original.throughput_ops_per_sec / 1000.0 << " K orders/sec\n";
        report << "  Optimized: " << optimized.throughput_ops_per_sec / 1000.0 << " K orders/sec\n";
        double throughput_improvement = ((optimized.throughput_ops_per_sec - original.throughput_ops_per_sec) 
                                         / original.throughput_ops_per_sec) * 100.0;
        report << "  Improvement: " << throughput_improvement << "%\n\n";
        
        report << "Latency:\n";
        report << "  Original Avg:  " << original.avg_latency.count() << " ns\n";
        report << "  Optimized Avg: " << optimized.avg_latency.count() << " ns\n";
        double latency_improvement = ((original.avg_latency.count() - optimized.avg_latency.count()) 
                                      / original.avg_latency.count()) * 100.0;
        report << "  Improvement: " << latency_improvement << "%\n\n";
        
        report << "Memory:\n";
        report << "  Original:  Dynamic allocation\n";
        report << "  Optimized: " << optimized.memory_pool_allocations << " allocations\n\n";
    }
    
    report.close();
    std::cout << "\nComparison report saved to: benchmark_comparison_report.txt\n";
}

int main() {
    std::cout << "High-Performance Matching Engine - Optimized Performance Benchmark\n";
    std::cout << "====================================================\n\n";
    
    std::vector<std::pair<BenchmarkResult, BenchmarkResult>> comparisons;
    
    // Test 1: Small scale
    std::cout << "Running Test 1: Small Scale (10K orders)...\n";
    std::cout << "  Original version...\n";
    auto orig1 = run_test_original("Small Scale (10K)", 10000, 100);
    std::cout << "  Optimized version...\n";
    auto opt1 = run_test_optimized("Small Scale (10K)", 10000, 100);
    print_comparison(orig1, opt1);
    comparisons.push_back({orig1, opt1});
    
    // Test 2: Medium scale
    std::cout << "\nRunning Test 2: Medium Scale (50K orders)...\n";
    std::cout << "  Original version...\n";
    auto orig2 = run_test_original("Medium Scale (50K)", 50000, 500);
    std::cout << "  Optimized version...\n";
    auto opt2 = run_test_optimized("Medium Scale (50K)", 50000, 500);
    print_comparison(orig2, opt2);
    comparisons.push_back({orig2, opt2});
    
    // Generate report
    generate_comparison_report(comparisons);
    
    return 0;
}
