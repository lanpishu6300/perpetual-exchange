#include "core/matching_engine.h"
#include "core/account.h"
#include "core/position.h"
#include "core/types.h"
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <atomic>
#include <thread>

using namespace perpetual;
using namespace std::chrono;

struct BenchmarkStats {
    uint64_t total_orders = 0;
    uint64_t total_trades = 0;
    uint64_t total_volume = 0;
    uint64_t total_errors = 0;
    
    nanoseconds total_time{0};
    nanoseconds min_latency{nanoseconds::max()};
    nanoseconds max_latency{nanoseconds::zero()};
    nanoseconds total_latency{0};
    
    void add_latency(nanoseconds latency) {
        total_latency += latency;
        if (latency < min_latency) min_latency = latency;
        if (latency > max_latency) max_latency = latency;
    }
    
    double avg_latency_ns() const {
        return total_orders > 0 ? 
            static_cast<double>(total_latency.count()) / total_orders : 0.0;
    }
    
    double throughput_ops_per_sec() const {
        return total_time.count() > 0 ?
            (static_cast<double>(total_orders) * 1e9) / total_time.count() : 0.0;
    }
};

class BenchmarkRunner {
public:
    BenchmarkRunner(InstrumentID instrument_id, size_t num_users = 1000)
        : engine_(instrument_id), instrument_id_(instrument_id), num_users_(num_users) {
        setup_users();
    }
    
    BenchmarkStats run_benchmark(size_t num_orders, double match_ratio = 0.5) {
        BenchmarkStats stats;
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<double> price_dist(40000.0, 60000.0);
        std::uniform_real_distribution<double> qty_dist(0.01, 1.0);
        std::uniform_real_distribution<double> side_dist(0.0, 1.0);
        std::uniform_real_distribution<double> match_dist(0.0, 1.0);
        
        auto start_time = high_resolution_clock::now();
        
        OrderID next_order_id = 1;
        std::vector<std::unique_ptr<Order>> orders;
        orders.reserve(num_orders);
        
        for (size_t i = 0; i < num_orders; ++i) {
            // Generate random order
            UserID user_id = (i % num_users_) + 1;
            OrderSide side = (side_dist(gen) < 0.5) ? OrderSide::BUY : OrderSide::SELL;
            Price price = double_to_price(price_dist(gen));
            Quantity quantity = double_to_quantity(qty_dist(gen));
            
            // Create order
            auto order = std::make_unique<Order>(
                next_order_id++, user_id, instrument_id_,
                side, price, quantity, OrderType::LIMIT
            );
            
            // Measure latency
            auto order_start = high_resolution_clock::now();
            
            try {
                auto trades = engine_.process_order(order.get());
                
                auto order_end = high_resolution_clock::now();
                auto latency = duration_cast<nanoseconds>(order_end - order_start);
                
                stats.total_orders++;
                stats.total_trades += trades.size();
                for (const auto& trade : trades) {
                    stats.total_volume += trade.quantity;
                }
                stats.add_latency(latency);
                
                // Keep order if not fully filled
                if (order && order->is_active()) {
                    orders.push_back(std::move(order));
                }
            } catch (const std::exception& e) {
                stats.total_errors++;
                std::cerr << "Error: " << e.what() << "\n";
            } catch (...) {
                stats.total_errors++;
            }
            
            // Periodically cancel some orders to simulate real trading
            if (i > 0 && i % 1000 == 0 && !orders.empty()) {
                size_t cancel_idx = i % orders.size();
                if (cancel_idx < orders.size() && orders[cancel_idx] && 
                    orders[cancel_idx]->is_active()) {
                    try {
                        engine_.cancel_order(orders[cancel_idx]->order_id, 
                                            orders[cancel_idx]->user_id);
                    } catch (...) {
                        // Ignore cancel errors
                    }
                }
            }
        }
        
        auto end_time = high_resolution_clock::now();
        stats.total_time = duration_cast<nanoseconds>(end_time - start_time);
        
        return stats;
    }
    
