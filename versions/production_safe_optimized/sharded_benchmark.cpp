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

using namespace perpetual;
using namespace std::chrono;

// 实时输出压测指标
void run_realtime_benchmark(size_t num_orders = 50000, size_t report_interval = 1000) {
    std::cout << "=== Sharded Matching Engine 实时压测 ===" << std::endl;
    std::cout << std::endl;
    
    // 初始化分片引擎（双重分片：交易模块按用户，撮合模块按币对）
    size_t num_cores = std::thread::hardware_concurrency();
    if (num_cores == 0) num_cores = 4;
    
    ShardedMatchingEngine engine(num_cores, num_cores);  // 交易分片数，撮合分片数
    
    // 禁用WAL以测试分片性能（避免WAL写入阻塞）
    bool enable_wal = false;  // 设置为false以加快benchmark速度
    if (!engine.initialize("config.ini", enable_wal)) {
        std::cerr << "Failed to initialize sharded engine" << std::endl;
        return;
    }
    
    std::cout << "引擎初始化完成 | 交易分片: " << engine.get_trading_shard_count() 
              << " | 撮合分片: " << engine.get_matching_shard_count() 
              << " | WAL: " << (enable_wal ? "已启用 ✅" : "已禁用 ⚡") << std::endl;
    std::cout << "测试订单数: " << num_orders << " | 报告间隔: " << report_interval << " 订单" << std::endl;
    std::cout << std::endl;
    std::cout << "开始压测..." << std::endl;
    
    // 生成测试订单
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<UserID> user_dist(1, 1000);  // 1000个用户
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
        orders.push_back(order);
    }
    
    // 预热（减少预热订单数以加快速度）
    const size_t warmup = 10;  // 进一步减少预热订单数
    std::cout << "预热中... (" << warmup << " 订单)" << std::flush;
    auto warmup_start = high_resolution_clock::now();
    for (size_t i = 0; i < warmup && i < orders.size(); ++i) {
        auto order_start = high_resolution_clock::now();
        auto trades = engine.process_order(&orders[i]);
        auto order_end = high_resolution_clock::now();
        auto order_latency = duration_cast<microseconds>(order_end - order_start).count();
        std::cout << "订单" << i << " 延迟: " << order_latency << "μs, 交易数: " << trades.size() << std::endl;
    }
    auto warmup_end = high_resolution_clock::now();
    auto warmup_time = duration_cast<milliseconds>(warmup_end - warmup_start).count();
    std::cout << "预热完成！耗时: " << warmup_time << "ms" << std::endl;
    std::cout << "开始压测..." << std::endl;
    
    // 压测
    std::vector<double> latencies;
    latencies.reserve(num_orders);
    
    // 跟踪每个撮合分片的订单数
    size_t num_matching_shards = engine.get_matching_shard_count();
    std::vector<size_t> matching_shard_order_counts(num_matching_shards, 0);
    std::vector<high_resolution_clock::time_point> matching_shard_start_times(num_matching_shards);
    std::vector<bool> matching_shard_started(num_matching_shards, false);
    
    // 跟踪每个交易分片的订单数
    size_t num_trading_shards = engine.get_trading_shard_count();
    std::vector<size_t> trading_shard_order_counts(num_trading_shards, 0);
    std::vector<high_resolution_clock::time_point> trading_shard_start_times(num_trading_shards);
    std::vector<bool> trading_shard_started(num_trading_shards, false);
    
    auto start_time = high_resolution_clock::now();
    size_t processed = 0;
    
    for (size_t i = warmup; i < orders.size(); ++i) {
        auto order_start = high_resolution_clock::now();
        
        // 获取订单将路由到的分片ID
        size_t trading_shard_id = engine.get_trading_shard_id(orders[i].user_id);
        size_t matching_shard_id = engine.get_matching_shard_id(orders[i].instrument_id);
        
        // 记录交易分片开始时间（第一次处理该分片的订单时）
        if (!trading_shard_started[trading_shard_id]) {
            trading_shard_start_times[trading_shard_id] = order_start;
            trading_shard_started[trading_shard_id] = true;
        }
        
        // 记录撮合分片开始时间（第一次处理该分片的订单时）
        if (!matching_shard_started[matching_shard_id]) {
            matching_shard_start_times[matching_shard_id] = order_start;
            matching_shard_started[matching_shard_id] = true;
        }
        
        auto trades = engine.process_order(&orders[i]);
        
        auto order_end = high_resolution_clock::now();
        auto latency = duration_cast<nanoseconds>(order_end - order_start).count() / 1000.0;  // μs
        latencies.push_back(latency);
        
        // 增加该交易分片的订单计数
        trading_shard_order_counts[trading_shard_id]++;
        // 增加该撮合分片的订单计数
        matching_shard_order_counts[matching_shard_id]++;
        
        processed++;
        
        // 实时报告 - 增加更频繁的输出
        if (processed % report_interval == 0 || (processed <= 100 && processed % 10 == 0) || processed == 1) {
            auto current_time = high_resolution_clock::now();
            auto elapsed = duration_cast<milliseconds>(current_time - start_time).count();
            
            // 计算统计
            size_t count = latencies.size();
            if (count == 0) continue;
            
            double sum = 0;
            double min_lat = latencies[0];
            double max_lat = latencies[0];
            for (size_t j = 0; j < count; ++j) {
                sum += latencies[j];
                if (latencies[j] < min_lat) min_lat = latencies[j];
                if (latencies[j] > max_lat) max_lat = latencies[j];
            }
            double avg = sum / count;
            
            // Create a copy for sorting to avoid modifying original
            std::vector<double> sorted_latencies;
            sorted_latencies.reserve(count);
            for (size_t j = 0; j < count; ++j) {
                sorted_latencies.push_back(latencies[j]);
            }
            std::sort(sorted_latencies.begin(), sorted_latencies.end());
            
            double p50 = sorted_latencies[std::min(static_cast<size_t>(count * 0.5), count - 1)];
            double p90 = sorted_latencies[std::min(static_cast<size_t>(count * 0.9), count - 1)];
            double p99 = sorted_latencies[std::min(static_cast<size_t>(count * 0.99), count - 1)];
            double p99_9 = sorted_latencies[std::min(static_cast<size_t>(count * 0.999), count - 1)];
            
            double throughput = (processed * 1000.0) / (elapsed > 0 ? elapsed : 1);  // K orders/sec
            
            // 获取统计
            auto stats = engine.get_stats();
            
            // 进度条
            double progress = (processed * 100.0) / num_orders;
            size_t bar_width = 40;
            size_t pos = static_cast<size_t>(bar_width * progress / 100.0);
            
            // 使用换行输出，便于实时查看
            std::cout << std::endl << "进度: [";
            for (size_t j = 0; j < bar_width; ++j) {
                if (j < pos) std::cout << "=";
                else if (j == pos) std::cout << ">";
                else std::cout << " ";
            }
            std::cout << "] " << std::fixed << std::setprecision(1) << progress << "%";
            std::cout << " | 已处理: " << processed << "/" << num_orders;
            std::cout << " | 耗时: " << elapsed << " ms";
            std::cout << " | 吞吐量: " << std::setprecision(2) << throughput << " K/s";
            std::cout << " | 延迟(avg/min/max): " << std::setprecision(2) << avg << "/" << min_lat << "/" << max_lat << " μs";
            std::cout << " | P50/P90/P99/P99.9: " << std::setprecision(2) << p50 << "/" << p90 << "/" << p99 << "/" << p99_9 << " μs";
            std::cout << " | 交易分片: " << engine.get_trading_shard_count() 
                      << " | 撮合分片: " << engine.get_matching_shard_count();
            if (stats.async_writes > 0 || stats.sync_writes > 0) {
                std::cout << " | 异步: " << stats.async_writes;
                std::cout << " | 同步: " << stats.sync_writes;
            }
            std::cout << std::endl;
            
            // 输出每个交易分片的TPS
            std::cout << "交易分片TPS: ";
            for (size_t shard_id = 0; shard_id < num_trading_shards; ++shard_id) {
                if (trading_shard_started[shard_id]) {
                    auto shard_elapsed = duration_cast<milliseconds>(current_time - trading_shard_start_times[shard_id]).count();
                    double shard_tps = (trading_shard_order_counts[shard_id] * 1000.0) / (shard_elapsed > 0 ? shard_elapsed : 1);
                    std::cout << "T" << shard_id << ": " << std::setprecision(2) << shard_tps << " K/s";
                    if (shard_id < num_trading_shards - 1) std::cout << " | ";
                }
            }
            std::cout << std::endl;
            
            // 输出每个撮合分片的TPS
            std::cout << "撮合分片TPS: ";
            for (size_t shard_id = 0; shard_id < num_matching_shards; ++shard_id) {
                if (matching_shard_started[shard_id]) {
                    auto shard_elapsed = duration_cast<milliseconds>(current_time - matching_shard_start_times[shard_id]).count();
                    double shard_tps = (matching_shard_order_counts[shard_id] * 1000.0) / (shard_elapsed > 0 ? shard_elapsed : 1);
                    std::cout << "M" << shard_id << ": " << std::setprecision(2) << shard_tps << " K/s";
                    if (shard_id < num_matching_shards - 1) std::cout << " | ";
                }
            }
            std::cout << "                    ";  // 清除之前的输出
            std::cout.flush();
        }
    }
    
    auto end_time = high_resolution_clock::now();
    auto total_time = duration_cast<milliseconds>(end_time - start_time).count();
    
    std::cout << std::endl;
    std::cout << std::endl;
    std::cout << "=== 最终结果 ===" << std::endl;
    std::cout << std::endl;
    
    // 最终统计
    size_t count = latencies.size();
    double sum = 0;
    double min_lat = latencies[0];
    double max_lat = latencies[0];
    
    for (size_t i = 0; i < count; ++i) {
        sum += latencies[i];
        if (latencies[i] < min_lat) min_lat = latencies[i];
        if (latencies[i] > max_lat) max_lat = latencies[i];
    }
    
    double avg = sum / count;
    double throughput = (processed * 1000.0) / (total_time > 0 ? total_time : 1);  // K orders/sec
    
    // Create a copy for sorting
    std::vector<double> sorted_latencies;
    sorted_latencies.reserve(count);
    sorted_latencies.assign(latencies.begin(), latencies.end());
    std::sort(sorted_latencies.begin(), sorted_latencies.end());
    
    double p50 = sorted_latencies[std::min(static_cast<size_t>(count * 0.5), count - 1)];
    double p90 = sorted_latencies[std::min(static_cast<size_t>(count * 0.9), count - 1)];
    double p99 = sorted_latencies[std::min(static_cast<size_t>(count * 0.99), count - 1)];
    
    auto stats = engine.get_stats();
    
    std::cout << "吞吐量: " << std::fixed << std::setprecision(2) << throughput << " K orders/sec" << std::endl;
    std::cout << "总耗时: " << total_time << " ms" << std::endl;
    std::cout << "交易分片数: " << engine.get_trading_shard_count() << std::endl;
    std::cout << "撮合分片数: " << engine.get_matching_shard_count() << std::endl;
    std::cout << std::endl;
    
    std::cout << "延迟统计:" << std::endl;
    std::cout << "  平均: " << std::setprecision(2) << avg << " μs" << std::endl;
    std::cout << "  最小: " << std::setprecision(2) << min_lat << " μs" << std::endl;
    std::cout << "  最大: " << std::setprecision(2) << max_lat << " μs" << std::endl;
    std::cout << "  P50:  " << std::setprecision(2) << p50 << " μs" << std::endl;
    std::cout << "  P90:  " << std::setprecision(2) << p90 << " μs" << std::endl;
    std::cout << "  P99:  " << std::setprecision(2) << p99 << " μs" << std::endl;
    std::cout << std::endl;
    
    std::cout << "WAL统计:" << std::endl;
    std::cout << "  异步写入: " << stats.async_writes << std::endl;
    std::cout << "  同步写入: " << stats.sync_writes << std::endl;
    std::cout << "  队列大小: " << stats.queue_size << std::endl;
    std::cout << "  WAL大小: " << stats.wal_size << " bytes" << std::endl;
    std::cout << std::endl;
    
    // 输出每个交易分片的详细TPS统计
    std::cout << "=== 交易分片TPS统计 (Trading Shards) ===" << std::endl;
    for (size_t shard_id = 0; shard_id < num_trading_shards; ++shard_id) {
        if (trading_shard_started[shard_id]) {
            auto shard_elapsed = duration_cast<milliseconds>(end_time - trading_shard_start_times[shard_id]).count();
            double shard_tps = (trading_shard_order_counts[shard_id] * 1000.0) / (shard_elapsed > 0 ? shard_elapsed : 1);
            double shard_percentage = (trading_shard_order_counts[shard_id] * 100.0) / processed;
            std::cout << "交易分片 " << shard_id << ":" << std::endl;
            std::cout << "  订单数: " << trading_shard_order_counts[shard_id] << " (" 
                      << std::setprecision(1) << shard_percentage << "%)" << std::endl;
            std::cout << "  TPS: " << std::setprecision(2) << shard_tps << " K orders/sec" << std::endl;
            std::cout << "  耗时: " << shard_elapsed << " ms" << std::endl;
        } else {
            std::cout << "交易分片 " << shard_id << ": 未使用" << std::endl;
        }
    }
    std::cout << std::endl;
    
    // 输出每个撮合分片的详细TPS统计
    std::cout << "=== 撮合分片TPS统计 (Matching Shards) ===" << std::endl;
    for (size_t shard_id = 0; shard_id < num_matching_shards; ++shard_id) {
        if (matching_shard_started[shard_id]) {
            auto shard_elapsed = duration_cast<milliseconds>(end_time - matching_shard_start_times[shard_id]).count();
            double shard_tps = (matching_shard_order_counts[shard_id] * 1000.0) / (shard_elapsed > 0 ? shard_elapsed : 1);
            double shard_percentage = (matching_shard_order_counts[shard_id] * 100.0) / processed;
            std::cout << "撮合分片 " << shard_id << ":" << std::endl;
            std::cout << "  订单数: " << matching_shard_order_counts[shard_id] << " (" 
                      << std::setprecision(1) << shard_percentage << "%)" << std::endl;
            std::cout << "  TPS: " << std::setprecision(2) << shard_tps << " K orders/sec" << std::endl;
            std::cout << "  耗时: " << shard_elapsed << " ms" << std::endl;
        } else {
            std::cout << "撮合分片 " << shard_id << ": 未使用" << std::endl;
        }
    }
    std::cout << std::endl;
    
    engine.shutdown();
}

int main(int argc, char* argv[]) {
    size_t num_orders = 50000;
    size_t report_interval = 1000;
    
    if (argc > 1) {
        num_orders = std::stoull(argv[1]);
    }
    if (argc > 2) {
        report_interval = std::stoull(argv[2]);
    }
    
    run_realtime_benchmark(num_orders, report_interval);
    
    return 0;
}
