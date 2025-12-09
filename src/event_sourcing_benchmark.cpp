#include "core/matching_engine_event_sourcing.h"
#include "core/event_sourcing_advanced.h"
#include "core/deterministic_calculator.h"
#include "core/types.h"
#include "core/order.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <iomanip>
#include <thread>
#include <atomic>

using namespace perpetual;
using namespace std::chrono;

struct BenchmarkResult {
    std::string name;
    size_t operations;
    nanoseconds total_time;
    nanoseconds avg_latency;
    nanoseconds min_latency;
    nanoseconds max_latency;
    nanoseconds p50_latency;
    nanoseconds p90_latency;
    nanoseconds p99_latency;
    double throughput;  // ops/sec
};

// Generate random orders for testing
std::vector<std::unique_ptr<Order>> generate_orders(size_t count, InstrumentID instrument_id) {
    std::vector<std::unique_ptr<Order>> orders;
    orders.reserve(count);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<UserID> user_dist(1000, 9999);
    std::uniform_int_distribution<Price> price_dist(double_to_price(49000.0), double_to_price(51000.0));
    std::uniform_int_distribution<Quantity> qty_dist(double_to_quantity(0.1), double_to_quantity(10.0));
    std::uniform_int_distribution<int> side_dist(0, 1);
    
    for (size_t i = 0; i < count; ++i) {
        OrderID order_id = 10000 + i;
        UserID user_id = user_dist(gen);
        Price price = price_dist(gen);
        Quantity quantity = qty_dist(gen);
        OrderSide side = (side_dist(gen) == 0) ? OrderSide::BUY : OrderSide::SELL;
        
        orders.push_back(std::make_unique<Order>(order_id, user_id, instrument_id, side, price, quantity));
    }
    
    return orders;
}

// Calculate percentile
nanoseconds percentile(const std::vector<nanoseconds>& latencies, double p) {
    if (latencies.empty()) return nanoseconds(0);
    
    std::vector<nanoseconds> sorted = latencies;
    std::sort(sorted.begin(), sorted.end());
    
    size_t index = static_cast<size_t>(sorted.size() * p);
    if (index >= sorted.size()) index = sorted.size() - 1;
    
    return sorted[index];
}

BenchmarkResult calculate_results(const std::string& name, 
                                 const std::vector<nanoseconds>& latencies,
                                 nanoseconds total_time) {
    BenchmarkResult result;
    result.name = name;
    result.operations = latencies.size();
    result.total_time = total_time;
    
    if (latencies.empty()) {
        return result;
    }
    
    // Calculate statistics
    nanoseconds sum(0);
    result.min_latency = latencies[0];
    result.max_latency = latencies[0];
    
    for (const auto& lat : latencies) {
        sum += lat;
        if (lat < result.min_latency) result.min_latency = lat;
        if (lat > result.max_latency) result.max_latency = lat;
    }
    
    result.avg_latency = nanoseconds(sum.count() / latencies.size());
    result.p50_latency = percentile(latencies, 0.50);
    result.p90_latency = percentile(latencies, 0.90);
    result.p99_latency = percentile(latencies, 0.99);
    
    // Calculate throughput
    double total_seconds = duration_cast<microseconds>(total_time).count() / 1000000.0;
    result.throughput = (total_seconds > 0) ? (latencies.size() / total_seconds) : 0;
    
    return result;
}

void print_result(const BenchmarkResult& result) {
    std::cout << "\n=== " << result.name << " ===" << std::endl;
    std::cout << "Operations: " << result.operations << std::endl;
    std::cout << "Total Time: " << duration_cast<milliseconds>(result.total_time).count() << " ms" << std::endl;
    std::cout << "Throughput: " << std::fixed << std::setprecision(2) << result.throughput << " ops/sec" << std::endl;
    std::cout << "\nLatency Statistics:" << std::endl;
    std::cout << "  Average: " << duration_cast<nanoseconds>(result.avg_latency).count() << " ns" << std::endl;
    std::cout << "  Min:     " << duration_cast<nanoseconds>(result.min_latency).count() << " ns" << std::endl;
    std::cout << "  P50:     " << duration_cast<nanoseconds>(result.p50_latency).count() << " ns" << std::endl;
    std::cout << "  P90:     " << duration_cast<nanoseconds>(result.p90_latency).count() << " ns" << std::endl;
    std::cout << "  P99:     " << duration_cast<nanoseconds>(result.p99_latency).count() << " ns" << std::endl;
    std::cout << "  Max:     " << duration_cast<nanoseconds>(result.max_latency).count() << " ns" << std::endl;
}

