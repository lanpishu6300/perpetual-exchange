#include "core/matching_engine.h"
#include "core/matching_engine_optimized.h"
#include "core/simd_utils.h"
#include "core/types.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <iomanip>
#include <fstream>

using namespace perpetual;
using namespace std::chrono;

struct PerformanceMetrics {
    std::string test_name;
    size_t num_operations;
    milliseconds total_time;
    nanoseconds avg_latency;
    nanoseconds min_latency;
    nanoseconds max_latency;
    double throughput;
    bool simd_enabled;
};

void print_metrics_table(const std::vector<PerformanceMetrics>& metrics) {
    std::cout << "\n" << std::string(100, '=') << "\n";
    std::cout << "Performance Comparison Summary\n";
    std::cout << std::string(100, '=') << "\n";
    std::cout << std::fixed << std::setprecision(2);
    
    std::cout << "\n" << std::left << std::setw(30) << "Test"
              << std::setw(15) << "Operations"
              << std::setw(15) << "Time (ms)"
              << std::setw(15) << "Avg Latency"
              << std::setw(15) << "Throughput"
              << std::setw(10) << "SIMD\n";
    std::cout << std::string(100, '-') << "\n";
    
    for (const auto& m : metrics) {
        std::cout << std::left << std::setw(30) << m.test_name
                  << std::setw(15) << m.num_operations
                  << std::setw(15) << m.total_time.count()
                  << std::setw(15) << (m.avg_latency.count() / 1000.0) << " μs"
                  << std::setw(15) << (m.throughput / 1000.0) << " K ops/s"
                  << std::setw(10) << (m.simd_enabled ? "Yes" : "No") << "\n";
    }
}

PerformanceMetrics run_benchmark_test(const std::string& name, size_t num_orders, 
                                      bool use_optimized, bool use_simd) {
    InstrumentID instrument_id = 1;
    
    std::unique_ptr<MatchingEngine> engine;
    if (use_optimized) {
        engine = std::make_unique<OptimizedMatchingEngine>(instrument_id);
    } else {
        engine = std::make_unique<MatchingEngine>(instrument_id);
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> price_dist(40000.0, 60000.0);
    std::uniform_real_distribution<double> qty_dist(0.01, 1.0);
    std::uniform_real_distribution<double> side_dist(0.0, 1.0);
    
    std::vector<std::unique_ptr<Order>> orders;
    orders.reserve(num_orders);
    
    uint64_t total_trades = 0;
    nanoseconds total_latency{0};
    nanoseconds min_latency{nanoseconds::max()};
    nanoseconds max_latency{nanoseconds::zero()};
    
    auto start = high_resolution_clock::now();
    
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
        
        std::vector<Trade> trades;
        if (use_optimized) {
            trades = static_cast<OptimizedMatchingEngine*>(engine.get())->process_order_optimized(order.get());
        } else {
            trades = engine->process_order(order.get());
        }
        
        auto order_end = high_resolution_clock::now();
        auto latency = duration_cast<nanoseconds>(order_end - order_start);
        
        total_latency += latency;
        if (latency < min_latency) min_latency = latency;
        if (latency > max_latency) max_latency = latency;
        
        total_trades += trades.size();
        
        if (order->is_active()) {
            orders.push_back(std::move(order));
        }
    }
    
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    
    PerformanceMetrics metrics;
    metrics.test_name = name;
    metrics.num_operations = num_orders;
    metrics.total_time = duration;
    metrics.avg_latency = nanoseconds(total_latency.count() / num_orders);
    metrics.min_latency = min_latency;
    metrics.max_latency = max_latency;
    metrics.throughput = (num_orders * 1000.0) / duration.count();
    metrics.simd_enabled = use_simd;
    
    return metrics;
}

