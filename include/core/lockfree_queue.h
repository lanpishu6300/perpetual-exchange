#pragma once

#include <atomic>
#include <cstddef>
#include <thread>
#ifdef __APPLE__
#include <mach/thread_policy.h>
#include <mach/thread_act.h>
#include <pthread.h>
#endif

namespace perpetual {

// Lock-free single-producer single-consumer queue
template<typename T>
class LockFreeSPSCQueue {
public:
    explicit LockFreeSPSCQueue(size_t capacity)
        : capacity_(capacity), mask_(capacity - 1) {
        // Capacity must be power of 2
        if ((capacity & mask_) != 0) {
            capacity_ = 1;
            while (capacity_ < capacity) {
                capacity_ <<= 1;
            }
            mask_ = capacity_ - 1;
        }
        
        buffer_ = new T[capacity_];
        write_pos_.store(0, std::memory_order_relaxed);
        read_pos_.store(0, std::memory_order_relaxed);
    }
    
    ~LockFreeSPSCQueue() {
        delete[] buffer_;
    }
    
    bool push(const T& item) {
        size_t current_write = write_pos_.load(std::memory_order_relaxed);
        size_t next_write = (current_write + 1) & mask_;
        
        // Check if queue is full
        if (next_write == read_pos_.load(std::memory_order_acquire)) {
            return false;  // Queue full
        }
        
        buffer_[current_write] = item;
        write_pos_.store(next_write, std::memory_order_release);
        return true;
    }
    
    bool pop(T& item) {
        size_t current_read = read_pos_.load(std::memory_order_relaxed);
        
        // Check if queue is empty
        if (current_read == write_pos_.load(std::memory_order_acquire)) {
            return false;  // Queue empty
        }
        
        item = buffer_[current_read];
        read_pos_.store((current_read + 1) & mask_, std::memory_order_release);
        return true;
    }
    
    bool empty() const {
        return read_pos_.load(std::memory_order_acquire) == 
               write_pos_.load(std::memory_order_acquire);
    }
    
    size_t size() const {
        size_t write = write_pos_.load(std::memory_order_acquire);
        size_t read = read_pos_.load(std::memory_order_acquire);
        return (write - read) & mask_;
    }
    
private:
    T* buffer_;
    size_t capacity_;
    size_t mask_;
    alignas(64) std::atomic<size_t> write_pos_;  // Cache line aligned
    alignas(64) std::atomic<size_t> read_pos_;   // Cache line aligned
};

// Lock-free multi-producer multi-consumer queue (using CAS)
template<typename T>
class LockFreeMPMCQueue {
public:
    explicit LockFreeMPMCQueue(size_t capacity)
        : capacity_(capacity), mask_(capacity - 1) {
        if ((capacity & mask_) != 0) {
            capacity_ = 1;
            while (capacity_ < capacity) {
                capacity_ <<= 1;
            }
            mask_ = capacity_ - 1;
        }
        
        buffer_ = new std::atomic<T*>[capacity_];
        for (size_t i = 0; i < capacity_; ++i) {
            buffer_[i].store(nullptr, std::memory_order_relaxed);
        }
        
        write_pos_.store(0, std::memory_order_relaxed);
        read_pos_.store(0, std::memory_order_relaxed);
    }
    
    ~LockFreeMPMCQueue() {
        delete[] buffer_;
    }
    
    bool push(const T& item) {
        T* item_ptr = new T(item);
        size_t pos = write_pos_.fetch_add(1, std::memory_order_relaxed);
        size_t index = pos & mask_;
        
        // Wait until slot is available
        T* expected = nullptr;
        while (!buffer_[index].compare_exchange_weak(expected, item_ptr,
                                                     std::memory_order_release,
                                                     std::memory_order_relaxed)) {
            expected = nullptr;
            std::this_thread::yield();
        }
        
        return true;
    }
    
    bool pop(T& item) {
        size_t pos = read_pos_.fetch_add(1, std::memory_order_relaxed);
        size_t index = pos & mask_;
        
        // Wait until item is available
        T* item_ptr = nullptr;
        while (!(item_ptr = buffer_[index].exchange(nullptr,
                                                    std::memory_order_acquire))) {
            std::this_thread::yield();
        }
        
        item = *item_ptr;
        delete item_ptr;
        return true;
    }
    
private:
    std::atomic<T*>* buffer_;
    size_t capacity_;
    size_t mask_;
    alignas(64) std::atomic<size_t> write_pos_;
    alignas(64) std::atomic<size_t> read_pos_;
};

} // namespace perpetual