// Benchmark 1: Event Sourcing Basic Operations
BenchmarkResult benchmark_event_sourcing_basic(size_t num_orders) {
    std::cout << "\n[Benchmark 1] Event Sourcing Basic Operations (" << num_orders << " orders)..." << std::endl;
    
    MatchingEngineEventSourcing engine(1);
    engine.initialize("./benchmark_data");
    engine.set_deterministic_mode(true);
    
    auto orders = generate_orders(num_orders, 1);
    
    std::vector<nanoseconds> latencies;
    latencies.reserve(num_orders);
    
    auto start = high_resolution_clock::now();
    
    for (size_t i = 0; i < num_orders; ++i) {
        auto order_start = high_resolution_clock::now();
        engine.process_order_es(orders[i].release());
        auto order_end = high_resolution_clock::now();
        
        latencies.push_back(duration_cast<nanoseconds>(order_end - order_start));
    }
    
    auto end = high_resolution_clock::now();
    auto total_time = duration_cast<nanoseconds>(end - start);
    
    return calculate_results("Event Sourcing Basic", latencies, total_time);
}

// Benchmark 2: Deterministic Calculation Performance
BenchmarkResult benchmark_deterministic_calculation(size_t iterations) {
    std::cout << "\n[Benchmark 2] Deterministic Calculation (" << iterations << " iterations)..." << std::endl;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<Price> price_dist(double_to_price(49000.0), double_to_price(51000.0));
    std::uniform_int_distribution<Quantity> qty_dist(double_to_quantity(0.1), double_to_quantity(10.0));
    
    std::vector<nanoseconds> latencies;
    latencies.reserve(iterations);
    
    auto start = high_resolution_clock::now();
    
    for (size_t i = 0; i < iterations; ++i) {
        auto calc_start = high_resolution_clock::now();
        
        Price price1 = price_dist(gen);
        Price price2 = price_dist(gen);
        Quantity qty1 = qty_dist(gen);
        Quantity qty2 = qty_dist(gen);
        
        // Test various deterministic calculations
        DeterministicCalculator::can_match(price1, price2, true);
        DeterministicCalculator::calculate_match_price(price1, price2);
        DeterministicCalculator::calculate_trade_quantity(qty1, qty2);
        DeterministicCalculator::calculate_pnl(price1, price2, qty1, true);
        DeterministicCalculator::calculate_margin(price1, qty1, 1000);  // 10% margin
        DeterministicCalculator::sequence_to_timestamp(i);
        
        auto calc_end = high_resolution_clock::now();
        latencies.push_back(duration_cast<nanoseconds>(calc_end - calc_start));
    }
    
    auto end = high_resolution_clock::now();
    auto total_time = duration_cast<nanoseconds>(end - start);
    
    return calculate_results("Deterministic Calculation", latencies, total_time);
}

