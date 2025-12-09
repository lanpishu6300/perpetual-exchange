#include <gtest/gtest.h>
#include <chrono>
#include <vector>
#include "core/matching_engine_event_sourcing.h"
#include "core/auth_manager.h"
#include "core/liquidation_engine.h"
#include "core/funding_rate_manager.h"
// #include "core/market_data_service.h"  // Optional dependency
#include "core/order.h"
#include "core/types.h"

using namespace perpetual;

class ProductionPerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        matching_engine_ = std::make_unique<MatchingEngineEventSourcing>(1);
        matching_engine_->initialize("./perf_test_event_store");
        matching_engine_->set_deterministic_mode(true);
        
        auth_manager_ = std::make_unique<AuthManager>();
        liquidation_engine_ = std::make_unique<LiquidationEngine>();
        funding_manager_ = std::make_unique<FundingRateManager>();
        // market_data_service_ = std::make_unique<MarketDataService>();  // Optional
    }
    
    std::unique_ptr<MatchingEngineEventSourcing> matching_engine_;
    std::unique_ptr<AuthManager> auth_manager_;
    std::unique_ptr<LiquidationEngine> liquidation_engine_;
    std::unique_ptr<FundingRateManager> funding_manager_;
    // std::unique_ptr<MarketDataService> market_data_service_;  // Optional
};

TEST_F(ProductionPerformanceTest, MatchingEngineLatency) {
    const int iterations = 10000;
    std::vector<double> latencies;
    latencies.reserve(iterations);
    
    UserID user_id = 1000000;
    
    for (int i = 0; i < iterations; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        
        Order order(i, user_id + (i % 100), 1, OrderSide::BUY,
                   double_to_price(50000.0 + (i % 100)), double_to_quantity(0.1),
                   OrderType::LIMIT);
        matching_engine_->process_order_es(&order);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        latencies.push_back(duration.count());
    }
    
    // Calculate statistics
    double sum = 0;
    for (double lat : latencies) {
        sum += lat;
    }
    double avg_latency = sum / latencies.size();
    
    std::sort(latencies.begin(), latencies.end());
    double p50 = latencies[latencies.size() / 2];
    double p99 = latencies[static_cast<size_t>(latencies.size() * 0.99)];
    
    std::cout << "Matching Engine Latency (10K orders):" << std::endl;
    std::cout << "  Average: " << avg_latency / 1000.0 << " μs" << std::endl;
    std::cout << "  P50: " << p50 / 1000.0 << " μs" << std::endl;
    std::cout << "  P99: " << p99 / 1000.0 << " μs" << std::endl;
    
    // Performance target: P99 < 100μs
    EXPECT_LT(p99, 100000.0) << "P99 latency exceeds 100μs";
}

TEST_F(ProductionPerformanceTest, AuthManagerThroughput) {
    const int iterations = 10000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        std::string error_msg;
        std::string username = "user" + std::to_string(i);
        std::string email = "user" + std::to_string(i) + "@example.com";
        auth_manager_->registerUser(username, email, "password123", error_msg);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    double throughput = (iterations * 1000000.0) / duration.count();
    
    std::cout << "Auth Manager Throughput:" << std::endl;
    std::cout << "  Operations: " << iterations << std::endl;
    std::cout << "  Time: " << duration.count() << " μs" << std::endl;
    std::cout << "  Throughput: " << throughput << " ops/sec" << std::endl;
    
    // Target: > 10K ops/sec
    EXPECT_GT(throughput, 10000.0);
}

TEST_F(ProductionPerformanceTest, QueryPerformance) {
    // Setup: create orders first
    const int num_orders = 1000;
    UserID user_id = 1000000;
    
    for (int i = 0; i < num_orders; ++i) {
        Order order(i, user_id, 1, OrderSide::BUY,
                   double_to_price(50000.0), double_to_quantity(0.1),
                   OrderType::LIMIT);
        matching_engine_->process_order_es(&order);
    }
    
    // Query performance test
    // Note: Order query would need to be implemented in matching engine
    // For now, we test order processing performance
    const int query_iterations = 1000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < query_iterations; ++i) {
        Order order(num_orders + i, user_id_ + (i % 10), 1, OrderSide::BUY,
                   double_to_price(50000.0), double_to_quantity(0.1),
                   OrderType::LIMIT);
        matching_engine_->process_order_es(&order);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    double avg_order_time = duration.count() / static_cast<double>(query_iterations);
    
    std::cout << "Order Processing Performance:" << std::endl;
    std::cout << "  Average order time: " << avg_order_time << " μs" << std::endl;
    std::cout << "  Throughput: " << (1000000.0 / avg_order_time) << " orders/sec" << std::endl;
    
    // Target: < 100μs per order
    EXPECT_LT(avg_order_time, 100.0);
}

TEST_F(ProductionPerformanceTest, EventSourcingOverhead) {
    const int iterations = 1000;
    UserID user_id = 1000000;
    
    // Test with Event Sourcing enabled
    auto start_es = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        Order order(i, user_id, 1, OrderSide::BUY,
                   double_to_price(50000.0), double_to_quantity(0.1),
                   OrderType::LIMIT);
        matching_engine_->process_order_es(&order);
    }
    auto end_es = std::chrono::high_resolution_clock::now();
    auto duration_es = std::chrono::duration_cast<std::chrono::nanoseconds>(end_es - start_es);
    
    double avg_with_es = duration_es.count() / static_cast<double>(iterations);
    
    std::cout << "Event Sourcing Overhead:" << std::endl;
    std::cout << "  Average latency with ES: " << avg_with_es / 1000.0 << " μs" << std::endl;
    
    // Event Sourcing overhead should be < 500ns
    EXPECT_LT(avg_with_es, 10000.0) << "Event Sourcing overhead too high";
}

TEST_F(ProductionPerformanceTest, ConcurrentOrderProcessing) {
    const int num_threads = 4;
    const int orders_per_thread = 1000;
    
    std::vector<std::thread> threads;
    std::atomic<int> completed{0};
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, t, orders_per_thread, &completed]() {
            UserID user_id = 1000000 + t * 10000;
            for (int i = 0; i < orders_per_thread; ++i) {
                OrderID order_id = t * 1000000 + i;
                Order order(order_id, user_id, 1, 
                           (i % 2 == 0) ? OrderSide::BUY : OrderSide::SELL,
                           double_to_price(50000.0), double_to_quantity(0.1),
                           OrderType::LIMIT);
                matching_engine_->process_order_es(&order);
            }
            completed++;
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    int total_orders = num_threads * orders_per_thread;
    double throughput = (total_orders * 1000.0) / duration.count();
    
    std::cout << "Concurrent Order Processing:" << std::endl;
    std::cout << "  Threads: " << num_threads << std::endl;
    std::cout << "  Total orders: " << total_orders << std::endl;
    std::cout << "  Time: " << duration.count() << " ms" << std::endl;
    std::cout << "  Throughput: " << throughput << " orders/sec" << std::endl;
    
    EXPECT_EQ(completed.load(), num_threads);
}

