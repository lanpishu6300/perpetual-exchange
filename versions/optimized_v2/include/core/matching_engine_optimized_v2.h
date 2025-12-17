#pragma once

#include "core/matching_engine_optimized.h"
#include <algorithm>
#include <cstring>

namespace perpetual {

// Further optimized matching engine with hot path optimizations
class MatchingEngineOptimizedV2 : public OptimizedMatchingEngine {
public:
    MatchingEngineOptimizedV2(InstrumentID instrument_id);
    
    // Optimized order processing with inlined hot path
    std::vector<Trade> process_order_optimized_v2(Order* order);
    
private:
    // Pre-allocated trade vector to avoid reallocation
    std::vector<Trade> trade_buffer_;
    static constexpr size_t INITIAL_TRADE_BUFFER_SIZE = 16;
};

} // namespace perpetual
