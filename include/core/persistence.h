#pragma once

#include "order.h"
#include "types.h"
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <fstream>

namespace perpetual {

// Trade log entry for persistence
struct TradeLogEntry {
    Trade trade;
    Timestamp timestamp;
    std::string toCSV() const;
};

// Order log entry for persistence
struct OrderLogEntry {
    OrderID order_id;
    UserID user_id;
    InstrumentID instrument_id;
    OrderSide side;
    Price price;
    Quantity quantity;
    OrderStatus status;
    Timestamp timestamp;
    std::string toCSV() const;
};

// Persistence manager for production data durability
class PersistenceManager {
public:
    PersistenceManager();
    ~PersistenceManager();
    
    bool initialize(const std::string& data_dir);
    
    // Log trades
    void logTrade(const Trade& trade);
    
    // Log order events
    void logOrder(const Order& order, const std::string& event_type);
    
    // Flush pending writes
    void flush();
    
    // Checkpoint (snapshot)
    bool createCheckpoint(const std::string& checkpoint_name);
    
    // Recovery
    bool recoverFromCheckpoint(const std::string& checkpoint_name);
    
private:
    void writeTradeLog(const TradeLogEntry& entry);
    void writeOrderLog(const OrderLogEntry& entry);
    
    std::string data_dir_;
    std::unique_ptr<std::ofstream> trade_log_;
    std::unique_ptr<std::ofstream> order_log_;
    std::mutex trade_log_mutex_;
    std::mutex order_log_mutex_;
    bool initialized_ = false;
};

} // namespace perpetual
