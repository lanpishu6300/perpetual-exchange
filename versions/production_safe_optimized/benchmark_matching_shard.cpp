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
#include <thread>
#include <memory>
#include <mutex>
#include <atomic>
#include <limits>

using namespace std::chrono;
using namespace perpetual;

int main(int argc, char* argv[]) {
    size_t num_orders = 1000000;  // 默认100万订单，压测时间更长
    size_t num_shards = std::thread::hardware_concurrency();
    if (num_shards == 0) num_shards = 4;
    bool enable_wal = false;  // 禁用WAL以测试纯撮合性能
    
    if (argc > 1) {
        num_orders = std::stoull(argv[1]);
    }
    if (argc > 2) {
        num_shards = std::stoull(argv[2]);
    }
    if (argc > 3) {
        enable_wal = (std::stoull(argv[3]) != 0);
    }
    
    std::cout << "╔══════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║    撮合分片独立压测 (Matching Shard Stress Test)            ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;
    std::cout << "测试配置:" << std::endl;
    std::cout << "  • 订单总数: " << num_orders << std::endl;
    std::cout << "  • 分片数量: " << num_shards << std::endl;
    std::cout << "  • WAL状态: " << (enable_wal ? "启用 (异步写入)" : "禁用") << std::endl;
    std::cout << "  • 线程数: " << num_shards << " (每分片1线程)" << std::endl;
    std::cout << "  • CPU核心数: " << std::thread::hardware_concurrency() << std::endl;
    std::cout << std::endl;
    
    // 创建多个撮合分片（每个分片处理不同的instrument_id）
    std::vector<std::unique_ptr<ProductionMatchingEngineSafeOptimized>> matching_shards;
    for (size_t i = 0; i < num_shards; ++i) {
        InstrumentID instrument_id = i + 1;
        auto shard = std::make_unique<ProductionMatchingEngineSafeOptimized>(instrument_id);
        std::string wal_path = "./data/wal/matching_shard_" + std::to_string(i);
        if (!shard->initialize("", enable_wal, wal_path)) {
            std::cerr << "Failed to initialize matching shard " << i << std::endl;
            return 1;
        }
        shard->disable_rate_limiting();
        matching_shards.push_back(std::move(shard));
    }
    
    std::cout << "撮合分片初始化完成" << std::endl;
    std::cout << std::endl;
    
    // 生成测试订单
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<InstrumentID> instrument_dist(1, num_shards);
    std::uniform_int_distribution<Price> price_dist(10000, 20000);
    std::uniform_int_distribution<Quantity> qty_dist(1, 100);
    std::uniform_int_distribution<int> side_dist(0, 1);
    
    std::vector<Order> orders;
    orders.reserve(num_orders);
    
    for (size_t i = 0; i < num_orders; ++i) {
        Order order;
        order.order_id = i + 1;
        order.user_id = 1;  // 固定用户ID，因为我们只测试撮合
        order.instrument_id = instrument_dist(gen);
        order.side = (side_dist(gen) == 0) ? OrderSide::BUY : OrderSide::SELL;
        order.order_type = OrderType::LIMIT;
        order.price = price_dist(gen);
        order.quantity = qty_dist(gen);
        order.remaining_quantity = order.quantity;
        order.status = OrderStatus::PENDING;
        // 确保指针被正确初始化（避免内存问题）
        order.prev_same_price = nullptr;
        order.next_same_price = nullptr;
        order.parent = nullptr;
        order.left = nullptr;
        order.right = nullptr;
        order.color = 0;
        orders.push_back(order);
    }
    
    // 按分片分组订单（为多线程并行处理做准备）
    std::vector<std::vector<size_t>> shard_order_indices(num_shards);
    for (size_t i = 0; i < orders.size(); ++i) {
        size_t shard_id = (orders[i].instrument_id - 1) % num_shards;
        shard_order_indices[shard_id].push_back(i);
    }
    
    // 预热 - 使用动态分配的Order对象，让matching engine接管所有权
    const size_t warmup = 100;
    std::cout << "预热中... (" << warmup << " 订单)" << std::endl;
    for (size_t i = 0; i < warmup && i < orders.size(); ++i) {
        Order* warmup_order = new Order();
        *warmup_order = orders[i];
        warmup_order->filled_quantity = 0;
        warmup_order->remaining_quantity = orders[i].quantity;
        warmup_order->status = OrderStatus::PENDING;
        warmup_order->sequence_id = 0;
        warmup_order->prev_same_price = nullptr;
        warmup_order->next_same_price = nullptr;
        warmup_order->parent = nullptr;
        warmup_order->left = nullptr;
        warmup_order->right = nullptr;
        warmup_order->color = 0;
        warmup_order->total_quantity = 0;
        
        size_t shard_id = (warmup_order->instrument_id - 1) % num_shards;
        matching_shards[shard_id]->process_order_optimized(warmup_order);
    }
    std::cout << "预热完成" << std::endl;
    std::cout << std::endl;
    
    // 多线程按分片并行处理
    std::cout << "开始多线程并行压测..." << std::endl;
    
    // 线程安全的统计数据结构
    std::vector<std::atomic<size_t>> shard_order_counts(num_shards);
    std::vector<std::atomic<size_t>> shard_trade_counts(num_shards);
    std::vector<std::atomic<bool>> shard_started(num_shards);
    std::vector<high_resolution_clock::time_point> shard_start_times(num_shards);
    std::vector<std::mutex> shard_latency_mutexes(num_shards);
    std::vector<std::vector<double>> shard_latencies(num_shards);
    
    std::atomic<size_t> total_processed(0);
    std::atomic<size_t> total_trades(0);
    std::mutex progress_mutex;
    
    auto start_time = high_resolution_clock::now();
    
    // 工作线程函数：每个线程处理一个分片的所有订单
    auto worker_thread = [&](size_t shard_id) {
        auto& shard_orders = shard_order_indices[shard_id];
        if (shard_orders.empty()) return;
        
        auto shard_start = high_resolution_clock::now();
        if (!shard_started[shard_id].exchange(true)) {
            shard_start_times[shard_id] = shard_start;
        }
        
        for (size_t idx : shard_orders) {
            if (idx < warmup) continue;  // 跳过预热订单
            
            auto order_start = high_resolution_clock::now();
            
            Order* order = new Order();
            *order = orders[idx];
            order->filled_quantity = 0;
            order->remaining_quantity = orders[idx].quantity;
            order->status = OrderStatus::PENDING;
            order->sequence_id = 0;
            order->prev_same_price = nullptr;
            order->next_same_price = nullptr;
            order->parent = nullptr;
            order->left = nullptr;
            order->right = nullptr;
            order->color = 0;
            order->total_quantity = 0;
            
            auto trades = matching_shards[shard_id]->process_order_optimized(order);
            
            auto order_end = high_resolution_clock::now();
            auto latency = duration_cast<nanoseconds>(order_end - order_start).count() / 1000.0;  // μs
            
            shard_order_counts[shard_id]++;
            shard_trade_counts[shard_id] += trades.size();
            total_trades += trades.size();
            
            {
                std::lock_guard<std::mutex> lock(shard_latency_mutexes[shard_id]);
                shard_latencies[shard_id].push_back(latency);
            }
            
            size_t processed = total_processed.fetch_add(1) + 1;
            
            // 实时报告（线程安全，每5万订单报告一次）
            if (processed % 50000 == 0 || processed == 1) {
                auto current_time = high_resolution_clock::now();
                auto elapsed = duration_cast<milliseconds>(current_time - start_time).count();
                double elapsed_sec = elapsed / 1000.0;
                // 修复单位：elapsed是毫秒，orders/sec = (processed * 1000.0) / elapsed
                // K orders/sec = orders/sec / 1000 = processed / elapsed
                double throughput = processed / elapsed_sec;  // orders/sec
                double throughput_k = throughput / 1000.0;  // K orders/sec
                double progress = (processed * 100.0) / (orders.size() - warmup);
                
                std::lock_guard<std::mutex> lock(progress_mutex);
                std::cout << "[" << std::fixed << std::setprecision(1) << progress << "%] "
                          << "已处理: " << processed << " | "
                          << "耗时: " << elapsed << " ms (" << std::setprecision(2) << elapsed_sec << "s) | "
                          << "吞吐量: " << std::setprecision(2) << throughput_k << " K orders/sec | "
                          << "交易数: " << total_trades.load() << std::endl;
            }
        }
    };
    
    // 启动工作线程（每个分片一个线程）
    std::vector<std::thread> threads;
    for (size_t shard_id = 0; shard_id < num_shards; ++shard_id) {
        threads.emplace_back(worker_thread, shard_id);
    }
    
    // 等待所有线程完成
    for (auto& t : threads) {
        t.join();
    }
    
    auto end_time = high_resolution_clock::now();
    auto total_time = duration_cast<milliseconds>(end_time - start_time).count();
    
    size_t final_processed = total_processed.load();
    size_t final_trades = total_trades.load();
    double total_time_sec = total_time / 1000.0;
    
    // 修复单位：total_time是毫秒，orders/sec = (final_processed * 1000.0) / total_time
    // K orders/sec = orders/sec / 1000 = final_processed / total_time
    double total_throughput_orders_per_sec = (final_processed * 1000.0) / (total_time > 0 ? total_time : 1);
    double total_throughput_k = total_throughput_orders_per_sec / 1000.0;  // K orders/sec
    
    std::cout << std::endl;
    std::cout << "╔══════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                    压测结果汇总                             ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;
    
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "总体性能指标" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "  处理订单数: " << final_processed << " 订单" << std::endl;
    std::cout << "  总耗时:     " << std::fixed << std::setprecision(2) << total_time_sec << " 秒 (" << total_time << " 毫秒)" << std::endl;
    std::cout << "  总吞吐量:   " << std::setprecision(2) << total_throughput_k << " K orders/sec (" 
              << std::setprecision(0) << total_throughput_orders_per_sec << " orders/sec)" << std::endl;
    std::cout << "  总交易数:   " << final_trades << " 笔" << std::endl;
    std::cout << "  成交率:     " << std::setprecision(2) << (final_trades * 100.0 / final_processed) << "%" << std::endl;
    std::cout << std::endl;
    
    // 输出每个分片的详细统计
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "各分片性能详情" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << std::endl;
    
    // 计算统计信息
    double min_tps = std::numeric_limits<double>::max();
    double max_tps = 0;
    double sum_tps = 0;
    size_t active_shards = 0;
    
    for (size_t shard_id = 0; shard_id < num_shards; ++shard_id) {
        size_t shard_orders = shard_order_counts[shard_id].load();
        if (shard_started[shard_id].load() && shard_orders > 0) {
            auto shard_elapsed = duration_cast<milliseconds>(end_time - shard_start_times[shard_id]).count();
            // 修复单位：shard_elapsed是毫秒，orders/sec = (shard_orders * 1000.0) / shard_elapsed
            // K orders/sec = orders/sec / 1000 = shard_orders / shard_elapsed
            double shard_tps_orders_per_sec = (shard_orders * 1000.0) / (shard_elapsed > 0 ? shard_elapsed : 1);
            double shard_tps = shard_tps_orders_per_sec / 1000.0;  // K orders/sec
            double shard_percentage = (shard_orders * 100.0) / (final_processed > 0 ? final_processed : 1);
            
            // 计算延迟统计（需要加锁访问）
            std::vector<double> latencies_copy;
            {
                std::lock_guard<std::mutex> lock(shard_latency_mutexes[shard_id]);
                latencies_copy = shard_latencies[shard_id];
            }
            
            double avg_lat = 0;
            double p50 = 0;
            double p99 = 0;
            
            if (!latencies_copy.empty()) {
                std::sort(latencies_copy.begin(), latencies_copy.end());
                for (auto l : latencies_copy) avg_lat += l;
                avg_lat /= latencies_copy.size();
                p50 = latencies_copy[latencies_copy.size() * 0.5];
                p99 = latencies_copy[latencies_copy.size() * 0.99];
            }
            
            // 获取WAL统计
            auto stats = matching_shards[shard_id]->get_stats();
            
            // 更新统计
            min_tps = std::min(min_tps, shard_tps);
            max_tps = std::max(max_tps, shard_tps);
            sum_tps += shard_tps;
            active_shards++;
            
            std::cout << "┌─ 撮合分片 " << shard_id << " ─────────────────────────────────────────────┐" << std::endl;
            std::cout << "│ 订单处理: " << std::setw(10) << shard_orders << " (" 
                      << std::setprecision(1) << std::setw(5) << shard_percentage << "%)" << std::endl;
            std::cout << "│ 交易执行: " << std::setw(10) << shard_trade_counts[shard_id].load() << " 笔" << std::endl;
            std::cout << "│ 吞吐量:   " << std::setw(10) << std::setprecision(2) << shard_tps << " K orders/sec" << std::endl;
            std::cout << "│ 耗时:     " << std::setw(10) << shard_elapsed << " ms (" 
                      << std::setprecision(2) << (shard_elapsed / 1000.0) << "s)" << std::endl;
            if (!latencies_copy.empty()) {
                std::cout << "│ 延迟统计:" << std::endl;
                std::cout << "│   • 平均延迟: " << std::setw(8) << std::setprecision(2) << avg_lat << " μs" << std::endl;
                std::cout << "│   • P50延迟:   " << std::setw(8) << std::setprecision(2) << p50 << " μs" << std::endl;
                std::cout << "│   • P99延迟:   " << std::setw(8) << std::setprecision(2) << p99 << " μs" << std::endl;
            }
            if (enable_wal) {
                std::cout << "│ WAL统计:" << std::endl;
                std::cout << "│   • 异步写入:   " << std::setw(10) << stats.async_writes << " 次" << std::endl;
                std::cout << "│   • 队列大小:   " << std::setw(10) << stats.queue_size << std::endl;
            }
            std::cout << "└────────────────────────────────────────────────────────────┘" << std::endl;
            std::cout << std::endl;
        } else {
            std::cout << "撮合分片 " << shard_id << ": 未使用" << std::endl;
        }
    }
    // 输出统计摘要
    if (active_shards > 0) {
        double avg_tps = sum_tps / active_shards;
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        std::cout << "分片性能统计" << std::endl;
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        std::cout << "  平均TPS: " << std::setprecision(2) << avg_tps << " K orders/sec" << std::endl;
        std::cout << "  最高TPS: " << std::setprecision(2) << max_tps << " K orders/sec" << std::endl;
        std::cout << "  最低TPS: " << std::setprecision(2) << min_tps << " K orders/sec" << std::endl;
        std::cout << "  TPS差异: " << std::setprecision(2) << ((max_tps - min_tps) / avg_tps * 100.0) << "%" << std::endl;
        std::cout << std::endl;
    }
    
    std::cout << "╔══════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                    压测完成                                  ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;
    
    // 关闭所有分片
    for (auto& shard : matching_shards) {
        shard->shutdown();
    }
    
    return 0;
}