// Benchmark 3: Event Store Read/Write Performance
BenchmarkResult benchmark_event_store_io(size_t num_events) {
    std::cout << "\n[Benchmark 3] Event Store I/O (" << num_events << " events)..." << std::endl;
    
    EventStore store;
    store.initialize("./benchmark_data_io");
    
    // Generate events
    std::vector<Event> events;
    events.reserve(num_events);
    
    for (size_t i = 0; i < num_events; ++i) {
        Event event;
        event.type = EventType::ORDER_PLACED;
        event.sequence_id = i + 1;
        event.instrument_id = 1;
        event.data.order_placed.order_id = 1000 + i;
        event.data.order_placed.user_id = 2000 + i;
        event.data.order_placed.side = OrderSide::BUY;
        event.data.order_placed.order_type = OrderType::LIMIT;
        event.data.order_placed.price = double_to_price(50000.0);
        event.data.order_placed.quantity = double_to_quantity(1.0);
        events.push_back(event);
    }
    
    // Write benchmark
    std::vector<nanoseconds> write_latencies;
    write_latencies.reserve(num_events);
    
    auto write_start = high_resolution_clock::now();
    for (const auto& event : events) {
        auto w_start = high_resolution_clock::now();
        store.append_event(event);
        auto w_end = high_resolution_clock::now();
        write_latencies.push_back(duration_cast<nanoseconds>(w_end - w_start));
    }
    auto write_end = high_resolution_clock::now();
    auto write_total = duration_cast<nanoseconds>(write_end - write_start);
    
    // Read benchmark
    std::vector<nanoseconds> read_latencies;
    read_latencies.reserve(100);  // Read 100 times
    
    auto read_start = high_resolution_clock::now();
    for (size_t i = 0; i < 100; ++i) {
        auto r_start = high_resolution_clock::now();
        auto read_events = store.get_events(1, num_events);
        auto r_end = high_resolution_clock::now();
        read_latencies.push_back(duration_cast<nanoseconds>(r_end - r_start));
    }
    auto read_end = high_resolution_clock::now();
    auto read_total = duration_cast<nanoseconds>(read_end - read_start);
    
    std::cout << "\n--- Write Performance ---" << std::endl;
    auto write_result = calculate_results("Event Store Write", write_latencies, write_total);
    print_result(write_result);
    
    std::cout << "\n--- Read Performance ---" << std::endl;
    auto read_result = calculate_results("Event Store Read", read_latencies, read_total);
    print_result(read_result);
    
    return write_result;  // Return write result as primary
}

// Benchmark 4: Event Stream Processing
BenchmarkResult benchmark_event_stream_processing(size_t num_events) {
    std::cout << "\n[Benchmark 4] Event Stream Processing (" << num_events << " events)..." << std::endl;
    
    EventStore store;
    store.initialize("./benchmark_data_stream");
    
    EventStreamProcessor processor(&store);
    
    std::atomic<size_t> events_processed(0);
    
    // Subscribe to events
    processor.subscribe([&events_processed](const Event& event) {
        events_processed++;
    });
    
    // Generate and append events
    for (size_t i = 0; i < num_events; ++i) {
        Event event;
        event.type = EventType::ORDER_PLACED;
        event.sequence_id = i + 1;
        event.instrument_id = 1;
        event.data.order_placed.order_id = 1000 + i;
        event.data.order_placed.user_id = 2000;
        event.data.order_placed.side = OrderSide::BUY;
        event.data.order_placed.order_type = OrderType::LIMIT;
        event.data.order_placed.price = double_to_price(50000.0);
        event.data.order_placed.quantity = double_to_quantity(1.0);
        store.append_event(event);
    }
    
    // Start processing
    auto start = high_resolution_clock::now();
    processor.start_processing();
    
    // Wait for all events to be processed
    while (events_processed.load() < num_events) {
        std::this_thread::sleep_for(milliseconds(10));
        processor.process_new_events();
    }
    
    auto end = high_resolution_clock::now();
    auto total_time = duration_cast<nanoseconds>(end - start);
    
    processor.stop_processing();
    
    BenchmarkResult result;
    result.name = "Event Stream Processing";
    result.operations = events_processed.load();
    result.total_time = total_time;
    double total_seconds = duration_cast<microseconds>(total_time).count() / 1000000.0;
    result.throughput = (total_seconds > 0) ? (events_processed.load() / total_seconds) : 0;
    
    return result;
}

