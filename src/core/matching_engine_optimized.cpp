#include "core/matching_engine_optimized.h"
#include <algorithm>

namespace perpetual {

OptimizedMatchingEngine::OptimizedMatchingEngine(InstrumentID instrument_id)
    : MatchingEngine(instrument_id), order_queue_(1024 * 1024) {
    setup_numa_affinity();
}

OptimizedMatchingEngine::~OptimizedMatchingEngine() {
}

void OptimizedMatchingEngine::setup_numa_affinity() {
    // Bind main thread to CPU 0
    NUMAUtils::bind_thread_to_cpu(0);
    numa_configured_.store(true, std::memory_order_release);
}

std::vector<Trade> OptimizedMatchingEngine::process_order_optimized(Order* order) {
    // For now, use base implementation
    // In production, this would use memory pool allocation and SIMD matching
    return MatchingEngine::process_order(order);
}

std::vector<Trade> OptimizedMatchingEngine::match_order_simd(Order* order) {
    // For now, fallback to base implementation
    // In production, this would use SIMD for batch price comparisons
    return MatchingEngine::process_order(order);
}

std::vector<Trade> OptimizedMatchingEngine::process_orders_batch(
    const std::vector<Order*>& orders) {
    std::vector<Trade> all_trades;
    all_trades.reserve(orders.size());
    
    // Process in batches of 4 for SIMD optimization
    for (size_t i = 0; i < orders.size(); i += 4) {
        size_t batch_size = std::min<size_t>(4, orders.size() - i);
        
        for (size_t j = 0; j < batch_size; ++j) {
            auto trades = process_order(orders[i + j]);
            all_trades.insert(all_trades.end(), trades.begin(), trades.end());
        }
    }
    
    return all_trades;
}

} // namespace perpetual
