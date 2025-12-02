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
};

class PerformanceComparator {
public:
    std::vector<std::unique_ptr<Order>> generateOrders(size_t num_orders, InstrumentID instrument_id) {
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
        
        return orders;
    }
    
    BenchmarkResult calculateResults(const std::string& name,
                                    const std::vector<nanoseconds>& latencies,
                                    uint64_t total_trades,
                                    size_t num_orders,
                                    size_t warmup,
                                    high_resolution_clock::time_point start) {
        BenchmarkResult result;
        result.version_name = name;
        result.num_orders = num_orders;
        
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end - start);
        
        result.total_time = duration;
        result.throughput = ((num_orders - warmup) * 1000.0) / duration.count();
        result.total_trades = total_trades;
        result.trade_rate = (total_trades * 100.0) / (num_orders - warmup);
        
        if (!latencies.empty()) {
            std::vector<nanoseconds> sorted_latencies = latencies;
            std::sort(sorted_latencies.begin(), sorted_latencies.end());
            
            int64_t total_ns = 0;
            for (const auto& lat : sorted_latencies) {
                total_ns += lat.count();
            }
            result.avg_latency = nanoseconds(total_ns / sorted_latencies.size());
            result.min_latency = sorted_latencies.front();
            result.max_latency = sorted_latencies.back();
            result.p50_latency = sorted_latencies[sorted_latencies.size() * 0.5];
            result.p90_latency = sorted_latencies[sorted_latencies.size() * 0.9];
            result.p99_latency = sorted_latencies[sorted_latencies.size() * 0.99];
        }
        
