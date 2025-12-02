#pragma once

#include "art_tree_simd.h"
#include <immintrin.h>

namespace perpetual {

// Enhanced ART+SIMD with additional optimizations
class ARTTreeSIMDEnhanced : public ARTTreeSIMD {
public:
    ARTTreeSIMDEnhanced() : ARTTreeSIMD() {}
    
    // SIMD-optimized batch insert (insert multiple prices at once)
    void batch_insert(const Price* prices, void** values, size_t count);
    
    // SIMD-optimized range query (find all prices in range)
    void range_query(Price min_price, Price max_price, std::vector<Price>& results) const;
    
    // SIMD-optimized top N prices (for depth data)
    void get_top_prices(size_t n, bool ascending, std::vector<Price>& prices) const;
    
    // SIMD-optimized price comparison for matching
    // Returns true if order_price can match against best_price
    static inline bool can_match_price(Price order_price, Price best_price, bool is_buy) {
#ifdef __AVX2__
        // Use SIMD for comparison (though simple comparison is already fast)
        // This is more for demonstration of SIMD usage
        alignas(32) int64_t prices[4] = {order_price, best_price, 0, 0};
        __m256i v_prices = _mm256_load_si256(reinterpret_cast<const __m256i*>(prices));
        
        if (is_buy) {
            // For buy: order_price >= best_price
            __m256i cmp = _mm256_cmpgt_epi64(v_prices, _mm256_setzero_si256());
            return order_price >= best_price;
        } else {
            // For sell: order_price <= best_price
            return order_price <= best_price;
        }
#else
        if (is_buy) {
            return order_price >= best_price;
        } else {
            return order_price <= best_price;
        }
#endif
    }
    
    // SIMD-optimized quantity aggregation
    static inline Quantity aggregate_quantities_simd(const Quantity* quantities, size_t count) {
        if (count == 0) return 0;
        
#ifdef __AVX2__
        Quantity sum = 0;
        size_t simd_count = (count / 4) * 4;
        
        // Process 4 quantities at a time
        for (size_t i = 0; i < simd_count; i += 4) {
            alignas(32) int64_t qty_arr[4];
            for (size_t j = 0; j < 4 && (i + j) < count; ++j) {
                qty_arr[j] = quantities[i + j];
            }
            
            __m256i v_qty = _mm256_load_si256(reinterpret_cast<const __m256i*>(qty_arr));
            
            // Horizontal sum
            __m256i v_permuted = _mm256_permute4x64_epi64(v_qty, 0x4E);
            __m256i v_sum1 = _mm256_add_epi64(v_qty, v_permuted);
            __m256i v_permuted2 = _mm256_permute4x64_epi64(v_sum1, 0xB1);
            __m256i v_sum = _mm256_add_epi64(v_sum1, v_permuted2);
            
            alignas(32) int64_t result[4];
            _mm256_store_si256(reinterpret_cast<__m256i*>(result), v_sum);
            sum += result[0];
        }
        
        // Handle remaining
        for (size_t i = simd_count; i < count; ++i) {
            sum += quantities[i];
        }
        
        return sum;
#else
        Quantity sum = 0;
        for (size_t i = 0; i < count; ++i) {
            sum += quantities[i];
        }
        return sum;
#endif
    }
};

} // namespace perpetual

