# 性能优化代码示例

本文档提供可以直接应用的优化代码示例。

## 优化1：移除 ensure_wal_written() 从关键路径

### 修改前（当前代码）

```cpp
// 在 process_order_optimized() 中
if (wal_queue_->push(entry)) {
    async_writes_.fetch_add(1, std::memory_order_relaxed);
    
    // ... 处理trades ...
    
    // ❌ 问题：这个检查在关键路径上，有开销
    ensure_wal_written(seq_id);
}
```

### 修改后（优化代码）

```cpp
// 在 process_order_optimized() 中
if (wal_queue_->push(entry)) {
    async_writes_.fetch_add(1, std::memory_order_relaxed);
    
    // ... 处理trades ...
    
    // ✅ 优化：移除 ensure_wal_written，依赖批量同步保证
    // 如果需要零数据丢失，应该使用 process_order_guaranteed_zero_loss()
    // ensure_wal_written(seq_id);  // 已移除
}
```

**预期收益**：延迟降低 0.5-1.0 μs，吞吐量提升 5-10%

---

## 优化2：优化 sync_write_critical() 中的等待

### 修改前（当前代码）

```cpp
void ProductionMatchingEngineSafeOptimized::sync_write_critical(...) {
    // 1. Wait for WAL writer to catch up
    if (wal_queue_ && !wal_queue_->empty()) {
        // ❌ 问题：固定sleep 100μs，即使队列很快处理完
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    
    // ... 写入和sync ...
}
```

### 修改后（优化代码）

```cpp
void ProductionMatchingEngineSafeOptimized::sync_write_critical(...) {
    // 1. Wait for WAL writer to catch up (智能等待)
    if (wal_queue_ && !wal_queue_->empty()) {
        // ✅ 优化：使用yield + 有限重试，而不是固定sleep
        int retries = 0;
        const int max_retries = 10;  // 最多等待10次yield
        while (!wal_queue_->empty() && retries < max_retries) {
            std::this_thread::yield();
            retries++;
        }
    }
    
    // ... 写入和sync ...
}
```

**预期收益**：关键订单延迟降低 50-100 μs

---

## 优化3：WAL Writer 批量处理

### 修改前（当前代码）

```cpp
void ProductionMatchingEngineSafeOptimized::wal_writer_thread() {
    while (running_.load(std::memory_order_relaxed) || !wal_queue_->empty()) {
        WALEntry entry;
        
        // ❌ 问题：逐个处理，每次write()都有系统调用开销
        if (wal_queue_->pop(entry)) {
            if (entry.type == WALEntry::Type::ORDER) {
                wal_->append(entry.order);
            } else if (entry.type == WALEntry::Type::TRADE) {
                wal_->append(entry.trade);
            }
            last_written_sequence_.store(entry.sequence_id, std::memory_order_release);
        } else {
            std::this_thread::yield();
        }
    }
}
```

### 修改后（优化代码）

```cpp
void ProductionMatchingEngineSafeOptimized::wal_writer_thread() {
    LOG_INFO("WAL writer thread started");
    
    const size_t BATCH_SIZE = 100;  // 批量处理100个条目
    std::vector<WALEntry> batch;
    batch.reserve(BATCH_SIZE);
    
    std::vector<Order> orders;
    std::vector<Trade> trades;
    orders.reserve(BATCH_SIZE);
    trades.reserve(BATCH_SIZE);
    
    while (running_.load(std::memory_order_relaxed) || !wal_queue_->empty()) {
        // ✅ 优化：批量收集条目
        batch.clear();
        orders.clear();
        trades.clear();
        
        for (size_t i = 0; i < BATCH_SIZE; ++i) {
            WALEntry entry;
            if (wal_queue_->pop(entry)) {
                batch.push_back(entry);
            } else {
                break;  // 队列为空
            }
        }
        
        if (!batch.empty()) {
            // ✅ 优化：批量写入
            uint64_t max_seq = 0;
            
            // 分类收集
            for (const auto& entry : batch) {
                if (entry.sequence_id > max_seq) {
                    max_seq = entry.sequence_id;
                }
                
                if (entry.type == WALEntry::Type::ORDER) {
                    orders.push_back(entry.order);
                } else if (entry.type == WALEntry::Type::TRADE) {
                    trades.push_back(entry.trade);
                }
            }
            
            // 批量写入orders（如果WAL支持）
            if (!orders.empty()) {
                for (const auto& order : orders) {
                    wal_->append(order);
                }
            }
            
            // 批量写入trades（如果WAL支持）
            if (!trades.empty()) {
                for (const auto& trade : trades) {
                    wal_->append(trade);
                }
            }
            
            // 更新序列号
            last_written_sequence_.store(max_seq, std::memory_order_release);
        } else {
            std::this_thread::yield();
        }
    }
    
    LOG_INFO("WAL writer thread stopped");
}
```