#include <atomic>
#include <cstddef>
#include <thread>
#ifdef __APPLE__
#include <mach/thread_policy.h>
#include <mach/thread_act.h>
#include <pthread.h>
#endif

namespace perpetual {

// Lock-free single-producer single-consumer queue
template<typename T>
class LockFreeSPSCQueue {
public:
    explicit LockFreeSPSCQueue(size_t capacity)
        : capacity_(capacity), mask_(capacity - 1) {
        // Capacity must be power of 2
        if ((capacity & mask_) != 0) {
            capacity_ = 1;
            while (capacity_ < capacity) {
                capacity_ <<= 1;
            }
            mask_ = capacity_ - 1;
        }
        
        buffer_ = new T[capacity_];
        write_pos_.store(0, std::memory_order_relaxed);
        read_pos_.store(0, std::memory_order_relaxed);
    }
    
    ~LockFreeSPSCQueue() {
        delete[] buffer_;
    }
    
    bool push(const T& item) {
        size_t current_write = write_pos_.load(std::memory_order_relaxed);
        size_t next_write = (current_write + 1) & mask_;
        
        // Check if queue is full
        if (next_write == read_pos_.load(std::memory_order_acquire)) {
            return false;  // Queue full
        }
        
        buffer_[current_write] = item;
        write_pos_.store(next_write, std::memory_order_release);
        return true;
    }
    
    bool pop(T& item) {
        size_t current_read = read_pos_.load(std::memory_order_relaxed);
        
        // Check if queue is empty
        if (current_read == write_pos_.load(std::memory_order_acquire)) {
            return false;  // Queue empty
        }
        
        item = buffer_[current_read];
        read_pos_.store((current_read + 1) & mask_, std::memory_order_release);
        return true;
    }
    
    bool empty() const {
        return read_pos_.load(std::memory_order_acquire) == 
               write_pos_.load(std::memory_order_acquire);
    }
    
    size_t size() const {
        size_t write = write_pos_.load(std::memory_order_acquire);
        size_t read = read_pos_.load(std::memory_order_acquire);
        return (write - read) & mask_;
    }
    
private:
    T* buffer_;
    size_t capacity_;
    size_t mask_;
    alignas(64) std::atomic<size_t> write_pos_;  // Cache line aligned
    alignas(64) std::atomic<size_t> read_pos_;   // Cache line aligned
};

// Lock-free multi-producer multi-consumer queue (using CAS)
template<typename T>
class LockFreeMPMCQueue {
public:
    explicit LockFreeMPMCQueue(size_t capacity)
        : capacity_(capacity), mask_(capacity - 1) {
        if ((capacity & mask_) != 0) {
            capacity_ = 1;
            while (capacity_ < capacity) {
                capacity_ <<= 1;
            }
            mask_ = capacity_ - 1;
        }
        
        buffer_ = new std::atomic<T*>[capacity_];
        for (size_t i = 0; i < capacity_; ++i) {
            buffer_[i].store(nullptr, std::memory_order_relaxed);
        }
        
        write_pos_.store(0, std::memory_order_relaxed);
        read_pos_.store(0, std::memory_order_relaxed);
    }
    
    ~LockFreeMPMCQueue() {
        delete[] buffer_;
    }
    
    bool push(const T& item) {
        T* item_ptr = new T(item);
        size_t pos = write_pos_.fetch_add(1, std::memory_order_relaxed);
        size_t index = pos & mask_;
        
        // Wait until slot is available
        T* expected = nullptr;
        while (!buffer_[index].compare_exchange_weak(expected, item_ptr,
                                                     std::memory_order_release,
                                                     std::memory_order_relaxed)) {
            expected = nullptr;
            std::this_thread::yield();
        }
        
        return true;
    }
    
    bool pop(T& item) {
        size_t pos = read_pos_.fetch_add(1, std::memory_order_relaxed);
        size_t index = pos & mask_;
        
        // Wait until item is available
        T* item_ptr = nullptr;
        while (!(item_ptr = buffer_[index].exchange(nullptr,
                                                    std::memory_order_acquire))) {
            std::this_thread::yield();
        }
        
        item = *item_ptr;
        delete item_ptr;
        return true;
    }
    
private:
    std::atomic<T*>* buffer_;
    size_t capacity_;
    size_t mask_;
    alignas(64) std::atomic<size_t> write_pos_;
    alignas(64) std::atomic<size_t> read_pos_;
};

} // namespace perpetual
