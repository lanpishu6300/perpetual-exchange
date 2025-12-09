#pragma once

#include "types.h"
#include "orderbook.h"
#include <vector>
#include <unordered_map>
#include <mutex>
#include <functional>
#include <memory>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

namespace perpetual {

// K线数据
struct KLine {
    InstrumentID instrument_id;
    int64_t timestamp;
    Price open;
    Price high;
    Price low;
    Price close;
    Quantity volume;
    int period;  // 周期（秒）：60, 300, 900, 3600, 14400, 86400
};

// 24小时统计
struct Ticker24H {
    InstrumentID instrument_id;
    Price last_price;
    Price high_24h;
    Price low_24h;
    Price open_24h;
    Quantity volume_24h;
    double change_24h;      // 24小时涨跌幅
    double change_rate_24h; // 24小时涨跌幅百分比
};

// 市场数据服务
class MarketDataService {
public:
    // 订阅主题
    enum SubscriptionType {
        SUBSCRIBE_DEPTH,     // 深度数据
        SUBSCRIBE_TRADE,     // 成交数据
        SUBSCRIBE_TICKER,    // 24小时统计
        SUBSCRIBE_KLINE,     // K线数据
    };
    
    MarketDataService();
    
    // 订阅市场数据
    void subscribe(UserID user_id, InstrumentID instrument_id, 
                  SubscriptionType type, int period = 0);
    
    // 取消订阅
    void unsubscribe(UserID user_id, InstrumentID instrument_id, 
                    SubscriptionType type);
    
    // 更新订单簿（由撮合引擎调用）
    void updateOrderBook(InstrumentID instrument_id, const OrderBook& orderbook);
    
    // 更新成交（由撮合引擎调用）
    void updateTrade(InstrumentID instrument_id, const Trade& trade);
    
    // 获取深度数据
    void getDepth(InstrumentID instrument_id, int limit, 
                 std::vector<PriceLevel>& bids,
                 std::vector<PriceLevel>& asks);
    
    // 获取K线数据
    std::vector<KLine> getKLine(InstrumentID instrument_id, int period, 
                               int64_t start_time, int64_t end_time);
    
    // 获取24小时统计
    Ticker24H getTicker24H(InstrumentID instrument_id);
    
    // WebSocket连接管理
    void onWebSocketConnect(void* connection);
    void onWebSocketDisconnect(void* connection);
    void onWebSocketMessage(void* connection, const std::string& message);
    
    // 推送数据到客户端
    void broadcastDepth(InstrumentID instrument_id);
    void broadcastTrade(InstrumentID instrument_id, const Trade& trade);
    void broadcastTicker(InstrumentID instrument_id);
    void broadcastKLine(InstrumentID instrument_id, int period);
    
private:
    // 更新K线
    void updateKLine(InstrumentID instrument_id, Price price, Quantity volume, 
                    int period);
    
    // 更新24小时统计
    void updateTicker24H(InstrumentID instrument_id, Price price, Quantity volume);
    
    mutable std::mutex mutex_;
    
    // 订单簿快照（最新）
    std::unordered_map<InstrumentID, OrderBook> orderbooks_;
    
    // 最新成交
    std::unordered_map<InstrumentID, std::vector<Trade>> recent_trades_;
    
    // K线数据
    std::unordered_map<InstrumentID, std::unordered_map<int, std::vector<KLine>>> klines_;
    
    // 24小时统计
    std::unordered_map<InstrumentID, Ticker24H> tickers_24h_;
    
    // 订阅关系：user_id -> instrument_id -> subscription_type
    std::unordered_map<UserID, 
                      std::unordered_map<InstrumentID, 
                                        std::vector<SubscriptionType>>> subscriptions_;
    
    // WebSocket连接
    std::unordered_map<void*, UserID> connections_;
};

} // namespace perpetual

