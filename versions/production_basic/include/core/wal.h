#pragma once

#include "core/types.h"
#include "core/order.h"  // For Order struct
#include <string>
#include <vector>
#include <fstream>
#include <atomic>
#include <mutex>

namespace perpetual {

// Trade structure forward declaration
struct Trade;

// Write-Ahead Log for data durability
class WriteAheadLog {
public:
    explicit WriteAheadLog(const std::string& path);
    ~WriteAheadLog();
    
    // Append a record to WAL with fsync
    bool append(const Order& order);
    bool append(const Trade& trade);
    
    // Mark records as committed (can be truncated)
    void mark_committed(Timestamp timestamp);
    
    // Read uncommitted records for recovery
    std::vector<Order> read_uncommitted_orders();
    std::vector<Trade> read_uncommitted_trades();
    
    // Truncate committed portion of WAL
    void truncate();
    
    // Sync to disk
    void sync();
    
    // Get WAL statistics
    uint64_t size() const { return current_offset_.load(); }
    uint64_t uncommitted_count() const;
    
private:
    struct WALRecord {
        enum class Type : uint8_t {
            ORDER = 1,
            TRADE = 2,
            CHECKPOINT = 3
        };
        
        Type type;
        uint32_t length;
        uint32_t checksum;
        Timestamp timestamp;
        // Data follows
    };
    
    std::string serialize_order(const Order* order);
    std::string serialize_trade(const Trade* trade);
    void deserialize_order(const std::string& data, Order* order);
    void deserialize_trade(const std::string& data, Trade* trade);
    
    uint32_t calculate_checksum(const std::string& data);
    bool verify_checksum(const std::string& data, uint32_t expected);
    
    bool write_record(WALRecord::Type type, const std::string& data, Timestamp ts);
    
    Timestamp read_last_committed();
    void write_last_committed(Timestamp ts);
    
    std::string path_;
    int wal_fd_;
    std::atomic<uint64_t> current_offset_{0};
    std::mutex write_mutex_;
    Timestamp last_committed_ts_{0};
};

} // namespace perpetual
