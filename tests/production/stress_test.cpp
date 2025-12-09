#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>
#include <random>
#include "core/matching_engine_event_sourcing.h"
#include "core/account_manager.h"
#include "core/monitoring_system.h"
#include "core/order.h"
#include "core/types.h"
#include <filesystem>

using namespace perpetual;

// Stress test - push system to limits
class ProductionStressTest {
public:
    ProductionStressTest() {
        matching_engine_ = std::make_unique<MatchingEngineEventSourcing>(1);
        matching_engine_->initialize("./stress_test_event_store");
        matching_engine_->set_deterministic_mode(true);
        
        account_manager_ = std::make_unique<AccountBalanceManager>();
        monitoring_ = std::make_unique<MonitoringSystem>();
    }
    
    ~ProductionStressTest() {
        if (std::filesystem::exists("./stress_test_event_store")) {
            std::filesystem::remove_all("./stress_test_event_store");
        }
    }
    
    void runStressTest(int num_threads, int orders_per_thread) {
        std::cout << "========================================" << std::endl;
        std::cout << "Production Stress Test" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Threads: " << num_threads << std::endl;
        std::cout << "Orders per thread: " << orders_per_thread << std::endl;
        std::cout << "Total orders: " << (num_threads * orders_per_thread) << std::endl;
        std::cout << "========================================" << std::endl;
        
        std::atomic<uint64_t> completed{0};
        std::atomic<uint64_t> errors{0};
        std::vector<std::thread> threads;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([this, t, orders_per_thread, &completed, &errors]() {
                UserID user_id = 1000000 + t * 10000;
                account_manager_->setBalance(user_id, 100000.0);
                
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> price_dist(49000, 51000);
                
                for (int i = 0; i < orders_per_thread; ++i) {
                    try {
                        OrderID order_id = t * 1000000ULL + i;
                        OrderSide side = (i % 2 == 0) ? OrderSide::BUY : OrderSide::SELL;
                        Price price = double_to_price(price_dist(gen));
                        Quantity quantity = double_to_quantity(0.1);
                        
                        Order order(order_id, user_id, 1, side, price, quantity, OrderType::LIMIT);
                        
                        auto trade_start = std::chrono::high_resolution_clock::now();
                        auto trades = matching_engine_->process_order_es(&order);
                        auto trade_end = std::chrono::high_resolution_clock::now();
                        
                        auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(
                            trade_end - trade_start).count();
                        monitoring_->recordMatchingLatency(latency);
                        
                        completed++;
                    } catch (const std::exception& e) {
                        errors++;
                    }
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "Stress Test Results" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Completed: " << completed.load() << std::endl;
        std::cout << "Errors: " << errors.load() << std::endl;
        std::cout << "Duration: " << duration.count() << " ms" << std::endl;
        std::cout << "Throughput: " << (completed.load() * 1000.0 / duration.count()) << " orders/sec" << std::endl;
        std::cout << "========================================" << std::endl;
    }
    
private:
    std::unique_ptr<MatchingEngineEventSourcing> matching_engine_;
    std::unique_ptr<AccountBalanceManager> account_manager_;
    std::unique_ptr<MonitoringSystem> monitoring_;
};

int main(int argc, char* argv[]) {
    int num_threads = 8;
    int orders_per_thread = 10000;
    
    if (argc > 1) {
        num_threads = std::stoi(argv[1]);
    }
    if (argc > 2) {
        orders_per_thread = std::stoi(argv[2]);
    }
    
    ProductionStressTest test;
    test.runStressTest(num_threads, orders_per_thread);
    
    return 0;
}

