#include "core/sharded_matching_engine.h"
#include "core/order.h"
#include "core/types.h"
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>

using namespace perpetual;
using namespace std::chrono;

int main() {
    std::cout << "=== 分片引擎快速压测 ===" << std::endl;
    std::cout << std::endl;
    
    // 初始化
    size_t num_cores = std::thread::hardware_concurrency();
    if (num_cores == 0) num_cores = 4;
    
    ShardedMatchingEngine engine(num_cores, num_cores);
    
    // 禁用WAL以加快速度
    if (!engine.initialize("", false)) {
        std::cerr << "初始化失败" << std::endl;
        return 1;
    }
    
    std::cout << "配置: " << engine.get_trading_shard_count() << " 交易分片, " 
              << engine.get_matching_shard_count() << " 撮合分片" << std::endl;
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
    
    std::cout << "开始压测 " << num_orders << " 订单..." << std::endl;
    std::cout << std::endl;
    
    // 压测
    auto start = high_resolution_clock::now();
    size_t total_trades = 0;
    size_t processed = 0;
    std::vector<double> latencies;
    latencies.reserve(num_orders);
    
    // 添加调试：先测试几个订单，确保能正常处理
    std::cout << "测试前3个订单..." << std::flush;
    for (size_t i = 0; i < std::min(3UL, orders.size()); ++i) {
        std::cout << "." << std::flush;
        try {
            auto trades = engine.process_order(&orders[i]);
            processed++;
            total_trades += trades.size();
        } catch (const std::exception& e) {
            std::cerr << "\n订单" << (i+1) << "异常: " << e.what() << std::endl;
        }
    }
    std::cout << " 完成！" << std::endl;
    std::cout << "开始正式压测..." << std::endl;
    std::cout << std::endl;
    
    for (size_t i = 0; i < orders.size(); ++i) {
        auto order_start = high_resolution_clock::now();
        
        try {
            auto trades = engine.process_order(&orders[i]);
            
            auto order_end = high_resolution_clock::now();
            auto latency = duration_cast<microseconds>(order_end - order_start).count();
            latencies.push_back(latency);
            
            total_trades += trades.size();
            processed++;
        } catch (const std::exception& e) {
            std::cerr << "订单" << (i+1) << "处理异常: " << e.what() << std::endl;
            continue;
        }
        
        // 实时输出：每50订单输出一次（更频繁）
        if ((i + 1) % 50 == 0) {
            auto now = high_resolution_clock::now();
            auto elapsed = duration_cast<milliseconds>(now - start).count();
            if (elapsed > 0) {
                double throughput = (processed * 1000.0) / elapsed;
                
                // 计算延迟统计
                if (latencies.size() > 0) {
                    double sum = 0;
                    for (size_t j = 0; j < latencies.size(); ++j) {
                        sum += latencies[j];
                    }
                    double avg_latency = sum / latencies.size();
                    
                    std::vector<double> sorted = latencies;
                    std::sort(sorted.begin(), sorted.end());
                    double p99 = sorted.size() > 0 ? sorted[std::min(sorted.size() - 1, size_t(sorted.size() * 0.99))] : 0;
                    
                    std::cout << "进度: " << std::fixed << std::setprecision(1) 
                              << (processed * 100.0 / num_orders) << "%"
                              << " | 已处理: " << processed << "/" << num_orders
                              << " | 吞吐量: " << std::setprecision(2) << throughput << " K/s"
                              << " | 平均延迟: " << std::setprecision(2) << avg_latency << " μs"
                              << " | P99延迟: " << std::setprecision(2) << p99 << " μs"
                              << " | 交易数: " << total_trades
                              << std::endl;
                }
            }
        }
    }
    
    std::cout << std::endl;  // 换行
    
    auto end = high_resolution_clock::now();
    auto total_time = duration_cast<milliseconds>(end - start).count();
    double throughput = (processed * 1000.0) / total_time;
    
    std::cout << std::endl;
    std::cout << "=== 压测结果 ===" << std::endl;
    std::cout << "总订单数: " << processed << std::endl;
    std::cout << "总交易数: " << total_trades << std::endl;
    std::cout << "总耗时: " << total_time << " ms" << std::endl;
    std::cout << "吞吐量: " << std::fixed << std::setprecision(2) << throughput << " K orders/sec" << std::endl;
    
    auto stats = engine.get_stats();
    std::cout << "异步写入: " << stats.async_writes << std::endl;
    std::cout << "同步写入: " << stats.sync_writes << std::endl;
    
    engine.shutdown();
    
    return 0;
}


