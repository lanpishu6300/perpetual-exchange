#include "core/matching_engine.h"
#include "core/matching_engine_optimized.h"
#include "core/matching_engine_optimized_v2.h"
#include "core/matching_engine_production.h"
#include "core/types.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <numeric>

using namespace perpetual;
using namespace std::chrono;

struct BenchmarkResult {
    std::string version_name;
    size_t num_orders;
    milliseconds total_time;
    nanoseconds avg_latency;
    nanoseconds min_latency;
    nanoseconds max_latency;
    nanoseconds p50_latency;
    nanoseconds p90_latency;
    nanoseconds p99_latency;
    double throughput;
    uint64_t total_trades;
    double trade_rate;
    size_t memory_usage_mb;
};

class PerformanceComparator {
public:
    PerformanceComparator() {
        results_.reserve(10);
    }
    
    template<typename EngineType>
    BenchmarkResult benchmark(const std::string& name, size_t num_orders, 
                             InstrumentID instrument_id) {
        BenchmarkResult result;
        result.version_name = name;
        result.num_orders = num_orders;
        
        // Create engine
        EngineType engine(instrument_id);
        
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
        
        // Warm up
        const size_t warmup = std::min<size_t>(1000, num_orders / 10);
        for (size_t i = 0; i < warmup; ++i) {
            if constexpr (std::is_same_v<EngineType, OptimizedMatchingEngine>) {
                engine.process_order_optimized(orders[i].get());
            } else if constexpr (std::is_same_v<EngineType, MatchingEngineOptimizedV2>) {
                engine.process_order_optimized_v2(orders[i].get());
            } else if constexpr (std::is_same_v<EngineType, ProductionMatchingEngine>) {
                engine.process_order_production(orders[i].get());
            } else {
                engine.process_order(orders[i].get());
            }
        }
        
        // Benchmark
        std::vector<nanoseconds> latencies;
        latencies.reserve(num_orders - warmup);
        
        uint64_t total_trades = 0;
        auto start = high_resolution_clock::now();
        
        for (size_t i = warmup; i < num_orders; ++i) {
            auto order_start = high_resolution_clock::now();
            
            std::vector<Trade> trades;
            if constexpr (std::is_same_v<EngineType, OptimizedMatchingEngine>) {
                trades = engine.process_order_optimized(orders[i].get());
            } else if constexpr (std::is_same_v<EngineType, MatchingEngineOptimizedV2>) {
                trades = engine.process_order_optimized_v2(orders[i].get());
            } else if constexpr (std::is_same_v<EngineType, ProductionMatchingEngine>) {
                trades = engine.process_order_production(orders[i].get());
            } else {
                trades = engine.process_order(orders[i].get());
            }
            
            auto order_end = high_resolution_clock::now();
            auto latency = duration_cast<nanoseconds>(order_end - order_start);
            latencies.push_back(latency);
            
            total_trades += trades.size();
        }
        
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end - start);
        
        // Calculate statistics
        result.total_time = duration;
        result.throughput = ((num_orders - warmup) * 1000.0) / duration.count();
        result.total_trades = total_trades;
        result.trade_rate = (total_trades * 100.0) / (num_orders - warmup);
        
        // Latency statistics
        std::sort(latencies.begin(), latencies.end());
        nanoseconds total_latency{0};
        for (const auto& lat : latencies) {
            total_latency += lat;
        }
        result.avg_latency = nanoseconds(total_latency.count() / latencies.size());
        result.min_latency = latencies.front();
        result.max_latency = latencies.back();
        result.p50_latency = latencies[latencies.size() * 0.5];
        result.p90_latency = latencies[latencies.size() * 0.9];
        result.p99_latency = latencies[latencies.size() * 0.99];
        
        // Memory usage (simplified)
        result.memory_usage_mb = 0; // Would need actual memory profiling
        
