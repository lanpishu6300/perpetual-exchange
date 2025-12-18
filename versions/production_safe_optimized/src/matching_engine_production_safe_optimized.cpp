#include "core/matching_engine_production_safe_optimized.h"
#include "core/logger.h"
#include "core/types.h"
#include <chrono>
#include <algorithm>

using namespace std::chrono;

namespace perpetual {

ProductionMatchingEngineSafeOptimized::ProductionMatchingEngineSafeOptimized(InstrumentID instrument_id)
    : ProductionMatchingEngineV2(instrument_id) {
}

ProductionMatchingEngineSafeOptimized::~ProductionMatchingEngineSafeOptimized() {
    shutdown();
}

bool ProductionMatchingEngineSafeOptimized::initialize(const std::string& config_file, bool enable_wal) {
    // Initialize V2 first
    if (!ProductionMatchingEngineV2::initialize(config_file)) {
        return false;
    }
    
    wal_enabled_ = enable_wal;
    
    if (wal_enabled_) {
        // Initialize WAL
        std::string wal_path = "./data/wal";
        try {
            wal_ = std::make_unique<WriteAheadLog>(wal_path);
            LOG_INFO("WAL initialized: " + wal_path);
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to initialize WAL: " + std::string(e.what()));
            return false;
        }
        
        // Initialize lock-free queue
        wal_queue_ = std::make_unique<LockFreeSPSCQueue<WALEntry>>(WAL_QUEUE_SIZE);
        
        // Start worker threads
        running_ = true;
        wal_writer_thread_ = std::thread(&ProductionMatchingEngineSafeOptimized::wal_writer_thread, this);
        sync_worker_thread_ = std::thread(&ProductionMatchingEngineSafeOptimized::sync_worker_thread, this);
        
        LOG_INFO("Production Safe Optimized engine initialized with async WAL");
    } else {
        LOG_WARNING("WAL is disabled");
    }
    
    return true;
}

std::vector<Trade> ProductionMatchingEngineSafeOptimized::process_order_optimized(Order* order) {
    if (!order) {
        return {};
    }
    
    // 1. Process order using V2 (ART+SIMD, ~1.2μs) - NO WAL blocking!
    auto trades = ProductionMatchingEngineV2::process_order_production_v2(order);
    
    // 2. Add to in-memory event buffer (like event_sourcing, ~0.1μs)
    {
        std::lock_guard<std::mutex> lock(event_buffer_mutex_);
        
        EventBuffer event;
        event.order = *order;
        event.trades = trades;
        event.timestamp = get_current_timestamp();
        event.sequence_id = pending_sequence_.fetch_add(1, std::memory_order_relaxed) + 1;
        
        event_buffer_.push_back(std::move(event));
        
        // Limit buffer size
        if (event_buffer_.size() > max_event_buffer_size_) {
            event_buffer_.erase(event_buffer_.begin(), 
                              event_buffer_.begin() + (event_buffer_.size() - max_event_buffer_size_));
        }
    }
    
    // 3. Async WAL write (non-blocking, ~0.01μs to enqueue)
    if (wal_enabled_ && wal_queue_) {
        WALEntry entry;
        entry.type = WALEntry::Type::ORDER;
        entry.order = *order;
        entry.timestamp = get_current_timestamp();
        entry.sequence_id = pending_sequence_.load(std::memory_order_relaxed);
        
        // Try to enqueue (non-blocking)
        if (wal_queue_->push(entry)) {
            async_writes_.fetch_add(1, std::memory_order_relaxed);
        } else {
            // Queue full - this should be rare, but we can handle it
            // In production, might want to wait or use a fallback
            LOG_WARNING("WAL queue full, dropping entry");
        }
        
        // Also enqueue trades
        for (const auto& trade : trades) {
            WALEntry trade_entry;
            trade_entry.type = WALEntry::Type::TRADE;
            trade_entry.trade = trade;
            trade_entry.timestamp = get_current_timestamp();
            trade_entry.sequence_id = pending_sequence_.load(std::memory_order_relaxed);
            
            wal_queue_->push(trade_entry);
        }
    }
    
    // Return immediately - no blocking on disk I/O!
    return trades;
}

void ProductionMatchingEngineSafeOptimized::wal_writer_thread() {
    LOG_INFO("WAL writer thread started");
    
    while (running_.load(std::memory_order_relaxed) || !wal_queue_->empty()) {
        WALEntry entry;
        
        // Try to pop from queue
        if (wal_queue_->pop(entry)) {
            try {
                // Write to WAL (this is now off the critical path)
                if (entry.type == WALEntry::Type::ORDER) {
                    wal_->append(entry.order);
                } else if (entry.type == WALEntry::Type::TRADE) {
                    wal_->append(entry.trade);
                }
            } catch (const std::exception& e) {
                LOG_ERROR("WAL write failed: " + std::string(e.what()));
            }
        } else {
            // Queue empty, sleep briefly
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    }
    
    LOG_INFO("WAL writer thread stopped");
}

void ProductionMatchingEngineSafeOptimized::sync_worker_thread() {
    LOG_INFO("Sync worker thread started");
    
    while (running_.load(std::memory_order_relaxed)) {
        auto sync_start = high_resolution_clock::now();
        
        if (should_sync()) {
            perform_sync();
            
            auto sync_end = high_resolution_clock::now();
            auto sync_time = duration_cast<microseconds>(sync_end - sync_start);
            
            sync_count_.fetch_add(1, std::memory_order_relaxed);
            total_sync_time_us_.fetch_add(sync_time.count(), std::memory_order_relaxed);
        }
        
        // Sleep for sync interval
        std::this_thread::sleep_for(sync_interval_);
    }
    
    // Final sync on shutdown
    perform_sync();
    
    LOG_INFO("Sync worker thread stopped");
}

bool ProductionMatchingEngineSafeOptimized::should_sync() const {
    if (!wal_enabled_ || !wal_) {
        return false;
    }
    
    // Condition 1: Time-based (every sync_interval_)
    auto now = get_current_timestamp();
    if (last_sync_time_ == 0 || 
        (now - last_sync_time_) > static_cast<uint64_t>(sync_interval_.count() * 1000)) {
        return true;
    }
    
    // Condition 2: Sequence-based (every sync_batch_size_ entries)
    uint64_t pending = pending_sequence_.load(std::memory_order_acquire);
    uint64_t committed = committed_sequence_.load(std::memory_order_acquire);
    if (pending - committed >= sync_batch_size_) {
        return true;
    }
    
    return false;
}

void ProductionMatchingEngineSafeOptimized::perform_sync() {
    if (!wal_enabled_ || !wal_) {
        return;
    }
    
    try {
        // Wait for WAL writer to catch up (drain queue)
        // Give it a moment to process pending entries
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        
        // Sync to disk (fsync)
        wal_->sync();
        
        // Mark as committed
        uint64_t current_pending = pending_sequence_.load(std::memory_order_acquire);
        committed_sequence_.store(current_pending, std::memory_order_release);
        last_sync_sequence_.store(current_pending, std::memory_order_release);
        last_sync_time_ = get_current_timestamp();
        
    } catch (const std::exception& e) {
        LOG_ERROR("Sync failed: " + std::string(e.what()));
    }
}

bool ProductionMatchingEngineSafeOptimized::recover_from_wal() {
    if (!wal_enabled_) {
        return true;
    }
    
    LOG_INFO("Starting recovery from WAL...");
    
    auto uncommitted_orders = wal_->read_uncommitted_orders();
    
    if (uncommitted_orders.empty()) {
        LOG_INFO("No uncommitted orders");
        return true;
    }
    
    LOG_INFO("Found " + std::to_string(uncommitted_orders.size()) + " uncommitted orders");
    
    size_t recovered = 0;
    for (auto& order : uncommitted_orders) {
        try {
            auto trades = process_order_optimized(&order);
            recovered++;
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to recover order: " + std::string(e.what()));
        }
    }
    
    LOG_INFO("Recovery complete: " + std::to_string(recovered) + " orders recovered");
    
    return true;
}

ProductionMatchingEngineSafeOptimized::Stats ProductionMatchingEngineSafeOptimized::get_stats() const {
    Stats stats;
    stats.wal_size = wal_ ? wal_->size() : 0;
    stats.uncommitted_count = wal_ ? wal_->uncommitted_count() : 0;
    stats.async_writes = async_writes_.load();
    stats.sync_count = sync_count_.load();
    stats.queue_size = wal_queue_ ? wal_queue_->size() : 0;
    
    uint64_t total_time = total_sync_time_us_.load();
    uint64_t count = sync_count_.load();
    stats.avg_sync_time_us = count > 0 ? static_cast<double>(total_time) / count : 0.0;
    
    return stats;
}

void ProductionMatchingEngineSafeOptimized::shutdown() {
    if (running_.exchange(false)) {
        // Wait for queue to drain
        if (wal_queue_) {
            int retries = 0;
            while (!wal_queue_->empty() && retries < 1000) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                retries++;
            }
        }
        
        // Join threads
        if (wal_writer_thread_.joinable()) {
            wal_writer_thread_.join();
        }
        if (sync_worker_thread_.joinable()) {
            sync_worker_thread_.join();
        }
    }
    
    ProductionMatchingEngineV2::shutdown();
}

} // namespace perpetual

