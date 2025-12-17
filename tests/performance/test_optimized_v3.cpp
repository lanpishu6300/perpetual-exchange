#include <iostream>
#include <iomanip>
#include <chrono>
#include <vector>
#include <thread>
#include <atomic>
#include <algorithm>
#include "core/matching_engine_optimized_v3.h"
#include "core/order.h"
#include "core/types.h"
#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#error "No filesystem support"
#endif

using namespace perpetual;

// 测试优化版本V3的性能
int main(int argc, char* argv[]) {
    int num_threads = 8;
    int duration_seconds = 20;  // 默认20秒
    int orders_per_second = 10000;
    
    if (argc > 1) num_threads = std::stoi(argv[1]);
    if (argc > 2) duration_seconds = std::stoi(argv[2]);
    if (argc > 3) orders_per_second = std::stoi(argv[3]);
    
    std::cout << "========================================" << std::endl;
    std::cout << "Testing MatchingEngineOptimizedV3" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Threads: " << num_threads << std::endl;
    std::cout << "Duration: " << duration_seconds << " seconds" << std::endl;
    std::cout << "Orders/sec/thread: " << orders_per_second << std::endl;
    std::cout << "Target TPS: " << (num_threads * orders_per_second) << std::endl;
    std::cout << "========================================" << std::endl;
    
    // 创建优化版本引擎
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    std::string event_dir = "./test_v3_event_" + std::to_string(timestamp);
    std::string persist_dir = "./test_v3_persist_" + std::to_string(timestamp);
    fs::create_directories(event_dir);
    fs::create_directories(persist_dir);
    
    MatchingEngineOptimizedV3 engine(1);
    engine.initialize(event_dir, persist_dir);
    engine.start();
    
    std::atomic<uint64_t> total_orders{0};
    std::atomic<uint64_t> total_trades{0};
    std::vector<double> latencies;
    latencies.reserve(num_threads * orders_per_second * duration_seconds / 10);
    std::mutex latencies_mutex;
    
    auto start_time = std::chrono::steady_clock::now();
    auto end_time = start_time + std::chrono::seconds(duration_seconds);
    
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            uint64_t order_id = static_cast<uint64_t>(t) * 10000000ULL;
            UserID user_id = 1000000 + t * 10000;
            
            while (std::chrono::steady_clock::now() < end_time) {
                auto op_start = std::chrono::high_resolution_clock::now();
                
                uint64_t current_order_id = order_id++;
                Order order(current_order_id, user_id, 1, 
                           (current_order_id % 2 == 0) ? OrderSide::BUY : OrderSide::SELL,
                           double_to_price(50000.0), double_to_quantity(0.1),
                           OrderType::LIMIT);
                
                auto trades = engine.process_order_es(&order);
                
                auto op_end = std::chrono::high_resolution_clock::now();
                auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    op_end - op_start).count();
                
                total_orders++;
                total_trades += trades.size();
                
                // 采样记录延迟
                if (order_id % 10 == 0) {
                    std::lock_guard<std::mutex> lock(latencies_mutex);
                    latencies.push_back(latency);
                }
                
                // Rate limiting
                std::this_thread::sleep_for(
                    std::chrono::microseconds(1000000 / orders_per_second));
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    engine.stop();
    
    auto actual_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start_time).count();
    
    // 计算统计
    std::sort(latencies.begin(), latencies.end());
    double sum = 0;
    for (double l : latencies) {
        sum += l;
    }
    double avg_latency = latencies.empty() ? 0 : sum / latencies.size();
    double p50 = latencies.empty() ? 0 : latencies[latencies.size() / 2];
    double p99 = latencies.empty() ? 0 : latencies[static_cast<size_t>(latencies.size() * 0.99)];
    
    double throughput = actual_duration > 0 ? 
        (total_orders.load() * 1000.0 / actual_duration) : 0;
    
    // 获取引擎统计
    auto stats = engine.getStatistics();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Performance Results" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Total orders: " << total_orders.load() << std::endl;
    std::cout << "Total trades: " << total_trades.load() << std::endl;
    std::cout << "Throughput: " << std::fixed << std::setprecision(2) << throughput << " orders/sec" << std::endl;
    std::cout << "\nLatency Statistics:" << std::endl;
    std::cout << "  Average: " << avg_latency / 1000.0 << " μs" << std::endl;
    std::cout << "  P50: " << p50 / 1000.0 << " μs" << std::endl;
    std::cout << "  P99: " << p99 / 1000.0 << " μs" << std::endl;
    std::cout << "\nEngine Statistics:" << std::endl;
    std::cout << "  Orders processed: " << stats.orders_processed << std::endl;
    std::cout << "  Trades executed: " << stats.trades_executed << std::endl;
    std::cout << "  Avg matching latency: " << stats.avg_matching_latency_ns / 1000.0 << " μs" << std::endl;
    std::cout << "\nPersistence Statistics:" << std::endl;
    std::cout << "  Orders persisted: " << stats.persistence_stats.orders_persisted << std::endl;
    std::cout << "  Trades persisted: " << stats.persistence_stats.trades_persisted << std::endl;
    std::cout << "  Batches persisted: " << stats.persistence_stats.batches_persisted << std::endl;
    std::cout << "  Queue size: " << stats.persistence_stats.queue_size << std::endl;
    std::cout << "  Avg persist latency: " << stats.persistence_stats.avg_persist_latency_ns / 1000.0 << " μs" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // 清理
    if (fs::exists(event_dir)) {
        fs::remove_all(event_dir);
    }
    if (fs::exists(persist_dir)) {
        fs::remove_all(persist_dir);
    }
    
    return 0;
}

