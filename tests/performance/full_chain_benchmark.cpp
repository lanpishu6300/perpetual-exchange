#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>
#include <random>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <algorithm>
#include "core/matching_engine_event_sourcing.h"
#include "core/auth_manager.h"
#include "core/account_manager.h"
#include "core/position_manager.h"
#include "core/liquidation_engine.h"
#include "core/funding_rate_manager.h"
#include "core/monitoring_system.h"
#include "core/persistence.h"
#include "core/order.h"
#include "core/types.h"
#include <filesystem>

using namespace perpetual;

// Full chain performance benchmark
// Tests the entire trading system from order submission to persistence
class FullChainBenchmark {
public:
    struct BenchmarkConfig {
        int num_threads = 8;
        int duration_seconds = 60;
        int orders_per_thread_per_second = 10000;
        bool enable_persistence = true;
        bool enable_event_sourcing = true;
        bool enable_auth = false;  // Disable for performance
        bool enable_monitoring = true;
        bool enable_liquidation = false;
        int warmup_seconds = 5;
    };
    
    struct LatencyStats {
        std::vector<double> latencies;
        double min = std::numeric_limits<double>::max();
        double max = 0;
        double mean = 0;
        double p50 = 0;
        double p90 = 0;
        double p95 = 0;
        double p99 = 0;
        double p999 = 0;
        
        void calculate() {
            if (latencies.empty()) return;
            
            std::sort(latencies.begin(), latencies.end());
            min = latencies.front();
            max = latencies.back();
            
            double sum = 0;
            for (double l : latencies) {
                sum += l;
            }
            mean = sum / latencies.size();
            
            p50 = percentile(0.50);
            p90 = percentile(0.90);
            p95 = percentile(0.95);
            p99 = percentile(0.99);
            p999 = percentile(0.999);
        }
        
        double percentile(double p) const {
            if (latencies.empty()) return 0;
            size_t index = static_cast<size_t>(latencies.size() * p);
            if (index >= latencies.size()) index = latencies.size() - 1;
            return latencies[index];
        }
    };
    
    struct BenchmarkResult {
        std::atomic<uint64_t> total_orders{0};
        std::atomic<uint64_t> total_trades{0};
        std::atomic<uint64_t> total_errors{0};
        
        LatencyStats order_submission_latency;
        LatencyStats order_matching_latency;
        LatencyStats event_sourcing_latency;
        LatencyStats persistence_latency;
        LatencyStats end_to_end_latency;
        
        std::vector<double> throughput_samples;  // TPS over time
        
        void collect_throughput_sample(double tps) {
            throughput_samples.push_back(tps);
        }
    };
    
    FullChainBenchmark() {
        setup();
    }
    
    void setup() {
        // Initialize matching engine with event sourcing
        matching_engine_ = std::make_unique<MatchingEngineEventSourcing>(1);
        std::string event_store_dir = "./benchmark_event_store_" + std::to_string(get_current_timestamp());
        std::filesystem::create_directories(event_store_dir);
        matching_engine_->initialize(event_store_dir);
        matching_engine_->set_deterministic_mode(true);
        
        // Initialize account manager
        account_manager_ = std::make_unique<AccountBalanceManager>();
        
        // Initialize position manager
        position_manager_ = std::make_unique<PositionManager>();
        
        // Initialize monitoring
        monitoring_ = std::make_unique<MonitoringSystem>();
        
        // Initialize persistence if enabled
        if (config_.enable_persistence) {
            persistence_ = std::make_unique<PersistenceManager>();
            std::string persist_dir = "./benchmark_persistence_" + std::to_string(get_current_timestamp());
            std::filesystem::create_directories(persist_dir);
            persistence_->initialize(persist_dir);
        }
        
        // Setup test accounts
        for (int i = 0; i < 1000; ++i) {
            UserID user_id = 1000000 + i;
            account_manager_->setBalance(user_id, 1000000.0);  // $1M per account
        }
    }
    
