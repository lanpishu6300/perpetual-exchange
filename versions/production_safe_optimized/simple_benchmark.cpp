#include "core/sharded_matching_engine.h"
#include "core/order.h"
#include "core/types.h"
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>
#include <algorithm>

using namespace perpetual;
using namespace std::chrono;

int main() {
    std::cout << "=== 分片架构性能Benchmark ===" << std::endl;
    std::cout << std::endl;
    
    // 初始化
    size_t num_cores = std::thread::hardware_concurrency();
    if (num_cores == 0) num_cores = 4;
    
    ShardedMatchingEngine engine(num_cores, num_cores);
    
    // 禁用WAL以测试分片性能
    bool enable_wal = false;
    if (!engine.initialize("", enable_wal)) {
        std::cerr << "初始化失败" << std::endl;
        return 1;
    }
    
    std::cout << "配置: " << engine.get_trading_shard_count() << " 交易分片, " 
              << engine.get_matching_shard_count() << " 撮合分片" << std::endl;
    std::cout << "WAL: " << (enable_wal ? "已启用" : "已禁用") << std::endl;
    std::cout << std::endl;
    
    // 生成测试订单
    const size_t num_orders = 5000;
    std::vector<Order> orders;
    orders.reserve(num_orders);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<UserID> user_dist(1, 100);
    std::uniform_int_distribution<InstrumentID> inst_dist(1, 10);
    std::uniform_int_distribution<Price> price_dist(10000, 20000);
    std::uniform_int_distribution<Quantity> qty_dist(1, 100);
    std::uniform_int_distribution<int> side_dist(0, 1);
    
    for (size_t i = 0; i < num_orders; ++i) {
        Order order;
        order.order_id = i + 1;
        order.user_id = user_dist(gen);
        order.instrument_id = inst_dist(gen);
        order.side = (side_dist(gen) == 0) ? OrderSide::BUY : OrderSide::SELL;
        order.order_type = OrderType::LIMIT;
        order.price = price_dist(gen);
        order.quantity = qty_dist(gen);
        order.remaining_quantity = order.quantity;
        order.status = OrderStatus::PENDING;
        orders.push_back(order);
    }
    
    std::cout << "开始性能测试 " << num_orders << " 订单..." << std::endl;
    std::cout << std::endl;
    
    // 预热
    const size_t warmup = 10;
    for (size_t i = 0; i < warmup && i < orders.size(); ++i) {
        engine.process_order(&orders[i]);
    }
    
    // 性能测试 - 实时输出
    std::cout << "实时性能指标:" << std::endl;
    std::cout << "订单数 | 耗时(ms) | 吞吐量(K/s) | 平均延迟(μs) | P50(μs) | P90(μs) | P99(μs)" << std::endl;
    std::cout << "-------------------------------------------------------------------------------" << std::endl;
    
    auto start = high_resolution_clock::now();
    std::vector<double> latencies;
    latencies.reserve(num_orders - warmup);
    size_t total_trades = 0;
    const size_t report_interval = 500;
    
    for (size_t i = warmup; i < orders.size(); ++i) {
        auto order_start = high_resolution_clock::now();
        auto trades = engine.process_order(&orders[i]);
        auto order_end = high_resolution_clock::now();
        
        auto latency = duration_cast<nanoseconds>(order_end - order_start).count() / 1000.0;
        latencies.push_back(latency);
        total_trades += trades.size();
        
        // 实时报告
        if ((i - warmup + 1) % report_interval == 0 || i == orders.size() - 1) {
            auto current = high_resolution_clock::now();
            auto elapsed = duration_cast<milliseconds>(current - start).count();
            double throughput = (latencies.size() * 1000.0) / elapsed;
            
            double sum = 0;
            for (auto lat : latencies) sum += lat;
            double avg = sum / latencies.size();
            
            std::vector<double> sorted = latencies;
            std::sort(sorted.begin(), sorted.end());
            
            double p50 = sorted[sorted.size() * 0.5];
            double p90 = sorted[sorted.size() * 0.9];
            double p99 = sorted[sorted.size() * 0.99];
            
            std::cout << std::setw(6) << latencies.size() << " | "
                      << std::setw(8) << elapsed << " | "
                      << std::fixed << std::setprecision(2) << std::setw(11) << throughput << " | "
                      << std::setw(13) << avg << " | "
                      << std::setw(7) << p50 << " | "
                      << std::setw(7) << p90 << " | "
                      << std::setw(7) << p99 << std::endl;
        }
    }
    
    auto end = high_resolution_clock::now();
    auto total_time = duration_cast<milliseconds>(end - start).count();
    
    // 最终统计
    size_t count = latencies.size();
    double sum = 0;
    for (auto lat : latencies) sum += lat;
    double avg = sum / count;
    
    std::sort(latencies.begin(), latencies.end());
    double p50 = latencies[count * 0.5];
    double p90 = latencies[count * 0.9];
    double p99 = latencies[count * 0.99];
    
    double throughput = (count * 1000.0) / total_time;
    
    std::cout << std::endl;
    std::cout << "=== 最终结果 ===" << std::endl;
    std::cout << "测试订单数: " << count << std::endl;
    std::cout << "总耗时: " << total_time << " ms" << std::endl;
    std::cout << "总交易数: " << total_trades << std::endl;
    std::cout << "吞吐量: " << std::fixed << std::setprecision(2) << throughput << " K orders/sec" << std::endl;
    std::cout << "平均延迟: " << std::setprecision(2) << avg << " μs" << std::endl;
    std::cout << "P50延迟: " << std::setprecision(2) << p50 << " μs" << std::endl;
    std::cout << "P90延迟: " << std::setprecision(2) << p90 << " μs" << std::endl;
    std::cout << "P99延迟: " << std::setprecision(2) << p99 << " μs" << std::endl;
    std::cout << "最小延迟: " << std::setprecision(2) << latencies[0] << " μs" << std::endl;
    std::cout << "最大延迟: " << std::setprecision(2) << latencies[count-1] << " μs" << std::endl;
    
    return 0;
}


