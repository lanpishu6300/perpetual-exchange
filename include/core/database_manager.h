#pragma once

#include "types.h"
#include "order.h"
#include "position.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace perpetual {

// 数据库管理器（支持多种数据库）
class DatabaseManager {
public:
    // 数据库类型
    enum DBType {
        SQLITE,
        MYSQL,
        POSTGRESQL,
        MONGODB
    };
    
    DatabaseManager(DBType type, const std::string& connection_string);
    ~DatabaseManager();
    
    // 连接管理
    bool connect();
    void disconnect();
    bool isConnected() const;
    
    // 订单相关
    bool insertOrder(const Order& order);
    bool updateOrder(const Order& order);
    Order* getOrder(OrderID order_id);
    std::vector<Order> getOrdersByUser(UserID user_id, int64_t start_time = 0, 
                                      int64_t end_time = 0);
    
    // 交易相关
    bool insertTrade(const Trade& trade);
    std::vector<Trade> getTradesByUser(UserID user_id, int64_t start_time = 0,
                                      int64_t end_time = 0);
    std::vector<Trade> getTradesByInstrument(InstrumentID instrument_id,
                                            int64_t start_time = 0,
                                            int64_t end_time = 0);
    
    // 账户相关
    bool insertAccountBalance(UserID user_id, double balance, double frozen,
                             double margin, int64_t timestamp);
    bool updateAccountBalance(UserID user_id, double balance, double frozen,
                             double margin);
    struct AccountSnapshot {
        UserID user_id;
        double balance;
        double frozen;
        double margin;
        int64_t timestamp;
    };
    AccountSnapshot getAccountSnapshot(UserID user_id, int64_t timestamp);
    std::vector<AccountSnapshot> getAccountHistory(UserID user_id, 
                                                   int64_t start_time,
                                                   int64_t end_time);
    
    // 持仓相关
    bool insertPosition(UserID user_id, InstrumentID instrument_id,
                       Quantity size, Price entry_price, int64_t timestamp);
    bool updatePosition(UserID user_id, InstrumentID instrument_id,
                       Quantity size, Price entry_price);
    struct PositionSnapshot {
        UserID user_id;
        InstrumentID instrument_id;
        Quantity size;
        Price entry_price;
        int64_t timestamp;
    };
    PositionSnapshot getPositionSnapshot(UserID user_id, InstrumentID instrument_id,
                                        int64_t timestamp);
    std::vector<PositionSnapshot> getPositionHistory(UserID user_id,
                                                    InstrumentID instrument_id,
                                                    int64_t start_time,
                                                    int64_t end_time);
    
    // 批量操作
    bool batchInsertOrders(const std::vector<Order>& orders);
    bool batchInsertTrades(const std::vector<Trade>& trades);
    
    // 事务支持
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();
    
    // 查询优化
    void createIndexes();
    
private:
    DBType db_type_;
    std::string connection_string_;
    void* connection_handle_;  // 数据库连接句柄（类型取决于DBType）
    
    // 数据库特定实现
    void* sqlite_connect();
    void* mysql_connect();
    void* postgresql_connect();
    void* mongodb_connect();
};

} // namespace perpetual