// Benchmark 5: CQRS Performance
BenchmarkResult benchmark_cqrs(size_t num_commands) {
    std::cout << "\n[Benchmark 5] CQRS Performance (" << num_commands << " commands)..." << std::endl;
    
    MatchingEngineEventSourcing engine(1);
    EventStore store;
    store.initialize("./benchmark_data_cqrs");
    engine.initialize("./benchmark_data_cqrs");
    
    CQRSManager cqrs(&engine, &store);
    cqrs.start_cache_update();
    
    std::vector<nanoseconds> latencies;
    latencies.reserve(num_commands);
    
    auto start = high_resolution_clock::now();
    
    for (size_t i = 0; i < num_commands; ++i) {
        Command cmd;
        cmd.type = Command::PLACE_ORDER;
        cmd.order_id = 1000 + i;
        cmd.user_id = 2000;
        cmd.instrument_id = 1;
        cmd.side = (i % 2 == 0) ? OrderSide::BUY : OrderSide::SELL;
        cmd.price = double_to_price(50000.0);
        cmd.quantity = double_to_quantity(1.0);
        cmd.order_type = OrderType::LIMIT;
        
        auto cmd_start = high_resolution_clock::now();
        cqrs.execute_command(cmd);
        auto cmd_end = high_resolution_clock::now();
        
        latencies.push_back(duration_cast<nanoseconds>(cmd_end - cmd_start));
    }
    
    auto end = high_resolution_clock::now();
    auto total_time = duration_cast<nanoseconds>(end - start);
    
    cqrs.stop_cache_update();
    
    return calculate_results("CQRS Command Execution", latencies, total_time);
}

// Benchmark 6: Event Compression
BenchmarkResult benchmark_event_compression(size_t num_events) {
    std::cout << "\n[Benchmark 6] Event Compression (" << num_events << " events)..." << std::endl;
    
    EventStore store;
    store.initialize("./benchmark_data_compression");
    
    // Generate events
    for (size_t i = 0; i < num_events; ++i) {
        Event event;
        event.type = EventType::ORDER_PLACED;
        event.sequence_id = i + 1;
        event.instrument_id = 1;
        event.data.order_placed.order_id = 1000 + i;
        event.data.order_placed.user_id = 2000;
        event.data.order_placed.side = OrderSide::BUY;
        event.data.order_placed.order_type = OrderType::LIMIT;
        event.data.order_placed.price = double_to_price(50000.0);
        event.data.order_placed.quantity = double_to_quantity(1.0);
        store.append_event(event);
    }
    
    EventCompressor compressor(&store);
    compressor.set_strategy(CompressionStrategy::SNAPSHOT_ONLY);
    
    auto start = high_resolution_clock::now();
    size_t compressed = compressor.compress_events(num_events / 2);
    auto end = high_resolution_clock::now();
    auto total_time = duration_cast<nanoseconds>(end - start);
    
    BenchmarkResult result;
    result.name = "Event Compression";
    result.operations = compressed;
    result.total_time = total_time;
    double total_seconds = duration_cast<microseconds>(total_time).count() / 1000000.0;
    result.throughput = (total_seconds > 0) ? (compressed / total_seconds) : 0;
    
    return result;
}

int main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "Event Sourcing Performance Benchmark" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Parse command line arguments
    size_t num_orders = 10000;
    if (argc > 1) {
        num_orders = std::stoul(argv[1]);
    }
    
    std::cout << "\nConfiguration:" << std::endl;
    std::cout << "  Orders/Events: " << num_orders << std::endl;
    std::cout << "  Test Directory: ./benchmark_data*" << std::endl;
    
    std::vector<BenchmarkResult> results;
    
    // Run benchmarks
    try {
        results.push_back(benchmark_event_sourcing_basic(num_orders));
        results.push_back(benchmark_deterministic_calculation(num_orders * 10));
        results.push_back(benchmark_event_store_io(num_orders));
        results.push_back(benchmark_event_stream_processing(num_orders));
        results.push_back(benchmark_cqrs(num_orders));
        results.push_back(benchmark_event_compression(num_orders));
    } catch (const std::exception& e) {
        std::cerr << "Error during benchmark: " << e.what() << std::endl;
        return 1;
    }
    
    // Print summary
    std::cout << "\n\n========================================" << std::endl;
    std::cout << "Performance Summary" << std::endl;
    std::cout << "========================================" << std::endl;
    
    for (const auto& result : results) {
        print_result(result);
    }
    
    // Print comparison table
    std::cout << "\n\n========================================" << std::endl;
    std::cout << "Throughput Comparison (ops/sec)" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::left << std::setw(40) << "Benchmark" 
              << std::right << std::setw(15) << "Throughput" << std::endl;
    std::cout << std::string(55, '-') << std::endl;
    
    for (const auto& result : results) {
        std::cout << std::left << std::setw(40) << result.name
                  << std::right << std::setw(15) << std::fixed << std::setprecision(2) 
                  << result.throughput << std::endl;
    }
    
    std::cout << "\nBenchmark completed!" << std::endl;
    
    return 0;
}

