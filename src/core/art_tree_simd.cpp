#include "core/art_tree_simd.h"
#include <algorithm>
#include <cstring>

#ifdef __AVX2__
#include <immintrin.h>
#endif

namespace perpetual {

#ifdef __AVX2__

void* ARTTreeSIMD::find_simd(Price key) const {
    if (root_ == nullptr) {
        return nullptr;
    }
    
    uint8_t key_bytes[8];
    price_to_bytes(key, key_bytes);
    return find_recursive(root_, key_bytes, 0);
}

int ARTTreeSIMD::check_prefix_simd(ARTNode* node, const uint8_t* key, int depth) const {
    uint8_t prefix_len = node->prefix_len();
    if (prefix_len == 0) {
        return 0;
    }
    
    const uint8_t* prefix = node->prefix();
    int remaining = 8 - depth;
    int max_cmp = (prefix_len < remaining) ? prefix_len : remaining;
    
    // Use SIMD for prefix matching if prefix is long enough
    if (max_cmp >= 8) {
        return check_prefix_avx2(node, key, depth);
    }
    
    // Scalar fallback for short prefixes
    for (int i = 0; i < max_cmp; ++i) {
        if (prefix[i] != key[depth + i]) {
            return i;
        }
    }
    return max_cmp;
}

int ARTTreeSIMD::check_prefix_avx2(ARTNode* node, const uint8_t* key, int depth) const {
    const uint8_t* prefix = node->prefix();
    uint8_t prefix_len = node->prefix_len();
    int remaining = 8 - depth;
    int max_cmp = (prefix_len < remaining) ? prefix_len : remaining;
    
    // Use AVX2 for 8-byte comparison
    if (max_cmp >= 8) {
        __m128i prefix_vec = _mm_loadu_si128(reinterpret_cast<const __m128i*>(prefix));
        __m128i key_vec = _mm_loadu_si128(reinterpret_cast<const __m128i*>(key + depth));
        
        __m128i cmp = _mm_cmpeq_epi8(prefix_vec, key_vec);
        int mask = _mm_movemask_epi8(cmp);
        
        // Find first mismatch
        if (mask == 0xFFFF) {
            return 8;  // All match
        }
        
        // Find first zero bit (mismatch)
        return __builtin_ctz(~mask);
    }
    
    // Fallback to scalar
    for (int i = 0; i < max_cmp; ++i) {
        if (prefix[i] != key[depth + i]) {
            return i;
        }
    }
    return max_cmp;
}

ARTNode* ARTTreeSIMD::find_child_simd_node16(ARTNode16* node, uint8_t byte) const {
    if (node == nullptr || node->count_ == 0) {
        return nullptr;
    }
    
#ifdef __AVX2__
    // Use SIMD to compare all keys at once
    __m128i keys_vec = _mm_loadu_si128(reinterpret_cast<const __m128i*>(node->keys_));
    __m128i byte_vec = _mm_set1_epi8(byte);
    __m128i cmp = _mm_cmpeq_epi8(keys_vec, byte_vec);
    int mask = _mm_movemask_epi8(cmp);
    
    if (mask == 0) {
        return nullptr;  // Not found
    }
    
    // Find first match
    int index = __builtin_ctz(mask);
    if (index < node->count_) {
        return node->children_[index];
    }
#else
    // Scalar fallback
    for (int i = 0; i < node->count_; ++i) {
        if (node->keys_[i] == byte) {
            return node->children_[i];
        }
    }
#endif
    
    return nullptr;
}

int ARTTreeSIMD::find_child_index_simd_node16(const uint8_t* keys, uint8_t count, uint8_t byte) const {
    if (count == 0) {
        return -1;
    }
    
    // Use SIMD to compare all keys at once
    __m128i keys_vec = _mm_loadu_si128(reinterpret_cast<const __m128i*>(keys));
    __m128i byte_vec = _mm_set1_epi8(byte);
    __m128i cmp = _mm_cmpeq_epi8(keys_vec, byte_vec);
    int mask = _mm_movemask_epi8(cmp);
    
    if (mask == 0) {
        return -1;  // Not found
    }
    
    // Find first match
    int index = __builtin_ctz(mask);
    if (index < count) {
        return index;
    }
    
    return -1;
}

ARTNode* ARTTreeSIMD::find_child_simd_node48(ARTNode48* node, uint8_t byte) const {
    if (node == nullptr) {
        return nullptr;
    }
    
    if (node->keys_[byte] == 48) {
        return nullptr;
    }
    
    int pos = node->keys_[byte];
    if (pos < 48 && node->children_[pos] != nullptr) {
        return node->children_[pos];
    }
    
    return nullptr;
}

void ARTTreeSIMD::batch_compare_prices(const Price* prices, size_t count, Price target, bool* results) {
    if (count == 0) {
        return;
    }
    
    // Convert target to bytes
    uint8_t target_bytes[8];
    price_to_bytes(target, target_bytes);
    
    // Process 8 prices at a time using SIMD
    size_t simd_count = (count / 8) * 8;
    
    for (size_t i = 0; i < simd_count; i += 8) {
        // Load 8 prices
        uint8_t price_bytes[8][8];
        for (size_t j = 0; j < 8 && (i + j) < count; ++j) {
            price_to_bytes(prices[i + j], price_bytes[j]);
        }
        
        // Compare using SIMD
        __m128i target_vec = _mm_loadu_si128(reinterpret_cast<const __m128i*>(target_bytes));
        
        for (size_t j = 0; j < 8 && (i + j) < count; ++j) {
            __m128i price_vec = _mm_loadu_si128(reinterpret_cast<const __m128i*>(price_bytes[j]));
            __m128i cmp = _mm_cmpeq_epi8(target_vec, price_vec);
            int mask = _mm_movemask_epi8(cmp);
            results[i + j] = (mask == 0xFFFF);
        }
    }
    
    // Handle remaining prices
    for (size_t i = simd_count; i < count; ++i) {
        results[i] = (prices[i] == target);
    }
}

Price ARTTreeSIMD::min_key_simd() const {
    return min_key();  // Use base implementation for now
}

Price ARTTreeSIMD::max_key_simd() const {
    return max_key();  // Use base implementation for now
}

#else
// Fallback implementations when AVX2 is not available

void* ARTTreeSIMD::find_simd(Price key) const {
    return find(key);
}

int ARTTreeSIMD::check_prefix_simd(ARTNode* node, const uint8_t* key, int depth) const {
    return check_prefix(node, key, depth);
}

ARTNode* ARTTreeSIMD::find_child_simd_node16(ARTNode16* node, uint8_t byte) const {
    if (node == nullptr || node->count_ == 0) {
        return nullptr;
    }
    
    // Scalar fallback
    for (int i = 0; i < node->count_; ++i) {
        if (node->keys_[i] == byte) {
            return node->children_[i];
        }
    }
    return nullptr;
}

ARTNode* ARTTreeSIMD::find_child_simd_node48(ARTNode48* node, uint8_t byte) const {
    if (node == nullptr) {
        return nullptr;
    }
    
    if (node->keys_[byte] == 48) {
        return nullptr;
    }
    
    int pos = node->keys_[byte];
    if (pos < 48 && node->children_[pos] != nullptr) {
        return node->children_[pos];
    }
    
    return nullptr;
}

void ARTTreeSIMD::batch_compare_prices(const Price* prices, size_t count, Price target, bool* results) {
    for (size_t i = 0; i < count; ++i) {
        results[i] = (prices[i] == target);
    }
}

Price ARTTreeSIMD::min_key_simd() const {
    return min_key();
}

Price ARTTreeSIMD::max_key_simd() const {
    return max_key();
}

#endif // __AVX2__

} // namespace perpetual

