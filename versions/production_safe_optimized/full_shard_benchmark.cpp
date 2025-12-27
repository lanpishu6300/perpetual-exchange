#include "core/sharded_matching_engine.h"
#include "core/logger.h"
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
#include <mutex>
#include <atomic>
#include <limits>

using namespace perpetual;
using namespace std::chrono;

int main(int argc, char* argv[]) {
    size_t num_orders = 1000000;  // 默认100万订单
    size_t num_trading_shards = std::thread::hardware_concurrency();
    size_t num_matching_shards = std::thread::hardware_concurrency();
    if (num_trading_shards == 0) num_trading_shards = 4;
    if (num_matching_shards == 0) num_matching_shards = 4;
    bool enable_wal = false;
    
    if (argc > 1) {
        num_orders = std::stoull(argv[1]);
    }
    if (argc > 2) {
        num_trading_shards = std::stoull(argv[2]);
    }
    if (argc > 3) {
        num_matching_shards = std::stoull(argv[3]);
    }
    if (argc > 4) {
        enable_wal = (std::stoull(argv[4]) != 0);
    }
    
    std::cout << "╔══════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║    完整分片压测 (Full Shard Stress Test)                     ║" << std::endl;
    std::cout << "║    包含 Trading Shard + Matching Shard                      ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;
    std::cout << "测试配置:" << std::endl;
    std::cout << "  • 订单总数: " << num_orders << std::endl;
    std::cout << "  • 交易分片数: " << num_trading_shards << std::endl;
    std::cout << "  • 撮合分片数: " << num_matching_shards << std::endl;
    std::cout << "  • WAL状态: " << (enable_wal ? "启用 (异步写入)" : "禁用") << std::endl;
    std::cout << "  • CPU核心数: " << std::thread::hardware_concurrency() << std::endl;
    std::cout << std::endl;
    
    // 初始化分片引擎（双重分片：交易模块按用户，撮合模块按币对）
    ShardedMatchingEngine engine(num_trading_shards, num_matching_shards);
    
    if (!engine.initialize("config.ini", enable_wal)) {
        std::cerr << "Failed to initialize sharded engine" << std::endl;
        return 1;
    }
    
    std::cout << "引擎初始化完成" << std::endl;
    std::cout << std::endl;
    
    // 生成测试订单
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<UserID> user_dist(1, 10000);  // 10000个用户
    std::uniform_int_distribution<InstrumentID> instrument_dist(1, 10);  // 10个币对
    std::uniform_int_distribution<Price> price_dist(10000, 20000);
    std::uniform_int_distribution<Quantity> qty_dist(1, 100);
    std::uniform_int_distribution<int> side_dist(0, 1);
    
    std::vector<Order> orders;
    orders.reserve(num_orders);
    
    for (size_t i = 0; i < num_orders; ++i) {
        Order order;
        order.order_id = i + 1;
        order.user_id = user_dist(gen);
        order.instrument_id = instrument_dist(gen);
        order.side = (side_dist(gen) == 0) ? OrderSide::BUY : OrderSide::SELL;
        order.order_type = OrderType::LIMIT;
        order.price = price_dist(gen);
        order.quantity = qty_dist(gen);
        order.remaining_quantity = order.quantity;
        order.status = OrderStatus::PENDING;
        order.prev_same_price = nullptr;
        order.next_same_price = nullptr;
        order.parent = nullptr;
        order.left = nullptr;
        order.right = nullptr;
        order.color = 0;
        order.total_quantity = 0;
        orders.push_back(order);
    }
    
    // 预热 - 使用动态分配的Order对象，让engine接管所有权
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
        // engine会接管Order的所有权，我们不需要delete
        engine.process_order(warmup_order);
    }
    std::cout << "预热完成" << std::endl;
    std::cout << std::endl;
    
    // 跟踪统计
    std::vector<double> latencies;
    latencies.reserve(num_orders);
    
    // 交易分片统计
    std::vector<std::atomic<size_t>> trading_shard_order_counts(num_trading_shards);
    std::vector<std::atomic<size_t>> trading_shard_trade_counts(num_trading_shards);
    std::vector<high_resolution_clock::time_point> trading_shard_start_times(num_trading_shards);
    std::vector<std::atomic<bool>> trading_shard_started(num_trading_shards);
    std::vector<std::mutex> trading_shard_latency_mutexes(num_trading_shards);
    std::vector<std::vector<double>> trading_shard_latencies(num_trading_shards);
    
    // 撮合分片统计
    std::vector<std::atomic<size_t>> matching_shard_order_counts(num_matching_shards);
    std::vector<std::atomic<size_t>> matching_shard_trade_counts(num_matching_shards);
    std::vector<high_resolution_clock::time_point> matching_shard_start_times(num_matching_shards);
    std::vector<std::atomic<bool>> matching_shard_started(num_matching_shards);
    std::vector<std::mutex> matching_shard_latency_mutexes(num_matching_shards);
    std::vector<std::vector<double>> matching_shard_latencies(num_matching_shards);
    
    std::atomic<size_t> total_processed(0);
    std::atomic<size_t> total_trades(0);
    std::mutex progress_mutex;
    
    std::cout << "开始压测..." << std::endl;
    auto start_time = high_resolution_clock::now();
    
    // 压测循环
    for (size_t i = warmup; i < orders.size(); ++i) {
        auto order_start = high_resolution_clock::now();
        
        // 动态分配Order对象，engine会接管所有权
        Order* order = new Order();
        *order = orders[i];
        order->filled_quantity = 0;
        order->remaining_quantity = orders[i].quantity;
        order->status = OrderStatus::PENDING;
        order->sequence_id = 0;
        order->prev_same_price = nullptr;
        order->next_same_price = nullptr;
        order->parent = nullptr;
        order->left = nullptr;
        order->right = nullptr;
        order->color = 0;
        order->total_quantity = 0;
        
        // 获取分片ID
        size_t trading_shard_id = engine.get_trading_shard_id(order->user_id);
        size_t matching_shard_id = engine.get_matching_shard_id(order->instrument_id);
        
        // 记录交易分片开始时间
        bool expected = false;
        if (trading_shard_started[trading_shard_id].compare_exchange_strong(expected, true)) {
            trading_shard_start_times[trading_shard_id] = order_start;
        }
        
        // 记录撮合分片开始时间
        expected = false;
        if (matching_shard_started[matching_shard_id].compare_exchange_strong(expected, true)) {
            matching_shard_start_times[matching_shard_id] = order_start;
        }
        
        // 处理订单（包含trading shard验证和matching shard撮合）
        // engine会接管Order的所有权，我们不需要delete
        auto trades = engine.process_order(order);
        
        auto order_end = high_resolution_clock::now();
        auto latency = duration_cast<nanoseconds>(order_end - order_start).count() / 1000.0;  // μs
        latencies.push_back(latency);
        
        // 更新统计
        trading_shard_order_counts[trading_shard_id]++;
        matching_shard_order_counts[matching_shard_id]++;
        trading_shard_trade_counts[trading_shard_id] += trades.size();
        matching_shard_trade_counts[matching_shard_id] += trades.size();
        total_trades += trades.size();
        
        {
            std::lock_guard<std::mutex> lock(trading_shard_latency_mutexes[trading_shard_id]);
            trading_shard_latencies[trading_shard_id].push_back(latency);
        }
        {
            std::lock_guard<std::mutex> lock(matching_shard_latency_mutexes[matching_shard_id]);
            matching_shard_latencies[matching_shard_id].push_back(latency);
        }
        
        size_t processed = total_processed.fetch_add(1) + 1;
        
        // 实时报告
        if (processed % 50000 == 0 || processed == 1) {
            auto current_time = high_resolution_clock::now();
            auto elapsed = duration_cast<milliseconds>(current_time - start_time).count();
            double elapsed_sec = elapsed / 1000.0;
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
    
    auto end_time = high_resolution_clock::now();
    auto total_time = duration_cast<milliseconds>(end_time - start_time).count();
    double total_time_sec = total_time / 1000.0;
    
    size_t final_processed = total_processed.load();
    size_t final_trades = total_trades.load();
    
    // 计算总体吞吐量
    double total_throughput_orders_per_sec = (final_processed * 1000.0) / (total_time > 0 ? total_time : 1);
    double total_throughput_k = total_throughput_orders_per_sec / 1000.0;
    
    // 计算延迟统计
    std::sort(latencies.begin(), latencies.end());
    double avg_lat = 0;
    for (auto l : latencies) avg_lat += l;
    avg_lat /= latencies.size();
    double p50 = latencies[latencies.size() * 0.5];
    double p99 = latencies[latencies.size() * 0.99];
    
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
    std::cout << "  平均延迟:   " << std::setprecision(2) << avg_lat << " μs" << std::endl;
    std::cout << "  P50延迟:    " << std::setprecision(2) << p50 << " μs" << std::endl;
    std::cout << "  P99延迟:    " << std::setprecision(2) << p99 << " μs" << std::endl;
    std::cout << std::endl;
    
    // 交易分片统计
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "交易分片性能详情" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << std::endl;
    
    double trading_min_tps = std::numeric_limits<double>::max();
    double trading_max_tps = 0;
    double trading_sum_tps = 0;
    size_t trading_active_shards = 0;
    
    for (size_t shard_id = 0; shard_id < num_trading_shards; ++shard_id) {
        size_t shard_orders = trading_shard_order_counts[shard_id].load();
        if (trading_shard_started[shard_id].load() && shard_orders > 0) {
            auto shard_elapsed = duration_cast<milliseconds>(end_time - trading_shard_start_times[shard_id]).count();
            double shard_tps_orders_per_sec = (shard_orders * 1000.0) / (shard_elapsed > 0 ? shard_elapsed : 1);
            double shard_tps = shard_tps_orders_per_sec / 1000.0;
            double shard_percentage = (shard_orders * 100.0) / (final_processed > 0 ? final_processed : 1);
            
            std::vector<double> latencies_copy;
            {
                std::lock_guard<std::mutex> lock(trading_shard_latency_mutexes[shard_id]);
                latencies_copy = trading_shard_latencies[shard_id];
            }
            
            double avg_lat_shard = 0;
            double p50_shard = 0;
            double p99_shard = 0;
            
            if (!latencies_copy.empty()) {
                std::sort(latencies_copy.begin(), latencies_copy.end());
                for (auto l : latencies_copy) avg_lat_shard += l;
                avg_lat_shard /= latencies_copy.size();
                p50_shard = latencies_copy[latencies_copy.size() * 0.5];
                p99_shard = latencies_copy[latencies_copy.size() * 0.99];
            }
            
            trading_min_tps = std::min(trading_min_tps, shard_tps);
            trading_max_tps = std::max(trading_max_tps, shard_tps);
            trading_sum_tps += shard_tps;
            trading_active_shards++;
            
            std::cout << "┌─ 交易分片 " << shard_id << " ─────────────────────────────────────────────┐" << std::endl;
            std::cout << "│ 订单处理: " << std::setw(10) << shard_orders << " (" 
                      << std::setprecision(1) << std::setw(5) << shard_percentage << "%)" << std::endl;
            std::cout << "│ 交易执行: " << std::setw(10) << trading_shard_trade_counts[shard_id].load() << " 笔" << std::endl;
            std::cout << "│ 吞吐量:   " << std::setw(10) << std::setprecision(2) << shard_tps << " K orders/sec" << std::endl;
            std::cout << "│ 耗时:     " << std::setw(10) << shard_elapsed << " ms (" 
                      << std::setprecision(2) << (shard_elapsed / 1000.0) << "s)" << std::endl;
            if (!latencies_copy.empty()) {
                std::cout << "│ 延迟统计:" << std::endl;
                std::cout << "│   • 平均延迟: " << std::setw(8) << std::setprecision(2) << avg_lat_shard << " μs" << std::endl;
                std::cout << "│   • P50延迟:   " << std::setw(8) << std::setprecision(2) << p50_shard << " μs" << std::endl;
                std::cout << "│   • P99延迟:   " << std::setw(8) << std::setprecision(2) << p99_shard << " μs" << std::endl;
            }
            std::cout << "└────────────────────────────────────────────────────────────┘" << std::endl;
            std::cout << std::endl;
        }
    }
    
    if (trading_active_shards > 0) {
        double trading_avg_tps = trading_sum_tps / trading_active_shards;
        std::cout << "交易分片性能统计:" << std::endl;
        std::cout << "  平均TPS: " << std::setprecision(2) << trading_avg_tps << " K orders/sec" << std::endl;
        std::cout << "  最高TPS: " << std::setprecision(2) << trading_max_tps << " K orders/sec" << std::endl;
        std::cout << "  最低TPS: " << std::setprecision(2) << trading_min_tps << " K orders/sec" << std::endl;
        std::cout << "  TPS差异: " << std::setprecision(2) << ((trading_max_tps - trading_min_tps) / trading_avg_tps * 100.0) << "%" << std::endl;
        std::cout << std::endl;
    }
    
    // 撮合分片统计
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "撮合分片性能详情" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << std::endl;
    
    double matching_min_tps = std::numeric_limits<double>::max();
    double matching_max_tps = 0;
    double matching_sum_tps = 0;
    size_t matching_active_shards = 0;
    
    for (size_t shard_id = 0; shard_id < num_matching_shards; ++shard_id) {
        size_t shard_orders = matching_shard_order_counts[shard_id].load();
        if (matching_shard_started[shard_id].load() && shard_orders > 0) {
            auto shard_elapsed = duration_cast<milliseconds>(end_time - matching_shard_start_times[shard_id]).count();
            double shard_tps_orders_per_sec = (shard_orders * 1000.0) / (shard_elapsed > 0 ? shard_elapsed : 1);
            double shard_tps = shard_tps_orders_per_sec / 1000.0;
            double shard_percentage = (shard_orders * 100.0) / (final_processed > 0 ? final_processed : 1);
            
            std::vector<double> latencies_copy;
            {
                std::lock_guard<std::mutex> lock(matching_shard_latency_mutexes[shard_id]);
                latencies_copy = matching_shard_latencies[shard_id];
            }
            
            double avg_lat_shard = 0;
            double p50_shard = 0;
            double p99_shard = 0;
            
            if (!latencies_copy.empty()) {
                std::sort(latencies_copy.begin(), latencies_copy.end());
                for (auto l : latencies_copy) avg_lat_shard += l;
                avg_lat_shard /= latencies_copy.size();
                p50_shard = latencies_copy[latencies_copy.size() * 0.5];
                p99_shard = latencies_copy[latencies_copy.size() * 0.99];
            }
            
            matching_min_tps = std::min(matching_min_tps, shard_tps);
            matching_max_tps = std::max(matching_max_tps, shard_tps);
            matching_sum_tps += shard_tps;
            matching_active_shards++;
            
            std::cout << "┌─ 撮合分片 " << shard_id << " ─────────────────────────────────────────────┐" << std::endl;
            std::cout << "│ 订单处理: " << std::setw(10) << shard_orders << " (" 
                      << std::setprecision(1) << std::setw(5) << shard_percentage << "%)" << std::endl;
            std::cout << "│ 交易执行: " << std::setw(10) << matching_shard_trade_counts[shard_id].load() << " 笔" << std::endl;
            std::cout << "│ 吞吐量:   " << std::setw(10) << std::setprecision(2) << shard_tps << " K orders/sec" << std::endl;
            std::cout << "│ 耗时:     " << std::setw(10) << shard_elapsed << " ms (" 
                      << std::setprecision(2) << (shard_elapsed / 1000.0) << "s)" << std::endl;
            if (!latencies_copy.empty()) {
                std::cout << "│ 延迟统计:" << std::endl;
                std::cout << "│   • 平均延迟: " << std::setw(8) << std::setprecision(2) << avg_lat_shard << " μs" << std::endl;
                std::cout << "│   • P50延迟:   " << std::setw(8) << std::setprecision(2) << p50_shard << " μs" << std::endl;
                std::cout << "│   • P99延迟:   " << std::setw(8) << std::setprecision(2) << p99_shard << " μs" << std::endl;
            }
            std::cout << "└────────────────────────────────────────────────────────────┘" << std::endl;
            std::cout << std::endl;
        }
    }
    
    if (matching_active_shards > 0) {
        double matching_avg_tps = matching_sum_tps / matching_active_shards;
        std::cout << "撮合分片性能统计:" << std::endl;
        std::cout << "  平均TPS: " << std::setprecision(2) << matching_avg_tps << " K orders/sec" << std::endl;
        std::cout << "  最高TPS: " << std::setprecision(2) << matching_max_tps << " K orders/sec" << std::endl;
        std::cout << "  最低TPS: " << std::setprecision(2) << matching_min_tps << " K orders/sec" << std::endl;
        std::cout << "  TPS差异: " << std::setprecision(2) << ((matching_max_tps - matching_min_tps) / matching_avg_tps * 100.0) << "%" << std::endl;
        std::cout << std::endl;
    }
    
    std::cout << "╔══════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                    压测完成                                  ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;
    
    engine.shutdown();
    return 0;
}