#include "core/event_sourcing_advanced.h"
#include "core/deterministic_calculator.h"
#include "core/types.h"
#include "core/order.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <iomanip>
#include <thread>
#include <atomic>

using namespace perpetual;
using namespace std::chrono;

struct BenchmarkResult {
    std::string name;
    size_t operations;
    nanoseconds total_time;
    nanoseconds avg_latency;
    nanoseconds min_latency;
    nanoseconds max_latency;
    nanoseconds p50_latency;
    nanoseconds p90_latency;
    nanoseconds p99_latency;
    double throughput;  // ops/sec
};

// Generate random orders for testing
std::vector<std::unique_ptr<Order>> generate_orders(size_t count, InstrumentID instrument_id) {
    std::vector<std::unique_ptr<Order>> orders;
    orders.reserve(count);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<UserID> user_dist(1000, 9999);
    std::uniform_int_distribution<Price> price_dist(double_to_price(49000.0), double_to_price(51000.0));
    std::uniform_int_distribution<Quantity> qty_dist(double_to_quantity(0.1), double_to_quantity(10.0));
    std::uniform_int_distribution<int> side_dist(0, 1);
    
    for (size_t i = 0; i < count; ++i) {
        OrderID order_id = 10000 + i;
        UserID user_id = user_dist(gen);
        Price price = price_dist(gen);
        Quantity quantity = qty_dist(gen);
        OrderSide side = (side_dist(gen) == 0) ? OrderSide::BUY : OrderSide::SELL;
        
        orders.push_back(std::make_unique<Order>(order_id, user_id, instrument_id, side, price, quantity));
    }
    
    return orders;
}

// Calculate percentile
nanoseconds percentile(const std::vector<nanoseconds>& latencies, double p) {
    if (latencies.empty()) return nanoseconds(0);
    
    std::vector<nanoseconds> sorted = latencies;
    std::sort(sorted.begin(), sorted.end());
    
    size_t index = static_cast<size_t>(sorted.size() * p);
    if (index >= sorted.size()) index = sorted.size() - 1;
    
    return sorted[index];
}

BenchmarkResult calculate_results(const std::string& name, 
                                 const std::vector<nanoseconds>& latencies,
                                 nanoseconds total_time) {
    BenchmarkResult result;
    result.name = name;
    result.operations = latencies.size();
    result.total_time = total_time;
    
    if (latencies.empty()) {
        return result;
    }
    
    // Calculate statistics
    nanoseconds sum(0);
    result.min_latency = latencies[0];
    result.max_latency = latencies[0];
    
    for (const auto& lat : latencies) {
        sum += lat;
        if (lat < result.min_latency) result.min_latency = lat;
        if (lat > result.max_latency) result.max_latency = lat;
    }
    
    result.avg_latency = nanoseconds(sum.count() / latencies.size());
    result.p50_latency = percentile(latencies, 0.50);
    result.p90_latency = percentile(latencies, 0.90);
    result.p99_latency = percentile(latencies, 0.99);
    
    // Calculate throughput
    double total_seconds = duration_cast<microseconds>(total_time).count() / 1000000.0;
    result.throughput = (total_seconds > 0) ? (latencies.size() / total_seconds) : 0;
    
    return result;
}

void print_result(const BenchmarkResult& result) {
    std::cout << "\n=== " << result.name << " ===" << std::endl;
    std::cout << "Operations: " << result.operations << std::endl;
    std::cout << "Total Time: " << duration_cast<milliseconds>(result.total_time).count() << " ms" << std::endl;
    std::cout << "Throughput: " << std::fixed << std::setprecision(2) << result.throughput << " ops/sec" << std::endl;
    std::cout << "\nLatency Statistics:" << std::endl;
    std::cout << "  Average: " << duration_cast<nanoseconds>(result.avg_latency).count() << " ns" << std::endl;
    std::cout << "  Min:     " << duration_cast<nanoseconds>(result.min_latency).count() << " ns" << std::endl;
    std::cout << "  P50:     " << duration_cast<nanoseconds>(result.p50_latency).count() << " ns" << std::endl;
    std::cout << "  P90:     " << duration_cast<nanoseconds>(result.p90_latency).count() << " ns" << std::endl;
    std::cout << "  P99:     " << duration_cast<nanoseconds>(result.p99_latency).count() << " ns" << std::endl;
    std::cout << "  Max:     " << duration_cast<nanoseconds>(result.max_latency).count() << " ns" << std::endl;
}

