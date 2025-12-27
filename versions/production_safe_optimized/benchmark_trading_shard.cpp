#include "core/trading_shard.h"
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

using namespace std::chrono;
using namespace perpetual;

int main(int argc, char* argv[]) {
    size_t num_orders = 100000;
    size_t num_shards = std::thread::hardware_concurrency();
    if (num_shards == 0) num_shards = 4;
    
    if (argc > 1) {
        num_orders = std::stoull(argv[1]);
    }
    if (argc > 2) {
        num_shards = std::stoull(argv[2]);
    }
    
    std::cout << "=== 交易分片独立压测 (Trading Shard Only) ===" << std::endl;
    std::cout << "订单数: " << num_orders << std::endl;
    std::cout << "分片数: " << num_shards << std::endl;
    std::cout << std::endl;
    
    // 创建多个交易分片
    std::vector<std::unique_ptr<TradingShard>> trading_shards;
    for (size_t i = 0; i < num_shards; ++i) {
        auto shard = std::make_unique<TradingShard>(i);
        if (!shard->initialize()) {
            std::cerr << "Failed to initialize trading shard " << i << std::endl;
            return 1;
        }
        trading_shards.push_back(std::move(shard));
    }
    
    std::cout << "交易分片初始化完成" << std::endl;
    std::cout << std::endl;
    
    // 生成测试订单
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<UserID> user_dist(1, 10000);
    std::uniform_int_distribution<InstrumentID> instrument_dist(1, 10);
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
    
    // 跟踪每个分片的统计
    std::vector<size_t> shard_order_counts(num_shards, 0);
    std::vector<high_resolution_clock::time_point> shard_start_times(num_shards);
    std::vector<bool> shard_started(num_shards, false);
    std::vector<std::vector<double>> shard_latencies(num_shards);
    
    // 预热
    const size_t warmup = 100;
    std::cout << "预热中... (" << warmup << " 订单)" << std::endl;
    for (size_t i = 0; i < warmup && i < orders.size(); ++i) {
        size_t shard_id = orders[i].user_id % num_shards;
        trading_shards[shard_id]->validate_and_prepare_order(&orders[i]);
    }
    std::cout << "预热完成" << std::endl;
    std::cout << std::endl;
    
    // 压测
    auto start_time = high_resolution_clock::now();
    size_t processed = 0;
    size_t validated = 0;
    
    for (size_t i = warmup; i < orders.size(); ++i) {
        size_t shard_id = orders[i].user_id % num_shards;
        
        auto order_start = high_resolution_clock::now();
        
        // 记录分片开始时间
        if (!shard_started[shard_id]) {
            shard_start_times[shard_id] = order_start;
            shard_started[shard_id] = true;
        }
        
        // 只测试交易分片的验证功能
        bool result = trading_shards[shard_id]->validate_and_prepare_order(&orders[i]);
        
        auto order_end = high_resolution_clock::now();
        auto latency = duration_cast<nanoseconds>(order_end - order_start).count() / 1000.0;  // μs
        
        shard_order_counts[shard_id]++;
        shard_latencies[shard_id].push_back(latency);
        
        if (result) validated++;
        processed++;
        
        // 实时报告
        if (processed % 10000 == 0 || processed == 1) {
            auto current_time = high_resolution_clock::now();
            auto elapsed = duration_cast<milliseconds>(current_time - start_time).count();
            double throughput = (processed * 1000.0) / (elapsed > 0 ? elapsed : 1);
            
            std::cout << "已处理: " << processed << " | 耗时: " << elapsed << " ms | "
                      << "吞吐量: " << std::setprecision(2) << throughput << " K orders/sec" << std::endl;
        }
    }
    
    auto end_time = high_resolution_clock::now();
    auto total_time = duration_cast<milliseconds>(end_time - start_time).count();
    
    std::cout << std::endl;
    std::cout << "=== 最终结果 ===" << std::endl;
    std::cout << std::endl;
    
    double total_throughput = (processed * 1000.0) / (total_time > 0 ? total_time : 1);
    std::cout << "总吞吐量: " << std::fixed << std::setprecision(2) << total_throughput << " K orders/sec" << std::endl;
    std::cout << "总耗时: " << total_time << " ms" << std::endl;
    std::cout << "验证通过: " << validated << " / " << processed << std::endl;
    std::cout << std::endl;
    
    // 输出每个分片的详细统计
    std::cout << "=== 各交易分片TPS统计 ===" << std::endl;
    for (size_t shard_id = 0; shard_id < num_shards; ++shard_id) {
        if (shard_started[shard_id] && !shard_latencies[shard_id].empty()) {
            auto shard_elapsed = duration_cast<milliseconds>(end_time - shard_start_times[shard_id]).count();
            double shard_tps = (shard_order_counts[shard_id] * 1000.0) / (shard_elapsed > 0 ? shard_elapsed : 1);
            double shard_percentage = (shard_order_counts[shard_id] * 100.0) / processed;
            
            // 计算延迟统计
            auto& latencies = shard_latencies[shard_id];
            std::sort(latencies.begin(), latencies.end());
            double avg_lat = 0;
            for (auto l : latencies) avg_lat += l;
            avg_lat /= latencies.size();
            double p50 = latencies[latencies.size() * 0.5];
            double p99 = latencies[latencies.size() * 0.99];
            
            std::cout << "交易分片 " << shard_id << ":" << std::endl;
            std::cout << "  订单数: " << shard_order_counts[shard_id] << " (" 
                      << std::setprecision(1) << shard_percentage << "%)" << std::endl;
            std::cout << "  TPS: " << std::setprecision(2) << shard_tps << " K orders/sec" << std::endl;
            std::cout << "  耗时: " << shard_elapsed << " ms" << std::endl;
            std::cout << "  平均延迟: " << std::setprecision(2) << avg_lat << " μs" << std::endl;
            std::cout << "  P50延迟: " << std::setprecision(2) << p50 << " μs" << std::endl;
            std::cout << "  P99延迟: " << std::setprecision(2) << p99 << " μs" << std::endl;
        } else {
            std::cout << "交易分片 " << shard_id << ": 未使用" << std::endl;
        }
    }
    std::cout << std::endl;
    
    return 0;
}

