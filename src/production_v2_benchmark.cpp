#include "core/matching_engine_production_v2.h"
#include "core/types.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <iomanip>

using namespace perpetual;
using namespace std::chrono;

int main(int argc, char* argv[]) {
    size_t num_orders = 5000;
    if (argc > 1) {
        num_orders = std::stoi(argv[1]);
    }
    
    std::cout << "========================================\n";
    std::cout << "Production V2 Performance Benchmark\n";
    std::cout << "========================================\n";
    std::cout << "Testing " << num_orders << " orders\n\n";
    
    InstrumentID instrument_id = 1;
    ProductionMatchingEngineV2 engine(instrument_id);
    
    // Initialize with performance config
    if (!engine.initialize("config_performance.ini")) {
        std::cerr << "Failed to initialize engine\n";
        return 1;
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
    const size_t warmup = std::min<size_t>(500, num_orders / 10);
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
    std::sort(latencies.begin(), latencies.end());
    
    int64_t total_ns = 0;
    for (const auto& lat : latencies) {
        total_ns += lat.count();
    }
    
    auto avg_latency = nanoseconds(total_ns / latencies.size());
    auto min_latency = latencies.front();
    auto max_latency = latencies.back();
    auto p50_latency = latencies[latencies.size() * 0.5];
    auto p90_latency = latencies[latencies.size() * 0.9];
    auto p99_latency = latencies[latencies.size() * 0.99];
    
    double throughput = ((num_orders - warmup) * 1000.0) / duration.count();
    double trade_rate = (total_trades * 100.0) / (num_orders - warmup);
    
    // Print results
    std::cout << "========================================\n";
    std::cout << "Benchmark Results\n";
    std::cout << "========================================\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Total Orders:    " << (num_orders - warmup) << "\n";
    std::cout << "Total Trades:    " << total_trades << "\n";
    std::cout << "Errors:          " << errors << "\n";
    std::cout << "Duration:        " << duration.count() << " ms\n";
    std::cout << "\n";
    std::cout << "Throughput:      " << throughput / 1000.0 << " K orders/sec\n";
    std::cout << "Trade Rate:      " << trade_rate << " %\n";
    std::cout << "\n";
    std::cout << "Latency Statistics:\n";
    std::cout << "  Average:       " << avg_latency.count() / 1000.0 << " μs\n";
    std::cout << "  Minimum:       " << min_latency.count() / 1000.0 << " μs\n";
    std::cout << "  Maximum:       " << max_latency.count() / 1000.0 << " μs\n";
    std::cout << "  P50:           " << p50_latency.count() / 1000.0 << " μs\n";
    std::cout << "  P90:           " << p90_latency.count() / 1000.0 << " μs\n";
    std::cout << "  P99:           " << p99_latency.count() / 1000.0 << " μs\n";
    std::cout << "\n";
    
    // Compare with targets
    std::cout << "Performance Comparison:\n";
    std::cout << "  vs Original (300K/s):   ";
    if (throughput > 300000) {
        std::cout << "✅ " << ((throughput - 300000) / 3000.0) << "% faster\n";
    } else {
        std::cout << "⚠️  " << ((300000 - throughput) / 3000.0) << "% slower\n";
    }
    
    std::cout << "  vs ART+SIMD (750K/s):   ";
    if (throughput > 750000) {
        std::cout << "✅ " << ((throughput - 750000) / 7500.0) << "% faster\n";
    } else {
        std::cout << "⚠️  " << ((750000 - throughput) / 7500.0) << "% slower\n";
    }
    
    std::cout << "  vs Production (15K/s):  ";
    if (throughput > 15000) {
        std::cout << "✅ " << ((throughput - 15000) / 150.0) << "% faster 🎉\n";
    } else {
        std::cout << "⚠️  Slower than expected\n";
    }
    
    std::cout << "\n";
    std::cout << "========================================\n";
    
    // Get metrics
    std::cout << "\nEngine Metrics:\n";
    std::cout << engine.getMetrics();
    
    // Shutdown
    engine.shutdown();
    
    std::cout << "\nBenchmark completed successfully!\n";
    
    return 0;
}