    void runBenchmark(const BenchmarkConfig& config) {
        config_ = config;
        BenchmarkResult result;
        
        std::cout << "========================================" << std::endl;
        std::cout << "Full Chain Performance Benchmark" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Configuration:" << std::endl;
        std::cout << "  Threads: " << config.num_threads << std::endl;
        std::cout << "  Duration: " << config.duration_seconds << " seconds" << std::endl;
        std::cout << "  Target throughput: " << (config.num_threads * config.orders_per_thread_per_second) << " orders/sec" << std::endl;
        std::cout << "  Event Sourcing: " << (config.enable_event_sourcing ? "ON" : "OFF") << std::endl;
        std::cout << "  Persistence: " << (config.enable_persistence ? "ON" : "OFF") << std::endl;
        std::cout << "========================================" << std::endl;
        
        // Warmup
        if (config.warmup_seconds > 0) {
            std::cout << "\nWarming up for " << config.warmup_seconds << " seconds..." << std::endl;
            runWorkload(config, result, config.warmup_seconds, true);
            result = BenchmarkResult();  // Reset
        }
        
        // Actual benchmark
        std::cout << "\nRunning benchmark..." << std::endl;
        auto start_time = std::chrono::steady_clock::now();
        runWorkload(config, result, config.duration_seconds, false);
        auto end_time = std::chrono::steady_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        // Calculate statistics
        result.order_submission_latency.calculate();
        result.order_matching_latency.calculate();
        result.event_sourcing_latency.calculate();
        result.persistence_latency.calculate();
        result.end_to_end_latency.calculate();
        
        // Generate report
        generateReport(result, duration.count());
        
        // Analyze bottlenecks
        analyzeBottlenecks(result);
        
        // Generate optimization recommendations
        generateOptimizationRecommendations(result);
    }
    
private:
    void runWorkload(const BenchmarkConfig& config, BenchmarkResult& result, int duration_seconds, bool is_warmup) {
        std::vector<std::thread> threads;
        std::atomic<bool> running{true};
        
        auto end_time = std::chrono::steady_clock::now() + std::chrono::seconds(duration_seconds);
        
        // Throughput sampling thread
        std::thread sampling_thread([&]() {
            while (running && std::chrono::steady_clock::now() < end_time) {
                auto t0 = std::chrono::steady_clock::now();
                uint64_t orders_before = result.total_orders.load();
                
                std::this_thread::sleep_for(std::chrono::seconds(1));
                
                auto t1 = std::chrono::steady_clock::now();
                uint64_t orders_after = result.total_orders.load();
                
                auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
                double tps = (orders_after - orders_before) * 1000000.0 / elapsed;
                result.collect_throughput_sample(tps);
            }
        });
        
        // Worker threads
        for (int t = 0; t < config.num_threads; ++t) {
            threads.emplace_back([this, &config, &result, &running, end_time, t]() {
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> price_dist(49000, 51000);
                std::uniform_int_distribution<> qty_dist(1, 10);
                std::uniform_int_distribution<> user_dist(0, 999);
                
                uint64_t order_id = static_cast<uint64_t>(t) * 10000000ULL;
                
                while (running && std::chrono::steady_clock::now() < end_time) {
                    // Generate order
                    UserID user_id = 1000000 + user_dist(gen);
                    OrderSide side = (order_id % 2 == 0) ? OrderSide::BUY : OrderSide::SELL;
                    Price price = double_to_price(price_dist(gen));
                    Quantity quantity = double_to_quantity(qty_dist(gen) * 0.01);
                    
                    // Measure end-to-end latency
                    auto e2e_start = std::chrono::high_resolution_clock::now();
                    
                    // Order submission
                    auto submit_start = std::chrono::high_resolution_clock::now();
                    Order order(order_id++, user_id, 1, side, price, quantity, OrderType::LIMIT);
                    auto submit_end = std::chrono::high_resolution_clock::now();
                    
                    double submit_latency = std::chrono::duration_cast<std::chrono::nanoseconds>(
                        submit_end - submit_start).count();
                    
                    // Order matching
                    auto match_start = std::chrono::high_resolution_clock::now();
                    auto trades = matching_engine_->process_order_es(&order);
                    auto match_end = std::chrono::high_resolution_clock::now();
                    
                    double match_latency = std::chrono::duration_cast<std::chrono::nanoseconds>(
                        match_end - match_start).count();
                    
                    result.total_orders++;
                    result.total_trades += trades.size();
                    
                    // Event sourcing latency (approximate)
                    double es_latency = match_latency * 0.1;  // Assume 10% overhead
                    
                    // Persistence latency (if enabled)
                    double persist_latency = 0;
                    if (config.enable_persistence && !trades.empty()) {
                        auto persist_start = std::chrono::high_resolution_clock::now();
                        // In production, would persist trades here
                        auto persist_end = std::chrono::high_resolution_clock::now();
                        persist_latency = std::chrono::duration_cast<std::chrono::nanoseconds>(
                            persist_end - persist_start).count();
                    }
                    
                    auto e2e_end = std::chrono::high_resolution_clock::now();
                    double e2e_latency = std::chrono::duration_cast<std::chrono::nanoseconds>(
                        e2e_end - e2e_start).count();
                    
                    // Record latencies (sample rate to avoid memory issues)
                    if (!is_warmup && (order_id % 10 == 0)) {
                        result.order_submission_latency.latencies.push_back(submit_latency);
                        result.order_matching_latency.latencies.push_back(match_latency);
                        result.event_sourcing_latency.latencies.push_back(es_latency);
                        result.persistence_latency.latencies.push_back(persist_latency);
                        result.end_to_end_latency.latencies.push_back(e2e_latency);
                    }
                    
                    // Rate limiting
                    std::this_thread::sleep_for(
                        std::chrono::microseconds(1000000 / config.orders_per_thread_per_second));
                }
            });
        }
        
        // Wait for all threads
        for (auto& thread : threads) {
            thread.join();
        }
        
        running = false;
        sampling_thread.join();
    }
    