        return result;
    }
    
    BenchmarkResult benchmark_original(size_t num_orders, InstrumentID instrument_id) {
        MatchingEngine engine(instrument_id);
        auto orders = generateOrders(num_orders, instrument_id);
        const size_t warmup = std::min<size_t>(1000, num_orders / 10);
        
        for (size_t i = 0; i < warmup; ++i) {
            engine.process_order(orders[i].get());
        }
        
        std::vector<nanoseconds> latencies;
        latencies.reserve(num_orders - warmup);
        uint64_t total_trades = 0;
        auto start = high_resolution_clock::now();
        
        for (size_t i = warmup; i < num_orders; ++i) {
            auto order_start = high_resolution_clock::now();
            auto trades = engine.process_order(orders[i].get());
            auto order_end = high_resolution_clock::now();
            
            latencies.push_back(duration_cast<nanoseconds>(order_end - order_start));
            total_trades += trades.size();
        }
        
        return calculateResults("Original", latencies, total_trades, num_orders, warmup, start);
    }
    
    BenchmarkResult benchmark_optimized(size_t num_orders, InstrumentID instrument_id) {
        OptimizedMatchingEngine engine(instrument_id);
        auto orders = generateOrders(num_orders, instrument_id);
        const size_t warmup = std::min<size_t>(1000, num_orders / 10);
        
        for (size_t i = 0; i < warmup; ++i) {
            engine.process_order_optimized(orders[i].get());
        }
        
        std::vector<nanoseconds> latencies;
        latencies.reserve(num_orders - warmup);
        uint64_t total_trades = 0;
        auto start = high_resolution_clock::now();
        
        for (size_t i = warmup; i < num_orders; ++i) {
            auto order_start = high_resolution_clock::now();
            auto trades = engine.process_order_optimized(orders[i].get());
            auto order_end = high_resolution_clock::now();
            
            latencies.push_back(duration_cast<nanoseconds>(order_end - order_start));
            total_trades += trades.size();
        }
        
        return calculateResults("Optimized", latencies, total_trades, num_orders, warmup, start);
    }
    
    BenchmarkResult benchmark_optimized_v2(size_t num_orders, InstrumentID instrument_id) {
        MatchingEngineOptimizedV2 engine(instrument_id);
        auto orders = generateOrders(num_orders, instrument_id);
        const size_t warmup = std::min<size_t>(1000, num_orders / 10);
        
        for (size_t i = 0; i < warmup; ++i) {
            engine.process_order_optimized_v2(orders[i].get());
        }
        
        std::vector<nanoseconds> latencies;
        latencies.reserve(num_orders - warmup);
        uint64_t total_trades = 0;
        auto start = high_resolution_clock::now();
        
        for (size_t i = warmup; i < num_orders; ++i) {
            auto order_start = high_resolution_clock::now();
            auto trades = engine.process_order_optimized_v2(orders[i].get());
            auto order_end = high_resolution_clock::now();
            
            latencies.push_back(duration_cast<nanoseconds>(order_end - order_start));
            total_trades += trades.size();
        }
        
        return calculateResults("Optimized V2", latencies, total_trades, num_orders, warmup, start);
    }
    
    BenchmarkResult benchmark_production(size_t num_orders, InstrumentID instrument_id) {
        ProductionMatchingEngine engine(instrument_id);
        engine.initialize(""); // Initialize with defaults
        
        auto orders = generateOrders(num_orders, instrument_id);
        const size_t warmup = std::min<size_t>(1000, num_orders / 10);
        
        for (size_t i = 0; i < warmup; ++i) {
            engine.process_order_production(orders[i].get());
        }
        
        std::vector<nanoseconds> latencies;
        latencies.reserve(num_orders - warmup);
        uint64_t total_trades = 0;
        auto start = high_resolution_clock::now();
        
        for (size_t i = warmup; i < num_orders; ++i) {
            auto order_start = high_resolution_clock::now();
            auto trades = engine.process_order_production(orders[i].get());
            auto order_end = high_resolution_clock::now();
            
            latencies.push_back(duration_cast<nanoseconds>(order_end - order_start));
            total_trades += trades.size();
        }
        
        engine.shutdown();
        return calculateResults("Production", latencies, total_trades, num_orders, warmup, start);
    }
    
    void generateReport(const std::vector<BenchmarkResult>& results, const std::string& filename) {
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
        
        for (const auto& r : results) {
            report << std::left << std::setw(25) << r.version_name
                   << std::setw(15) << (r.throughput / 1000.0) << " K"
                   << std::setw(15) << (r.avg_latency.count() / 1000.0) << " μs"
                   << std::setw(15) << (r.p99_latency.count() / 1000.0) << " μs"
                   << std::setw(15) << r.trade_rate << " %"
                   << std::setw(15) << r.total_trades << "\n";
        }
        
        // Latency distribution
        report << "\n\nLatency Distribution (μs)\n";
        report << std::string(120, '=') << "\n";
        report << std::left << std::setw(25) << "Version"
               << std::setw(15) << "Min"
               << std::setw(15) << "P50"
               << std::setw(15) << "P90"
               << std::setw(15) << "P99"
               << std::setw(15) << "Max\n";
        report << std::string(120, '-') << "\n";
        
        for (const auto& r : results) {
            report << std::left << std::setw(25) << r.version_name
                   << std::setw(15) << (r.min_latency.count() / 1000.0)
                   << std::setw(15) << (r.p50_latency.count() / 1000.0)
                   << std::setw(15) << (r.p90_latency.count() / 1000.0)
                   << std::setw(15) << (r.p99_latency.count() / 1000.0)
                   << std::setw(15) << (r.max_latency.count() / 1000.0) << "\n";
        }
        
        // Improvement analysis
        if (results.size() >= 2) {
            report << "\n\nPerformance Improvements (vs Baseline)\n";
            report << std::string(120, '=') << "\n";
            
            const auto& baseline = results[0];
            
            for (size_t i = 1; i < results.size(); ++i) {
                const auto& current = results[i];
                
                double throughput_improvement = 
                    ((current.throughput - baseline.throughput) / baseline.throughput) * 100.0;
                double latency_improvement = 
                    ((baseline.avg_latency.count() - current.avg_latency.count()) / 
                     baseline.avg_latency.count()) * 100.0;
                double p99_improvement = 
                    ((baseline.p99_latency.count() - current.p99_latency.count()) / 
                     baseline.p99_latency.count()) * 100.0;
                
                report << "\n" << current.version_name << ":\n";
                report << "  Throughput: +" << throughput_improvement << "%\n";
                report << "  Avg Latency: -" << latency_improvement << "%\n";
                report << "  P99 Latency: -" << p99_improvement << "%\n";
            }
        }
        
        report << "\n\n========================================\n";
        report << "End of Report\n";
        report << "========================================\n";
        
        report.close();
    }
    
    void printSummary(const std::vector<BenchmarkResult>& results) {
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
        
        for (const auto& r : results) {
            std::cout << std::left << std::setw(25) << r.version_name
                      << std::setw(15) << (r.throughput / 1000.0) << " K"
                      << std::setw(15) << (r.avg_latency.count() / 1000.0) << " μs"
                      << std::setw(15) << (r.p99_latency.count() / 1000.0) << " μs"
                      << std::setw(15) << r.trade_rate << " %\n";
        }
        
        std::cout << "\n";
    }
};

