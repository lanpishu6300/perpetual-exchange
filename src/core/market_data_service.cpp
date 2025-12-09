#include "core/market_data_service.h"
#include "core/orderbook.h"
#include "core/types.h"
#include <algorithm>
#include <cmath>

namespace perpetual {

MarketDataService::MarketDataService() {
    // Initialize
}

void MarketDataService::subscribe(UserID user_id, InstrumentID instrument_id,
                                 SubscriptionType type, int period) {
    std::lock_guard<std::mutex> lock(mutex_);
    subscriptions_[user_id][instrument_id].push_back(type);
}

void MarketDataService::unsubscribe(UserID user_id, InstrumentID instrument_id,
                                   SubscriptionType type) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto user_it = subscriptions_.find(user_id);
    if (user_it == subscriptions_.end()) {
        return;
    }
    
    auto inst_it = user_it->second.find(instrument_id);
    if (inst_it == user_it->second.end()) {
        return;
    }
    
    auto& types = inst_it->second;
    types.erase(std::remove(types.begin(), types.end(), type), types.end());
    
    if (types.empty()) {
        user_it->second.erase(inst_it);
    }
}

void MarketDataService::updateOrderBook(InstrumentID instrument_id, const OrderBook& orderbook) {
    std::lock_guard<std::mutex> lock(mutex_);
    orderbooks_[instrument_id] = orderbook;
    
    // Broadcast depth update
    broadcastDepth(instrument_id);
}

void MarketDataService::updateTrade(InstrumentID instrument_id, const Trade& trade) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Add to recent trades
    recent_trades_[instrument_id].push_back(trade);
    
    // Keep only last 100 trades
    if (recent_trades_[instrument_id].size() > 100) {
        recent_trades_[instrument_id].erase(recent_trades_[instrument_id].begin());
    }
    
    // Update K-line
    updateKLine(instrument_id, trade.price, trade.quantity, 60);   // 1m
    updateKLine(instrument_id, trade.price, trade.quantity, 300);  // 5m
    updateKLine(instrument_id, trade.price, trade.quantity, 900);  // 15m
    updateKLine(instrument_id, trade.price, trade.quantity, 3600); // 1h
    updateKLine(instrument_id, trade.price, trade.quantity, 14400); // 4h
    updateKLine(instrument_id, trade.price, trade.quantity, 86400); // 1d
    
    // Update 24h ticker
    updateTicker24H(instrument_id, trade.price, trade.quantity);
    
    // Broadcast trade
    broadcastTrade(instrument_id, trade);
}

void MarketDataService::getDepth(InstrumentID instrument_id, int limit,
                                std::vector<PriceLevel>& bids,
                                std::vector<PriceLevel>& asks) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = orderbooks_.find(instrument_id);
    if (it == orderbooks_.end()) {
        return;
    }
    
    const OrderBook& orderbook = it->second;
    orderbook.get_depth(limit, bids, asks);
}

std::vector<KLine> MarketDataService::getKLine(InstrumentID instrument_id, int period,
                                              int64_t start_time, int64_t end_time) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto inst_it = klines_.find(instrument_id);
    if (inst_it == klines_.end()) {
        return {};
    }
    
    auto period_it = inst_it->second.find(period);
    if (period_it == inst_it->second.end()) {
        return {};
    }
    
    std::vector<KLine> result;
    for (const auto& kline : period_it->second) {
        if (kline.timestamp >= start_time && kline.timestamp <= end_time) {
            result.push_back(kline);
        }
    }
    
    return result;
}

Ticker24H MarketDataService::getTicker24H(InstrumentID instrument_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = tickers_24h_.find(instrument_id);
    if (it != tickers_24h_.end()) {
        return it->second;
    }
    
    // Return empty ticker
    Ticker24H ticker;
    ticker.instrument_id = instrument_id;
    return ticker;
}

void MarketDataService::onWebSocketConnect(void* connection) {
    std::lock_guard<std::mutex> lock(mutex_);
    // In production, store connection and associate with user
    // connections_[connection] = user_id;
}

void MarketDataService::onWebSocketDisconnect(void* connection) {
    std::lock_guard<std::mutex> lock(mutex_);
    // Remove connection
    connections_.erase(connection);
}

void MarketDataService::onWebSocketMessage(void* connection, const std::string& message) {
    // Parse subscription message (simplified)
    // Format: {"action": "subscribe", "channel": "depth", "instrument_id": 1}
    // In production, use JSON parser
}

void MarketDataService::broadcastDepth(InstrumentID instrument_id) {
    // Send depth data to subscribed WebSocket connections
    // In production, iterate through subscriptions and send to connections_
}

void MarketDataService::broadcastTrade(InstrumentID instrument_id, const Trade& trade) {
    // Send trade data to subscribed connections
}

void MarketDataService::broadcastTicker(InstrumentID instrument_id) {
    // Send ticker data to subscribed connections
}

void MarketDataService::broadcastKLine(InstrumentID instrument_id, int period) {
    // Send K-line data to subscribed connections
}

void MarketDataService::updateKLine(InstrumentID instrument_id, Price price, Quantity volume,
                                   int period) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    int64_t current_time = get_current_timestamp() / 1000000000;  // Convert to seconds
    int64_t period_start = (current_time / period) * period;
    
    auto& klines = klines_[instrument_id][period];
    
    // Find or create K-line for this period
    KLine* kline = nullptr;
    if (!klines.empty() && klines.back().timestamp == period_start) {
        kline = &klines.back();
    } else {
        KLine new_kline;
        new_kline.instrument_id = instrument_id;
        new_kline.timestamp = period_start;
        new_kline.period = period;
        new_kline.open = price;
        new_kline.high = price;
        new_kline.low = price;
        new_kline.close = price;
        new_kline.volume = volume;
        klines.push_back(new_kline);
        kline = &klines.back();
    }
    
    // Update K-line
    kline->high = std::max(kline->high, price);
    kline->low = std::min(kline->low, price);
    kline->close = price;
    kline->volume += volume;
    
    // Keep only last 1000 periods
    if (klines.size() > 1000) {
        klines.erase(klines.begin(), klines.begin() + (klines.size() - 1000));
    }
}

void MarketDataService::updateTicker24H(InstrumentID instrument_id, Price price, Quantity volume) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    Ticker24H& ticker = tickers_24h_[instrument_id];
    ticker.instrument_id = instrument_id;
    
    int64_t current_time = get_current_timestamp() / 1000000000;
    int64_t day_start = (current_time / 86400) * 86400;
    
    // Initialize ticker
    if (ticker.open_24h == 0) {
        ticker.open_24h = price;
        ticker.high_24h = price;
        ticker.low_24h = price;
    }
    
    // Reset at start of new day
    int64_t ticker_timestamp = ticker.open_24h != 0 ? day_start : 0;
    if (ticker_timestamp != day_start) {
        ticker.open_24h = price;
        ticker.high_24h = price;
        ticker.low_24h = price;
        ticker.volume_24h = volume;
    } else {
        ticker.high_24h = std::max(ticker.high_24h, price);
        ticker.low_24h = std::min(ticker.low_24h, price);
        ticker.volume_24h += volume;
    }
    
    ticker.last_price = price;
    
    // Calculate change
    if (ticker.open_24h > 0) {
        double change = static_cast<double>(price - ticker.open_24h);
        ticker.change_24h = change / PRICE_SCALE;
        ticker.change_rate_24h = (change / static_cast<double>(ticker.open_24h)) * 100.0;
    }
}

} // namespace perpetual