#include "core/order.h"
#include "core/types.h"
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>
#include <algorithm>

using namespace perpetual;
using namespace std::chrono;

int main() {
    std::cout << "=== 分片架构性能Benchmark ===" << std::endl;
    std::cout << std::endl;
    
    // 初始化
    size_t num_cores = std::thread::hardware_concurrency();
    if (num_cores == 0) num_cores = 4;
    
    ShardedMatchingEngine engine(num_cores, num_cores);
    
    // 禁用WAL以测试分片性能
    bool enable_wal = false;
    if (!engine.initialize("", enable_wal)) {
        std::cerr << "初始化失败" << std::endl;
        return 1;
    }
    
    std::cout << "配置: " << engine.get_trading_shard_count() << " 交易分片, " 
              << engine.get_matching_shard_count() << " 撮合分片" << std::endl;
    std::cout << "WAL: " << (enable_wal ? "已启用" : "已禁用") << std::endl;
    std::cout << std::endl;
    
    // 生成测试订单
    const size_t num_orders = 5000;
    std::vector<Order> orders;
    orders.reserve(num_orders);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<UserID> user_dist(1, 100);
    std::uniform_int_distribution<InstrumentID> inst_dist(1, 10);
    std::uniform_int_distribution<Price> price_dist(10000, 20000);
    std::uniform_int_distribution<Quantity> qty_dist(1, 100);
    std::uniform_int_distribution<int> side_dist(0, 1);
    
    for (size_t i = 0; i < num_orders; ++i) {
        Order order;
        order.order_id = i + 1;
        order.user_id = user_dist(gen);
        order.instrument_id = inst_dist(gen);
        order.side = (side_dist(gen) == 0) ? OrderSide::BUY : OrderSide::SELL;
        order.order_type = OrderType::LIMIT;
        order.price = price_dist(gen);
        order.quantity = qty_dist(gen);
        order.remaining_quantity = order.quantity;
        order.status = OrderStatus::PENDING;
        orders.push_back(order);
    }
    
    std::cout << "开始性能测试 " << num_orders << " 订单..." << std::endl;
    std::cout << std::endl;
    
    // 预热
    const size_t warmup = 10;
    for (size_t i = 0; i < warmup && i < orders.size(); ++i) {
        engine.process_order(&orders[i]);
    }
    
    // 性能测试 - 实时输出
    std::cout << "实时性能指标:" << std::endl;
    std::cout << "订单数 | 耗时(ms) | 吞吐量(K/s) | 平均延迟(μs) | P50(μs) | P90(μs) | P99(μs)" << std::endl;
    std::cout << "-------------------------------------------------------------------------------" << std::endl;
    
    auto start = high_resolution_clock::now();
    std::vector<double> latencies;
    latencies.reserve(num_orders - warmup);
    size_t total_trades = 0;
    const size_t report_interval = 500;
    
    for (size_t i = warmup; i < orders.size(); ++i) {
        auto order_start = high_resolution_clock::now();
        auto trades = engine.process_order(&orders[i]);
        auto order_end = high_resolution_clock::now();
        
        auto latency = duration_cast<nanoseconds>(order_end - order_start).count() / 1000.0;
        latencies.push_back(latency);
        total_trades += trades.size();
        
        // 实时报告
        if ((i - warmup + 1) % report_interval == 0 || i == orders.size() - 1) {
            auto current = high_resolution_clock::now();
            auto elapsed = duration_cast<milliseconds>(current - start).count();
            double throughput = (latencies.size() * 1000.0) / elapsed;
            
            double sum = 0;
            for (auto lat : latencies) sum += lat;
            double avg = sum / latencies.size();
            
            std::vector<double> sorted = latencies;
            std::sort(sorted.begin(), sorted.end());
            
            double p50 = sorted[sorted.size() * 0.5];
            double p90 = sorted[sorted.size() * 0.9];
            double p99 = sorted[sorted.size() * 0.99];
            
            std::cout << std::setw(6) << latencies.size() << " | "
                      << std::setw(8) << elapsed << " | "
                      << std::fixed << std::setprecision(2) << std::setw(11) << throughput << " | "
                      << std::setw(13) << avg << " | "
                      << std::setw(7) << p50 << " | "
                      << std::setw(7) << p90 << " | "
                      << std::setw(7) << p99 << std::endl;
        }
    }
    
    auto end = high_resolution_clock::now();
    auto total_time = duration_cast<milliseconds>(end - start).count();
    
    // 最终统计
    size_t count = latencies.size();
    double sum = 0;
    for (auto lat : latencies) sum += lat;
    double avg = sum / count;
    
    std::sort(latencies.begin(), latencies.end());
    double p50 = latencies[count * 0.5];
    double p90 = latencies[count * 0.9];
    double p99 = latencies[count * 0.99];
    
    double throughput = (count * 1000.0) / total_time;
    
    std::cout << std::endl;
    std::cout << "=== 最终结果 ===" << std::endl;
    std::cout << "测试订单数: " << count << std::endl;
    std::cout << "总耗时: " << total_time << " ms" << std::endl;
    std::cout << "总交易数: " << total_trades << std::endl;
    std::cout << "吞吐量: " << std::fixed << std::setprecision(2) << throughput << " K orders/sec" << std::endl;
    std::cout << "平均延迟: " << std::setprecision(2) << avg << " μs" << std::endl;
    std::cout << "P50延迟: " << std::setprecision(2) << p50 << " μs" << std::endl;
    std::cout << "P90延迟: " << std::setprecision(2) << p90 << " μs" << std::endl;
    std::cout << "P99延迟: " << std::setprecision(2) << p99 << " μs" << std::endl;
    std::cout << "最小延迟: " << std::setprecision(2) << latencies[0] << " μs" << std::endl;
    std::cout << "最大延迟: " << std::setprecision(2) << latencies[count-1] << " μs" << std::endl;
    
    return 0;
}


