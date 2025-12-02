#include "core/matching_engine.h"
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
};

BenchmarkResult run_test(const std::string& name, size_t num_orders, size_t num_users = 1000) {
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
    
    return result;
}

void print_result(const BenchmarkResult& result) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "Test: " << result.test_name << "\n";
    std::cout << std::string(60, '=') << "\n";
    std::cout << std::fixed << std::setprecision(2);
    
    std::cout << "Total Orders:      " << result.num_orders << "\n";
    std::cout << "Total Trades:     " << result.num_trades << "\n";
    std::cout << "Total Volume:     " << quantity_to_double(result.total_volume) << "\n";
    std::cout << "Total Time:       " << result.total_time.count() << " ms\n";
    std::cout << "Throughput:       " << result.throughput_ops_per_sec / 1000.0 
              << " K orders/sec\n";
    std::cout << "\n";
    
    std::cout << "Latency Statistics:\n";
    std::cout << "  Average:        " << result.avg_latency.count() << " ns\n";
    std::cout << "  Average:        " << result.avg_latency.count() / 1000.0 << " μs\n";
    if (result.min_latency != nanoseconds::max()) {
        std::cout << "  Min:            " << result.min_latency.count() << " ns\n";
        std::cout << "  Min:            " << result.min_latency.count() / 1000.0 << " μs\n";
    }
    if (result.max_latency != nanoseconds::zero()) {
        std::cout << "  Max:            " << result.max_latency.count() << " ns\n";
        std::cout << "  Max:            " << result.max_latency.count() / 1000.0 << " μs\n";
    }
    std::cout << "\n";
}

void generate_report(const std::vector<BenchmarkResult>& results) {
    std::ofstream report("benchmark_report.txt");
    
    report << "Perpetual Exchange - Performance Benchmark Report\n";
    report << "==================================================\n\n";
    
    auto now = system_clock::now();
    auto time_t = system_clock::to_time_t(now);
    report << "Generated: " << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "\n\n";
    
    report << std::fixed << std::setprecision(2);
    
    for (const auto& result : results) {
        report << "Test: " << result.test_name << "\n";
        report << std::string(50, '-') << "\n";
        report << "Total Orders:      " << result.num_orders << "\n";
        report << "Total Trades:      " << result.num_trades << "\n";
        report << "Total Volume:      " << quantity_to_double(result.total_volume) << "\n";
        report << "Total Time:        " << result.total_time.count() << " ms\n";
        report << "Throughput:        " << result.throughput_ops_per_sec / 1000.0 
               << " K orders/sec\n";
        report << "Avg Latency:       " << result.avg_latency.count() << " ns\n";
        report << "Avg Latency:       " << result.avg_latency.count() / 1000.0 << " μs\n";
        report << "Min Latency:       " << result.min_latency.count() << " ns\n";
        report << "Max Latency:       " << result.max_latency.count() << " ns\n";
        report << "\n";
    }
    
    // Summary
    report << "\n" << std::string(50, '=') << "\n";
    report << "Summary\n";
    report << std::string(50, '=') << "\n";
    
    auto max_throughput = std::max_element(results.begin(), results.end(),
        [](const BenchmarkResult& a, const BenchmarkResult& b) {
            return a.throughput_ops_per_sec < b.throughput_ops_per_sec;
        });
    report << "Best Throughput: " << max_throughput->throughput_ops_per_sec / 1000.0 
           << " K orders/sec (" << max_throughput->test_name << ")\n";
    
    auto min_latency_it = std::min_element(results.begin(), results.end(),
        [](const BenchmarkResult& a, const BenchmarkResult& b) {
            return a.avg_latency.count() < b.avg_latency.count();
        });
    report << "Best Avg Latency: " << min_latency_it->avg_latency.count() << " ns ("
           << min_latency_it->avg_latency.count() / 1000.0 << " μs) (" 
           << min_latency_it->test_name << ")\n";
    
    report.close();
    std::cout << "\nBenchmark report saved to: benchmark_report.txt\n";
}

int main() {
    std::cout << "Perpetual Exchange - Full Performance Benchmark\n";
    std::cout << "==============================================\n\n";
    
    std::vector<BenchmarkResult> results;
    
    // Test 1: Small scale
    std::cout << "Running Test 1: Small Scale (1K orders)...\n";
    results.push_back(run_test("Small Scale (1K orders)", 1000, 100));
    print_result(results.back());
    
    // Test 2: Medium scale
    std::cout << "\nRunning Test 2: Medium Scale (10K orders)...\n";
    results.push_back(run_test("Medium Scale (10K orders)", 10000, 500));
    print_result(results.back());
    
    // Test 3: Large scale
    std::cout << "\nRunning Test 3: Large Scale (50K orders)...\n";
    results.push_back(run_test("Large Scale (50K orders)", 50000, 1000));
    print_result(results.back());
    
    // Test 4: Very large scale
    std::cout << "\nRunning Test 4: Very Large Scale (100K orders)...\n";
    results.push_back(run_test("Very Large Scale (100K orders)", 100000, 2000));
    print_result(results.back());
    
    // Generate report
    generate_report(results);
    
    // Print summary
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "Summary\n";
    std::cout << std::string(60, '=') << "\n";
    
    auto max_throughput = std::max_element(results.begin(), results.end(),
        [](const BenchmarkResult& a, const BenchmarkResult& b) {
            return a.throughput_ops_per_sec < b.throughput_ops_per_sec;
        });
    std::cout << "Best Throughput: " << max_throughput->throughput_ops_per_sec / 1000.0 
              << " K orders/sec (" << max_throughput->test_name << ")\n";
    
    auto min_latency_it = std::min_element(results.begin(), results.end(),
        [](const BenchmarkResult& a, const BenchmarkResult& b) {
            return a.avg_latency.count() < b.avg_latency.count();
        });
    std::cout << "Best Avg Latency: " << min_latency_it->avg_latency.count() << " ns ("
              << min_latency_it->avg_latency.count() / 1000.0 << " μs) (" 
              << min_latency_it->test_name << ")\n";
    
    return 0;
}