**预期收益**：WAL writer 吞吐量提升 50-100%，减少系统调用

**进一步优化**：如果WAL支持 `append_batch()`，可以使用批量写入：

```cpp
// 如果WAL支持批量写入
if (!orders.empty() && wal_->append_batch(orders)) {
    // 成功
}
```

---

## 优化4：使用 Move 语义避免拷贝

### 修改前（当前代码）

```cpp
// 在 process_order_optimized() 中
WALEntry entry;
entry.type = WALEntry::Type::ORDER;
entry.order = *order;  // ❌ 拷贝
entry.timestamp = ts;
entry.sequence_id = seq_id;

wal_queue_->push(entry);  // ❌ 再次拷贝

// 对于trades
for (const auto& trade : trades) {
    WALEntry trade_entry;
    trade_entry.type = WALEntry::Type::TRADE;
    trade_entry.trade = trade;  // ❌ 拷贝
    trade_entry.timestamp = ts;
    trade_entry.sequence_id = seq_id;
    
    wal_queue_->push(trade_entry);  // ❌ 再次拷贝
}
```

### 修改后（优化代码）

```cpp
// 在 process_order_optimized() 中
WALEntry entry;
entry.type = WALEntry::Type::ORDER;
entry.order = *order;  // 这里仍然需要拷贝（因为order可能被修改）
entry.timestamp = ts;
entry.sequence_id = seq_id;

wal_queue_->push(std::move(entry));  // ✅ 使用move

// 对于trades
for (const auto& trade : trades) {
    WALEntry trade_entry;
    trade_entry.type = WALEntry::Type::TRADE;
    trade_entry.trade = trade;  // 这里仍然需要拷贝（因为trade在vector中）
    trade_entry.timestamp = ts;
    trade_entry.sequence_id = seq_id;
    
    wal_queue_->push(std::move(trade_entry));  // ✅ 使用move
}
```

**注意**：由于 `LockFreeSPSCQueue::push()` 使用的是 `const T&`，move可能不会完全生效。需要检查队列实现。

**如果队列支持move，可以进一步优化**：

```cpp
// 需要修改队列接口支持emplace或move
wal_queue_->push(std::move(entry));  // 如果队列支持
```

**预期收益**：延迟降低 0.1-0.2 μs（如果队列支持move）

---

## 优化5：CPU 亲和性绑定

### 添加CPU亲和性支持

```cpp
// 在头文件中添加
#include <pthread.h>
#include <sched.h>

// 在类中添加辅助方法
private:
    void set_thread_affinity(std::thread& thread, int cpu_id) {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(cpu_id, &cpuset);
        
        int result = pthread_setaffinity_np(
            thread.native_handle(),
            sizeof(cpu_set_t),
            &cpuset
        );
        
        if (result != 0) {
            LOG_WARNING("Failed to set thread affinity for CPU " + 
                       std::to_string(cpu_id));
        }
    }
```

### 在初始化时绑定

```cpp
bool ProductionMatchingEngineSafeOptimized::initialize(...) {
    // ... 现有代码 ...
    
    if (wal_enabled_) {
        // ... WAL初始化 ...
        
        // Start worker threads
        running_ = true;
        wal_writer_thread_ = std::thread(
            &ProductionMatchingEngineSafeOptimized::wal_writer_thread, this);
        sync_worker_thread_ = std::thread(
            &ProductionMatchingEngineSafeOptimized::sync_worker_thread, this);
        
        // ✅ 优化：绑定线程到特定CPU核心
        // 假设主线程使用CPU 0，WAL writer使用CPU 1，sync worker使用CPU 2
        set_thread_affinity(wal_writer_thread_, 1);
        set_thread_affinity(sync_worker_thread_, 2);
        
        LOG_INFO("Production Safe Optimized engine initialized with async WAL");
    }
    
    return true;
}
```

