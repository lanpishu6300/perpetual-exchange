#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>

namespace perpetual {

// Lock-free memory pool for Order objects
template<typename T, size_t BlockSize = 1024>
class MemoryPool {
public:
    MemoryPool() : free_list_(nullptr), total_allocated_(0) {
        // Pre-allocate initial block
        allocate_block();
    }
    
    ~MemoryPool() {
        // Free all blocks
        for (auto& block : blocks_) {
            delete[] reinterpret_cast<char*>(block);
        }
    }
    
    // Allocate an object
    T* allocate() {
        // Try lock-free allocation first
        Node* node = free_list_.load(std::memory_order_acquire);
        while (node != nullptr) {
            Node* next = node->next;
            if (free_list_.compare_exchange_weak(node, next, 
                                                 std::memory_order_release,
                                                 std::memory_order_relaxed)) {
                return reinterpret_cast<T*>(node);
            }
        }
        
        // Fallback to block allocation
        std::lock_guard<std::mutex> lock(mutex_);
        return allocate_from_new_block();
    }
    
    // Deallocate an object
    void deallocate(T* ptr) {
        if (!ptr) return;
        
        // Add to free list (lock-free)
        Node* node = reinterpret_cast<Node*>(ptr);
        Node* head = free_list_.load(std::memory_order_relaxed);
        do {
            node->next = head;
        } while (!free_list_.compare_exchange_weak(head, node,
                                                   std::memory_order_release,
                                                   std::memory_order_relaxed));
    }
    
    size_t total_allocated() const { return total_allocated_.load(); }
    size_t block_count() const { return blocks_.size(); }
    
private:
    struct Node {
        Node* next;
    };
    
    void allocate_block() {
        char* block = new char[sizeof(T) * BlockSize];
        blocks_.push_back(block);
        
        // Initialize free list
        for (size_t i = 0; i < BlockSize; ++i) {
            Node* node = reinterpret_cast<Node*>(block + i * sizeof(T));
            node->next = free_list_.load(std::memory_order_relaxed);
            free_list_.store(node, std::memory_order_release);
        }
        
        total_allocated_ += BlockSize;
    }
    
    T* allocate_from_new_block() {
        allocate_block();
        Node* node = free_list_.load(std::memory_order_acquire);
        if (node) {
            Node* next = node->next;
            free_list_.store(next, std::memory_order_release);
            return reinterpret_cast<T*>(node);
        }
        return nullptr;
    }
    
    std::atomic<Node*> free_list_;
    std::vector<char*> blocks_;
    std::mutex mutex_;
    std::atomic<size_t> total_allocated_;
};

// Thread-local memory pool for better performance
template<typename T>
class ThreadLocalMemoryPool {
public:
    T* allocate() {
        if (!pool_) {
            pool_ = std::make_unique<MemoryPool<T>>();
        }
        return pool_->allocate();
    }
    
    void deallocate(T* ptr) {
        if (pool_) {
            pool_->deallocate(ptr);
        }
    }
    
private:
    thread_local static std::unique_ptr<MemoryPool<T>> pool_;
};

template<typename T>
thread_local std::unique_ptr<MemoryPool<T>> ThreadLocalMemoryPool<T>::pool_;

} // namespace perpetual



#include <cstddef>
#include <cstdint>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>

namespace perpetual {

// Lock-free memory pool for Order objects
template<typename T, size_t BlockSize = 1024>
class MemoryPool {
public:
    MemoryPool() : free_list_(nullptr), total_allocated_(0) {
        // Pre-allocate initial block
        allocate_block();
    }
    
    ~MemoryPool() {
        // Free all blocks
        for (auto& block : blocks_) {
            delete[] reinterpret_cast<char*>(block);
        }
    }
    
    // Allocate an object
    T* allocate() {
        // Try lock-free allocation first
        Node* node = free_list_.load(std::memory_order_acquire);
        while (node != nullptr) {
            Node* next = node->next;
            if (free_list_.compare_exchange_weak(node, next, 
                                                 std::memory_order_release,
                                                 std::memory_order_relaxed)) {
                return reinterpret_cast<T*>(node);
            }
        }
        
        // Fallback to block allocation
        std::lock_guard<std::mutex> lock(mutex_);
        return allocate_from_new_block();
    }
    
    // Deallocate an object
    void deallocate(T* ptr) {
        if (!ptr) return;
        
        // Add to free list (lock-free)
        Node* node = reinterpret_cast<Node*>(ptr);
        Node* head = free_list_.load(std::memory_order_relaxed);
        do {
            node->next = head;
        } while (!free_list_.compare_exchange_weak(head, node,
                                                   std::memory_order_release,
                                                   std::memory_order_relaxed));
    }
    
    size_t total_allocated() const { return total_allocated_.load(); }
    size_t block_count() const { return blocks_.size(); }
    
private:
    struct Node {
        Node* next;
    };
    
    void allocate_block() {
        char* block = new char[sizeof(T) * BlockSize];
        blocks_.push_back(block);
        
        // Initialize free list
        for (size_t i = 0; i < BlockSize; ++i) {
            Node* node = reinterpret_cast<Node*>(block + i * sizeof(T));
            node->next = free_list_.load(std::memory_order_relaxed);
            free_list_.store(node, std::memory_order_release);
        }
        
        total_allocated_ += BlockSize;
    }
    
    T* allocate_from_new_block() {
        allocate_block();
        Node* node = free_list_.load(std::memory_order_acquire);
        if (node) {
            Node* next = node->next;
            free_list_.store(next, std::memory_order_release);
            return reinterpret_cast<T*>(node);
        }
        return nullptr;
    }
    
    std::atomic<Node*> free_list_;
    std::vector<char*> blocks_;
    std::mutex mutex_;
    std::atomic<size_t> total_allocated_;
};

// Thread-local memory pool for better performance
template<typename T>
class ThreadLocalMemoryPool {
public:
    T* allocate() {
        if (!pool_) {
            pool_ = std::make_unique<MemoryPool<T>>();
        }
        return pool_->allocate();
    }
    
    void deallocate(T* ptr) {
        if (pool_) {
            pool_->deallocate(ptr);
        }
    }
    
private:
    thread_local static std::unique_ptr<MemoryPool<T>> pool_;
};

template<typename T>
thread_local std::unique_ptr<MemoryPool<T>> ThreadLocalMemoryPool<T>::pool_;

} // namespace perpetual