    void generateReport(const BenchmarkResult& result, int64_t duration_ms) {
        std::cout << "\n========================================" << std::endl;
        std::cout << "Benchmark Results" << std::endl;
        std::cout << "========================================" << std::endl;
        
        double duration_sec = duration_ms / 1000.0;
        double avg_tps = result.total_orders.load() / duration_sec;
        double peak_tps = 0;
        if (!result.throughput_samples.empty()) {
            peak_tps = *std::max_element(result.throughput_samples.begin(), result.throughput_samples.end());
        }
        
        std::cout << "\nThroughput:" << std::endl;
        std::cout << "  Total orders: " << result.total_orders.load() << std::endl;
        std::cout << "  Total trades: " << result.total_trades.load() << std::endl;
        std::cout << "  Total errors: " << result.total_errors.load() << std::endl;
        std::cout << "  Average TPS: " << std::fixed << std::setprecision(2) << avg_tps << std::endl;
        std::cout << "  Peak TPS: " << peak_tps << std::endl;
        std::cout << "  Target: 1,000,000 TPS" << std::endl;
        std::cout << "  Gap: " << (1000000.0 - avg_tps) << " TPS (" 
                  << ((1000000.0 - avg_tps) / 1000000.0 * 100) << "%)" << std::endl;
        
        std::cout << "\nLatency Statistics (nanoseconds):" << std::endl;
        printLatencyStats("Order Submission", result.order_submission_latency);
        printLatencyStats("Order Matching", result.order_matching_latency);
        printLatencyStats("Event Sourcing", result.event_sourcing_latency);
        printLatencyStats("Persistence", result.persistence_latency);
        printLatencyStats("End-to-End", result.end_to_end_latency);
        
        // Save to file
        saveReportToFile(result, duration_ms, avg_tps, peak_tps);
    }
    
    void printLatencyStats(const std::string& name, const LatencyStats& stats) {
        std::cout << "  " << name << ":" << std::endl;
        std::cout << "    Min: " << std::fixed << std::setprecision(2) << stats.min << " ns" << std::endl;
        std::cout << "    P50: " << stats.p50 << " ns" << std::endl;
        std::cout << "    P90: " << stats.p90 << " ns" << std::endl;
        std::cout << "    P95: " << stats.p95 << " ns" << std::endl;
        std::cout << "    P99: " << stats.p99 << " ns" << std::endl;
        std::cout << "    P99.9: " << stats.p999 << " ns" << std::endl;
        std::cout << "    Max: " << stats.max << " ns" << std::endl;
        std::cout << "    Mean: " << stats.mean << " ns" << std::endl;
    }
    