**预期收益**：吞吐量提升 5-10%，减少缓存失效

**注意**：
- 需要根据实际CPU核心数调整
- 在某些系统上可能需要特殊权限
- 可以使用配置文件来设置CPU绑定

---

## 优化6：使用更轻量的时间戳（可选）

### 添加TSC时间戳支持

```cpp
// 在类中添加辅助方法
private:
    inline uint64_t get_tsc_timestamp() {
#ifdef __x86_64__
        return __rdtsc();
#elif defined(__aarch64__)
        uint64_t val;
        asm volatile("mrs %0, cntvct_el0" : "=r" (val));
        return val;
#else
        // Fallback to normal timestamp
        return get_current_timestamp();
#endif
    }
    
    // 如果使用TSC，需要转换为实际时间戳
    // 可以在启动时校准一次
    uint64_t tsc_to_timestamp(uint64_t tsc) {
        // 需要根据CPU频率转换
        // 这里需要额外的校准逻辑
        return tsc / tsc_frequency_mhz_;
    }
```

**注意**：TSC需要校准，实施复杂度较高。建议作为可选优化。

---

## 完整优化代码示例

以下是完整的 `process_order_optimized()` 优化版本：

```cpp
std::vector<Trade> ProductionMatchingEngineSafeOptimized::process_order_optimized(Order* order) {
    if (!order) {
        return {};
    }
    
    // 1. Process order using V2 (ART+SIMD, ~1.2μs) - NO WAL blocking!
    auto trades = ProductionMatchingEngineV2::process_order_production_v2(order);
    
    // 2. Update sequence (removed event buffer mutex from critical path for performance)
    uint64_t seq_id = pending_sequence_.fetch_add(1, std::memory_order_relaxed) + 1;
    
    // 3. Check if critical order (optimized: check trades first, fastest path)
    bool is_critical = !trades.empty() || (zero_loss_mode_);
    
    // Only check thresholds if not already critical
    if (!is_critical && (critical_quantity_threshold_ > 0 || critical_order_threshold_ > 0)) {
        is_critical = is_critical_order(order, trades);
    }
    
    if (is_critical && wal_enabled_) {
        // Critical order: sync write immediately (zero data loss)
        sync_write_critical(order, trades);
        sync_writes_.fetch_add(1, std::memory_order_relaxed);
    } else {
        // Normal order: async WAL write
        if (wal_enabled_ && wal_queue_) {
            // Optimize: get timestamp once, reuse for all entries
            Timestamp ts = get_current_timestamp();
            
            WALEntry entry;
            entry.type = WALEntry::Type::ORDER;
            entry.order = *order;
            entry.timestamp = ts;
            entry.sequence_id = seq_id;
            
            // Try to enqueue (non-blocking)
            if (wal_queue_->push(entry)) {
                async_writes_.fetch_add(1, std::memory_order_relaxed);
                
                // Also enqueue trades (only if there are trades)
                if (!trades.empty()) {
                    for (const auto& trade : trades) {
                        WALEntry trade_entry;
                        trade_entry.type = WALEntry::Type::TRADE;
                        trade_entry.trade = trade;
                        trade_entry.timestamp = ts;  // Reuse timestamp
                        trade_entry.sequence_id = seq_id;  // Reuse sequence
                        
                        wal_queue_->push(trade_entry);
                    }
                }
                
                // ✅ 优化：移除了 ensure_wal_written()，依赖批量同步保证
                // 如果需要零数据丢失，使用 process_order_guaranteed_zero_loss()
            } else {
                // Queue full - fallback to sync write for safety
                sync_write_critical(order, trades);
                sync_writes_.fetch_add(1, std::memory_order_relaxed);
            }
        }
    }
    
    // Return immediately - no blocking on disk I/O for normal orders!
    return trades;
}
```

---

## 实施建议

1. **逐步实施**：一次实施一个优化，测试验证后再继续
2. **性能测试**：每个优化后都运行benchmark，记录性能变化
3. **功能测试**：确保数据安全性不受影响
4. **回退机制**：保留可配置开关，方便回退

## 测试命令

```bash
# 运行基准测试
cd versions/production_safe_optimized
mkdir -p build && cd build
cmake ..
make production_safe_optimized_benchmark
./production_safe_optimized_benchmark

# 对比优化前后性能
# 记录吞吐量、延迟等指标
```




