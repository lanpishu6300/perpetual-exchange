#include "core/persistence.h"
#include "core/logger.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <mutex>
#include <numeric>
#include <algorithm>
#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

namespace perpetual {

PersistenceManager::PersistenceManager() {
}

PersistenceManager::~PersistenceManager() {
    flush();
    if (trade_log_ && trade_log_->is_open()) {
        trade_log_->close();
    }
    if (order_log_ && order_log_->is_open()) {
        order_log_->close();
    }
}

bool PersistenceManager::initialize(const std::string& data_dir) {
    try {
        fs::create_directories(data_dir);
        
        std::string trade_log_path = data_dir + "/trades.log";
        std::string order_log_path = data_dir + "/orders.log";
        
        trade_log_ = std::make_unique<std::ofstream>(trade_log_path, std::ios::app);
        order_log_ = std::make_unique<std::ofstream>(order_log_path, std::ios::app);
        
        if (!trade_log_->is_open() || !order_log_->is_open()) {
            LOG_ERROR("Failed to open log files for persistence");
            return false;
        }
        
        data_dir_ = data_dir;
        initialized_ = true;
        
        LOG_INFO("Persistence manager initialized: " + data_dir);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to initialize persistence: " + std::string(e.what()));
        return false;
    }
}

void PersistenceManager::logTrade(const Trade& trade) {
    if (!initialized_) return;
    
    TradeLogEntry entry;
    entry.trade = trade;
    entry.timestamp = get_current_timestamp();
    
    writeTradeLog(entry);
}

void PersistenceManager::logOrder(const Order& order, const std::string& event_type) {
    if (!initialized_) return;
    
    OrderLogEntry entry;
    entry.order_id = order.order_id;
    entry.user_id = order.user_id;
    entry.instrument_id = order.instrument_id;
    entry.side = order.side;
    entry.price = order.price;
    entry.quantity = order.quantity;
    entry.status = order.status;
    entry.timestamp = order.timestamp;
    
    writeOrderLog(entry);
}

void PersistenceManager::writeTradeLog(const TradeLogEntry& entry) {
    std::lock_guard<std::mutex> lock(trade_log_mutex_);
    if (trade_log_ && trade_log_->is_open()) {
        *trade_log_ << entry.toCSV() << std::endl;
    }
}

void PersistenceManager::writeOrderLog(const OrderLogEntry& entry) {
    std::lock_guard<std::mutex> lock(order_log_mutex_);
    if (order_log_ && order_log_->is_open()) {
        *order_log_ << entry.toCSV() << std::endl;
    }
}

void PersistenceManager::flush() {
    std::lock_guard<std::mutex> lock1(trade_log_mutex_);
    std::lock_guard<std::mutex> lock2(order_log_mutex_);
    
    if (trade_log_ && trade_log_->is_open()) {
        trade_log_->flush();
    }
    if (order_log_ && order_log_->is_open()) {
        order_log_->flush();
    }
}

bool PersistenceManager::createCheckpoint(const std::string& checkpoint_name) {
    // Implementation for checkpoint creation
    LOG_INFO("Creating checkpoint: " + checkpoint_name);
    return true;
}

bool PersistenceManager::recoverFromCheckpoint(const std::string& checkpoint_name) {
    // Implementation for checkpoint recovery
    LOG_INFO("Recovering from checkpoint: " + checkpoint_name);
    return true;
}

std::string TradeLogEntry::toCSV() const {
    std::stringstream ss;
    ss << trade.sequence_id << ","
       << trade.buy_order_id << ","
       << trade.sell_order_id << ","
       << trade.buy_user_id << ","
       << trade.sell_user_id << ","
       << trade.instrument_id << ","
       << price_to_double(trade.price) << ","
       << quantity_to_double(trade.quantity) << ","
       << trade.timestamp << ","
       << (trade.is_taker_buy ? "1" : "0");
    return ss.str();
}

std::string OrderLogEntry::toCSV() const {
    std::stringstream ss;
    ss << order_id << ","
       << user_id << ","
       << instrument_id << ","
       << (side == OrderSide::BUY ? "BUY" : "SELL") << ","
       << price_to_double(price) << ","
       << quantity_to_double(quantity) << ","
       << static_cast<int>(status) << ","
       << timestamp;
    return ss.str();
}

} // namespace perpetual