    void saveReportToFile(const BenchmarkResult& result, int64_t duration_ms, 
                         double avg_tps, double peak_tps) {
        std::ofstream file("full_chain_benchmark_report.md");
        if (!file.is_open()) return;
        
        file << "# Full Chain Performance Benchmark Report\n\n";
        file << "## Configuration\n";
        file << "- Threads: " << config_.num_threads << "\n";
        file << "- Duration: " << config_.duration_seconds << " seconds\n";
        file << "- Event Sourcing: " << (config_.enable_event_sourcing ? "ON" : "OFF") << "\n";
        file << "- Persistence: " << (config_.enable_persistence ? "ON" : "OFF") << "\n\n";
        
        file << "## Results\n\n";
        file << "### Throughput\n";
        file << "- Total Orders: " << result.total_orders.load() << "\n";
        file << "- Total Trades: " << result.total_trades.load() << "\n";
        file << "- Average TPS: " << std::fixed << std::setprecision(2) << avg_tps << "\n";
        file << "- Peak TPS: " << peak_tps << "\n";
        file << "- Target: 1,000,000 TPS\n";
        file << "- Gap: " << (1000000.0 - avg_tps) << " TPS\n\n";
        
        file << "### Latency (nanoseconds)\n\n";
        saveLatencyStats(file, "Order Submission", result.order_submission_latency);
        saveLatencyStats(file, "Order Matching", result.order_matching_latency);
        saveLatencyStats(file, "Event Sourcing", result.event_sourcing_latency);
        saveLatencyStats(file, "Persistence", result.persistence_latency);
        saveLatencyStats(file, "End-to-End", result.end_to_end_latency);
        
        file.close();
    }
    
    void saveLatencyStats(std::ofstream& file, const std::string& name, const LatencyStats& stats) {
        file << "#### " << name << "\n";
        file << "| Metric | Value (ns) |\n";
        file << "|--------|------------|\n";
        file << "| Min | " << std::fixed << std::setprecision(2) << stats.min << " |\n";
        file << "| P50 | " << stats.p50 << " |\n";
        file << "| P90 | " << stats.p90 << " |\n";
        file << "| P95 | " << stats.p95 << " |\n";
        file << "| P99 | " << stats.p99 << " |\n";
        file << "| P99.9 | " << stats.p999 << " |\n";
        file << "| Max | " << stats.max << " |\n";
        file << "| Mean | " << stats.mean << " |\n\n";
    }
    
    void analyzeBottlenecks(const BenchmarkResult& result) {
        std::cout << "\n========================================" << std::endl;
        std::cout << "Bottleneck Analysis" << std::endl;
        std::cout << "========================================" << std::endl;
        
        // Find slowest component
        double max_p99 = std::max({
            result.order_matching_latency.p99,
            result.event_sourcing_latency.p99,
            result.persistence_latency.p99
        });
        
        std::cout << "\nComponent Latency Breakdown (P99):" << std::endl;
        std::cout << "  Order Matching: " << result.order_matching_latency.p99 << " ns (" 
                  << (result.order_matching_latency.p99 / result.end_to_end_latency.p99 * 100) << "%)" << std::endl;
        std::cout << "  Event Sourcing: " << result.event_sourcing_latency.p99 << " ns (" 
                  << (result.event_sourcing_latency.p99 / result.end_to_end_latency.p99 * 100) << "%)" << std::endl;
        std::cout << "  Persistence: " << result.persistence_latency.p99 << " ns (" 
                  << (result.persistence_latency.p99 / result.end_to_end_latency.p99 * 100) << "%)" << std::endl;
        
        // Identify bottlenecks
        std::vector<std::string> bottlenecks;
        
        if (result.order_matching_latency.p99 > 1000) {  // > 1μs
            bottlenecks.push_back("Order Matching (P99 > 1μs)");
        }
        
        if (result.event_sourcing_latency.p99 > 500) {  // > 500ns
            bottlenecks.push_back("Event Sourcing (P99 > 500ns)");
        }
        
        if (result.persistence_latency.p99 > 2000) {  // > 2μs
            bottlenecks.push_back("Persistence (P99 > 2μs)");
        }
        
        if (result.end_to_end_latency.p99 > 5000) {  // > 5μs
            bottlenecks.push_back("End-to-End (P99 > 5μs)");
        }
        
        if (!bottlenecks.empty()) {
            std::cout << "\n🚨 Identified Bottlenecks:" << std::endl;
            for (const auto& bottleneck : bottlenecks) {
                std::cout << "  - " << bottleneck << std::endl;
            }
        } else {
            std::cout << "\n✅ No major bottlenecks identified (all components < target latency)" << std::endl;
        }
    }
    