本文档提供可以直接应用的优化代码示例。

## 优化1：移除 ensure_wal_written() 从关键路径

### 修改前（当前代码）

```cpp
// 在 process_order_optimized() 中
if (wal_queue_->push(entry)) {
    async_writes_.fetch_add(1, std::memory_order_relaxed);
    
    // ... 处理trades ...
    
    // ❌ 问题：这个检查在关键路径上，有开销
    ensure_wal_written(seq_id);
}
```

### 修改后（优化代码）

```cpp
// 在 process_order_optimized() 中
if (wal_queue_->push(entry)) {
    async_writes_.fetch_add(1, std::memory_order_relaxed);
    
    // ... 处理trades ...
    
    // ✅ 优化：移除 ensure_wal_written，依赖批量同步保证
    // 如果需要零数据丢失，应该使用 process_order_guaranteed_zero_loss()
    // ensure_wal_written(seq_id);  // 已移除
}
```

**预期收益**：延迟降低 0.5-1.0 μs，吞吐量提升 5-10%

---

## 优化2：优化 sync_write_critical() 中的等待

### 修改前（当前代码）

```cpp
void ProductionMatchingEngineSafeOptimized::sync_write_critical(...) {
    // 1. Wait for WAL writer to catch up
    if (wal_queue_ && !wal_queue_->empty()) {
        // ❌ 问题：固定sleep 100μs，即使队列很快处理完
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    
    // ... 写入和sync ...
}
```

### 修改后（优化代码）

```cpp
void ProductionMatchingEngineSafeOptimized::sync_write_critical(...) {
    // 1. Wait for WAL writer to catch up (智能等待)
    if (wal_queue_ && !wal_queue_->empty()) {
        // ✅ 优化：使用yield + 有限重试，而不是固定sleep
        int retries = 0;
        const int max_retries = 10;  // 最多等待10次yield
        while (!wal_queue_->empty() && retries < max_retries) {
            std::this_thread::yield();
            retries++;
        }
    }
    
    // ... 写入和sync ...
}
```

**预期收益**：关键订单延迟降低 50-100 μs

---

## 优化3：WAL Writer 批量处理

### 修改前（当前代码）

```cpp
void ProductionMatchingEngineSafeOptimized::wal_writer_thread() {
    while (running_.load(std::memory_order_relaxed) || !wal_queue_->empty()) {
        WALEntry entry;
        
        // ❌ 问题：逐个处理，每次write()都有系统调用开销
        if (wal_queue_->pop(entry)) {
            if (entry.type == WALEntry::Type::ORDER) {
                wal_->append(entry.order);
            } else if (entry.type == WALEntry::Type::TRADE) {
                wal_->append(entry.trade);
            }
            last_written_sequence_.store(entry.sequence_id, std::memory_order_release);
        } else {
            std::this_thread::yield();
        }
    }
}
```

### 修改后（优化代码）

```cpp
void ProductionMatchingEngineSafeOptimized::wal_writer_thread() {
    LOG_INFO("WAL writer thread started");
    
    const size_t BATCH_SIZE = 100;  // 批量处理100个条目
    std::vector<WALEntry> batch;
    batch.reserve(BATCH_SIZE);
    
    std::vector<Order> orders;
    std::vector<Trade> trades;
    orders.reserve(BATCH_SIZE);
    trades.reserve(BATCH_SIZE);
    
    while (running_.load(std::memory_order_relaxed) || !wal_queue_->empty()) {
        // ✅ 优化：批量收集条目
        batch.clear();
        orders.clear();
        trades.clear();
        
        for (size_t i = 0; i < BATCH_SIZE; ++i) {
            WALEntry entry;
            if (wal_queue_->pop(entry)) {
                batch.push_back(entry);
            } else {
                break;  // 队列为空
            }
        }
        
        if (!batch.empty()) {
            // ✅ 优化：批量写入
            uint64_t max_seq = 0;
            
            // 分类收集
            for (const auto& entry : batch) {
                if (entry.sequence_id > max_seq) {
                    max_seq = entry.sequence_id;
                }
                
                if (entry.type == WALEntry::Type::ORDER) {
                    orders.push_back(entry.order);
                } else if (entry.type == WALEntry::Type::TRADE) {
                    trades.push_back(entry.trade);
                }
            }
            
            // 批量写入orders（如果WAL支持）
            if (!orders.empty()) {
                for (const auto& order : orders) {
                    wal_->append(order);
                }
            }
            
            // 批量写入trades（如果WAL支持）
            if (!trades.empty()) {
                for (const auto& trade : trades) {
                    wal_->append(trade);
                }
            }
            
            // 更新序列号
            last_written_sequence_.store(max_seq, std::memory_order_release);
        } else {
            std::this_thread::yield();
        }
    }
    
    LOG_INFO("WAL writer thread stopped");
}
```

