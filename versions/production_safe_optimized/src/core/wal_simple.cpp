#include "core/wal.h"
#include "core/types.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/uio.h>  // For writev
#include <sys/mman.h>  // For mmap
#include <cstring>
#include <sstream>
#include <fstream>
#include <cerrno>
#include <algorithm>

namespace perpetual {

WriteAheadLog::WriteAheadLog(const std::string& path) : path_(path) {
    // Create directory if not exists
    mkdir(path_.c_str(), 0755);
    
    // Open WAL file
    std::string wal_file = path_ + "/wal.log";
    wal_fd_ = open(wal_file.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    
    if (wal_fd_ < 0) {
        throw std::runtime_error("Failed to open WAL file: " + wal_file);
    }
    
    // Get current file size
    struct stat st;
    if (fstat(wal_fd_, &st) == 0) {
        current_offset_.store(st.st_size);
    }
}

WriteAheadLog::~WriteAheadLog() {
    // Stop async sync thread
    async_sync_running_ = false;
    async_sync_cv_.notify_all();
    if (async_sync_thread_.joinable()) {
        async_sync_thread_.join();
    }
    
    // Unmap if using mmap
    if (use_mmap_ && mmap_addr_ != nullptr) {
        // Sync before unmapping
        if (mmap_addr_ != MAP_FAILED) {
            msync(mmap_addr_, mmap_size_, MS_SYNC);
            munmap(mmap_addr_, mmap_size_);
        }
    }
    
    if (wal_fd_ >= 0) {
        close(wal_fd_);
    }
}

bool WriteAheadLog::append(const Order& order) {
    std::lock_guard<std::mutex> lock(write_mutex_);
    
    uint64_t offset = current_offset_.load();
    size_t data_size = sizeof(Order);
    
    // Check if we need to remap (for mmap mode)
    if (use_mmap_) {
        if (offset + data_size > mmap_capacity_) {
            remap_if_needed(offset + data_size);
        }
        
        if (mmap_addr_ != nullptr && mmap_addr_ != MAP_FAILED) {
            // Write to mmap
            memcpy(static_cast<char*>(mmap_addr_) + offset, &order, data_size);
            current_offset_.fetch_add(data_size, std::memory_order_release);
            return true;
        }
    }
    
    // Fallback to regular write
    ssize_t n = write(wal_fd_, &order, sizeof(Order));
    if (n != sizeof(Order)) {
        return false;
    }
    
    current_offset_.fetch_add(sizeof(Order), std::memory_order_relaxed);
    return true;
}

bool WriteAheadLog::append(const Trade& trade) {
    std::lock_guard<std::mutex> lock(write_mutex_);
    
    uint64_t offset = current_offset_.load();
    size_t data_size = sizeof(Trade);
    
    // Check if we need to remap (for mmap mode)
    if (use_mmap_) {
        if (offset + data_size > mmap_capacity_) {
            remap_if_needed(offset + data_size);
        }
        
        if (mmap_addr_ != nullptr && mmap_addr_ != MAP_FAILED) {
            // Write to mmap
            memcpy(static_cast<char*>(mmap_addr_) + offset, &trade, data_size);
            current_offset_.fetch_add(data_size, std::memory_order_release);
            return true;
        }
    }
    
    // Fallback to regular write
    ssize_t n = write(wal_fd_, &trade, sizeof(Trade));
    if (n != sizeof(Trade)) {
        return false;
    }
    
    current_offset_.fetch_add(sizeof(Trade), std::memory_order_relaxed);
    return true;
}

bool WriteAheadLog::append_batch(const std::vector<Order>& orders) {
    std::lock_guard<std::mutex> lock(write_mutex_);
    
    if (orders.empty()) {
        return true;
    }
    
    uint64_t offset = current_offset_.load();
    size_t data_size = orders.size() * sizeof(Order);
    
    // Check if we need to remap (for mmap mode)
    if (use_mmap_) {
        if (offset + data_size > mmap_capacity_) {
            remap_if_needed(offset + data_size);
        }
        
        if (mmap_addr_ != nullptr && mmap_addr_ != MAP_FAILED) {
            // Write to mmap (batch copy)
            char* dest = static_cast<char*>(mmap_addr_) + offset;
            for (const auto& order : orders) {
                memcpy(dest, &order, sizeof(Order));
                dest += sizeof(Order);
            }
            current_offset_.fetch_add(data_size, std::memory_order_release);
            return true;
        }
    }
    
    // Fallback to writev() for regular file
    const size_t max_iovec = 1024;
    
    if (orders.size() <= max_iovec) {
        // Build iovec array for writev()
        std::vector<struct iovec> iovs;
        iovs.reserve(orders.size());
        
        for (const auto& order : orders) {
            struct iovec iov;
            iov.iov_base = const_cast<void*>(static_cast<const void*>(&order));
            iov.iov_len = sizeof(Order);
            iovs.push_back(iov);
        }
        
        // Single writev() call for all orders
        ssize_t total_written = writev(wal_fd_, iovs.data(), static_cast<int>(iovs.size()));
        if (total_written < 0) {
            return false;
        }
        
        // Verify all bytes were written
        size_t expected_bytes = orders.size() * sizeof(Order);
        if (static_cast<size_t>(total_written) != expected_bytes) {
            return false;
        }
        
        current_offset_.fetch_add(expected_bytes, std::memory_order_relaxed);
    } else {
        // For very large batches, process in chunks
        for (size_t i = 0; i < orders.size(); i += max_iovec) {
            size_t chunk_size = std::min(max_iovec, orders.size() - i);
            std::vector<struct iovec> iovs;
            iovs.reserve(chunk_size);
            
            for (size_t j = 0; j < chunk_size; ++j) {
                struct iovec iov;
                iov.iov_base = const_cast<void*>(static_cast<const void*>(&orders[i + j]));
                iov.iov_len = sizeof(Order);
                iovs.push_back(iov);
            }
            
            ssize_t total_written = writev(wal_fd_, iovs.data(), static_cast<int>(iovs.size()));
            if (total_written < 0) {
                return false;
            }
            
            size_t expected_bytes = chunk_size * sizeof(Order);
            if (static_cast<size_t>(total_written) != expected_bytes) {
                return false;
            }
            
            current_offset_.fetch_add(expected_bytes, std::memory_order_relaxed);
        }
    }
    
    return true;
}

bool WriteAheadLog::append_batch_trades(const std::vector<Trade>& trades) {
    std::lock_guard<std::mutex> lock(write_mutex_);
    
    if (trades.empty()) {
        return true;
    }
    
    uint64_t offset = current_offset_.load();
    size_t data_size = trades.size() * sizeof(Trade);
    
    // Check if we need to remap (for mmap mode)
    if (use_mmap_) {
        if (offset + data_size > mmap_capacity_) {
            remap_if_needed(offset + data_size);
        }
        
        if (mmap_addr_ != nullptr && mmap_addr_ != MAP_FAILED) {
            // Write to mmap (batch copy)
            char* dest = static_cast<char*>(mmap_addr_) + offset;
            for (const auto& trade : trades) {
                memcpy(dest, &trade, sizeof(Trade));
                dest += sizeof(Trade);
            }
            current_offset_.fetch_add(data_size, std::memory_order_release);
            return true;
        }
    }
    
    // Fallback to writev() for regular file
    const size_t max_iovec = 1024;
    
    if (trades.size() <= max_iovec) {
        std::vector<struct iovec> iovs;
        iovs.reserve(trades.size());
        
        for (const auto& trade : trades) {
            struct iovec iov;
            iov.iov_base = const_cast<void*>(static_cast<const void*>(&trade));
            iov.iov_len = sizeof(Trade);
            iovs.push_back(iov);
        }
        
        ssize_t total_written = writev(wal_fd_, iovs.data(), static_cast<int>(iovs.size()));
        if (total_written < 0 || static_cast<size_t>(total_written) != data_size) {
            return false;
        }
        
        current_offset_.fetch_add(data_size, std::memory_order_relaxed);
    } else {
        // Process in chunks
        for (size_t i = 0; i < trades.size(); i += max_iovec) {
            size_t chunk_size = std::min(max_iovec, trades.size() - i);
            std::vector<struct iovec> iovs;
            iovs.reserve(chunk_size);
            
            for (size_t j = 0; j < chunk_size; ++j) {
                struct iovec iov;
                iov.iov_base = const_cast<void*>(static_cast<const void*>(&trades[i + j]));
                iov.iov_len = sizeof(Trade);
                iovs.push_back(iov);
            }
            
            ssize_t total_written = writev(wal_fd_, iovs.data(), static_cast<int>(iovs.size()));
            if (total_written < 0) {
                return false;
            }
            
            size_t expected_bytes = chunk_size * sizeof(Trade);
            if (static_cast<size_t>(total_written) != expected_bytes) {
                return false;
            }
            
            current_offset_.fetch_add(expected_bytes, std::memory_order_relaxed);
        }
    }
    
    return true;
}

void WriteAheadLog::sync() {
    if (use_mmap_ && mmap_addr_ != nullptr) {
        // Sync mmap
        if (msync(mmap_addr_, mmap_size_, MS_SYNC) != 0) {
            throw std::runtime_error("msync failed: " + std::string(strerror(errno)));
        }
    } else {
        // Sync file
        if (fsync(wal_fd_) != 0) {
            throw std::runtime_error("fsync failed: " + std::string(strerror(errno)));
        }
    }
    last_synced_offset_.store(current_offset_.load(), std::memory_order_release);
}

void WriteAheadLog::async_sync() {
    if (!async_sync_running_.load()) {
        // Start async sync thread if not running
        async_sync_running_ = true;
        async_sync_thread_ = std::thread(&WriteAheadLog::async_sync_worker, this);
    }
    
    // Add current offset to sync queue
    uint64_t offset = current_offset_.load();
    {
        std::lock_guard<std::mutex> lock(async_sync_mutex_);
        async_sync_queue_.push(offset);
    }
    async_sync_cv_.notify_one();
}

void WriteAheadLog::wait_async_sync() {
    uint64_t target_offset = current_offset_.load();
    
    // Wait until target offset is synced
    while (last_synced_offset_.load(std::memory_order_acquire) < target_offset) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
}

void WriteAheadLog::async_sync_worker() {
    while (async_sync_running_.load() || !async_sync_queue_.empty()) {
        std::unique_lock<std::mutex> lock(async_sync_mutex_);
        
        // Wait for sync request or shutdown
        async_sync_cv_.wait(lock, [this]() {
            return !async_sync_queue_.empty() || !async_sync_running_.load();
        });
        
        // Process all pending sync requests
        while (!async_sync_queue_.empty()) {
            uint64_t offset = async_sync_queue_.front();
            async_sync_queue_.pop();
            lock.unlock();
            
            // Perform sync
            try {
                if (use_mmap_ && mmap_addr_ != nullptr) {
                    msync(mmap_addr_, offset, MS_ASYNC);
                } else {
                    fsync(wal_fd_);
                }
                last_synced_offset_.store(offset, std::memory_order_release);
            } catch (...) {
                // Log error but continue
            }
            
            lock.lock();
        }
    }
}

bool WriteAheadLog::enable_mmap(size_t initial_size) {
    if (use_mmap_) {
        return true;  // Already enabled
    }
    
    // Ensure file is large enough
    if (ftruncate(wal_fd_, initial_size) != 0) {
        return false;
    }
    
    // Map file to memory
    mmap_addr_ = mmap(nullptr, initial_size, PROT_WRITE, MAP_SHARED, wal_fd_, 0);
    if (mmap_addr_ == MAP_FAILED) {
        return false;
    }
    
    use_mmap_ = true;
    mmap_size_ = initial_size;
    mmap_capacity_ = initial_size;
    
    return true;
}

void WriteAheadLog::remap_if_needed(size_t required_size) {
    if (!use_mmap_ || required_size <= mmap_capacity_) {
        return;
    }
    
    // Unmap old mapping
    if (mmap_addr_ != nullptr && mmap_addr_ != MAP_FAILED) {
        msync(mmap_addr_, mmap_size_, MS_SYNC);
        munmap(mmap_addr_, mmap_size_);
    }
    
    // Calculate new size (double the capacity)
    size_t new_size = std::max(required_size * 2, mmap_capacity_ * 2);
    
    // Extend file
    if (ftruncate(wal_fd_, new_size) != 0) {
        return;
    }
    
    // Remap
    mmap_addr_ = mmap(nullptr, new_size, PROT_WRITE, MAP_SHARED, wal_fd_, 0);
    if (mmap_addr_ != MAP_FAILED) {
        mmap_size_ = new_size;
        mmap_capacity_ = new_size;
    }
}

void WriteAheadLog::mark_committed(Timestamp timestamp) {
    last_committed_ts_ = timestamp;
}

std::vector<Order> WriteAheadLog::read_uncommitted_orders() {
    return std::vector<Order>();  // Simplified
}

std::vector<Trade> WriteAheadLog::read_uncommitted_trades() {
    return std::vector<Trade>();  // Simplified
}

void WriteAheadLog::truncate() {
    // Simplified
}

uint64_t WriteAheadLog::uncommitted_count() const {
    return current_offset_.load() / sizeof(Order);
}

std::string WriteAheadLog::serialize_order(const Order* order) {
    return "";  // Simplified
}

std::string WriteAheadLog::serialize_trade(const Trade* trade) {
    return "";  // Simplified
}

void WriteAheadLog::deserialize_order(const std::string& data, Order* order) {
    // Simplified
}

void WriteAheadLog::deserialize_trade(const std::string& data, Trade* trade) {
    // Simplified
}

uint32_t WriteAheadLog::calculate_checksum(const std::string& data) {
    return 0;  // Simplified
}

bool WriteAheadLog::verify_checksum(const std::string& data, uint32_t expected) {
    return true;  // Simplified
}

bool WriteAheadLog::write_record(WALRecord::Type type, const std::string& data, Timestamp ts) {
    return true;  // Simplified
}

Timestamp WriteAheadLog::read_last_committed() {
    return 0;
}

void WriteAheadLog::write_last_committed(Timestamp ts) {
    // Simplified
}

} // namespace perpetual
