#include "core/matching_engine_production_v2.h"
#include "core/types.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <iomanip>
#include <fstream>
#include <sstream>

using namespace perpetual;
using namespace std::chrono;

std::string generateReport(const std::string& version_name, size_t num_orders, 
                          double throughput, double trade_rate, 
                          const std::vector<nanoseconds>& latencies,
                          uint64_t total_trades, uint64_t errors,
                          milliseconds duration) {
    std::ostringstream report;
    report << std::fixed << std::setprecision(2);
    
    report << "========================================\n";
    report << "Production Fast Performance Benchmark Report\n";
    report << "========================================\n";
    report << "Version: " << version_name << "\n";
    report << "Test Date: " << std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() << "\n";
    report << "\n";
    
    report << "Test Configuration:\n";
    report << "  Total Orders:    " << num_orders << "\n";
    report << "  Duration:        " << duration.count() << " ms\n";
    report << "\n";
    
    report << "Results:\n";
    report << "  Throughput:      " << throughput / 1000.0 << " K orders/sec\n";
    report << "  Total Trades:    " << total_trades << "\n";
    report << "  Trade Rate:      " << trade_rate << " %\n";
    report << "  Errors:          " << errors << "\n";
    report << "\n";
    
    if (!latencies.empty()) {
        std::vector<nanoseconds> sorted_latencies = latencies;
        std::sort(sorted_latencies.begin(), sorted_latencies.end());
        
        int64_t total_ns = 0;
        for (const auto& lat : sorted_latencies) {
            total_ns += lat.count();
        }
        
        auto avg_latency = nanoseconds(total_ns / sorted_latencies.size());
        auto min_latency = sorted_latencies.front();
        auto max_latency = sorted_latencies.back();
        auto p50_latency = sorted_latencies[sorted_latencies.size() * 0.5];
        auto p90_latency = sorted_latencies[sorted_latencies.size() * 0.9];
        auto p99_latency = sorted_latencies[sorted_latencies.size() * 0.99];
        
        report << "Latency Statistics:\n";
        report << "  Average:       " << avg_latency.count() / 1000.0 << " μs\n";
        report << "  Minimum:       " << min_latency.count() / 1000.0 << " μs\n";
        report << "  Maximum:       " << max_latency.count() / 1000.0 << " μs\n";
        report << "  P50:           " << p50_latency.count() / 1000.0 << " μs\n";
        report << "  P90:           " << p90_latency.count() / 1000.0 << " μs\n";
        report << "  P99:           " << p99_latency.count() / 1000.0 << " μs\n";
        report << "\n";
    }
    
    report << "Performance Comparison:\n";
    report << "  vs Original (300K/s):   ";
    if (throughput > 300000) {
        report << "✅ " << ((throughput - 300000) / 3000.0) << "% faster\n";
    } else {
        report << "⚠️  " << ((300000 - throughput) / 3000.0) << "% slower\n";
    }
    
    report << "  vs ART+SIMD (750K/s):   ";
    if (throughput > 750000) {
        report << "✅ " << ((throughput - 750000) / 7500.0) << "% faster\n";
    } else {
        report << "⚠️  " << ((750000 - throughput) / 7500.0) << "% slower\n";
    }
    
    report << "  vs Production Basic (15K/s):  ";
    if (throughput > 15000) {
        report << "✅ " << ((throughput - 15000) / 150.0) << "% faster 🎉\n";
    } else {
        report << "⚠️  Slower than expected\n";
    }
    
    report << "\n";
    report << "========================================\n";
    
    return report.str();
}

int main(int argc, char* argv[]) {
    size_t num_orders = 100000;
    if (argc > 1) {
        num_orders = std::stoull(argv[1]);
    }
    
    std::cout << "========================================\n";
    std::cout << "Production Fast Performance Benchmark\n";
    std::cout << "========================================\n";
    std::cout << "Testing " << num_orders << " orders\n\n";
    
    InstrumentID instrument_id = 1;
    ProductionMatchingEngineV2 engine(instrument_id);
    
    // Initialize with performance config
    if (!engine.initialize("")) {
        std::cerr << "Failed to initialize engine, using defaults\n";
    }
    
    // Disable rate limiting for benchmark
    engine.disable_rate_limiting();
    
    std::cout << "Engine initialized successfully\n\n";
    
    // Generate test orders
    std::vector<std::unique_ptr<Order>> orders;
    orders.reserve(num_orders);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> price_dist(40000.0, 60000.0);
    std::uniform_real_distribution<double> qty_dist(0.01, 1.0);
    std::uniform_real_distribution<double> side_dist(0.0, 1.0);
    
    std::cout << "Generating " << num_orders << " test orders...\n";
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
    
    // Warmup
    const size_t warmup = std::min<size_t>(1000, num_orders / 10);
    std::cout << "Warmup with " << warmup << " orders...\n";
    for (size_t i = 0; i < warmup; ++i) {
        try {
            auto order_copy = std::make_unique<Order>(*orders[i]);
            engine.process_order_production_v2(order_copy.release());
        } catch (...) {
            // Ignore errors during warmup
        }
    }
    
    std::cout << "Starting benchmark...\n\n";
    
    // Benchmark
    std::vector<nanoseconds> latencies;
    latencies.reserve(num_orders - warmup);
    uint64_t total_trades = 0;
    uint64_t errors = 0;
    
    auto start = high_resolution_clock::now();
    
    for (size_t i = warmup; i < num_orders; ++i) {
        try {
            auto order_copy = std::make_unique<Order>(*orders[i]);
            auto order_start = high_resolution_clock::now();
            auto trades = engine.process_order_production_v2(order_copy.release());
            auto order_end = high_resolution_clock::now();
            
            latencies.push_back(duration_cast<nanoseconds>(order_end - order_start));
            total_trades += trades.size();
        } catch (const std::exception& e) {
            errors++;
            latencies.push_back(nanoseconds(100000));  // 100μs penalty
        }
    }
    
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    
    // Calculate statistics
    double throughput = ((num_orders - warmup) * 1000.0) / duration.count();
    double trade_rate = (total_trades * 100.0) / (num_orders - warmup);
    
    // Generate and print report
    std::string report = generateReport("production_fast", num_orders - warmup,
                                       throughput, trade_rate, latencies,
                                       total_trades, errors, duration);
    std::cout << report;
    
    // Get metrics
    std::cout << "\nEngine Metrics:\n";
    std::cout << engine.getMetrics();
    
    // Save report to file
    std::ofstream report_file("BENCHMARK_REPORT.md");
    if (report_file.is_open()) {
        report_file << "# Production Fast Benchmark Report\n\n";
        report_file << report;
        report_file << "\n## Engine Metrics\n\n";
        report_file << "```\n";
        report_file << engine.getMetrics();
        report_file << "```\n";
        report_file.close();
        std::cout << "\n✅ Report saved to BENCHMARK_REPORT.md\n";
    }
    
    // Shutdown
    engine.shutdown();
    
    std::cout << "\nBenchmark completed successfully!\n";
    
    return 0;
}