// Benchmark 1: Event Sourcing Basic Operations
BenchmarkResult benchmark_event_sourcing_basic(size_t num_orders) {
    std::cout << "\n[Benchmark 1] Event Sourcing Basic Operations (" << num_orders << " orders)..." << std::endl;
    
    MatchingEngineEventSourcing engine(1);
    engine.initialize("./benchmark_data");
    engine.set_deterministic_mode(true);
    
    auto orders = generate_orders(num_orders, 1);
    
    std::vector<nanoseconds> latencies;
    latencies.reserve(num_orders);
    
    auto start = high_resolution_clock::now();
    
    for (size_t i = 0; i < num_orders; ++i) {
        auto order_start = high_resolution_clock::now();
        engine.process_order_es(orders[i].release());
        auto order_end = high_resolution_clock::now();
        
        latencies.push_back(duration_cast<nanoseconds>(order_end - order_start));
    }
    
    auto end = high_resolution_clock::now();
    auto total_time = duration_cast<nanoseconds>(end - start);
    
    return calculate_results("Event Sourcing Basic", latencies, total_time);
}

// Benchmark 2: Deterministic Calculation Performance
BenchmarkResult benchmark_deterministic_calculation(size_t iterations) {
    std::cout << "\n[Benchmark 2] Deterministic Calculation (" << iterations << " iterations)..." << std::endl;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<Price> price_dist(double_to_price(49000.0), double_to_price(51000.0));
    std::uniform_int_distribution<Quantity> qty_dist(double_to_quantity(0.1), double_to_quantity(10.0));
    
    std::vector<nanoseconds> latencies;
    latencies.reserve(iterations);
    
    auto start = high_resolution_clock::now();
    
    for (size_t i = 0; i < iterations; ++i) {
        auto calc_start = high_resolution_clock::now();
        
        Price price1 = price_dist(gen);
        Price price2 = price_dist(gen);
        Quantity qty1 = qty_dist(gen);
        Quantity qty2 = qty_dist(gen);
        
        // Test various deterministic calculations
        DeterministicCalculator::can_match(price1, price2, true);
        DeterministicCalculator::calculate_match_price(price1, price2);
        DeterministicCalculator::calculate_trade_quantity(qty1, qty2);
        DeterministicCalculator::calculate_pnl(price1, price2, qty1, true);
        DeterministicCalculator::calculate_margin(price1, qty1, 1000);  // 10% margin
        DeterministicCalculator::sequence_to_timestamp(i);
        
        auto calc_end = high_resolution_clock::now();
        latencies.push_back(duration_cast<nanoseconds>(calc_end - calc_start));
    }
    
    auto end = high_resolution_clock::now();
    auto total_time = duration_cast<nanoseconds>(end - start);
    
    return calculate_results("Deterministic Calculation", latencies, total_time);
}