void generate_comparison_report(const std::vector<PerformanceMetrics>& metrics) {
    std::ofstream report("performance_comparison_final.txt");
    
    report << "Perpetual Exchange - Comprehensive Performance Comparison\n";
    report << "==========================================================\n\n";
    
    auto now = system_clock::now();
    auto time_t = system_clock::to_time_t(now);
    report << "Generated: " << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "\n\n";
    
    report << std::fixed << std::setprecision(2);
    
    // Find baseline (original version)
    auto baseline_it = std::find_if(metrics.begin(), metrics.end(),
        [](const PerformanceMetrics& m) {
            return m.test_name.find("Original") != std::string::npos && !m.simd_enabled;
        });
    
    if (baseline_it != metrics.end()) {
        const auto& baseline = *baseline_it;
        report << "Baseline (Original):\n";
        report << "  Throughput: " << baseline.throughput / 1000.0 << " K orders/sec\n";
        report << "  Avg Latency: " << baseline.avg_latency.count() / 1000.0 << " μs\n";
        report << "\n";
        
        report << "Improvements:\n";
        report << std::string(60, '-') << "\n";
        
        for (const auto& m : metrics) {
            if (&m != &baseline) {
                double throughput_improvement = ((m.throughput - baseline.throughput) / baseline.throughput) * 100.0;
                double latency_improvement = ((baseline.avg_latency.count() - m.avg_latency.count()) / baseline.avg_latency.count()) * 100.0;
                
                report << m.test_name << ":\n";
                report << "  Throughput: " << m.throughput / 1000.0 << " K orders/sec";
                if (throughput_improvement > 0) {
                    report << " (+" << throughput_improvement << "%)";
                } else {
                    report << " (" << throughput_improvement << "%)";
                }
                report << "\n";
                
                report << "  Avg Latency: " << m.avg_latency.count() / 1000.0 << " μs";
                if (latency_improvement > 0) {
                    report << " (-" << latency_improvement << "%)";
                } else {
                    report << " (+" << -latency_improvement << "%)";
                }
                report << "\n";
                
                report << "  SIMD Enabled: " << (m.simd_enabled ? "Yes" : "No") << "\n";
                report << "\n";
            }
        }
    }
    
    report.close();
    std::cout << "\nDetailed report saved to: performance_comparison_final.txt\n";
}

int main() {
    std::cout << "========================================\n";
    std::cout << "Comprehensive Performance Comparison\n";
    std::cout << "========================================\n";
    
    std::vector<PerformanceMetrics> all_metrics;
    
    const size_t test_size = 50000;
    
    // Test 1: Original version (baseline)
    std::cout << "\n[1/3] Testing Original Version (Baseline)...\n";
    auto orig_metrics = run_benchmark_test("Original (Baseline)", test_size, false, false);
    all_metrics.push_back(orig_metrics);
    std::cout << "  Throughput: " << orig_metrics.throughput / 1000.0 << " K orders/sec\n";
    std::cout << "  Avg Latency: " << orig_metrics.avg_latency.count() / 1000.0 << " μs\n";
    
    // Test 2: Optimized version (memory pool + lock-free)
    std::cout << "\n[2/3] Testing Optimized Version (Memory Pool + Lock-Free)...\n";
    auto opt_metrics = run_benchmark_test("Optimized (Memory Pool)", test_size, true, false);
    all_metrics.push_back(opt_metrics);
    std::cout << "  Throughput: " << opt_metrics.throughput / 1000.0 << " K orders/sec\n";
    std::cout << "  Avg Latency: " << opt_metrics.avg_latency.count() / 1000.0 << " μs\n";
    
    double opt_improvement = ((opt_metrics.throughput - orig_metrics.throughput) / orig_metrics.throughput) * 100.0;
    std::cout << "  Improvement: " << opt_improvement << "%\n";
    
    // Test 3: Check SIMD availability
    bool simd_available = false;
#if SIMD_AVAILABLE
    simd_available = true;
#endif
    
    if (simd_available) {
        std::cout << "\n[3/3] SIMD is available on this platform\n";
        std::cout << "  Note: Full SIMD benefits require x86_64 with AVX2\n";
        std::cout << "  Run in Docker x86_64 environment for best results\n";
    } else {
        std::cout << "\n[3/3] SIMD not available on this platform\n";
        std::cout << "  Current platform: " << 
#if defined(__x86_64__) || defined(_M_X64)
            "x86_64"
#elif defined(__aarch64__) || defined(__arm64__)
            "ARM64"
#else
            "Unknown"
#endif
            << "\n";
        std::cout << "  For SIMD testing, use Docker x86_64 environment\n";
    }
    
    // Print comparison table
    print_metrics_table(all_metrics);
    
    // Calculate improvements
    std::cout << "\n" << std::string(100, '=') << "\n";
    std::cout << "Performance Improvements\n";
    std::cout << std::string(100, '=') << "\n";
    
    double throughput_improvement = ((opt_metrics.throughput - orig_metrics.throughput) / orig_metrics.throughput) * 100.0;
    double latency_improvement = ((orig_metrics.avg_latency.count() - opt_metrics.avg_latency.count()) / orig_metrics.avg_latency.count()) * 100.0;
    
    std::cout << "\nOptimized vs Original:\n";
    std::cout << "  Throughput improvement: " << throughput_improvement << "%\n";
    std::cout << "  Latency reduction: " << latency_improvement << "%\n";
    
    std::cout << "\nExpected SIMD improvements (in x86_64 Docker):\n";
    std::cout << "  Price comparison: 2-3x faster\n";
    std::cout << "  Quantity sum: 2-4x faster\n";
    std::cout << "  PnL calculation: 2-3x faster\n";
    std::cout << "  Overall throughput: +10-20%\n";
    
    // Generate report
    generate_comparison_report(all_metrics);
    
    std::cout << "\n========================================\n";
    std::cout << "Comparison completed!\n";
    std::cout << "========================================\n";
    
    return 0;
}