**预期收益**：WAL writer 吞吐量提升 50-100%，减少系统调用

**进一步优化**：如果WAL支持 `append_batch()`，可以使用批量写入：

```cpp
// 如果WAL支持批量写入
if (!orders.empty() && wal_->append_batch(orders)) {
    // 成功
}
```

---

## 优化4：使用 Move 语义避免拷贝

### 修改前（当前代码）

```cpp
// 在 process_order_optimized() 中
WALEntry entry;
entry.type = WALEntry::Type::ORDER;
entry.order = *order;  // ❌ 拷贝
entry.timestamp = ts;
entry.sequence_id = seq_id;

wal_queue_->push(entry);  // ❌ 再次拷贝

// 对于trades
for (const auto& trade : trades) {
    WALEntry trade_entry;
    trade_entry.type = WALEntry::Type::TRADE;
    trade_entry.trade = trade;  // ❌ 拷贝
    trade_entry.timestamp = ts;
    trade_entry.sequence_id = seq_id;
    
    wal_queue_->push(trade_entry);  // ❌ 再次拷贝
}
```

### 修改后（优化代码）

```cpp
// 在 process_order_optimized() 中
WALEntry entry;
entry.type = WALEntry::Type::ORDER;
entry.order = *order;  // 这里仍然需要拷贝（因为order可能被修改）
entry.timestamp = ts;
entry.sequence_id = seq_id;

wal_queue_->push(std::move(entry));  // ✅ 使用move

// 对于trades
for (const auto& trade : trades) {
    WALEntry trade_entry;
    trade_entry.type = WALEntry::Type::TRADE;
    trade_entry.trade = trade;  // 这里仍然需要拷贝（因为trade在vector中）
    trade_entry.timestamp = ts;
    trade_entry.sequence_id = seq_id;
    
    wal_queue_->push(std::move(trade_entry));  // ✅ 使用move
}
```

**注意**：由于 `LockFreeSPSCQueue::push()` 使用的是 `const T&`，move可能不会完全生效。需要检查队列实现。

**如果队列支持move，可以进一步优化**：

```cpp
// 需要修改队列接口支持emplace或move
wal_queue_->push(std::move(entry));  // 如果队列支持
```

**预期收益**：延迟降低 0.1-0.2 μs（如果队列支持move）

---

## 优化5：CPU 亲和性绑定

### 添加CPU亲和性支持

```cpp
// 在头文件中添加
#include <pthread.h>
#include <sched.h>

// 在类中添加辅助方法
private:
    void set_thread_affinity(std::thread& thread, int cpu_id) {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(cpu_id, &cpuset);
        
        int result = pthread_setaffinity_np(
            thread.native_handle(),
            sizeof(cpu_set_t),
            &cpuset
        );
        
        if (result != 0) {
            LOG_WARNING("Failed to set thread affinity for CPU " + 
                       std::to_string(cpu_id));
        }
    }
```

### 在初始化时绑定

```cpp
bool ProductionMatchingEngineSafeOptimized::initialize(...) {
    // ... 现有代码 ...
    
    if (wal_enabled_) {
        // ... WAL初始化 ...
        
        // Start worker threads
        running_ = true;
        wal_writer_thread_ = std::thread(
            &ProductionMatchingEngineSafeOptimized::wal_writer_thread, this);
        sync_worker_thread_ = std::thread(
            &ProductionMatchingEngineSafeOptimized::sync_worker_thread, this);
        
        // ✅ 优化：绑定线程到特定CPU核心
        // 假设主线程使用CPU 0，WAL writer使用CPU 1，sync worker使用CPU 2
        set_thread_affinity(wal_writer_thread_, 1);
        set_thread_affinity(sync_worker_thread_, 2);
        
        LOG_INFO("Production Safe Optimized engine initialized with async WAL");
    }
    
    return true;
}
```

