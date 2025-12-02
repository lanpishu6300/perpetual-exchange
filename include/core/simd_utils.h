#pragma once

#include "types.h"
#include <cstdint>
#include <algorithm>

// Check for AVX2 support
#if (defined(__x86_64__) || defined(_M_X64)) && !defined(__APPLE__)
#include <immintrin.h>
#define SIMD_AVAILABLE 1
#elif defined(__AVX2__) && (defined(__x86_64__) || defined(_M_X64))
#include <immintrin.h>
#define SIMD_AVAILABLE 1
#else
#define SIMD_AVAILABLE 0
#endif

namespace perpetual {

// SIMD optimized price/quantity operations
class SIMDUtils {
public:
    // Compare 4 prices at once (AVX2 on x86_64, scalar fallback otherwise)
    static inline bool compare_prices_batch(Price p1, Price p2, Price p3, Price p4,
                                            Price threshold) {
#if SIMD_AVAILABLE
        alignas(32) int64_t prices[4] = {p1, p2, p3, p4};
        alignas(32) int64_t thresholds[4] = {threshold, threshold, threshold, threshold};
        
        __m256i v_prices = _mm256_load_si256(reinterpret_cast<const __m256i*>(prices));
        __m256i v_thresholds = _mm256_load_si256(reinterpret_cast<const __m256i*>(thresholds));
        __m256i v_result = _mm256_cmpgt_epi64(v_prices, v_thresholds);
        
        int mask = _mm256_movemask_epi8(v_result);
        return mask != 0;
#else
        return (p1 > threshold) || (p2 > threshold) || (p3 > threshold) || (p4 > threshold);
#endif
    }
    
    // Sum 4 quantities at once (AVX2 on x86_64, scalar fallback otherwise)
    static inline Quantity sum_quantities_batch(Quantity q1, Quantity q2, 
                                                 Quantity q3, Quantity q4) {
#if SIMD_AVAILABLE
        alignas(32) int64_t quantities[4] = {q1, q2, q3, q4};
        __m256i v_qty = _mm256_load_si256(reinterpret_cast<const __m256i*>(quantities));
        
        // Horizontal sum: add pairs, then combine
        __m256i v_permuted = _mm256_permute4x64_epi64(v_qty, 0x4E); // Swap high/low 128-bit
        __m256i v_sum1 = _mm256_add_epi64(v_qty, v_permuted);
        __m256i v_permuted2 = _mm256_permute4x64_epi64(v_sum1, 0xB1); // Swap 64-bit pairs
        __m256i v_sum = _mm256_add_epi64(v_sum1, v_permuted2);
        
        alignas(32) int64_t result[4];
        _mm256_store_si256(reinterpret_cast<__m256i*>(result), v_sum);
        return result[0];
#else
        return q1 + q2 + q3 + q4;
#endif
    }
    
    // Find minimum of 4 prices
    static inline Price min_price_batch(Price p1, Price p2, Price p3, Price p4) {
        Price min_val = p1;
        if (p2 < min_val) min_val = p2;
        if (p3 < min_val) min_val = p3;
        if (p4 < min_val) min_val = p4;
        return min_val;
    }
    
    // Calculate PnL for positions (batch processing with AVX2)
    static inline void calculate_pnl_batch(const Price* mark_prices,
                                           const Price* open_prices,
                                           const Quantity* sizes,
                                           int64_t* pnl_results,
                                           size_t count) {
#if SIMD_AVAILABLE
        for (size_t i = 0; i < count; i += 4) {
            size_t remaining = std::min<size_t>(4, count - i);
            
            if (remaining == 4) {
                // Process 4 at once with AVX2
                alignas(32) int64_t marks[4], opens[4], sizes_arr[4];
                for (size_t j = 0; j < 4; ++j) {
                    marks[j] = mark_prices[i + j];
                    opens[j] = open_prices[i + j];
                    sizes_arr[j] = sizes[i + j];
                }
                
                __m256i v_marks = _mm256_load_si256(reinterpret_cast<const __m256i*>(marks));
                __m256i v_opens = _mm256_load_si256(reinterpret_cast<const __m256i*>(opens));
                __m256i v_sizes = _mm256_load_si256(reinterpret_cast<const __m256i*>(sizes_arr));
                
                __m256i v_diff = _mm256_sub_epi64(v_marks, v_opens);
                
                // Store differences and multiply (AVX2 doesn't have 64-bit multiply)
                alignas(32) int64_t diffs[4];
                _mm256_store_si256(reinterpret_cast<__m256i*>(diffs), v_diff);
                
                // Use scalar multiply for 64-bit (or could use 32-bit split approach)
                for (size_t j = 0; j < 4; ++j) {
                    pnl_results[i + j] = sizes_arr[j] * diffs[j];
                }
            } else {
                // Fallback for remaining items
                for (size_t j = 0; j < remaining; ++j) {
                    pnl_results[i + j] = sizes[i + j] * (mark_prices[i + j] - open_prices[i + j]);
                }
            }
        }
#else
        // Scalar implementation
        for (size_t i = 0; i < count; ++i) {
            pnl_results[i] = sizes[i] * (mark_prices[i] - open_prices[i]);
        }
#endif
    }
};

} // namespace perpetual