#include "core/types.h"
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>

using namespace perpetual;
using namespace std::chrono;

int main() {
    std::cout << "=== 分片引擎快速压测 ===" << std::endl;
    std::cout << std::endl;
    
    // 初始化
    size_t num_cores = std::thread::hardware_concurrency();
    if (num_cores == 0) num_cores = 4;
    
    ShardedMatchingEngine engine(num_cores, num_cores);
    
    // 禁用WAL以加快速度
    if (!engine.initialize("", false)) {
        std::cerr << "初始化失败" << std::endl;
        return 1;
    }
    
    std::cout << "配置: " << engine.get_trading_shard_count() << " 交易分片, " 
              << engine.get_matching_shard_count() << " 撮合分片" << std::endl;
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
    
    std::cout << "开始压测 " << num_orders << " 订单..." << std::endl;
    std::cout << std::endl;
    
    // 压测
    auto start = high_resolution_clock::now();
    size_t total_trades = 0;
    size_t processed = 0;
    std::vector<double> latencies;
    latencies.reserve(num_orders);
    
    // 添加调试：先测试几个订单，确保能正常处理
    std::cout << "测试前3个订单..." << std::flush;
    for (size_t i = 0; i < std::min(3UL, orders.size()); ++i) {
        std::cout << "." << std::flush;
        try {
            auto trades = engine.process_order(&orders[i]);
            processed++;
            total_trades += trades.size();
        } catch (const std::exception& e) {
            std::cerr << "\n订单" << (i+1) << "异常: " << e.what() << std::endl;
        }
    }
    std::cout << " 完成！" << std::endl;
    std::cout << "开始正式压测..." << std::endl;
    std::cout << std::endl;
    
    for (size_t i = 0; i < orders.size(); ++i) {
        auto order_start = high_resolution_clock::now();
        
        try {
            auto trades = engine.process_order(&orders[i]);
            
            auto order_end = high_resolution_clock::now();
            auto latency = duration_cast<microseconds>(order_end - order_start).count();
            latencies.push_back(latency);
            
            total_trades += trades.size();
            processed++;
        } catch (const std::exception& e) {
            std::cerr << "订单" << (i+1) << "处理异常: " << e.what() << std::endl;
            continue;
        }
        
        // 实时输出：每50订单输出一次（更频繁）
        if ((i + 1) % 50 == 0) {
            auto now = high_resolution_clock::now();
            auto elapsed = duration_cast<milliseconds>(now - start).count();
            if (elapsed > 0) {
                double throughput = (processed * 1000.0) / elapsed;
                
                // 计算延迟统计
                if (latencies.size() > 0) {
                    double sum = 0;
                    for (size_t j = 0; j < latencies.size(); ++j) {
                        sum += latencies[j];
                    }
                    double avg_latency = sum / latencies.size();
                    
                    std::vector<double> sorted = latencies;
                    std::sort(sorted.begin(), sorted.end());
                    double p99 = sorted.size() > 0 ? sorted[std::min(sorted.size() - 1, size_t(sorted.size() * 0.99))] : 0;
                    
                    std::cout << "进度: " << std::fixed << std::setprecision(1) 
                              << (processed * 100.0 / num_orders) << "%"
                              << " | 已处理: " << processed << "/" << num_orders
                              << " | 吞吐量: " << std::setprecision(2) << throughput << " K/s"
                              << " | 平均延迟: " << std::setprecision(2) << avg_latency << " μs"
                              << " | P99延迟: " << std::setprecision(2) << p99 << " μs"
                              << " | 交易数: " << total_trades
                              << std::endl;
                }
            }
        }
    }
    
    std::cout << std::endl;  // 换行
    
    auto end = high_resolution_clock::now();
    auto total_time = duration_cast<milliseconds>(end - start).count();
    double throughput = (processed * 1000.0) / total_time;
    
    std::cout << std::endl;
    std::cout << "=== 压测结果 ===" << std::endl;
    std::cout << "总订单数: " << processed << std::endl;
    std::cout << "总交易数: " << total_trades << std::endl;
    std::cout << "总耗时: " << total_time << " ms" << std::endl;
    std::cout << "吞吐量: " << std::fixed << std::setprecision(2) << throughput << " K orders/sec" << std::endl;
    
    auto stats = engine.get_stats();
    std::cout << "异步写入: " << stats.async_writes << std::endl;
    std::cout << "同步写入: " << stats.sync_writes << std::endl;
    
    engine.shutdown();
    
    return 0;
}

