#include "core/matching_engine_production.h"
#include "core/persistence_optimized.h"
#include "core/types.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <iomanip>

using namespace perpetual;
using namespace std::chrono;

void benchmarkPersistence() {
    std::cout << "========================================\n";
    std::cout << "Persistence Performance Benchmark\n";
    std::cout << "========================================\n\n";
    
    // Initialize optimized persistence
    OptimizedPersistenceManager persistence;
    if (!persistence.initialize("./data/benchmark", 10000, 100)) {
        std::cerr << "Failed to initialize persistence\n";
        return;
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> price_dist(40000.0, 60000.0);
    std::uniform_real_distribution<double> qty_dist(0.01, 1.0);
    
    const size_t num_trades = 100000;
    const size_t num_orders = 100000;
    
    std::cout << "Testing " << num_trades << " trades and " << num_orders << " orders...\n\n";
    
    // Benchmark trade logging
    {
        std::vector<Trade> trades;
        trades.reserve(num_trades);
        
        for (size_t i = 0; i < num_trades; ++i) {
            Trade trade;
            trade.sequence_id = i + 1;
            trade.buy_order_id = i * 2 + 1;
            trade.sell_order_id = i * 2 + 2;
            trade.buy_user_id = (i % 1000) + 1;
            trade.sell_user_id = ((i + 1) % 1000) + 1;
            trade.instrument_id = 1;
            trade.price = double_to_price(price_dist(gen));
            trade.quantity = double_to_quantity(qty_dist(gen));
            trade.timestamp = get_current_timestamp();
            trade.is_taker_buy = (i % 2 == 0);
            trades.push_back(trade);
        }
        
        auto start = high_resolution_clock::now();
        
        for (const auto& trade : trades) {
            persistence.logTrade(trade);
        }
        
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start);
        
        double throughput = (num_trades * 1000000.0) / duration.count();
        double avg_latency = duration.count() / (double)num_trades;
        
        std::cout << "Trade Logging:\n";
        std::cout << "  Total time: " << duration.count() / 1000.0 << " ms\n";
        std::cout << "  Throughput: " << std::fixed << std::setprecision(0) 
                  << throughput << " trades/sec\n";
        std::cout << "  Avg latency: " << std::setprecision(2) 
                  << avg_latency << " μs\n\n";
    }
    
    // Benchmark order logging
    {
        std::vector<Order> orders;
        orders.reserve(num_orders);
        
        for (size_t i = 0; i < num_orders; ++i) {
            Order order;
            order.order_id = i + 1;
            order.user_id = (i % 1000) + 1;
            order.instrument_id = 1;
            order.side = (i % 2 == 0) ? OrderSide::BUY : OrderSide::SELL;
            order.price = double_to_price(price_dist(gen));
            order.quantity = double_to_quantity(qty_dist(gen));
            order.status = OrderStatus::FILLED;
            order.timestamp = get_current_timestamp();
            orders.push_back(order);
        }
        
        auto start = high_resolution_clock::now();
        
        for (const auto& order : orders) {
            persistence.logOrder(order, "PROCESSED");
        }
        
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start);
        
        double throughput = (num_orders * 1000000.0) / duration.count();
        double avg_latency = duration.count() / (double)num_orders;
        
        std::cout << "Order Logging:\n";
        std::cout << "  Total time: " << duration.count() / 1000.0 << " ms\n";
        std::cout << "  Throughput: " << std::fixed << std::setprecision(0) 
                  << throughput << " orders/sec\n";
        std::cout << "  Avg latency: " << std::setprecision(2) 
                  << avg_latency << " μs\n\n";
    }
    
    // Wait for async writes to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    persistence.flush();
    
    // Get statistics
    auto stats = persistence.getStats();
    
    std::cout << "Statistics:\n";
    std::cout << "  Trades logged: " << stats.trades_logged << "\n";
    std::cout << "  Orders logged: " << stats.orders_logged << "\n";
    std::cout << "  Batches written: " << stats.batches_written << "\n";
    std::cout << "  Bytes written: " << stats.bytes_written / 1024 / 1024 << " MB\n";
    std::cout << "  Write errors: " << stats.write_errors << "\n";
    std::cout << "  Avg write latency: " << std::setprecision(2) 
              << stats.avg_write_latency_us << " μs\n";
    
    persistence.shutdown();
    
    std::cout << "\n========================================\n";
    std::cout << "Benchmark completed!\n";
    std::cout << "========================================\n";
}

int main() {
    benchmarkPersistence();
    return 0;
}