    BenchmarkStats run_throughput_test(size_t num_orders, size_t num_threads = 1) {
        BenchmarkStats stats;
        std::atomic<uint64_t> order_counter{0};
        std::atomic<uint64_t> trade_counter{0};
        std::atomic<uint64_t> error_counter{0};
        
        std::vector<std::thread> threads;
        std::vector<nanoseconds> thread_times(num_threads);
        
        auto start_time = high_resolution_clock::now();
        
        for (size_t t = 0; t < num_threads; ++t) {
            threads.emplace_back([&, t]() {
                std::random_device rd;
                std::mt19937 gen(rd() + t);
                std::uniform_real_distribution<double> price_dist(40000.0, 60000.0);
                std::uniform_real_distribution<double> qty_dist(0.01, 1.0);
                std::uniform_real_distribution<double> side_dist(0.0, 1.0);
                
                auto thread_start = high_resolution_clock::now();
                OrderID base_order_id = t * 1000000ULL;
                
                for (size_t i = 0; i < num_orders / num_threads; ++i) {
                    UserID user_id = ((i + t * num_orders) % num_users_) + 1;
                    OrderSide side = (side_dist(gen) < 0.5) ? OrderSide::BUY : OrderSide::SELL;
                    Price price = double_to_price(price_dist(gen));
                    Quantity quantity = double_to_quantity(qty_dist(gen));
                    
                    auto order = std::make_unique<Order>(
                        base_order_id + i, user_id, instrument_id_,
                        side, price, quantity, OrderType::LIMIT
                    );
                    
                    try {
                        auto trades = engine_.process_order(order.get());
                        order_counter++;
                        trade_counter += trades.size();
                    } catch (...) {
                        error_counter++;
                    }
                }
                
                auto thread_end = high_resolution_clock::now();
                thread_times[t] = duration_cast<nanoseconds>(thread_end - thread_start);
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        auto end_time = high_resolution_clock::now();
        stats.total_time = duration_cast<nanoseconds>(end_time - start_time);
        stats.total_orders = order_counter.load();
        stats.total_trades = trade_counter.load();
        stats.total_errors = error_counter.load();
        
        return stats;
    }
    
    const MatchingEngine& engine() const { return engine_; }
    
private:
    void setup_users() {
        // Initialize user accounts (simplified)
        // In real implementation, this would set up account balances
    }
    
    MatchingEngine engine_;
    InstrumentID instrument_id_;
    size_t num_users_;
};

void print_stats(const BenchmarkStats& stats, const std::string& test_name) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "Benchmark: " << test_name << "\n";
    std::cout << std::string(60, '=') << "\n";
    std::cout << std::fixed << std::setprecision(2);
    
    std::cout << "Total Orders:      " << stats.total_orders << "\n";
    std::cout << "Total Trades:     " << stats.total_trades << "\n";
    std::cout << "Total Volume:     " << quantity_to_double(stats.total_volume) << "\n";
    std::cout << "Total Errors:     " << stats.total_errors << "\n";
    std::cout << "\n";
    
    std::cout << "Total Time:        " 
              << duration_cast<milliseconds>(stats.total_time).count() << " ms\n";
    std::cout << "Throughput:        " 
              << stats.throughput_ops_per_sec() / 1000.0 << " K orders/sec\n";
    std::cout << "\n";
    
    std::cout << "Latency Statistics:\n";
    std::cout << "  Average:        " << stats.avg_latency_ns() << " ns\n";
    std::cout << "  Average:        " << stats.avg_latency_ns() / 1000.0 << " μs\n";
    std::cout << "  Min:            " << stats.min_latency.count() << " ns\n";
    std::cout << "  Max:            " << stats.max_latency.count() << " ns\n";
    
    if (stats.min_latency != nanoseconds::max()) {
        std::cout << "  Min:            " << stats.min_latency.count() / 1000.0 << " μs\n";
    }
    if (stats.max_latency != nanoseconds::zero()) {
        std::cout << "  Max:            " << stats.max_latency.count() / 1000.0 << " μs\n";
    }
    
    std::cout << "\n";
}

void generate_report(const std::vector<std::pair<std::string, BenchmarkStats>>& results) {
    std::ofstream report("benchmark_report.txt");
    
    report << "Perpetual Exchange - Benchmark Report\n";
    report << "=====================================\n\n";
    report << "Generated: " << std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() << "\n\n";
    
    report << std::fixed << std::setprecision(2);
    
    for (const auto& [name, stats] : results) {
        report << "Test: " << name << "\n";
        report << std::string(50, '-') << "\n";
        report << "Total Orders:      " << stats.total_orders << "\n";
        report << "Total Trades:      " << stats.total_trades << "\n";
        report << "Total Volume:      " << quantity_to_double(stats.total_volume) << "\n";
        report << "Total Errors:      " << stats.total_errors << "\n";
        report << "Total Time:        " 
               << duration_cast<milliseconds>(stats.total_time).count() << " ms\n";
        report << "Throughput:        " 
               << stats.throughput_ops_per_sec() / 1000.0 << " K orders/sec\n";
        report << "Avg Latency:       " << stats.avg_latency_ns() << " ns\n";
        report << "Avg Latency:       " << stats.avg_latency_ns() / 1000.0 << " μs\n";
        report << "Min Latency:       " << stats.min_latency.count() << " ns\n";
        report << "Max Latency:       " << stats.max_latency.count() << " ns\n";
        report << "\n";
    }
    
    report.close();
    std::cout << "\nBenchmark report saved to: benchmark_report.txt\n";
}

int main(int argc, char* argv[]) {
    std::cout << "Perpetual Exchange - Performance Benchmark\n";
    std::cout << "==========================================\n\n";
    
    InstrumentID instrument_id = 1;
    std::vector<std::pair<std::string, BenchmarkStats>> results;
    
    // Test 1: Small scale test
    std::cout << "Running Test 1: Small Scale (1K orders)...\n";
    BenchmarkRunner runner1(instrument_id, 100);
    auto stats1 = runner1.run_benchmark(1000, 0.5);
    print_stats(stats1, "Small Scale (1K orders)");
    results.push_back({"Small Scale (1K orders)", stats1});
    
    // Test 2: Medium scale test
    std::cout << "\nRunning Test 2: Medium Scale (10K orders)...\n";
    BenchmarkRunner runner2(instrument_id, 500);
    auto stats2 = runner2.run_benchmark(10000, 0.5);
    print_stats(stats2, "Medium Scale (10K orders)");
    results.push_back({"Medium Scale (10K orders)", stats2});
    
    // Test 3: Large scale test
    std::cout << "\nRunning Test 3: Large Scale (100K orders)...\n";
    BenchmarkRunner runner3(instrument_id, 1000);
    auto stats3 = runner3.run_benchmark(100000, 0.5);
    print_stats(stats3, "Large Scale (100K orders)");
    results.push_back({"Large Scale (100K orders)", stats3});
    
    // Test 4: Throughput test (single thread)
    std::cout << "\nRunning Test 4: Throughput Test (Single Thread, 50K orders)...\n";
    BenchmarkRunner runner4(instrument_id, 1000);
    auto stats4 = runner4.run_throughput_test(50000, 1);
    print_stats(stats4, "Throughput Test (Single Thread)");
    results.push_back({"Throughput Test (Single Thread)", stats4});
    
    // Test 5: Throughput test (multi-thread)
    std::cout << "\nRunning Test 5: Throughput Test (4 Threads, 50K orders)...\n";
    BenchmarkRunner runner5(instrument_id, 1000);
    auto stats5 = runner5.run_throughput_test(50000, 4);
    print_stats(stats5, "Throughput Test (4 Threads)");
    results.push_back({"Throughput Test (4 Threads)", stats5});
    
    // Generate report
    generate_report(results);
    
    // Print summary
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "Summary\n";
    std::cout << std::string(60, '=') << "\n";
    std::cout << "Best Throughput: " 
              << std::max_element(results.begin(), results.end(),
                  [](const auto& a, const auto& b) {
                      return a.second.throughput_ops_per_sec() < 
                             b.second.throughput_ops_per_sec();
                  })->second.throughput_ops_per_sec() / 1000.0 
              << " K orders/sec\n";
    
    auto min_latency_it = std::min_element(results.begin(), results.end(),
        [](const auto& a, const auto& b) {
            return a.second.avg_latency_ns() < b.second.avg_latency_ns();
        });
    std::cout << "Best Avg Latency: " << min_latency_it->second.avg_latency_ns() 
              << " ns (" << min_latency_it->second.avg_latency_ns() / 1000.0 << " μs)\n";
    std::cout << "  Test: " << min_latency_it->first << "\n";
    
    return 0;
}

#include "core/position.h"
#include "core/types.h"
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <atomic>
#include <thread>

using namespace perpetual;
using namespace std::chrono;

struct BenchmarkStats {
    uint64_t total_orders = 0;
    uint64_t total_trades = 0;
    uint64_t total_volume = 0;
    uint64_t total_errors = 0;
    
    nanoseconds total_time{0};
    nanoseconds min_latency{nanoseconds::max()};
    nanoseconds max_latency{nanoseconds::zero()};
    nanoseconds total_latency{0};
    
    void add_latency(nanoseconds latency) {
        total_latency += latency;
        if (latency < min_latency) min_latency = latency;
        if (latency > max_latency) max_latency = latency;
    }
    
    double avg_latency_ns() const {
        return total_orders > 0 ? 
            static_cast<double>(total_latency.count()) / total_orders : 0.0;
    }
    
    double throughput_ops_per_sec() const {
        return total_time.count() > 0 ?
            (static_cast<double>(total_orders) * 1e9) / total_time.count() : 0.0;
    }
};

class BenchmarkRunner {
public:
    BenchmarkRunner(InstrumentID instrument_id, size_t num_users = 1000)
        : engine_(instrument_id), instrument_id_(instrument_id), num_users_(num_users) {
        setup_users();
    }
    
    BenchmarkStats run_benchmark(size_t num_orders, double match_ratio = 0.5) {
        BenchmarkStats stats;
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<double> price_dist(40000.0, 60000.0);
        std::uniform_real_distribution<double> qty_dist(0.01, 1.0);
        std::uniform_real_distribution<double> side_dist(0.0, 1.0);
        std::uniform_real_distribution<double> match_dist(0.0, 1.0);
        
        auto start_time = high_resolution_clock::now();
        
        OrderID next_order_id = 1;
        std::vector<std::unique_ptr<Order>> orders;
        orders.reserve(num_orders);
        
        for (size_t i = 0; i < num_orders; ++i) {
            // Generate random order
            UserID user_id = (i % num_users_) + 1;
            OrderSide side = (side_dist(gen) < 0.5) ? OrderSide::BUY : OrderSide::SELL;
            Price price = double_to_price(price_dist(gen));
            Quantity quantity = double_to_quantity(qty_dist(gen));
            
            // Create order
            auto order = std::make_unique<Order>(
                next_order_id++, user_id, instrument_id_,
                side, price, quantity, OrderType::LIMIT
            );
            
            // Measure latency
            auto order_start = high_resolution_clock::now();
            
            try {
                auto trades = engine_.process_order(order.get());
                
                auto order_end = high_resolution_clock::now();
                auto latency = duration_cast<nanoseconds>(order_end - order_start);
                
                stats.total_orders++;
                stats.total_trades += trades.size();
                for (const auto& trade : trades) {
                    stats.total_volume += trade.quantity;
                }
                stats.add_latency(latency);
                
                // Keep order if not fully filled
                if (order && order->is_active()) {
                    orders.push_back(std::move(order));
                }
            } catch (const std::exception& e) {
                stats.total_errors++;
                std::cerr << "Error: " << e.what() << "\n";
            } catch (...) {
                stats.total_errors++;
            }
            
            // Periodically cancel some orders to simulate real trading
            if (i > 0 && i % 1000 == 0 && !orders.empty()) {
                size_t cancel_idx = i % orders.size();
                if (cancel_idx < orders.size() && orders[cancel_idx] && 
                    orders[cancel_idx]->is_active()) {
                    try {
                        engine_.cancel_order(orders[cancel_idx]->order_id, 
                                            orders[cancel_idx]->user_id);
                    } catch (...) {
                        // Ignore cancel errors
                    }
                }
            }
        }
        
        auto end_time = high_resolution_clock::now();
        stats.total_time = duration_cast<nanoseconds>(end_time - start_time);
        
        return stats;
    }
    
    BenchmarkStats run_throughput_test(size_t num_orders, size_t num_threads = 1) {
        BenchmarkStats stats;
        std::atomic<uint64_t> order_counter{0};
        std::atomic<uint64_t> trade_counter{0};
        std::atomic<uint64_t> error_counter{0};
        
        std::vector<std::thread> threads;
        std::vector<nanoseconds> thread_times(num_threads);
        
        auto start_time = high_resolution_clock::now();
        
        for (size_t t = 0; t < num_threads; ++t) {
            threads.emplace_back([&, t]() {
                std::random_device rd;
                std::mt19937 gen(rd() + t);
                std::uniform_real_distribution<double> price_dist(40000.0, 60000.0);
                std::uniform_real_distribution<double> qty_dist(0.01, 1.0);
                std::uniform_real_distribution<double> side_dist(0.0, 1.0);
                
                auto thread_start = high_resolution_clock::now();
                OrderID base_order_id = t * 1000000ULL;
                
                for (size_t i = 0; i < num_orders / num_threads; ++i) {
                    UserID user_id = ((i + t * num_orders) % num_users_) + 1;
                    OrderSide side = (side_dist(gen) < 0.5) ? OrderSide::BUY : OrderSide::SELL;
                    Price price = double_to_price(price_dist(gen));
                    Quantity quantity = double_to_quantity(qty_dist(gen));
                    
                    auto order = std::make_unique<Order>(
                        base_order_id + i, user_id, instrument_id_,
                        side, price, quantity, OrderType::LIMIT
                    );
                    
                    try {
                        auto trades = engine_.process_order(order.get());
                        order_counter++;
                        trade_counter += trades.size();
                    } catch (...) {
                        error_counter++;
                    }
                }
                
                auto thread_end = high_resolution_clock::now();
                thread_times[t] = duration_cast<nanoseconds>(thread_end - thread_start);
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        auto end_time = high_resolution_clock::now();
        stats.total_time = duration_cast<nanoseconds>(end_time - start_time);
        stats.total_orders = order_counter.load();
        stats.total_trades = trade_counter.load();
        stats.total_errors = error_counter.load();
        
        return stats;
    }
    
    const MatchingEngine& engine() const { return engine_; }
    
private:
    void setup_users() {
        // Initialize user accounts (simplified)
        // In real implementation, this would set up account balances
    }
    
    MatchingEngine engine_;
    InstrumentID instrument_id_;
    size_t num_users_;
};

void print_stats(const BenchmarkStats& stats, const std::string& test_name) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "Benchmark: " << test_name << "\n";
    std::cout << std::string(60, '=') << "\n";
    std::cout << std::fixed << std::setprecision(2);
    
    std::cout << "Total Orders:      " << stats.total_orders << "\n";
    std::cout << "Total Trades:     " << stats.total_trades << "\n";
    std::cout << "Total Volume:     " << quantity_to_double(stats.total_volume) << "\n";
    std::cout << "Total Errors:     " << stats.total_errors << "\n";
    std::cout << "\n";
    
    std::cout << "Total Time:        " 
              << duration_cast<milliseconds>(stats.total_time).count() << " ms\n";
    std::cout << "Throughput:        " 
              << stats.throughput_ops_per_sec() / 1000.0 << " K orders/sec\n";
    std::cout << "\n";
    
    std::cout << "Latency Statistics:\n";
    std::cout << "  Average:        " << stats.avg_latency_ns() << " ns\n";
    std::cout << "  Average:        " << stats.avg_latency_ns() / 1000.0 << " μs\n";
    std::cout << "  Min:            " << stats.min_latency.count() << " ns\n";
    std::cout << "  Max:            " << stats.max_latency.count() << " ns\n";
    
    if (stats.min_latency != nanoseconds::max()) {
        std::cout << "  Min:            " << stats.min_latency.count() / 1000.0 << " μs\n";
    }
    if (stats.max_latency != nanoseconds::zero()) {
        std::cout << "  Max:            " << stats.max_latency.count() / 1000.0 << " μs\n";
    }
    
    std::cout << "\n";
}

void generate_report(const std::vector<std::pair<std::string, BenchmarkStats>>& results) {
    std::ofstream report("benchmark_report.txt");
    
    report << "Perpetual Exchange - Benchmark Report\n";
    report << "=====================================\n\n";
    report << "Generated: " << std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() << "\n\n";
    
    report << std::fixed << std::setprecision(2);
    
    for (const auto& [name, stats] : results) {
        report << "Test: " << name << "\n";
        report << std::string(50, '-') << "\n";
        report << "Total Orders:      " << stats.total_orders << "\n";
        report << "Total Trades:      " << stats.total_trades << "\n";
        report << "Total Volume:      " << quantity_to_double(stats.total_volume) << "\n";
        report << "Total Errors:      " << stats.total_errors << "\n";
        report << "Total Time:        " 
               << duration_cast<milliseconds>(stats.total_time).count() << " ms\n";
        report << "Throughput:        " 
               << stats.throughput_ops_per_sec() / 1000.0 << " K orders/sec\n";
        report << "Avg Latency:       " << stats.avg_latency_ns() << " ns\n";
        report << "Avg Latency:       " << stats.avg_latency_ns() / 1000.0 << " μs\n";
        report << "Min Latency:       " << stats.min_latency.count() << " ns\n";
        report << "Max Latency:       " << stats.max_latency.count() << " ns\n";
        report << "\n";
    }
    
    report.close();
    std::cout << "\nBenchmark report saved to: benchmark_report.txt\n";
}

int main(int argc, char* argv[]) {
    std::cout << "Perpetual Exchange - Performance Benchmark\n";
    std::cout << "==========================================\n\n";
    
    InstrumentID instrument_id = 1;
    std::vector<std::pair<std::string, BenchmarkStats>> results;
    
    // Test 1: Small scale test
    std::cout << "Running Test 1: Small Scale (1K orders)...\n";
    BenchmarkRunner runner1(instrument_id, 100);
    auto stats1 = runner1.run_benchmark(1000, 0.5);
    print_stats(stats1, "Small Scale (1K orders)");
    results.push_back({"Small Scale (1K orders)", stats1});
    
    // Test 2: Medium scale test
    std::cout << "\nRunning Test 2: Medium Scale (10K orders)...\n";
    BenchmarkRunner runner2(instrument_id, 500);
    auto stats2 = runner2.run_benchmark(10000, 0.5);
    print_stats(stats2, "Medium Scale (10K orders)");
    results.push_back({"Medium Scale (10K orders)", stats2});
    
    // Test 3: Large scale test
    std::cout << "\nRunning Test 3: Large Scale (100K orders)...\n";
    BenchmarkRunner runner3(instrument_id, 1000);
    auto stats3 = runner3.run_benchmark(100000, 0.5);
    print_stats(stats3, "Large Scale (100K orders)");
    results.push_back({"Large Scale (100K orders)", stats3});
    
    // Test 4: Throughput test (single thread)
    std::cout << "\nRunning Test 4: Throughput Test (Single Thread, 50K orders)...\n";
    BenchmarkRunner runner4(instrument_id, 1000);
    auto stats4 = runner4.run_throughput_test(50000, 1);
    print_stats(stats4, "Throughput Test (Single Thread)");
    results.push_back({"Throughput Test (Single Thread)", stats4});
    
    // Test 5: Throughput test (multi-thread)
    std::cout << "\nRunning Test 5: Throughput Test (4 Threads, 50K orders)...\n";
    BenchmarkRunner runner5(instrument_id, 1000);
    auto stats5 = runner5.run_throughput_test(50000, 4);
    print_stats(stats5, "Throughput Test (4 Threads)");
    results.push_back({"Throughput Test (4 Threads)", stats5});
    
    // Generate report
    generate_report(results);
    
    // Print summary
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "Summary\n";
    std::cout << std::string(60, '=') << "\n";
    std::cout << "Best Throughput: " 
              << std::max_element(results.begin(), results.end(),
                  [](const auto& a, const auto& b) {
                      return a.second.throughput_ops_per_sec() < 
                             b.second.throughput_ops_per_sec();
                  })->second.throughput_ops_per_sec() / 1000.0 
              << " K orders/sec\n";
    
    auto min_latency_it = std::min_element(results.begin(), results.end(),
        [](const auto& a, const auto& b) {
            return a.second.avg_latency_ns() < b.second.avg_latency_ns();
        });
    std::cout << "Best Avg Latency: " << min_latency_it->second.avg_latency_ns() 
              << " ns (" << min_latency_it->second.avg_latency_ns() / 1000.0 << " μs)\n";
    std::cout << "  Test: " << min_latency_it->first << "\n";
    
    return 0;
}