**预期收益**：吞吐量提升 5-10%，减少缓存失效

**注意**：
- 需要根据实际CPU核心数调整
- 在某些系统上可能需要特殊权限
- 可以使用配置文件来设置CPU绑定

---

## 优化6：使用更轻量的时间戳（可选）

### 添加TSC时间戳支持

```cpp
// 在类中添加辅助方法
private:
    inline uint64_t get_tsc_timestamp() {
#ifdef __x86_64__
        return __rdtsc();
#elif defined(__aarch64__)
        uint64_t val;
        asm volatile("mrs %0, cntvct_el0" : "=r" (val));
        return val;
#else
        // Fallback to normal timestamp
        return get_current_timestamp();
#endif
    }
    
    // 如果使用TSC，需要转换为实际时间戳
    // 可以在启动时校准一次
    uint64_t tsc_to_timestamp(uint64_t tsc) {
        // 需要根据CPU频率转换
        // 这里需要额外的校准逻辑
        return tsc / tsc_frequency_mhz_;
    }
```

**注意**：TSC需要校准，实施复杂度较高。建议作为可选优化。

---

## 完整优化代码示例

以下是完整的 `process_order_optimized()` 优化版本：

```cpp
std::vector<Trade> ProductionMatchingEngineSafeOptimized::process_order_optimized(Order* order) {
    if (!order) {
        return {};
    }
    
    // 1. Process order using V2 (ART+SIMD, ~1.2μs) - NO WAL blocking!
    auto trades = ProductionMatchingEngineV2::process_order_production_v2(order);
    
    // 2. Update sequence (removed event buffer mutex from critical path for performance)
    uint64_t seq_id = pending_sequence_.fetch_add(1, std::memory_order_relaxed) + 1;
    
    // 3. Check if critical order (optimized: check trades first, fastest path)
    bool is_critical = !trades.empty() || (zero_loss_mode_);
    
    // Only check thresholds if not already critical
    if (!is_critical && (critical_quantity_threshold_ > 0 || critical_order_threshold_ > 0)) {
        is_critical = is_critical_order(order, trades);
    }
    
    if (is_critical && wal_enabled_) {
        // Critical order: sync write immediately (zero data loss)
        sync_write_critical(order, trades);
        sync_writes_.fetch_add(1, std::memory_order_relaxed);
    } else {
        // Normal order: async WAL write
        if (wal_enabled_ && wal_queue_) {
            // Optimize: get timestamp once, reuse for all entries
            Timestamp ts = get_current_timestamp();
            
            WALEntry entry;
            entry.type = WALEntry::Type::ORDER;
            entry.order = *order;
            entry.timestamp = ts;
            entry.sequence_id = seq_id;
            
            // Try to enqueue (non-blocking)
            if (wal_queue_->push(entry)) {
                async_writes_.fetch_add(1, std::memory_order_relaxed);
                
                // Also enqueue trades (only if there are trades)
                if (!trades.empty()) {
                    for (const auto& trade : trades) {
                        WALEntry trade_entry;
                        trade_entry.type = WALEntry::Type::TRADE;
                        trade_entry.trade = trade;
                        trade_entry.timestamp = ts;  // Reuse timestamp
                        trade_entry.sequence_id = seq_id;  // Reuse sequence
                        
                        wal_queue_->push(trade_entry);
                    }
                }
                
                // ✅ 优化：移除了 ensure_wal_written()，依赖批量同步保证
                // 如果需要零数据丢失，使用 process_order_guaranteed_zero_loss()
            } else {
                // Queue full - fallback to sync write for safety
                sync_write_critical(order, trades);
                sync_writes_.fetch_add(1, std::memory_order_relaxed);
            }
        }
    }
    
    // Return immediately - no blocking on disk I/O for normal orders!
    return trades;
}
```

---

## 实施建议

1. **逐步实施**：一次实施一个优化，测试验证后再继续
2. **性能测试**：每个优化后都运行benchmark，记录性能变化
3. **功能测试**：确保数据安全性不受影响
4. **回退机制**：保留可配置开关，方便回退

## 测试命令

```bash
# 运行基准测试
cd versions/production_safe_optimized
mkdir -p build && cd build
cmake ..
make production_safe_optimized_benchmark
./production_safe_optimized_benchmark

# 对比优化前后性能
# 记录吞吐量、延迟等指标
```