int main() {
    std::cout << "========================================\n";
    std::cout << "Comprehensive Performance Comparison\n";
    std::cout << "========================================\n\n";
    
    PerformanceComparator comparator;
    InstrumentID instrument_id = 1;
    const size_t num_orders = 50000;
    
    std::cout << "Testing " << num_orders << " orders per version...\n\n";
    
    std::vector<BenchmarkResult> results;
    
    // Test Original Version
    std::cout << "[1/4] Testing Original Version...\n";
    auto result1 = comparator.benchmark_original(num_orders, instrument_id);
    results.push_back(result1);
    std::cout << "  Throughput: " << result1.throughput / 1000.0 << " K orders/sec\n";
    std::cout << "  Avg Latency: " << result1.avg_latency.count() / 1000.0 << " μs\n";
    
    // Test Optimized Version
    std::cout << "\n[2/4] Testing Optimized Version (Memory Pool + Lock-Free)...\n";
    auto result2 = comparator.benchmark_optimized(num_orders, instrument_id);
    results.push_back(result2);
    std::cout << "  Throughput: " << result2.throughput / 1000.0 << " K orders/sec\n";
    std::cout << "  Avg Latency: " << result2.avg_latency.count() / 1000.0 << " μs\n";
    double improvement2 = ((result2.throughput - result1.throughput) / result1.throughput) * 100.0;
    std::cout << "  Improvement: +" << improvement2 << "%\n";
    
    // Test Optimized V2 Version
    std::cout << "\n[3/4] Testing Optimized V2 Version (Hot Path)...\n";
    auto result3 = comparator.benchmark_optimized_v2(num_orders, instrument_id);
    results.push_back(result3);
    std::cout << "  Throughput: " << result3.throughput / 1000.0 << " K orders/sec\n";
    std::cout << "  Avg Latency: " << result3.avg_latency.count() / 1000.0 << " μs\n";
    double improvement3 = ((result3.throughput - result1.throughput) / result1.throughput) * 100.0;
    std::cout << "  Improvement: +" << improvement3 << "%\n";
    
    // Test Production Version
    std::cout << "\n[4/4] Testing Production Version (Full Features)...\n";
    auto result4 = comparator.benchmark_production(num_orders, instrument_id);
    results.push_back(result4);
    std::cout << "  Throughput: " << result4.throughput / 1000.0 << " K orders/sec\n";
    std::cout << "  Avg Latency: " << result4.avg_latency.count() / 1000.0 << " μs\n";
    double improvement4 = ((result4.throughput - result1.throughput) / result1.throughput) * 100.0;
    std::cout << "  Improvement: +" << improvement4 << "%\n";
    
    // Generate report
    comparator.printSummary(results);
    comparator.generateReport(results, "comprehensive_performance_report.txt");
    
    std::cout << "\n========================================\n";
    std::cout << "Detailed report saved to: comprehensive_performance_report.txt\n";
    std::cout << "========================================\n";
    
    return 0;
}
