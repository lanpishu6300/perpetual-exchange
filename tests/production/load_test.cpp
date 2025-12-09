#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>
#include <random>
#include "core/matching_engine_event_sourcing.h"
#include "core/auth_manager.h"
#include "core/account_manager.h"
#include "core/position_manager.h"
#include "core/liquidation_engine.h"
#include "core/funding_rate_manager.h"
// #include "core/market_data_service.h"  // Optional dependency
#include "core/monitoring_system.h"
#include "core/order.h"
#include "core/types.h"

using namespace perpetual;

// Load test for production components
class ProductionLoadTest {
public:
    ProductionLoadTest() {
        matching_engine_ = std::make_unique<MatchingEngineEventSourcing>(1);
        matching_engine_->initialize("./load_test_event_store");
        matching_engine_->set_deterministic_mode(true);
        
        auth_manager_ = std::make_unique<AuthManager>();
        account_manager_ = std::make_unique<AccountBalanceManager>();
        position_manager_ = std::make_unique<PositionManager>();
        liquidation_engine_ = std::make_unique<LiquidationEngine>();
        funding_manager_ = std::make_unique<FundingRateManager>();
        // market_data_service_ = std::make_unique<MarketDataService>();  // Optional
        monitoring_ = std::make_unique<MonitoringSystem>();
        
        liquidation_engine_->setAccountManager(account_manager_.get());
        liquidation_engine_->setPositionManager(position_manager_.get());
        funding_manager_->setAccountManager(account_manager_.get());
        funding_manager_->setPositionManager(position_manager_.get());
        
        // Setup test accounts
        for (int i = 0; i < 100; ++i) {
            UserID user_id = 1000000 + i;
            account_manager_->setBalance(user_id, 10000.0);
        }
    }
    
    void runLoadTest(int duration_seconds, int orders_per_second) {
        std::cout << "========================================" << std::endl;
        std::cout << "Production Load Test" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Duration: " << duration_seconds << " seconds" << std::endl;
        std::cout << "Target throughput: " << orders_per_second << " orders/sec" << std::endl;
        std::cout << "========================================" << std::endl;
        
        std::atomic<uint64_t> total_orders{0};
        std::atomic<uint64_t> total_trades{0};
        std::atomic<uint64_t> total_errors{0};
        std::vector<double> latencies;
        latencies.reserve(duration_seconds * orders_per_second);
        std::mutex latencies_mutex;
        
        auto start_time = std::chrono::steady_clock::now();
        auto end_time = start_time + std::chrono::seconds(duration_seconds);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> price_dist(49500, 50500);
        std::uniform_int_distribution<> qty_dist(1, 10);
        std::uniform_int_distribution<> user_dist(0, 99);
        
        std::vector<std::thread> worker_threads;
        const int num_workers = 4;
        
        for (int w = 0; w < num_workers; ++w) {
            worker_threads.emplace_back([&, w]() {
                uint64_t order_id = w * 10000000ULL;
                while (std::chrono::steady_clock::now() < end_time) {
                    auto order_start = std::chrono::high_resolution_clock::now();
                    
                    // Generate random order
                    UserID user_id = 1000000 + user_dist(gen);
                    OrderSide side = (order_id % 2 == 0) ? OrderSide::BUY : OrderSide::SELL;
                    Price price = double_to_price(price_dist(gen));
                    Quantity quantity = double_to_quantity(qty_dist(gen) * 0.01);
                    
                    Order order(order_id++, user_id, 1, side, price, quantity, OrderType::LIMIT);
                    
                    try {
                        auto trades = matching_engine_->process_order_es(&order);
                        total_orders++;
                        total_trades += trades.size();
                        
                        // Record metrics
                        monitoring_->recordOrderSubmitted(1);
                        if (!trades.empty()) {
                            monitoring_->recordOrderFilled(1);
                            for (const auto& trade : trades) {
                                monitoring_->recordTrade(1, trade.quantity);
                            }
                        }
                    } catch (...) {
                        total_errors++;
                    }
                    
                    auto order_end = std::chrono::high_resolution_clock::now();
                    auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(
                        order_end - order_start).count();
                    
                    {
                        std::lock_guard<std::mutex> lock(latencies_mutex);
                        latencies.push_back(latency);
                    }
                    
                    // Rate limiting
                    std::this_thread::sleep_for(
                        std::chrono::microseconds(1000000 / orders_per_second / num_workers));
                }
            });
        }
        
        for (auto& thread : worker_threads) {
            thread.join();
        }
        
        auto actual_duration = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - start_time).count();
        
        // Calculate statistics
        std::sort(latencies.begin(), latencies.end());
        double sum = 0;
        for (double lat : latencies) {
            sum += lat;
        }
        double avg_latency = latencies.empty() ? 0 : sum / latencies.size();
        double p50 = latencies.empty() ? 0 : latencies[latencies.size() / 2];
        double p90 = latencies.empty() ? 0 : latencies[static_cast<size_t>(latencies.size() * 0.90)];
        double p99 = latencies.empty() ? 0 : latencies[static_cast<size_t>(latencies.size() * 0.99)];
        
        double throughput = actual_duration > 0 ? 
            static_cast<double>(total_orders.load()) / actual_duration : 0;
        
        // Print results
        std::cout << "\n========================================" << std::endl;
        std::cout << "Load Test Results" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Total orders: " << total_orders.load() << std::endl;
        std::cout << "Total trades: " << total_trades.load() << std::endl;
        std::cout << "Total errors: " << total_errors.load() << std::endl;
        std::cout << "Actual duration: " << actual_duration << " seconds" << std::endl;
        std::cout << "Throughput: " << throughput << " orders/sec" << std::endl;
        std::cout << "\nLatency Statistics:" << std::endl;
        std::cout << "  Average: " << avg_latency / 1000.0 << " μs" << std::endl;
        std::cout << "  P50: " << p50 / 1000.0 << " μs" << std::endl;
        std::cout << "  P90: " << p90 / 1000.0 << " μs" << std::endl;
        std::cout << "  P99: " << p99 / 1000.0 << " μs" << std::endl;
        std::cout << "========================================" << std::endl;
        
        // Get monitoring metrics
        std::string metrics = monitoring_->getPrometheusMetrics();
        std::cout << "\nMonitoring Metrics (first 500 chars):" << std::endl;
        std::cout << metrics.substr(0, 500) << std::endl;
    }
    
private:
    std::unique_ptr<MatchingEngineEventSourcing> matching_engine_;
    std::unique_ptr<AuthManager> auth_manager_;
    std::unique_ptr<AccountBalanceManager> account_manager_;
    std::unique_ptr<PositionManager> position_manager_;
    std::unique_ptr<LiquidationEngine> liquidation_engine_;
    std::unique_ptr<FundingRateManager> funding_manager_;
    // std::unique_ptr<MarketDataService> market_data_service_;  // Optional
    std::unique_ptr<MonitoringSystem> monitoring_;
};

int main(int argc, char* argv[]) {
    int duration = 60;  // 60 seconds default
    int orders_per_second = 1000;  // 1000 orders/sec default
    
    if (argc > 1) {
        duration = std::stoi(argv[1]);
    }
    if (argc > 2) {
        orders_per_second = std::stoi(argv[2]);
    }
    
    ProductionLoadTest test;
    test.runLoadTest(duration, orders_per_second);
    
    return 0;
}