#include "core/persistence_optimized.h"
#include "core/types.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <iomanip>

using namespace perpetual;
using namespace std::chrono;

void benchmarkPersistence() {
    std::cout << "========================================\n";
    std::cout << "Persistence Performance Benchmark\n";
    std::cout << "========================================\n\n";
    
    // Initialize optimized persistence
    OptimizedPersistenceManager persistence;
    if (!persistence.initialize("./data/benchmark", 10000, 100)) {
        std::cerr << "Failed to initialize persistence\n";
        return;
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> price_dist(40000.0, 60000.0);
    std::uniform_real_distribution<double> qty_dist(0.01, 1.0);
    
    const size_t num_trades = 100000;
    const size_t num_orders = 100000;
    
    std::cout << "Testing " << num_trades << " trades and " << num_orders << " orders...\n\n";
    
    // Benchmark trade logging
    {
        std::vector<Trade> trades;
        trades.reserve(num_trades);
        
        for (size_t i = 0; i < num_trades; ++i) {
            Trade trade;
            trade.sequence_id = i + 1;
            trade.buy_order_id = i * 2 + 1;
            trade.sell_order_id = i * 2 + 2;
            trade.buy_user_id = (i % 1000) + 1;
            trade.sell_user_id = ((i + 1) % 1000) + 1;
            trade.instrument_id = 1;
            trade.price = double_to_price(price_dist(gen));
            trade.quantity = double_to_quantity(qty_dist(gen));
            trade.timestamp = get_current_timestamp();
            trade.is_taker_buy = (i % 2 == 0);
            trades.push_back(trade);
        }
        
        auto start = high_resolution_clock::now();
        
        for (const auto& trade : trades) {
            persistence.logTrade(trade);
        }
        
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start);
        
        double throughput = (num_trades * 1000000.0) / duration.count();
        double avg_latency = duration.count() / (double)num_trades;
        
        std::cout << "Trade Logging:\n";
        std::cout << "  Total time: " << duration.count() / 1000.0 << " ms\n";
        std::cout << "  Throughput: " << std::fixed << std::setprecision(0) 
                  << throughput << " trades/sec\n";
        std::cout << "  Avg latency: " << std::setprecision(2) 
                  << avg_latency << " μs\n\n";
    }
    
    // Benchmark order logging
    {
        std::vector<Order> orders;
        orders.reserve(num_orders);
        
        for (size_t i = 0; i < num_orders; ++i) {
            Order order;
            order.order_id = i + 1;
            order.user_id = (i % 1000) + 1;
            order.instrument_id = 1;
            order.side = (i % 2 == 0) ? OrderSide::BUY : OrderSide::SELL;
            order.price = double_to_price(price_dist(gen));
            order.quantity = double_to_quantity(qty_dist(gen));
            order.status = OrderStatus::FILLED;
            order.timestamp = get_current_timestamp();
            orders.push_back(order);
        }
        
        auto start = high_resolution_clock::now();
        
        for (const auto& order : orders) {
            persistence.logOrder(order, "PROCESSED");
        }
        
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start);
        
        double throughput = (num_orders * 1000000.0) / duration.count();
        double avg_latency = duration.count() / (double)num_orders;
        
        std::cout << "Order Logging:\n";
        std::cout << "  Total time: " << duration.count() / 1000.0 << " ms\n";
        std::cout << "  Throughput: " << std::fixed << std::setprecision(0) 
                  << throughput << " orders/sec\n";
        std::cout << "  Avg latency: " << std::setprecision(2) 
                  << avg_latency << " μs\n\n";
    }
    
    // Wait for async writes to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    persistence.flush();
    
    // Get statistics
    auto stats = persistence.getStats();
    
    std::cout << "Statistics:\n";
    std::cout << "  Trades logged: " << stats.trades_logged << "\n";
    std::cout << "  Orders logged: " << stats.orders_logged << "\n";
    std::cout << "  Batches written: " << stats.batches_written << "\n";
    std::cout << "  Bytes written: " << stats.bytes_written / 1024 / 1024 << " MB\n";
    std::cout << "  Write errors: " << stats.write_errors << "\n";
    std::cout << "  Avg write latency: " << std::setprecision(2) 
              << stats.avg_write_latency_us << " μs\n";
    
    persistence.shutdown();
    
    std::cout << "\n========================================\n";
    std::cout << "Benchmark completed!\n";
    std::cout << "========================================\n";
}

int main() {
    benchmarkPersistence();
    return 0;
}


