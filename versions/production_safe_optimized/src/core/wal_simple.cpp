#include "core/wal.h"
#include "core/types.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/uio.h>  // For writev
#include <cstring>
#include <sstream>
#include <fstream>
#include <cerrno>

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
    if (wal_fd_ >= 0) {
        close(wal_fd_);
    }
}

bool WriteAheadLog::append(const Order& order) {
    // No mutex needed - single writer thread (wal_writer_thread)
    // This is safe because only one thread calls append() from the queue
    
    // Simple append (in real implementation, use proper serialization)
    ssize_t n = write(wal_fd_, &order, sizeof(Order));
    if (n != sizeof(Order)) {
        return false;
    }
    
    current_offset_.fetch_add(sizeof(Order), std::memory_order_relaxed);
    return true;
}

bool WriteAheadLog::append(const Trade& trade) {
    // No mutex needed - single writer thread (wal_writer_thread)
    
    ssize_t n = write(wal_fd_, &trade, sizeof(Trade));
    if (n != sizeof(Trade)) {
        return false;
    }
    
    current_offset_.fetch_add(sizeof(Trade), std::memory_order_relaxed);
    return true;
}

bool WriteAheadLog::append_batch(const std::vector<Order>& orders) {
    // Optimized batch write using writev() for better performance
    // No mutex needed - single writer thread
    
    if (orders.empty()) {
        return true;
    }
    
    // Use writev() for small batches (up to IOV_MAX), fallback to loop for very large batches
    const size_t max_iovec = 1024;  // Reasonable limit (IOV_MAX is typically 1024)
    
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
    // Optimized batch write using writev() for better performance
    // No mutex needed - single writer thread
    
    if (trades.empty()) {
        return true;
    }
    
    // Use writev() for small batches (up to IOV_MAX), fallback to loop for very large batches
    const size_t max_iovec = 1024;  // Reasonable limit (IOV_MAX is typically 1024)
    
    if (trades.size() <= max_iovec) {
        // Build iovec array for writev()
        std::vector<struct iovec> iovs;
        iovs.reserve(trades.size());
        
        for (const auto& trade : trades) {
            struct iovec iov;
            iov.iov_base = const_cast<void*>(static_cast<const void*>(&trade));
            iov.iov_len = sizeof(Trade);
            iovs.push_back(iov);
        }
        
        // Single writev() call for all trades
        ssize_t total_written = writev(wal_fd_, iovs.data(), static_cast<int>(iovs.size()));
        if (total_written < 0) {
            return false;
        }
        
        // Verify all bytes were written
        size_t expected_bytes = trades.size() * sizeof(Trade);
        if (static_cast<size_t>(total_written) != expected_bytes) {
            return false;
        }
        
        current_offset_.fetch_add(expected_bytes, std::memory_order_relaxed);
    } else {
        // For very large batches, process in chunks
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
    if (fsync(wal_fd_) != 0) {
        throw std::runtime_error("fsync failed: " + std::string(strerror(errno)));
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
