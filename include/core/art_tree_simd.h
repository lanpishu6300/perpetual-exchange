#pragma once

#include "art_tree.h"
#include "simd_utils.h"
#include <immintrin.h>

namespace perpetual {

// SIMD-optimized ART Tree operations
// Uses AVX2/AVX-512 for faster prefix matching and key comparisons

class ARTTreeSIMD : public ARTTree {
public:
    ARTTreeSIMD() : ARTTree() {}
    
    // SIMD-optimized find
    void* find_simd(Price key) const;
    
    // SIMD-optimized prefix matching
    int check_prefix_simd(ARTNode* node, const uint8_t* key, int depth) const;
    
    // SIMD-optimized child finding in Node16
    ARTNode* find_child_simd_node16(ARTNode16* node, uint8_t byte) const;
    
    // SIMD-optimized child finding in Node48
    ARTNode* find_child_simd_node48(ARTNode48* node, uint8_t byte) const;
    
    // Batch price comparison using SIMD
    // Compare multiple prices at once
    static void batch_compare_prices(const Price* prices, size_t count, Price target, bool* results);
    
    // SIMD-optimized min/max key finding
    Price min_key_simd() const;
    Price max_key_simd() const;
    
private:
    // SIMD helper for prefix matching
    int check_prefix_avx2(ARTNode* node, const uint8_t* key, int depth) const;
    
    // SIMD helper for Node16 key search
    int find_child_index_simd_node16(const uint8_t* keys, uint8_t count, uint8_t byte) const;
};

} // namespace perpetual




#include "art_tree.h"
#include "simd_utils.h"
#include <immintrin.h>

namespace perpetual {

// SIMD-optimized ART Tree operations
// Uses AVX2/AVX-512 for faster prefix matching and key comparisons

class ARTTreeSIMD : public ARTTree {
public:
    ARTTreeSIMD() : ARTTree() {}
    
    // SIMD-optimized find
    void* find_simd(Price key) const;
    
    // SIMD-optimized prefix matching
    int check_prefix_simd(ARTNode* node, const uint8_t* key, int depth) const;
    
    // SIMD-optimized child finding in Node16
    ARTNode* find_child_simd_node16(ARTNode16* node, uint8_t byte) const;
    
    // SIMD-optimized child finding in Node48
    ARTNode* find_child_simd_node48(ARTNode48* node, uint8_t byte) const;
    
    // Batch price comparison using SIMD
    // Compare multiple prices at once
    static void batch_compare_prices(const Price* prices, size_t count, Price target, bool* results);
    
    // SIMD-optimized min/max key finding
    Price min_key_simd() const;
    Price max_key_simd() const;
    
private:
    // SIMD helper for prefix matching
    int check_prefix_avx2(ARTNode* node, const uint8_t* key, int depth) const;
    
    // SIMD helper for Node16 key search
    int find_child_index_simd_node16(const uint8_t* keys, uint8_t count, uint8_t byte) const;
};

} // namespace perpetual



