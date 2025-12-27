#include "core/matching_engine_production_safe_optimized.h"
#include "core/types.h"
#include "core/order.h"
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <cmath>

using namespace std::chrono;
using namespace perpetual;

void print_realtime_stats(size_t processed, size_t total, 
                         double throughput, double avg_latency,
                         double p50, double p90, double p99,
                         const ProductionMatchingEngineSafeOptimized::Stats& stats) {
    std::cout << "\r\033[K";  // Clear line
    
    double progress = static_cast<double>(processed) / total * 100.0;
    int bar_width = 40;
    int pos = static_cast<int>(bar_width * processed / total);
    
    std::cout << "进度: [";
    for (int i = 0; i < bar_width; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << std::fixed << std::setprecision(1) << progress << "%"
              << " | 吞吐量: " << std::setprecision(2) << throughput / 1000.0 << " K/s"
              << " | 延迟: " << std::setprecision(2) << avg_latency << " μs"
              << " | P50: " << std::setprecision(2) << p50 << " μs"
              << " | P90: " << std::setprecision(2) << p90 << " μs"
              << " | P99: " << std::setprecision(2) << p99 << " μs"
              << " | 队列: " << stats.queue_size
              << " | 异步: " << stats.async_writes
              << " | 同步: " << stats.sync_writes;
    std::cout.flush();
}

int main() {
    constexpr size_t NUM_ORDERS = 50000;
    constexpr size_t WARMUP_ORDERS = 1000;
    constexpr size_t REPORT_INTERVAL = 1000;
    constexpr InstrumentID INSTRUMENT_ID = 1;
    
    std::cout << "=== Production Safe Optimized 实时压测 ===\n\n";
    
    ProductionMatchingEngineSafeOptimized engine(INSTRUMENT_ID);
    if (!engine.initialize("", true)) {
        std::cerr << "Failed to initialize engine\n";
        return 1;
    }
    
    engine.disable_rate_limiting();
    std::cout << "引擎初始化完成 | WAL: 已启用 ✅\n";
    std::cout << "测试订单数: " << NUM_ORDERS << " | 报告间隔: " << REPORT_INTERVAL << " 订单\n\n";
    
    // Generate orders
    std::vector<std::unique_ptr<Order>> orders;
    orders.reserve(NUM_ORDERS);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> price_dist(100.0, 200.0);
    std::uniform_int_distribution<size_t> qty_dist(1, 100);
    std::uniform_int_distribution<int> side_dist(0, 1);
    
    for (size_t i = 0; i < NUM_ORDERS; ++i) {
        auto order = std::make_unique<Order>(
            i + 1, 1, INSTRUMENT_ID,
            static_cast<OrderSide>(side_dist(gen)),
            double_to_price(price_dist(gen)),
            double_to_quantity(qty_dist(gen)),
            OrderType::LIMIT
        );
        orders.push_back(std::move(order));
    }
    
    // Warmup
    for (size_t i = 0; i < WARMUP_ORDERS; ++i) {
        engine.process_order_optimized(orders[i].get());
    }
    
    // Benchmark with realtime reporting
    std::vector<double> latencies;
    latencies.reserve(NUM_ORDERS - WARMUP_ORDERS);
    
    auto overall_start = high_resolution_clock::now();
    auto interval_start = overall_start;
    size_t total_trades = 0;
    
    std::cout << "开始压测...\n";
    
    for (size_t i = WARMUP_ORDERS; i < NUM_ORDERS; ++i) {
        auto order_start = high_resolution_clock::now();
        auto trades = engine.process_order_optimized(orders[i].get());
        auto order_end = high_resolution_clock::now();
        
        auto latency = duration_cast<nanoseconds>(order_end - order_start).count() / 1000.0;
        latencies.push_back(latency);
        total_trades += trades.size();
        
        // Realtime reporting
        if ((i - WARMUP_ORDERS + 1) % REPORT_INTERVAL == 0 || i == NUM_ORDERS - 1) {
            size_t processed = i - WARMUP_ORDERS + 1;
            auto now = high_resolution_clock::now();
            auto elapsed = duration_cast<milliseconds>(now - overall_start).count();
            
            // Calculate current throughput
            double current_throughput = processed * 1000.0 / (elapsed > 0 ? elapsed : 1);
            
            // Calculate latency statistics for current interval
            if (latencies.size() > 0) {
                std::vector<double> sorted_latencies = latencies;
                std::sort(sorted_latencies.begin(), sorted_latencies.end());
                
                double avg = 0.0;
                for (auto l : sorted_latencies) avg += l;
                avg /= sorted_latencies.size();
                
                double p50 = sorted_latencies[sorted_latencies.size() * 0.50];
                double p90 = sorted_latencies[sorted_latencies.size() * 0.90];
                double p99 = sorted_latencies[sorted_latencies.size() * 0.99];
                
                auto stats = engine.get_stats();
                print_realtime_stats(processed, NUM_ORDERS - WARMUP_ORDERS,
                                   current_throughput, avg, p50, p90, p99, stats);
            }
        }
    }
    
    auto overall_end = high_resolution_clock::now();
    auto total_duration = duration_cast<milliseconds>(overall_end - overall_start).count();
    
    // Final statistics
    std::sort(latencies.begin(), latencies.end());
    
    double avg_latency = 0.0;
    for (auto l : latencies) avg_latency += l;
    avg_latency /= latencies.size();
    
    double min_latency = latencies.front();
    double max_latency = latencies.back();
    double p50 = latencies[latencies.size() * 0.50];
    double p90 = latencies[latencies.size() * 0.90];
    double p99 = latencies[latencies.size() * 0.99];
    
    double throughput = (NUM_ORDERS - WARMUP_ORDERS) * 1000.0 / total_duration;
    auto stats = engine.get_stats();
    
    std::cout << "\n\n=== 最终结果 ===\n\n";
    std::cout << "吞吐量: " << std::fixed << std::setprecision(2) << throughput / 1000.0 << " K orders/sec\n";
    std::cout << "总耗时: " << total_duration << " ms\n";
    std::cout << "总交易数: " << total_trades << "\n\n";
    
    std::cout << "延迟统计:\n";
    std::cout << "  平均: " << std::setprecision(2) << avg_latency << " μs\n";
    std::cout << "  最小: " << std::setprecision(2) << min_latency << " μs\n";
    std::cout << "  最大: " << std::setprecision(2) << max_latency << " μs\n";
    std::cout << "  P50:  " << std::setprecision(2) << p50 << " μs\n";
    std::cout << "  P90:  " << std::setprecision(2) << p90 << " μs\n";
    std::cout << "  P99:  " << std::setprecision(2) << p99 << " μs\n\n";
    
    std::cout << "WAL统计:\n";
    std::cout << "  异步写入: " << stats.async_writes << "\n";
    std::cout << "  同步写入: " << stats.sync_writes << "\n";
    std::cout << "  同步次数: " << stats.sync_count << "\n";
    std::cout << "  平均同步时间: " << std::setprecision(2) << stats.avg_sync_time_us << " μs\n";
    std::cout << "  队列大小: " << stats.queue_size << "\n";
    std::cout << "  WAL大小: " << stats.wal_size << " bytes\n";
    
    engine.shutdown();
    return 0;
}

#include "core/order.h"
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <cmath>

using namespace std::chrono;
using namespace perpetual;

void print_realtime_stats(size_t processed, size_t total, 
                         double throughput, double avg_latency,
                         double p50, double p90, double p99,
                         const ProductionMatchingEngineSafeOptimized::Stats& stats) {
    std::cout << "\r\033[K";  // Clear line
    
    double progress = static_cast<double>(processed) / total * 100.0;
    int bar_width = 40;
    int pos = static_cast<int>(bar_width * processed / total);
    
    std::cout << "进度: [";
    for (int i = 0; i < bar_width; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << std::fixed << std::setprecision(1) << progress << "%"
              << " | 吞吐量: " << std::setprecision(2) << throughput / 1000.0 << " K/s"
              << " | 延迟: " << std::setprecision(2) << avg_latency << " μs"
              << " | P50: " << std::setprecision(2) << p50 << " μs"
              << " | P90: " << std::setprecision(2) << p90 << " μs"
              << " | P99: " << std::setprecision(2) << p99 << " μs"
              << " | 队列: " << stats.queue_size
              << " | 异步: " << stats.async_writes
              << " | 同步: " << stats.sync_writes;
    std::cout.flush();
}

int main() {
    constexpr size_t NUM_ORDERS = 50000;
    constexpr size_t WARMUP_ORDERS = 1000;
    constexpr size_t REPORT_INTERVAL = 1000;
    constexpr InstrumentID INSTRUMENT_ID = 1;
    
    std::cout << "=== Production Safe Optimized 实时压测 ===\n\n";
    
    ProductionMatchingEngineSafeOptimized engine(INSTRUMENT_ID);
    if (!engine.initialize("", true)) {
        std::cerr << "Failed to initialize engine\n";
        return 1;
    }
    
    engine.disable_rate_limiting();
    std::cout << "引擎初始化完成 | WAL: 已启用 ✅\n";
    std::cout << "测试订单数: " << NUM_ORDERS << " | 报告间隔: " << REPORT_INTERVAL << " 订单\n\n";
    
    // Generate orders
    std::vector<std::unique_ptr<Order>> orders;
    orders.reserve(NUM_ORDERS);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> price_dist(100.0, 200.0);
    std::uniform_int_distribution<size_t> qty_dist(1, 100);
    std::uniform_int_distribution<int> side_dist(0, 1);
    
    for (size_t i = 0; i < NUM_ORDERS; ++i) {
        auto order = std::make_unique<Order>(
            i + 1, 1, INSTRUMENT_ID,
            static_cast<OrderSide>(side_dist(gen)),
            double_to_price(price_dist(gen)),
            double_to_quantity(qty_dist(gen)),
            OrderType::LIMIT
        );
        orders.push_back(std::move(order));
    }
    
    // Warmup
    for (size_t i = 0; i < WARMUP_ORDERS; ++i) {
        engine.process_order_optimized(orders[i].get());
    }
    
    // Benchmark with realtime reporting
    std::vector<double> latencies;
    latencies.reserve(NUM_ORDERS - WARMUP_ORDERS);
    
    auto overall_start = high_resolution_clock::now();
    auto interval_start = overall_start;
    size_t total_trades = 0;
    
    std::cout << "开始压测...\n";
    
    for (size_t i = WARMUP_ORDERS; i < NUM_ORDERS; ++i) {
        auto order_start = high_resolution_clock::now();
        auto trades = engine.process_order_optimized(orders[i].get());
        auto order_end = high_resolution_clock::now();
        
        auto latency = duration_cast<nanoseconds>(order_end - order_start).count() / 1000.0;
        latencies.push_back(latency);
        total_trades += trades.size();
        
        // Realtime reporting
        if ((i - WARMUP_ORDERS + 1) % REPORT_INTERVAL == 0 || i == NUM_ORDERS - 1) {
            size_t processed = i - WARMUP_ORDERS + 1;
            auto now = high_resolution_clock::now();
            auto elapsed = duration_cast<milliseconds>(now - overall_start).count();
            
            // Calculate current throughput
            double current_throughput = processed * 1000.0 / (elapsed > 0 ? elapsed : 1);
            
            // Calculate latency statistics for current interval
            if (latencies.size() > 0) {
                std::vector<double> sorted_latencies = latencies;
                std::sort(sorted_latencies.begin(), sorted_latencies.end());
                
                double avg = 0.0;
                for (auto l : sorted_latencies) avg += l;
                avg /= sorted_latencies.size();
                
                double p50 = sorted_latencies[sorted_latencies.size() * 0.50];
                double p90 = sorted_latencies[sorted_latencies.size() * 0.90];
                double p99 = sorted_latencies[sorted_latencies.size() * 0.99];
                
                auto stats = engine.get_stats();
                print_realtime_stats(processed, NUM_ORDERS - WARMUP_ORDERS,
                                   current_throughput, avg, p50, p90, p99, stats);
            }
        }
    }
    
    auto overall_end = high_resolution_clock::now();
    auto total_duration = duration_cast<milliseconds>(overall_end - overall_start).count();
    
    // Final statistics
    std::sort(latencies.begin(), latencies.end());
    
    double avg_latency = 0.0;
    for (auto l : latencies) avg_latency += l;
    avg_latency /= latencies.size();
    
    double min_latency = latencies.front();
    double max_latency = latencies.back();
    double p50 = latencies[latencies.size() * 0.50];
    double p90 = latencies[latencies.size() * 0.90];
    double p99 = latencies[latencies.size() * 0.99];
    
    double throughput = (NUM_ORDERS - WARMUP_ORDERS) * 1000.0 / total_duration;
    auto stats = engine.get_stats();
    
    std::cout << "\n\n=== 最终结果 ===\n\n";
    std::cout << "吞吐量: " << std::fixed << std::setprecision(2) << throughput / 1000.0 << " K orders/sec\n";
    std::cout << "总耗时: " << total_duration << " ms\n";
    std::cout << "总交易数: " << total_trades << "\n\n";
    
    std::cout << "延迟统计:\n";
    std::cout << "  平均: " << std::setprecision(2) << avg_latency << " μs\n";
    std::cout << "  最小: " << std::setprecision(2) << min_latency << " μs\n";
    std::cout << "  最大: " << std::setprecision(2) << max_latency << " μs\n";
    std::cout << "  P50:  " << std::setprecision(2) << p50 << " μs\n";
    std::cout << "  P90:  " << std::setprecision(2) << p90 << " μs\n";
    std::cout << "  P99:  " << std::setprecision(2) << p99 << " μs\n\n";
    
    std::cout << "WAL统计:\n";
    std::cout << "  异步写入: " << stats.async_writes << "\n";
    std::cout << "  同步写入: " << stats.sync_writes << "\n";
    std::cout << "  同步次数: " << stats.sync_count << "\n";
    std::cout << "  平均同步时间: " << std::setprecision(2) << stats.avg_sync_time_us << " μs\n";
    std::cout << "  队列大小: " << stats.queue_size << "\n";
    std::cout << "  WAL大小: " << stats.wal_size << " bytes\n";
    
    engine.shutdown();
    return 0;
}
