#pragma once

#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <cstddef>

namespace perpetual {

// Thread-Local内存池
// 每个线程独立的内存池，避免锁竞争
template<typename T>
class ThreadLocalMemoryPool {
public:
    ThreadLocalMemoryPool(size_t initial_size = 1000, size_t grow_size = 1000)
        : initial_size_(initial_size), grow_size_(grow_size) {
    }
    
    ~ThreadLocalMemoryPool() {
        // 清理所有线程的池
        std::lock_guard<std::mutex> lock(pools_mutex_);
        for (auto* pool : all_pools_) {
            delete pool;
        }
    }
    
    // 分配对象（从线程本地池）
    T* allocate() {
        Pool& pool = getLocalPool();
        
        if (pool.free_list_.empty()) {
            // 池为空，分配新对象
            growPool(pool);
        }
        
        if (pool.free_list_.empty()) {
            // 仍然为空，直接分配
            return new T();
        }
        
        T* obj = pool.free_list_.back();
        pool.free_list_.pop_back();
        pool.used_count_++;
        
        // 调用构造函数
        new (obj) T();
        
        return obj;
    }
    
    // 释放对象（返回到线程本地池）
    void deallocate(T* obj) {
        if (!obj) return;
        
        // 调用析构函数
        obj->~T();
        
        Pool& pool = getLocalPool();
        pool.free_list_.push_back(obj);
        pool.used_count_--;
    }
    
    // 获取统计信息
    struct Statistics {
        size_t total_allocated = 0;
        size_t total_used = 0;
        size_t total_free = 0;
        size_t pool_count = 0;
    };
    
    Statistics getStatistics() const {
        Statistics stats;
        std::lock_guard<std::mutex> lock(pools_mutex_);
        
        for (const auto* pool : all_pools_) {
            stats.total_allocated += pool->objects_.size();
            stats.total_used += pool->used_count_;
            stats.total_free += pool->free_list_.size();
        }
        stats.pool_count = all_pools_.size();
        
        return stats;
    }
    
private:
    struct Pool {
        std::vector<T*> objects_;      // 所有分配的对象
        std::vector<T*> free_list_;    // 空闲对象列表
        size_t used_count_ = 0;
        
        ~Pool() {
            for (T* obj : objects_) {
                delete obj;
            }
        }
    };
    
    // 获取线程本地池
    Pool& getLocalPool() {
        thread_local Pool* local_pool = nullptr;
        thread_local bool initialized = false;
        
        if (!initialized) {
            local_pool = new Pool();
            growPool(*local_pool, initial_size_);
            
            // 注册到全局列表
            std::lock_guard<std::mutex> lock(pools_mutex_);
            all_pools_.push_back(local_pool);
            
            initialized = true;
        }
        
        return *local_pool;
    }
    
    // 扩展池
    void growPool(Pool& pool, size_t count = 0) {
        if (count == 0) {
            count = grow_size_;
        }
        
        for (size_t i = 0; i < count; ++i) {
            T* obj = new T();
            pool.objects_.push_back(obj);
            pool.free_list_.push_back(obj);
        }
    }
    
    size_t initial_size_;
    size_t grow_size_;
    
    // 全局池列表（用于统计和清理）
    mutable std::mutex pools_mutex_;
    std::vector<Pool*> all_pools_;
};

} // namespace perpetual