        return result;
    }
    
    void addResult(const BenchmarkResult& result) {
        results_.push_back(result);
    }
    
    void generateReport(const std::string& filename) {
        std::ofstream report(filename);
        
        report << "========================================\n";
        report << "Comprehensive Performance Comparison Report\n";
        report << "========================================\n\n";
        
        auto now = system_clock::now();
        auto time_t = system_clock::to_time_t(now);
        report << "Generated: " << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "\n\n";
        
        report << std::fixed << std::setprecision(2);
        
        // Summary table
        report << "Performance Summary\n";
        report << std::string(120, '=') << "\n";
        report << std::left << std::setw(25) << "Version"
               << std::setw(15) << "Throughput"
               << std::setw(15) << "Avg Latency"
               << std::setw(15) << "P99 Latency"
               << std::setw(15) << "Trade Rate"
               << std::setw(15) << "Total Trades\n";
        report << std::string(120, '-') << "\n";
        
        for (const auto& r : results_) {
            report << std::left << std::setw(25) << r.version_name
                   << std::setw(15) << (r.throughput / 1000.0) << " K"
                   << std::setw(15) << (r.avg_latency.count() / 1000.0) << " μs"
                   << std::setw(15) << (r.p99_latency.count() / 1000.0) << " μs"
                   << std::setw(15) << r.trade_rate << " %"
                   << std::setw(15) << r.total_trades << "\n";
        }
        
        // Detailed latency distribution
        report << "\n\nLatency Distribution (μs)\n";
        report << std::string(120, '=') << "\n";
        report << std::left << std::setw(25) << "Version"
               << std::setw(15) << "Min"
               << std::setw(15) << "P50"
               << std::setw(15) << "P90"
               << std::setw(15) << "P99"
               << std::setw(15) << "Max\n";
        report << std::string(120, '-') << "\n";
        
        for (const auto& r : results_) {
            report << std::left << std::setw(25) << r.version_name
                   << std::setw(15) << (r.min_latency.count() / 1000.0)
                   << std::setw(15) << (r.p50_latency.count() / 1000.0)
                   << std::setw(15) << (r.p90_latency.count() / 1000.0)
                   << std::setw(15) << (r.p99_latency.count() / 1000.0)
                   << std::setw(15) << (r.max_latency.count() / 1000.0) << "\n";
        }
        
        // Improvement analysis
        if (results_.size() >= 2) {
            report << "\n\nPerformance Improvements (vs Baseline)\n";
            report << std::string(120, '=') << "\n";
            
            const auto& baseline = results_[0];
            
            for (size_t i = 1; i < results_.size(); ++i) {
                const auto& current = results_[i];
                
                double throughput_improvement = 
                    ((current.throughput - baseline.throughput) / baseline.throughput) * 100.0;
                double latency_improvement = 
                    ((baseline.avg_latency.count() - current.avg_latency.count()) / 
                     baseline.avg_latency.count()) * 100.0;
                double p99_improvement = 
                    ((baseline.p99_latency.count() - current.p99_latency.count()) / 
                     baseline.p99_latency.count()) * 100.0;
                
                report << "\n" << current.version_name << ":\n";
                report << "  Throughput: " << throughput_improvement << "% improvement\n";
                report << "  Avg Latency: " << latency_improvement << "% reduction\n";
                report << "  P99 Latency: " << p99_improvement << "% reduction\n";
            }
        }
        
        report << "\n\n========================================\n";
        report << "End of Report\n";
        report << "========================================\n";
        
        report.close();
    }
    
    void printSummary() {
        std::cout << "\n" << std::string(120, '=') << "\n";
        std::cout << "Performance Comparison Summary\n";
        std::cout << std::string(120, '=') << "\n\n";
        
        std::cout << std::fixed << std::setprecision(2);
        std::cout << std::left << std::setw(25) << "Version"
                  << std::setw(15) << "Throughput"
                  << std::setw(15) << "Avg Latency"
                  << std::setw(15) << "P99 Latency"
                  << std::setw(15) << "Trade Rate\n";
        std::cout << std::string(120, '-') << "\n";
        
        for (const auto& r : results_) {
            std::cout << std::left << std::setw(25) << r.version_name
                      << std::setw(15) << (r.throughput / 1000.0) << " K"
                      << std::setw(15) << (r.avg_latency.count() / 1000.0) << " μs"
                      << std::setw(15) << (r.p99_latency.count() / 1000.0) << " μs"
                      << std::setw(15) << r.trade_rate << " %\n";
        }
        
        std::cout << "\n";
    }
    
private:
    std::vector<BenchmarkResult> results_;
};