// Benchmark 3: Event Store Read/Write Performance
BenchmarkResult benchmark_event_store_io(size_t num_events) {
    std::cout << "\n[Benchmark 3] Event Store I/O (" << num_events << " events)..." << std::endl;
    
    EventStore store;
    store.initialize("./benchmark_data_io");
    
    // Generate events
    std::vector<Event> events;
    events.reserve(num_events);
    
    for (size_t i = 0; i < num_events; ++i) {
        Event event;
        event.type = EventType::ORDER_PLACED;
        event.sequence_id = i + 1;
        event.instrument_id = 1;
        event.data.order_placed.order_id = 1000 + i;
        event.data.order_placed.user_id = 2000 + i;
        event.data.order_placed.side = OrderSide::BUY;
        event.data.order_placed.order_type = OrderType::LIMIT;
        event.data.order_placed.price = double_to_price(50000.0);
        event.data.order_placed.quantity = double_to_quantity(1.0);
        events.push_back(event);
    }
    
    // Write benchmark
    std::vector<nanoseconds> write_latencies;
    write_latencies.reserve(num_events);
    
    auto write_start = high_resolution_clock::now();
    for (const auto& event : events) {
        auto w_start = high_resolution_clock::now();
        store.append_event(event);
        auto w_end = high_resolution_clock::now();
        write_latencies.push_back(duration_cast<nanoseconds>(w_end - w_start));
    }
    auto write_end = high_resolution_clock::now();
    auto write_total = duration_cast<nanoseconds>(write_end - write_start);
    
    // Read benchmark
    std::vector<nanoseconds> read_latencies;
    read_latencies.reserve(100);  // Read 100 times
    
    auto read_start = high_resolution_clock::now();
    for (size_t i = 0; i < 100; ++i) {
        auto r_start = high_resolution_clock::now();
        auto read_events = store.get_events(1, num_events);
        auto r_end = high_resolution_clock::now();
        read_latencies.push_back(duration_cast<nanoseconds>(r_end - r_start));
    }
    auto read_end = high_resolution_clock::now();
    auto read_total = duration_cast<nanoseconds>(read_end - read_start);
    
    std::cout << "\n--- Write Performance ---" << std::endl;
    auto write_result = calculate_results("Event Store Write", write_latencies, write_total);
    print_result(write_result);
    
    std::cout << "\n--- Read Performance ---" << std::endl;
    auto read_result = calculate_results("Event Store Read", read_latencies, read_total);
    print_result(read_result);
    
    return write_result;  // Return write result as primary
}

// Benchmark 4: Event Stream Processing
BenchmarkResult benchmark_event_stream_processing(size_t num_events) {
    std::cout << "\n[Benchmark 4] Event Stream Processing (" << num_events << " events)..." << std::endl;
    
    EventStore store;
    store.initialize("./benchmark_data_stream");
    
    EventStreamProcessor processor(&store);
    
    std::atomic<size_t> events_processed(0);
    
    // Subscribe to events
    processor.subscribe([&events_processed](const Event& event) {
        events_processed++;
    });
    
    // Generate and append events
    for (size_t i = 0; i < num_events; ++i) {
        Event event;
        event.type = EventType::ORDER_PLACED;
        event.sequence_id = i + 1;
        event.instrument_id = 1;
        event.data.order_placed.order_id = 1000 + i;
        event.data.order_placed.user_id = 2000;
        event.data.order_placed.side = OrderSide::BUY;
        event.data.order_placed.order_type = OrderType::LIMIT;
        event.data.order_placed.price = double_to_price(50000.0);
        event.data.order_placed.quantity = double_to_quantity(1.0);
        store.append_event(event);
    }
    
    // Start processing
    auto start = high_resolution_clock::now();
    processor.start_processing();
    
    // Wait for all events to be processed
    while (events_processed.load() < num_events) {
        std::this_thread::sleep_for(milliseconds(10));
        processor.process_new_events();
    }
    
    auto end = high_resolution_clock::now();
    auto total_time = duration_cast<nanoseconds>(end - start);
    
    processor.stop_processing();
    
    BenchmarkResult result;
    result.name = "Event Stream Processing";
    result.operations = events_processed.load();
    result.total_time = total_time;
    double total_seconds = duration_cast<microseconds>(total_time).count() / 1000000.0;
    result.throughput = (total_seconds > 0) ? (events_processed.load() / total_seconds) : 0;
    
    return result;
}

// Benchmark 5: CQRS Performance
BenchmarkResult benchmark_cqrs(size_t num_commands) {
    std::cout << "\n[Benchmark 5] CQRS Performance (" << num_commands << " commands)..." << std::endl;
    
    MatchingEngineEventSourcing engine(1);
    EventStore store;
    store.initialize("./benchmark_data_cqrs");
    engine.initialize("./benchmark_data_cqrs");
    
    CQRSManager cqrs(&engine, &store);
    cqrs.start_cache_update();
    
    std::vector<nanoseconds> latencies;
    latencies.reserve(num_commands);
    
    auto start = high_resolution_clock::now();
    
    for (size_t i = 0; i < num_commands; ++i) {
        Command cmd;
        cmd.type = Command::PLACE_ORDER;
        cmd.order_id = 1000 + i;
        cmd.user_id = 2000;
        cmd.instrument_id = 1;
        cmd.side = (i % 2 == 0) ? OrderSide::BUY : OrderSide::SELL;
        cmd.price = double_to_price(50000.0);
        cmd.quantity = double_to_quantity(1.0);
        cmd.order_type = OrderType::LIMIT;
        
        auto cmd_start = high_resolution_clock::now();
        cqrs.execute_command(cmd);
        auto cmd_end = high_resolution_clock::now();
        
        latencies.push_back(duration_cast<nanoseconds>(cmd_end - cmd_start));
    }
    
    auto end = high_resolution_clock::now();
    auto total_time = duration_cast<nanoseconds>(end - start);
    
    cqrs.stop_cache_update();
    
    return calculate_results("CQRS Command Execution", latencies, total_time);
}

