#!/bin/bash
# Benchmark模板生成脚本

create_benchmark() {
    local version_dir=$1
    local class_name=$2
    local method_name=$3
    local description=$4
    local performance=$5
    
    local header_file="core/matching_engine"
    if [ "$version_dir" = "optimized" ]; then
        header_file="core/matching_engine_optimized"
    elif [ "$version_dir" = "optimized_v2" ]; then
        header_file="core/matching_engine_optimized_v2"
    elif [ "$version_dir" = "art" ]; then
        header_file="core/matching_engine_art"
    elif [ "$version_dir" = "art_simd" ]; then
        header_file="core/matching_engine_art_simd"
    elif [ "$version_dir" = "event_sourcing" ]; then
        header_file="core/matching_engine_event_sourcing"
    elif [ "$version_dir" = "production_basic" ]; then
        header_file="core/matching_engine_production"
    fi
    
    cat > "$version_dir/benchmark.cpp" << BENCHMARK_CPP
#include "$header_file.h"
#include "core/types.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <algorithm>

using namespace perpetual;
using namespace std::chrono;

std::string generateReport(const std::string& version_name, size_t num_orders, 
                          double throughput, double trade_rate, 
                          const std::vector<nanoseconds>& latencies,
                          uint64_t total_trades, uint64_t errors,
                          milliseconds duration) {
    std::ostringstream report;
    report << std::fixed << std::setprecision(2);
    
    report << "# " << version_name << " Performance Benchmark Report\\n\\n";
    report << "## Test Overview\\n\\n";
    report << "- **Version**: " << version_name << "\\n";
    report << "- **Test Date**: " << std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() << "\\n";
    report << "- **Total Orders**: " << num_orders << "\\n";
    report << "- **Duration**: " << duration.count() << " ms\\n\\n";
    
    report << "## Results\\n\\n";
    report << "| Metric | Value |\\n";
    report << "|--------|-------|\\n";
    report << "| Throughput | " << throughput / 1000.0 << " K orders/sec |\\n";
    report << "| Total Trades | " << total_trades << " |\\n";
    report << "| Trade Rate | " << trade_rate << " % |\\n";
    report << "| Errors | " << errors << " |\\n\\n";
    
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
        
        report << "## Latency Statistics\\n\\n";
        report << "| Percentile | Latency (μs) |\\n";
        report << "|------------|---------------|\\n";
        report << "| Average | " << avg_latency.count() / 1000.0 << " |\\n";
        report << "| Minimum | " << min_latency.count() / 1000.0 << " |\\n";
        report << "| Maximum | " << max_latency.count() / 1000.0 << " |\\n";
        report << "| P50 | " << p50_latency.count() / 1000.0 << " |\\n";
        report << "| P90 | " << p90_latency.count() / 1000.0 << " |\\n";
        report << "| P99 | " << p99_latency.count() / 1000.0 << " |\\n\\n";
    }
    
    report << "## Version Characteristics\\n\\n";
    report << "- **Implementation**: " << "$description" << "\\n";
    report << "- **Performance Target**: " << "$performance" << "\\n";
    report << "- **Use Case**: Performance testing\\n\\n";
    
    return report.str();
}

int main(int argc, char* argv[]) {
    size_t num_orders = 100000;
    if (argc > 1) {
        num_orders = std::stoull(argv[1]);
    }
    
    std::cout << "========================================\\n";
    std::cout << "$version_dir Version Performance Benchmark\\n";
    std::cout << "========================================\\n";
    std::cout << "Testing " << num_orders << " orders\\n\\n";
    
    InstrumentID instrument_id = 1;
    $class_name engine(instrument_id);
    
    std::cout << "Engine initialized successfully\\n\\n";
    
    // Generate test orders
    std::vector<std::unique_ptr<Order>> orders;
    orders.reserve(num_orders);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> price_dist(40000.0, 60000.0);
    std::uniform_real_distribution<double> qty_dist(0.01, 1.0);
    std::uniform_real_distribution<double> side_dist(0.0, 1.0);
    
    std::cout << "Generating " << num_orders << " test orders...\\n";
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
    std::cout << "Warmup with " << warmup << " orders...\\n";
    for (size_t i = 0; i < warmup; ++i) {
        try {
            auto order_copy = std::make_unique<Order>(*orders[i]);
            engine.$method_name(order_copy.release());
        } catch (...) {
            // Ignore errors during warmup
        }
    }
    
    std::cout << "Starting benchmark...\\n\\n";
    
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
            auto trades = engine.$method_name(order_copy.release());
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
    std::string report = generateReport("$version_dir", num_orders - warmup,
                                       throughput, trade_rate, latencies,
                                       total_trades, errors, duration);
    std::cout << report;
    
    // Save report to file
    std::ofstream report_file("BENCHMARK_REPORT.md");
    if (report_file.is_open()) {
        report_file << report;
        report_file.close();
        std::cout << "\\n✅ Report saved to BENCHMARK_REPORT.md\\n";
    }
    
    std::cout << "\\nBenchmark completed successfully!\\n";
    
    return 0;
}
BENCHMARK_CPP
    echo "✅ Created benchmark.cpp for $version_dir"
}

# 创建所有版本的benchmark
create_benchmark "optimized" "OptimizedMatchingEngine" "process_order_optimized" "Memory Pool + Lock-Free" "~300K orders/sec, ~3μs"
create_benchmark "optimized_v2" "MatchingEngineOptimizedV2" "process_order_optimized_v2" "Hot Path Optimization" "~321K orders/sec, ~3μs"
create_benchmark "art" "MatchingEngineART" "process_order_art" "Adaptive Radix Tree" "~410K orders/sec, ~2.3μs"
create_benchmark "art_simd" "MatchingEngineARTSIMD" "process_order_art_simd" "ART + SIMD" "~750K orders/sec, ~1.2μs"
create_benchmark "event_sourcing" "MatchingEngineEventSourcing" "process_order_es" "Event Sourcing" "~300K orders/sec, ~3μs"
create_benchmark "production_basic" "ProductionMatchingEngine" "process_order_production" "Full Production Features" "~15K orders/sec, ~13μs"

echo "✅ All benchmarks created!"