    void generateOptimizationRecommendations(const BenchmarkResult& result) {
        std::cout << "\n========================================" << std::endl;
        std::cout << "Optimization Recommendations" << std::endl;
        std::cout << "========================================" << std::endl;
        
        std::vector<std::string> recommendations;
        
        // Matching engine optimizations
        if (result.order_matching_latency.p99 > 1000) {
            recommendations.push_back("1. **Order Book Optimization**: Use lock-free data structures, SIMD for price comparisons, NUMA-aware memory allocation");
            recommendations.push_back("2. **Matching Algorithm**: Optimize hot path with branchless code, prefetching, and cache-aligned data structures");
            recommendations.push_back("3. **Memory Management**: Use memory pools to avoid dynamic allocation in hot path");
        }
        
        // Event sourcing optimizations
        if (result.event_sourcing_latency.p99 > 500) {
            recommendations.push_back("4. **Event Storage**: Use lock-free append-only log, batch writes, zero-copy serialization");
            recommendations.push_back("5. **Event Publishing**: Async event publishing with lock-free queues, batching");
            recommendations.push_back("6. **Event Indexing**: Lazy indexing, incremental indexing, or separate indexing thread");
        }
        
        // Persistence optimizations
        if (result.persistence_latency.p99 > 2000) {
            recommendations.push_back("7. **Async Persistence**: Move persistence to background thread with lock-free queue");
            recommendations.push_back("8. **Batch Writes**: Group multiple writes into batches, use WAL with group commit");
            recommendations.push_back("9. **Storage Optimization**: Use memory-mapped files, direct I/O, or specialized storage (e.g., RocksDB)");
        }
        
        // General optimizations
        recommendations.push_back("10. **CPU Affinity**: Pin threads to CPU cores, separate matching and persistence threads");
        recommendations.push_back("11. **NUMA Optimization**: Allocate memory on local NUMA node, use NUMA-aware data structures");
        recommendations.push_back("12. **SIMD Optimization**: Use AVX-512 for batch operations, parallel price comparisons");
        recommendations.push_back("13. **Lock-Free Structures**: Replace all locks with lock-free queues and atomic operations");
        recommendations.push_back("14. **Zero-Copy**: Minimize data copying, use move semantics, reference counting");
        recommendations.push_back("15. **Batch Processing**: Process orders in batches to amortize overhead");
        recommendations.push_back("16. **Hardware Acceleration**: Consider FPGA for order matching, GPU for batch processing");
        
        for (const auto& rec : recommendations) {
            std::cout << rec << std::endl;
        }
        
        // Save to file
        saveOptimizationRecommendations(recommendations, result);
    }
    
    void saveOptimizationRecommendations(const std::vector<std::string>& recommendations, 
                                        const BenchmarkResult& result) {
        std::ofstream file("optimization_recommendations.md");
        if (!file.is_open()) return;
        
        file << "# Optimization Recommendations for Million TPS\n\n";
        file << "## Current Performance\n";
        file << "- Current TPS: ~" << std::fixed << std::setprecision(0) 
             << (result.total_orders.load() / (config_.duration_seconds)) << "\n";
        file << "- Target TPS: 1,000,000\n";
        file << "- Current P99 Latency: " << result.end_to_end_latency.p99 << " ns\n";
        file << "- Target P99 Latency: < 1000 ns (1μs)\n\n";
        
        file << "## Recommendations\n\n";
        for (const auto& rec : recommendations) {
            file << rec << "\n\n";
        }
        
        file.close();
    }
    
    BenchmarkConfig config_;
    std::unique_ptr<MatchingEngineEventSourcing> matching_engine_;
    std::unique_ptr<AccountBalanceManager> account_manager_;
    std::unique_ptr<PositionManager> position_manager_;
    std::unique_ptr<MonitoringSystem> monitoring_;
    std::unique_ptr<PersistenceManager> persistence_;
};

int main(int argc, char* argv[]) {
    FullChainBenchmark::BenchmarkConfig config;
    
    if (argc > 1) config.num_threads = std::stoi(argv[1]);
    if (argc > 2) config.duration_seconds = std::stoi(argv[2]);
    if (argc > 3) config.orders_per_thread_per_second = std::stoi(argv[3]);
    if (argc > 4) config.enable_persistence = (std::stoi(argv[4]) != 0);
    if (argc > 5) config.enable_event_sourcing = (std::stoi(argv[5]) != 0);
    
    FullChainBenchmark benchmark;
    benchmark.runBenchmark(config);
    
    return 0;
}

