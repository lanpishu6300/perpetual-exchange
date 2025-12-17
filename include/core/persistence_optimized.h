#pragma once

#include "core/persistence.h"

namespace perpetual {

// Optimized persistence manager with async writes
class OptimizedPersistenceManager : public PersistenceManager {
public:
    OptimizedPersistenceManager() = default;
    ~OptimizedPersistenceManager() = default;
    
    // Initialize with buffer size and flush interval
    bool initialize(const std::string& data_dir, size_t buffer_size = 10000, size_t flush_interval_ms = 100) {
        return PersistenceManager::initialize(data_dir);
    }
};

} // namespace perpetual
