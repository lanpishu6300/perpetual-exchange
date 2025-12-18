#include "core/art_tree.h"
#include <algorithm>
#include <cassert>

namespace perpetual {

ARTTree::ARTTree() : root_(nullptr), size_(0) {
}

ARTTree::~ARTTree() {
    clear();
}

void ARTTree::price_to_bytes(Price price, uint8_t* bytes) const {
    // Convert int64_t to bytes (big-endian for lexicographic ordering)
    for (int i = 0; i < 8; ++i) {
        bytes[i] = (price >> (56 - i * 8)) & 0xFF;
    }
}

Price ARTTree::bytes_to_price(const uint8_t* bytes) const {
    Price price = 0;
    for (int i = 0; i < 8; ++i) {
        price |= static_cast<Price>(bytes[i]) << (56 - i * 8);
    }
    return price;
}

void* ARTTree::find(Price key) const {
    if (root_ == nullptr) {
        return nullptr;
    }
    
    uint8_t key_bytes[8];
    price_to_bytes(key, key_bytes);
    return find_recursive(root_, key_bytes, 0);
}

void* ARTTree::find_recursive(ARTNode* node, const uint8_t* key, int depth) const {
    if (node == nullptr) {
        return nullptr;
    }
    
    // Check if leaf
    if (node->type_ == ARTNodeType::LEAF) {
        ARTLeaf* leaf = static_cast<ARTLeaf*>(node);
        return leaf->value_;
    }
    
    // Check prefix
    int prefix_diff = check_prefix(node, key, depth);
    if (prefix_diff != node->prefix_len()) {
        return nullptr;  // Prefix mismatch
    }
    depth += node->prefix_len();
    
    // Find child
    ARTNode* child = find_child(node, key[depth]);
    if (child == nullptr) {
        return nullptr;
    }
    
    return find_recursive(child, key, depth + 1);
}

bool ARTTree::insert(Price key, void* value) {
    uint8_t key_bytes[8];
    price_to_bytes(key, key_bytes);
    
    if (root_ == nullptr) {
        root_ = new ARTLeaf(value);
        size_++;
        return true;
    }
    
    bool result = insert_recursive(root_, key_bytes, 0, value);
    if (result) {
        size_++;
    }
    return result;
}

bool ARTTree::insert_recursive(ARTNode*& node, const uint8_t* key, int depth, void* value) {
    if (node == nullptr) {
        node = new ARTLeaf(value);
        return true;
    }
    
    // If leaf, need to split
    if (node->type_ == ARTNodeType::LEAF) {
        ARTLeaf* leaf = static_cast<ARTLeaf*>(node);
        uint8_t leaf_key[8];
        price_to_bytes(reinterpret_cast<Price>(leaf->value_), leaf_key);
        
        // Check if same key
        if (memcmp(key, leaf_key, 8) == 0) {
            leaf->value_ = value;  // Update value
            return false;  // Not a new insertion
        }
        
        // Create new node
        int prefix_len = 0;
        while (depth + prefix_len < 8 && key[depth + prefix_len] == leaf_key[depth + prefix_len]) {
            prefix_len++;
        }
        
        ARTNode4* new_node = new ARTNode4();
        new_node->set_prefix(key + depth, prefix_len);
        depth += prefix_len;
        
        add_child(new_node, key[depth], new ARTLeaf(value));
        add_child(new_node, leaf_key[depth], node);
        
        node = new_node;
        return true;
    }
    
    // Check prefix
    int prefix_diff = check_prefix(node, key, depth);
    if (prefix_diff != node->prefix_len()) {
        // Split prefix
        ARTNode4* new_node = new ARTNode4();
        new_node->set_prefix(node->prefix(), prefix_diff);
        
        node->set_prefix(node->prefix() + prefix_diff + 1, node->prefix_len() - prefix_diff - 1);
        add_child(new_node, node->prefix()[prefix_diff], node);
        add_child(new_node, key[depth + prefix_diff], new ARTLeaf(value));
        
        node = new_node;
        return true;
    }
    depth += node->prefix_len();
    
    // Find or create child
    ARTNode* child = find_child(node, key[depth]);
    if (child == nullptr) {
        add_child(node, key[depth], new ARTLeaf(value));
        return true;
    }
    
    return insert_recursive(child, key, depth + 1, value);
}

bool ARTTree::remove(Price key) {
    if (root_ == nullptr) {
        return false;
    }
    
    uint8_t key_bytes[8];
    price_to_bytes(key, key_bytes);
    
    bool result = remove_recursive(root_, key_bytes, 0);
    if (result) {
        size_--;
    }
    return result;
}

bool ARTTree::remove_recursive(ARTNode*& node, const uint8_t* key, int depth) {
    if (node == nullptr) {
        return false;
    }
    
    if (node->type_ == ARTNodeType::LEAF) {
        delete node;
        node = nullptr;
        return true;
    }
    
    int prefix_diff = check_prefix(node, key, depth);
    if (prefix_diff != node->prefix_len()) {
        return false;
    }
    depth += node->prefix_len();
    
    ARTNode* child = find_child(node, key[depth]);
    if (child == nullptr) {
        return false;
    }
    
    bool result = remove_recursive(child, key, depth + 1);
    if (!result) {
        return false;
    }
    
    // Remove child
    remove_child(node, key[depth]);
    
    // Shrink if needed
    if (node->type_ != ARTNodeType::NODE4) {
        ARTNode* shrunk = shrink_node(node);
        if (shrunk != node) {
            delete node;
            node = shrunk;
        }
    }
    
    return true;
}

Price ARTTree::min_key() const {
    if (root_ == nullptr) {
        return 0;
    }
    return min_key_recursive(root_, 0);
}

Price ARTTree::max_key() const {
    if (root_ == nullptr) {
        return 0;
    }
    return max_key_recursive(root_, 0);
}

Price ARTTree::min_key_recursive(ARTNode* node, int depth) const {
    if (node->type_ == ARTNodeType::LEAF) {
        ARTLeaf* leaf = static_cast<ARTLeaf*>(node);
        return reinterpret_cast<Price>(leaf->value_);
    }
    
    // Find leftmost child
    ARTNode* child = nullptr;
    if (node->type_ == ARTNodeType::NODE4) {
        ARTNode4* n4 = static_cast<ARTNode4*>(node);
        if (n4->count_ > 0) {
            child = n4->children_[0];
        }
    } else if (node->type_ == ARTNodeType::NODE16) {
        ARTNode16* n16 = static_cast<ARTNode16*>(node);
        if (n16->count_ > 0) {
            child = n16->children_[0];
        }
    } else if (node->type_ == ARTNodeType::NODE48) {
        ARTNode48* n48 = static_cast<ARTNode48*>(node);
        for (int i = 0; i < 256; ++i) {
            if (n48->keys_[i] != 48) {
                child = n48->children_[n48->keys_[i]];
                break;
            }
        }
    } else if (node->type_ == ARTNodeType::NODE256) {
        ARTNode256* n256 = static_cast<ARTNode256*>(node);
        for (int i = 0; i < 256; ++i) {
            if (n256->children_[i] != nullptr) {
                child = n256->children_[i];
                break;
            }
        }
    }
    
    if (child == nullptr) {
        return 0;
    }
    
    return min_key_recursive(child, depth + 1);
}

Price ARTTree::max_key_recursive(ARTNode* node, int depth) const {
    if (node->type_ == ARTNodeType::LEAF) {
        ARTLeaf* leaf = static_cast<ARTLeaf*>(node);
        return reinterpret_cast<Price>(leaf->value_);
    }
    
    // Find rightmost child
    ARTNode* child = nullptr;
    if (node->type_ == ARTNodeType::NODE4) {
        ARTNode4* n4 = static_cast<ARTNode4*>(node);
        if (n4->count_ > 0) {
            child = n4->children_[n4->count_ - 1];
        }
    } else if (node->type_ == ARTNodeType::NODE16) {
        ARTNode16* n16 = static_cast<ARTNode16*>(node);
        if (n16->count_ > 0) {
            child = n16->children_[n16->count_ - 1];
        }
    } else if (node->type_ == ARTNodeType::NODE48) {
        ARTNode48* n48 = static_cast<ARTNode48*>(node);
        for (int i = 255; i >= 0; --i) {
            if (n48->keys_[i] != 48) {
                child = n48->children_[n48->keys_[i]];
                break;
            }
        }
    } else if (node->type_ == ARTNodeType::NODE256) {
        ARTNode256* n256 = static_cast<ARTNode256*>(node);
        for (int i = 255; i >= 0; --i) {
            if (n256->children_[i] != nullptr) {
                child = n256->children_[i];
                break;
            }
        }
    }
    
    if (child == nullptr) {
        return 0;
    }
    
    return max_key_recursive(child, depth + 1);
}

Price ARTTree::successor(Price key) const {
    // Simplified: find next key greater than given key
    // For full implementation, would need to traverse tree
    return max_key();  // Placeholder
}

Price ARTTree::predecessor(Price key) const {
    // Simplified: find next key less than given key
    return min_key();  // Placeholder
}

int ARTTree::check_prefix(ARTNode* node, const uint8_t* key, int depth) const {
    int prefix_len = static_cast<int>(node->prefix_len());
    int remaining = 8 - depth;
    int max_cmp = (prefix_len < remaining) ? prefix_len : remaining;
    for (int i = 0; i < max_cmp; ++i) {
        if (node->prefix()[i] != key[depth + i]) {
            return i;
        }
    }
    return max_cmp;
}

void ARTTree::add_child(ARTNode* node, uint8_t byte, ARTNode* child) {
    if (node->type_ == ARTNodeType::NODE4) {
        ARTNode4* n4 = static_cast<ARTNode4*>(node);
        if (n4->count_ < 4) {
            int pos = 0;
            while (pos < n4->count_ && n4->keys_[pos] < byte) {
                pos++;
            }
            memmove(&n4->keys_[pos + 1], &n4->keys_[pos], n4->count_ - pos);
            memmove(&n4->children_[pos + 1], &n4->children_[pos], (n4->count_ - pos) * sizeof(ARTNode*));
            n4->keys_[pos] = byte;
            n4->children_[pos] = child;
            n4->count_++;
        } else {
            // Expand to Node16
            node = expand_node(node);
            add_child(node, byte, child);
        }
    } else if (node->type_ == ARTNodeType::NODE16) {
        ARTNode16* n16 = static_cast<ARTNode16*>(node);
        if (n16->count_ < 16) {
            int pos = 0;
            while (pos < n16->count_ && n16->keys_[pos] < byte) {
                pos++;
            }
            memmove(&n16->keys_[pos + 1], &n16->keys_[pos], n16->count_ - pos);
            memmove(&n16->children_[pos + 1], &n16->children_[pos], (n16->count_ - pos) * sizeof(ARTNode*));
            n16->keys_[pos] = byte;
            n16->children_[pos] = child;
            n16->count_++;
        } else {
            node = expand_node(node);
            add_child(node, byte, child);
        }
    } else if (node->type_ == ARTNodeType::NODE48) {
        ARTNode48* n48 = static_cast<ARTNode48*>(node);
        if (n48->count_ < 48) {
            int pos = 0;
            while (pos < 48 && n48->children_[pos] != nullptr) {
                pos++;
            }
            n48->keys_[byte] = pos;
            n48->children_[pos] = child;
            n48->count_++;
        } else {
            node = expand_node(node);
            add_child(node, byte, child);
        }
    } else if (node->type_ == ARTNodeType::NODE256) {
        ARTNode256* n256 = static_cast<ARTNode256*>(node);
        n256->children_[byte] = child;
    }
}

void ARTTree::remove_child(ARTNode* node, uint8_t byte) {
    if (node->type_ == ARTNodeType::NODE4) {
        ARTNode4* n4 = static_cast<ARTNode4*>(node);
        for (int i = 0; i < n4->count_; ++i) {
            if (n4->keys_[i] == byte) {
                memmove(&n4->keys_[i], &n4->keys_[i + 1], n4->count_ - i - 1);
                memmove(&n4->children_[i], &n4->children_[i + 1], (n4->count_ - i - 1) * sizeof(ARTNode*));
                n4->count_--;
                break;
            }
        }
    } else if (node->type_ == ARTNodeType::NODE16) {
        ARTNode16* n16 = static_cast<ARTNode16*>(node);
        for (int i = 0; i < n16->count_; ++i) {
            if (n16->keys_[i] == byte) {
                memmove(&n16->keys_[i], &n16->keys_[i + 1], n16->count_ - i - 1);
                memmove(&n16->children_[i], &n16->children_[i + 1], (n16->count_ - i - 1) * sizeof(ARTNode*));
                n16->count_--;
                break;
            }
        }
    } else if (node->type_ == ARTNodeType::NODE48) {
        ARTNode48* n48 = static_cast<ARTNode48*>(node);
        if (n48->keys_[byte] != 48) {
            int pos = n48->keys_[byte];
            n48->children_[pos] = nullptr;
            n48->keys_[byte] = 48;
            n48->count_--;
        }
    } else if (node->type_ == ARTNodeType::NODE256) {
        ARTNode256* n256 = static_cast<ARTNode256*>(node);
        n256->children_[byte] = nullptr;
    }
}

ARTNode* ARTTree::find_child(ARTNode* node, uint8_t byte) const {
    if (node->type_ == ARTNodeType::NODE4) {
        ARTNode4* n4 = static_cast<ARTNode4*>(node);
        for (int i = 0; i < n4->count_; ++i) {
            if (n4->keys_[i] == byte) {
                return n4->children_[i];
            }
        }
    } else if (node->type_ == ARTNodeType::NODE16) {
        ARTNode16* n16 = static_cast<ARTNode16*>(node);
        for (int i = 0; i < n16->count_; ++i) {
            if (n16->keys_[i] == byte) {
                return n16->children_[i];
            }
        }
    } else if (node->type_ == ARTNodeType::NODE48) {
        ARTNode48* n48 = static_cast<ARTNode48*>(node);
        if (n48->keys_[byte] != 48) {
            return n48->children_[n48->keys_[byte]];
        }
    } else if (node->type_ == ARTNodeType::NODE256) {
        ARTNode256* n256 = static_cast<ARTNode256*>(node);
        return n256->children_[byte];
    }
    return nullptr;
}

ARTNode* ARTTree::expand_node(ARTNode* node) {
    // Simplified: just return node (full implementation would expand)
    return node;
}

ARTNode* ARTTree::shrink_node(ARTNode* node) {
    // Simplified: just return node (full implementation would shrink)
    return node;
}

void ARTTree::clear() {
    // Simplified cleanup (full implementation would recursively delete)
    root_ = nullptr;
    size_ = 0;
}

} // namespace perpetual
