#pragma once

#include "art_tree.h"
#include "core/types.h"
#include <cstdint>

namespace perpetual {

// ART Tree with SIMD optimizations
// Inherits from ARTTree and adds SIMD-accelerated methods
class ARTTreeSIMD : public ARTTree {
public:
    ARTTreeSIMD() : ARTTree() {}
    ~ARTTreeSIMD() = default;
    
    // SIMD-optimized find
    void* find_simd(Price key) const;
    
    // SIMD-optimized prefix checking
    int check_prefix_simd(ARTNode* node, const uint8_t* key, int depth) const;
    
    // SIMD-optimized child finding
    ARTNode* find_child_simd_node16(ARTNode16* node, uint8_t byte) const;
    ARTNode* find_child_simd_node48(ARTNode48* node, uint8_t byte) const;
    
    // Batch operations
    void batch_compare_prices(const Price* prices, size_t count, Price target, bool* results);
    
    // SIMD-optimized min/max
    Price min_key_simd() const;
    Price max_key_simd() const;
    
private:
#ifdef __AVX2__
    int check_prefix_avx2(ARTNode* node, const uint8_t* key, int depth) const;
    int find_child_index_simd_node16(const uint8_t* keys, uint8_t count, uint8_t byte) const;
#endif
};

} // namespace perpetual

