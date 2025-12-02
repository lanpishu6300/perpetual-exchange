#include "core/matching_engine.h"
#include "core/simd_utils.h"
#include "core/types.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <iomanip>
#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#endif

using namespace perpetual;
using namespace std::chrono;

// Check AVX2 support
bool check_avx2_support() {
#if defined(__x86_64__) || defined(_M_X64)
    unsigned int eax, ebx, ecx, edx;
    __asm__ __volatile__("cpuid"
                         : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                         : "a"(7), "c"(0));
    return (ebx & (1 << 5)) != 0; // AVX2 bit
#else
    return false;
#endif
}

void test_simd_price_comparison() {
    std::cout << "\n=== SIMD Price Comparison Test ===\n";
    
    const size_t num_comparisons = 10000000;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int64_t> price_dist(40000000000LL, 60000000000LL);
    std::uniform_int_distribution<int64_t> threshold_dist(45000000000LL, 55000000000LL);
    
    // Prepare test data
    std::vector<Price> prices(num_comparisons);
    Price threshold = threshold_dist(gen);
    for (size_t i = 0; i < num_comparisons; ++i) {
        prices[i] = price_dist(gen);
    }
    
    // Scalar version
    auto start_scalar = high_resolution_clock::now();
    size_t scalar_count = 0;
    for (size_t i = 0; i < num_comparisons; i += 4) {
        size_t remaining = std::min<size_t>(4, num_comparisons - i);
        for (size_t j = 0; j < remaining; ++j) {
            if (prices[i + j] > threshold) {
                scalar_count++;
            }
        }
    }
    auto end_scalar = high_resolution_clock::now();
    auto scalar_time = duration_cast<microseconds>(end_scalar - start_scalar);
    
    // SIMD version
    auto start_simd = high_resolution_clock::now();
    size_t simd_count = 0;
    for (size_t i = 0; i < num_comparisons; i += 4) {
        size_t remaining = std::min<size_t>(4, num_comparisons - i);
        if (remaining == 4) {
            if (SIMDUtils::compare_prices_batch(
                    prices[i], prices[i+1], prices[i+2], prices[i+3], threshold)) {
                simd_count += 4; // Simplified counting
            }
        } else {
            for (size_t j = 0; j < remaining; ++j) {
                if (prices[i + j] > threshold) {
                    simd_count++;
                }
            }
        }
    }
    auto end_simd = high_resolution_clock::now();
    auto simd_time = duration_cast<microseconds>(end_simd - start_simd);
    
    std::cout << "Comparisons: " << num_comparisons << "\n";
    std::cout << "Scalar time: " << scalar_time.count() << " μs\n";
    std::cout << "SIMD time:   " << simd_time.count() << " μs\n";
    if (simd_time.count() > 0) {
        double speedup = static_cast<double>(scalar_time.count()) / simd_time.count();
        std::cout << "Speedup:     " << std::fixed << std::setprecision(2) << speedup << "x\n";
    }
    std::cout << "Results match: " << (scalar_count == simd_count ? "Yes" : "No") << "\n";
}

void test_simd_quantity_sum() {
    std::cout << "\n=== SIMD Quantity Sum Test ===\n";
    
    const size_t num_sums = 10000000;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int64_t> qty_dist(100000LL, 1000000LL);
    
    // Prepare test data
    std::vector<Quantity> quantities(num_sums * 4);
    for (size_t i = 0; i < quantities.size(); ++i) {
        quantities[i] = qty_dist(gen);
    }
    
    // Scalar version
    auto start_scalar = high_resolution_clock::now();
    int64_t scalar_total = 0;
    for (size_t i = 0; i < num_sums; ++i) {
        scalar_total += quantities[i*4] + quantities[i*4+1] + 
                       quantities[i*4+2] + quantities[i*4+3];
    }
    auto end_scalar = high_resolution_clock::now();
    auto scalar_time = duration_cast<microseconds>(end_scalar - start_scalar);
    
    // SIMD version
    auto start_simd = high_resolution_clock::now();
    int64_t simd_total = 0;
    for (size_t i = 0; i < num_sums; ++i) {
        simd_total += SIMDUtils::sum_quantities_batch(
            quantities[i*4], quantities[i*4+1], 
            quantities[i*4+2], quantities[i*4+3]);
    }
    auto end_simd = high_resolution_clock::now();
    auto simd_time = duration_cast<microseconds>(end_simd - start_simd);
    
    std::cout << "Sums: " << num_sums << "\n";
    std::cout << "Scalar time: " << scalar_time.count() << " μs\n";
    std::cout << "SIMD time:   " << simd_time.count() << " μs\n";
    if (simd_time.count() > 0) {
        double speedup = static_cast<double>(scalar_time.count()) / simd_time.count();
        std::cout << "Speedup:     " << std::fixed << std::setprecision(2) << speedup << "x\n";
    }
    std::cout << "Results match: " << (scalar_total == simd_total ? "Yes" : "No") << "\n";
}

