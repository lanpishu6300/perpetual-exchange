#include <iostream>
#include <chrono>
#include <vector>
#include <thread>
#include <atomic>
#include "core/matching_engine_optimized_v3.h"
#include "core/order.h"
#include "core/types.h"
#include <filesystem>
#include <iomanip>

using namespace perpetual;

// 快速性能测试 - 不限制速率，测试最大吞吐量
int main(int argc, char* argv[]) {
    int num_threads = 4;
    int duration_seconds = 5;
    int total_orders = 10000;  // 总订单数，而不是每秒速率
    
    if (argc > 1) num_threads = std::stoi(argv[1]);
    if (argc > 2) duration_seconds = std::stoi(argv[2]);
    if (argc > 3) total_orders = std::stoi(argv[3]);
    
    int orders_per_thread = total_orders / num_threads;
    
    std::cout << "========================================" << std::endl;
    std::cout << "Fast Performance Test: MatchingEngineOptimizedV3" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Threads: " << num_threads << std::endl;
    std::cout << "Duration limit: " << duration_seconds << " seconds" << std::endl;
    std::cout << "Total orders: " << total_orders << std::endl;
    std::cout << "Orders per thread: " << orders_per_thread << std::endl;
    std::cout << "========================================" << std::endl;
    
    // 创建优化版本引擎
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    std::string event_dir = "./test_v3_fast_event_" + std::to_string(timestamp);
    std::string persist_dir = "./test_v3_fast_persist_" + std::to_string(timestamp);
    std::filesystem::create_directories(event_dir);
    std::filesystem::create_directories(persist_dir);
    
    std::cout << "Initializing engine..." << std::endl;
    MatchingEngineOptimizedV3 engine(1);
    if (!engine.initialize(event_dir, persist_dir)) {
        std::cerr << "Failed to initialize engine!" << std::endl;
        return 1;
    }
    
    std::cout << "Starting engine..." << std::endl;
    engine.start();
    
    std::atomic<uint64_t> total_orders_processed{0};
    std::atomic<uint64_t> total_trades_executed{0};
    std::vector<double> latencies;
    latencies.reserve(total_orders);
    std::mutex latencies_mutex;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    auto end_time = start_time + std::chrono::seconds(duration_seconds);
    
    std::cout << "Starting test..." << std::endl;
    
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            uint64_t order_id = static_cast<uint64_t>(t) * 10000000ULL;
            UserID user_id = 1000000 + t * 10000;
            
            int orders_processed = 0;
            while (orders_processed < orders_per_thread && 
                   std::chrono::high_resolution_clock::now() < end_time) {
                auto op_start = std::chrono::high_resolution_clock::now();
                
                uint64_t current_order_id = order_id++;
                // 使用不同的价格以增加匹配机会
                Price price = double_to_price(50000.0 + (current_order_id % 100));
                Order order(current_order_id, user_id, 1, 
                           (current_order_id % 2 == 0) ? OrderSide::BUY : OrderSide::SELL,
                           price, double_to_quantity(0.1),
                           OrderType::LIMIT);
                
                auto trades = engine.process_order_es(&order);
                
                auto op_end = std::chrono::high_resolution_clock::now();
                auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    op_end - op_start).count();
                
                total_orders_processed++;
                total_trades_executed += trades.size();
                orders_processed++;
                
                // 采样记录延迟（每10个订单记录一次）
                if (orders_processed % 10 == 0) {
                    std::lock_guard<std::mutex> lock(latencies_mutex);
                    latencies.push_back(latency);
                }
            }
        });
    }
    
    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto actual_end_time = std::chrono::high_resolution_clock::now();
    auto actual_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        actual_end_time - start_time).count();
    
    std::cout << "Stopping engine..." << std::endl;
    engine.stop();
    
    // 计算统计
    std::sort(latencies.begin(), latencies.end());
    double sum = 0;
    for (double l : latencies) {
        sum += l;
    }
    double avg_latency = latencies.empty() ? 0 : sum / latencies.size();
    double p50 = latencies.empty() ? 0 : latencies[latencies.size() / 2];
    double p90 = latencies.empty() ? 0 : latencies[static_cast<size_t>(latencies.size() * 0.90)];
    double p99 = latencies.empty() ? 0 : latencies[static_cast<size_t>(latencies.size() * 0.99)];
    double min_latency = latencies.empty() ? 0 : latencies[0];
    double max_latency = latencies.empty() ? 0 : latencies.back();
    
    double throughput = actual_duration > 0 ? 
        (total_orders_processed.load() * 1000.0 / actual_duration) : 0;
    
    // 获取引擎统计
    auto stats = engine.getStatistics();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Performance Results" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Total orders processed: " << total_orders_processed.load() << std::endl;
    std::cout << "Total trades executed: " << total_trades_executed.load() << std::endl;
    std::cout << "Actual duration: " << actual_duration << " ms" << std::endl;
    std::cout << "Throughput: " << std::fixed << std::setprecision(2) << throughput << " orders/sec" << std::endl;
    std::cout << "\nLatency Statistics (nanoseconds):" << std::endl;
    std::cout << "  Min: " << std::fixed << std::setprecision(2) << min_latency << " ns" << std::endl;
    std::cout << "  Average: " << avg_latency << " ns (" << (avg_latency / 1000.0) << " μs)" << std::endl;
    std::cout << "  P50: " << p50 << " ns (" << (p50 / 1000.0) << " μs)" << std::endl;
    std::cout << "  P90: " << p90 << " ns (" << (p90 / 1000.0) << " μs)" << std::endl;
    std::cout << "  P99: " << p99 << " ns (" << (p99 / 1000.0) << " μs)" << std::endl;
    std::cout << "  Max: " << max_latency << " ns (" << (max_latency / 1000.0) << " μs)" << std::endl;
    std::cout << "\nEngine Statistics:" << std::endl;
    std::cout << "  Orders processed: " << stats.orders_processed << std::endl;
    std::cout << "  Trades executed: " << stats.trades_executed << std::endl;
    std::cout << "  Avg matching latency: " << stats.avg_matching_latency_ns << " ns (" 
              << (stats.avg_matching_latency_ns / 1000.0) << " μs)" << std::endl;
    std::cout << "\nPersistence Statistics:" << std::endl;
    std::cout << "  Orders persisted: " << stats.persistence_stats.orders_persisted << std::endl;
    std::cout << "  Trades persisted: " << stats.persistence_stats.trades_persisted << std::endl;
    std::cout << "  Batches persisted: " << stats.persistence_stats.batches_persisted << std::endl;
    std::cout << "  Avg persist latency: " << stats.persistence_stats.avg_persist_latency_ns << " ns (" 
              << (stats.persistence_stats.avg_persist_latency_ns / 1000.0) << " μs)" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // 清理
    if (std::filesystem::exists(event_dir)) {
        std::filesystem::remove_all(event_dir);
    }
    if (std::filesystem::exists(persist_dir)) {
        std::filesystem::remove_all(persist_dir);
    }
    
    std::cout << "\n✅ Performance test completed successfully!" << std::endl;
    return 0;
}