int main() {
    std::cout << "========================================\n";
    std::cout << "Comprehensive Performance Comparison\n";
    std::cout << "========================================\n\n";
    
    PerformanceComparator comparator;
    InstrumentID instrument_id = 1;
    const size_t num_orders = 50000;
    
    std::cout << "Testing " << num_orders << " orders per version...\n\n";
    
    // Test Original Version
    std::cout << "[1/4] Testing Original Version...\n";
    auto result1 = comparator.benchmark<MatchingEngine>(
        "Original", num_orders, instrument_id);
    comparator.addResult(result1);
    std::cout << "  Throughput: " << result1.throughput / 1000.0 << " K orders/sec\n";
    std::cout << "  Avg Latency: " << result1.avg_latency.count() / 1000.0 << " μs\n";
    
    // Test Optimized Version
    std::cout << "\n[2/4] Testing Optimized Version (Memory Pool + Lock-Free)...\n";
    auto result2 = comparator.benchmark<OptimizedMatchingEngine>(
        "Optimized", num_orders, instrument_id);
    comparator.addResult(result2);
    std::cout << "  Throughput: " << result2.throughput / 1000.0 << " K orders/sec\n";
    std::cout << "  Avg Latency: " << result2.avg_latency.count() / 1000.0 << " μs\n";
    double improvement2 = ((result2.throughput - result1.throughput) / result1.throughput) * 100.0;
    std::cout << "  Improvement: " << improvement2 << "%\n";
    
    // Test Optimized V2 Version
    std::cout << "\n[3/4] Testing Optimized V2 Version (Hot Path)...\n";
    auto result3 = comparator.benchmark<MatchingEngineOptimizedV2>(
        "Optimized V2", num_orders, instrument_id);
    comparator.addResult(result3);
    std::cout << "  Throughput: " << result3.throughput / 1000.0 << " K orders/sec\n";
    std::cout << "  Avg Latency: " << result3.avg_latency.count() / 1000.0 << " μs\n";
    double improvement3 = ((result3.throughput - result1.throughput) / result1.throughput) * 100.0;
    std::cout << "  Improvement: " << improvement3 << "%\n";
    
    // Test Production Version (with all features)
    std::cout << "\n[4/4] Testing Production Version (Full Features)...\n";
    ProductionMatchingEngine engine_prod(instrument_id);
    engine_prod.initialize(""); // Initialize with defaults
    
    // Manual benchmark for production version
    BenchmarkResult result4;
    result4.version_name = "Production";
    result4.num_orders = num_orders;
    
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
    
    const size_t warmup = 1000;
    for (size_t i = 0; i < warmup; ++i) {
        engine_prod.process_order_production(orders[i].get());
    }
    
    std::vector<nanoseconds> latencies;
    latencies.reserve(num_orders - warmup);
    uint64_t total_trades = 0;
    
    auto start = high_resolution_clock::now();
    for (size_t i = warmup; i < num_orders; ++i) {
        auto order_start = high_resolution_clock::now();
        auto trades = engine_prod.process_order_production(orders[i].get());
        auto order_end = high_resolution_clock::now();
        
        latencies.push_back(duration_cast<nanoseconds>(order_end - order_start));
        total_trades += trades.size();
    }
    auto end = high_resolution_clock::now();
    
    result4.total_time = duration_cast<milliseconds>(end - start);
    result4.throughput = ((num_orders - warmup) * 1000.0) / result4.total_time.count();
    result4.total_trades = total_trades;
    result4.trade_rate = (total_trades * 100.0) / (num_orders - warmup);
    
    std::sort(latencies.begin(), latencies.end());
    nanoseconds total_latency{0};
    for (const auto& lat : latencies) {
        total_latency += lat;
    }
    result4.avg_latency = nanoseconds(total_latency.count() / latencies.size());
    result4.min_latency = latencies.front();
    result4.max_latency = latencies.back();
    result4.p50_latency = latencies[latencies.size() * 0.5];
    result4.p90_latency = latencies[latencies.size() * 0.9];
    result4.p99_latency = latencies[latencies.size() * 0.99];
    
    comparator.addResult(result4);
    std::cout << "  Throughput: " << result4.throughput / 1000.0 << " K orders/sec\n";
    std::cout << "  Avg Latency: " << result4.avg_latency.count() / 1000.0 << " μs\n";
    double improvement4 = ((result4.throughput - result1.throughput) / result1.throughput) * 100.0;
    std::cout << "  Improvement: " << improvement4 << "%\n";
    
    // Generate report
    comparator.printSummary();
    comparator.generateReport("comprehensive_performance_report.txt");
    
    std::cout << "\n========================================\n";
    std::cout << "Detailed report saved to: comprehensive_performance_report.txt\n";
    std::cout << "========================================\n";
    
    return 0;
}