void test_simd_pnl_calculation() {
    std::cout << "\n=== SIMD PnL Calculation Test ===\n";
    
    const size_t num_positions = 1000000;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int64_t> price_dist(40000000000LL, 60000000000LL);
    std::uniform_int_distribution<int64_t> qty_dist(100000LL, 1000000LL);
    
    // Prepare test data
    std::vector<Price> mark_prices(num_positions);
    std::vector<Price> open_prices(num_positions);
    std::vector<Quantity> sizes(num_positions);
    
    for (size_t i = 0; i < num_positions; ++i) {
        mark_prices[i] = price_dist(gen);
        open_prices[i] = price_dist(gen);
        sizes[i] = qty_dist(gen);
    }
    
    // Scalar version
    std::vector<int64_t> scalar_results(num_positions);
    auto start_scalar = high_resolution_clock::now();
    for (size_t i = 0; i < num_positions; ++i) {
        scalar_results[i] = sizes[i] * (mark_prices[i] - open_prices[i]);
    }
    auto end_scalar = high_resolution_clock::now();
    auto scalar_time = duration_cast<microseconds>(end_scalar - start_scalar);
    
    // SIMD version
    std::vector<int64_t> simd_results(num_positions);
    auto start_simd = high_resolution_clock::now();
    SIMDUtils::calculate_pnl_batch(
        mark_prices.data(), open_prices.data(), 
        sizes.data(), simd_results.data(), num_positions);
    auto end_simd = high_resolution_clock::now();
    auto simd_time = duration_cast<microseconds>(end_simd - start_simd);
    
    // Verify results
    bool results_match = true;
    for (size_t i = 0; i < num_positions; ++i) {
        if (scalar_results[i] != simd_results[i]) {
            results_match = false;
            break;
        }
    }
    
    std::cout << "Positions: " << num_positions << "\n";
    std::cout << "Scalar time: " << scalar_time.count() << " μs\n";
    std::cout << "SIMD time:   " << simd_time.count() << " μs\n";
    if (simd_time.count() > 0) {
        double speedup = static_cast<double>(scalar_time.count()) / simd_time.count();
        std::cout << "Speedup:     " << std::fixed << std::setprecision(2) << speedup << "x\n";
    }
    std::cout << "Results match: " << (results_match ? "Yes" : "No") << "\n";
}

void test_matching_engine_with_simd() {
    std::cout << "\n=== Matching Engine Performance Test ===\n";
    
    InstrumentID instrument_id = 1;
    MatchingEngine engine(instrument_id);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> price_dist(40000.0, 60000.0);
    std::uniform_real_distribution<double> qty_dist(0.01, 1.0);
    std::uniform_real_distribution<double> side_dist(0.0, 1.0);
    
    const size_t num_orders = 100000;
    std::vector<std::unique_ptr<Order>> orders;
    orders.reserve(num_orders);
    
    uint64_t total_trades = 0;
    nanoseconds total_latency{0};
    nanoseconds min_latency{nanoseconds::max()};
    nanoseconds max_latency{nanoseconds::zero()};
    
    auto start = high_resolution_clock::now();
    
    for (size_t i = 0; i < num_orders; ++i) {
        UserID user_id = (i % 1000) + 1;
        OrderSide side = (side_dist(gen) < 0.5) ? OrderSide::BUY : OrderSide::SELL;
        Price price = double_to_price(price_dist(gen));
        Quantity quantity = double_to_quantity(qty_dist(gen));
        
        auto order = std::make_unique<Order>(
            i + 1, user_id, instrument_id,
            side, price, quantity, OrderType::LIMIT
        );
        
        auto order_start = high_resolution_clock::now();
        auto trades = engine.process_order(order.get());
        auto order_end = high_resolution_clock::now();
        
        auto latency = duration_cast<nanoseconds>(order_end - order_start);
        total_latency += latency;
        if (latency < min_latency) min_latency = latency;
        if (latency > max_latency) max_latency = latency;
        
        total_trades += trades.size();
        
        if (order->is_active()) {
            orders.push_back(std::move(order));
        }
        
        if ((i + 1) % 10000 == 0) {
            std::cout << "Processed " << (i + 1) << " orders, " 
                      << total_trades << " trades\n";
        }
    }
    
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    
    std::cout << "\nResults:\n";
    std::cout << "Total Orders: " << num_orders << "\n";
    std::cout << "Total Trades: " << total_trades << "\n";
    std::cout << "Total Time: " << duration.count() << " ms\n";
    std::cout << "Throughput: " << (num_orders * 1000.0 / duration.count()) / 1000.0 
              << " K orders/sec\n";
    std::cout << "Avg Latency: " << (total_latency.count() / num_orders) << " ns\n";
    std::cout << "Min Latency: " << min_latency.count() << " ns\n";
    std::cout << "Max Latency: " << max_latency.count() << " ns\n";
}

int main() {
    std::cout << "========================================\n";
    std::cout << "SIMD Performance Benchmark (x86_64)\n";
    std::cout << "========================================\n";
    
    // Check AVX2 support
    bool avx2_supported = check_avx2_support();
    std::cout << "\nAVX2 Support: " << (avx2_supported ? "Yes" : "No") << "\n";
    
    if (!avx2_supported) {
        std::cout << "Warning: AVX2 not supported. SIMD optimizations may not work.\n";
    }
    
    // Run SIMD tests
    test_simd_price_comparison();
    test_simd_quantity_sum();
    test_simd_pnl_calculation();
    test_matching_engine_with_simd();
    
    std::cout << "\n========================================\n";
    std::cout << "Benchmark completed!\n";
    std::cout << "========================================\n";
    
    return 0;
}
