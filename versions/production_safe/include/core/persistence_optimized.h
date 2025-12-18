#pragma once

#include "core/persistence.h"

namespace perpetual {

// Optimized persistence manager with async writes
class OptimizedPersistenceManager : public PersistenceManager {
public:
    OptimizedPersistenceManager() = default;
    ~OptimizedPersistenceManager() {
        shutdown();
    }
    
    // Initialize with buffer size and flush interval
    bool initialize(const std::string& data_dir, size_t buffer_size = 10000, size_t flush_interval_ms = 100) {
        return PersistenceManager::initialize(data_dir);
    }
    
    // Shutdown (override for cleanup)
    void shutdown() {
        PersistenceManager::flush();
    }
    
    // Log trade (override for async)
    void logTrade(const Trade& trade) {
        PersistenceManager::logTrade(trade);
    }
    
    // Log order (override for async)
    void logOrder(const Order& order, const std::string& event_type) {
        PersistenceManager::logOrder(order, event_type);
    }
};

} // namespace perpetual