// Benchmark 6: Event Compression
BenchmarkResult benchmark_event_compression(size_t num_events) {
    std::cout << "\n[Benchmark 6] Event Compression (" << num_events << " events)..." << std::endl;
    
    EventStore store;
    store.initialize("./benchmark_data_compression");
    
    // Generate events
    for (size_t i = 0; i < num_events; ++i) {
        Event event;
        event.type = EventType::ORDER_PLACED;
        event.sequence_id = i + 1;
        event.instrument_id = 1;
        event.data.order_placed.order_id = 1000 + i;
        event.data.order_placed.user_id = 2000;
        event.data.order_placed.side = OrderSide::BUY;
        event.data.order_placed.order_type = OrderType::LIMIT;
        event.data.order_placed.price = double_to_price(50000.0);
        event.data.order_placed.quantity = double_to_quantity(1.0);
        store.append_event(event);
    }
    
    EventCompressor compressor(&store);
    compressor.set_strategy(CompressionStrategy::SNAPSHOT_ONLY);
    
    auto start = high_resolution_clock::now();
    size_t compressed = compressor.compress_events(num_events / 2);
    auto end = high_resolution_clock::now();
    auto total_time = duration_cast<nanoseconds>(end - start);
    
    BenchmarkResult result;
    result.name = "Event Compression";
    result.operations = compressed;
    result.total_time = total_time;
    double total_seconds = duration_cast<microseconds>(total_time).count() / 1000000.0;
    result.throughput = (total_seconds > 0) ? (compressed / total_seconds) : 0;
    
    return result;
}

int main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "Event Sourcing Performance Benchmark" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Parse command line arguments
    size_t num_orders = 10000;
    if (argc > 1) {
        num_orders = std::stoul(argv[1]);
    }
    
    std::cout << "\nConfiguration:" << std::endl;
    std::cout << "  Orders/Events: " << num_orders << std::endl;
    std::cout << "  Test Directory: ./benchmark_data*" << std::endl;
    
    std::vector<BenchmarkResult> results;
    
    // Run benchmarks
    try {
        results.push_back(benchmark_event_sourcing_basic(num_orders));
        results.push_back(benchmark_deterministic_calculation(num_orders * 10));
        results.push_back(benchmark_event_store_io(num_orders));
        results.push_back(benchmark_event_stream_processing(num_orders));
        results.push_back(benchmark_cqrs(num_orders));
        results.push_back(benchmark_event_compression(num_orders));
    } catch (const std::exception& e) {
        std::cerr << "Error during benchmark: " << e.what() << std::endl;
        return 1;
    }
    
    // Print summary
    std::cout << "\n\n========================================" << std::endl;
    std::cout << "Performance Summary" << std::endl;
    std::cout << "========================================" << std::endl;
    
    for (const auto& result : results) {
        print_result(result);
    }
    
    // Print comparison table
    std::cout << "\n\n========================================" << std::endl;
    std::cout << "Throughput Comparison (ops/sec)" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::left << std::setw(40) << "Benchmark" 
              << std::right << std::setw(15) << "Throughput" << std::endl;
    std::cout << std::string(55, '-') << std::endl;
    
    for (const auto& result : results) {
        std::cout << std::left << std::setw(40) << result.name
                  << std::right << std::setw(15) << std::fixed << std::setprecision(2) 
                  << result.throughput << std::endl;
    }
    
    std::cout << "\nBenchmark completed!" << std::endl;
    
    return 0;
}

