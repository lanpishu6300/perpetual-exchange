#pragma once

#include "types.h"
#include <cstdint>
#include <cstring>
#include <memory>
#include <array>

namespace perpetual {

// Adaptive Radix Tree (ART) for efficient price-based order book
// Optimized for int64_t keys (Price)

enum class ARTNodeType {
    NODE4 = 0,
    NODE16 = 1,
    NODE48 = 2,
    NODE256 = 3,
    LEAF = 4
};

class ARTNode {
public:
    ARTNodeType type_;
    
    ARTNode(ARTNodeType type) : type_(type) {}
    virtual ~ARTNode() = default;
    
    // Get prefix length
    uint8_t prefix_len() const { return prefix_len_; }
    void set_prefix_len(uint8_t len) { prefix_len_ = len; }
    
    // Get prefix
    const uint8_t* prefix() const { return prefix_; }
    void set_prefix(const uint8_t* p, uint8_t len) {
        prefix_len_ = len;
        if (len > 0) {
            memcpy(prefix_, p, std::min(len, static_cast<uint8_t>(10)));
        }
    }
    
protected:
    uint8_t prefix_len_ = 0;
    uint8_t prefix_[10] = {0};  // Max prefix length for int64_t
};

// Leaf node stores the value (PriceLevel pointer)
class ARTLeaf : public ARTNode {
public:
    void* value_;
    
    ARTLeaf(void* value) : ARTNode(ARTNodeType::LEAF), value_(value) {}
};

// Node4: 4 children
class ARTNode4 : public ARTNode {
public:
    uint8_t keys_[4];
    ARTNode* children_[4];
    uint8_t count_;
    
    ARTNode4() : ARTNode(ARTNodeType::NODE4), count_(0) {
        memset(keys_, 0, 4);
        memset(children_, 0, sizeof(children_));
    }
};

// Node16: 16 children
class ARTNode16 : public ARTNode {
public:
    uint8_t keys_[16];
    ARTNode* children_[16];
    uint8_t count_;
    
    ARTNode16() : ARTNode(ARTNodeType::NODE16), count_(0) {
        memset(keys_, 0, 16);
        memset(children_, 0, sizeof(children_));
    }
};

// Node48: 256 children indexed via array
class ARTNode48 : public ARTNode {
public:
    uint8_t keys_[256];
    ARTNode* children_[48];
    uint8_t count_;
    
    ARTNode48() : ARTNode(ARTNodeType::NODE48), count_(0) {
        memset(keys_, 48, 256);  // 48 means empty
        memset(children_, 0, sizeof(children_));
    }
};

// Node256: 256 children (direct array)
class ARTNode256 : public ARTNode {
public:
    ARTNode* children_[256];
    
    ARTNode256() : ARTNode(ARTNodeType::NODE256) {
        memset(children_, 0, sizeof(children_));
    }
};

// Adaptive Radix Tree for Price keys
class ARTTree {
public:
    ARTTree();
    ~ARTTree();
    
    // Insert key-value pair
    bool insert(Price key, void* value);
    
    // Find value by key
    void* find(Price key) const;
    
    // Remove key
    bool remove(Price key);
    
    // Get minimum key
    Price min_key() const;
    
    // Get maximum key
    Price max_key() const;
    
    // Get successor (next key greater than given key)
    Price successor(Price key) const;
    
    // Get predecessor (next key less than given key)
    Price predecessor(Price key) const;
    
    // Check if empty
    bool empty() const { return root_ == nullptr; }
    
    // Get size (approximate)
    size_t size() const { return size_; }
    
    // Clear all nodes
    void clear();
    
private:
    // Helper functions
    void* find_recursive(ARTNode* node, const uint8_t* key, int depth) const;
    bool insert_recursive(ARTNode*& node, const uint8_t* key, int depth, void* value);
    bool remove_recursive(ARTNode*& node, const uint8_t* key, int depth);
    
    // Node management
    ARTNode* alloc_node(ARTNodeType type);
    void free_node(ARTNode* node);
    
    // Node operations
    void add_child(ARTNode* node, uint8_t byte, ARTNode* child);
    void remove_child(ARTNode* node, uint8_t byte);
    ARTNode* find_child(ARTNode* node, uint8_t byte) const;
    
    // Node expansion/contraction
    ARTNode* expand_node(ARTNode* node);
    ARTNode* shrink_node(ARTNode* node);
    
    // Prefix operations
    int check_prefix(ARTNode* node, const uint8_t* key, int depth) const;
    void copy_prefix(ARTNode* src, ARTNode* dst);
    
    // Key conversion
    void price_to_bytes(Price price, uint8_t* bytes) const;
    Price bytes_to_price(const uint8_t* bytes) const;
    
    // Min/max helpers
    Price min_key_recursive(ARTNode* node, int depth) const;
    Price max_key_recursive(ARTNode* node, int depth) const;
    
private:
    ARTNode* root_;
    size_t size_;
    
    // Memory pool for nodes (optional optimization)
    static constexpr size_t POOL_SIZE = 1024;
    std::vector<std::unique_ptr<ARTNode>> node_pool_;
};

} // namespace perpetual

