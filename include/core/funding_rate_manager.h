#pragma once

#include "types.h"
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <vector>

namespace perpetual {

// 资金费率管理器
class FundingRateManager {
public:
    // 资金费率记录
    struct FundingRateRecord {
        InstrumentID instrument_id;
        double rate;              // 资金费率
        int64_t timestamp;        // 时间戳
        double premium_index;     // 溢价指数
        double interest_rate;     // 利率
        int64_t settlement_time;  // 结算时间
    };
    
    // 资金费率结算记录
    struct FundingSettlement {
        UserID user_id;
        InstrumentID instrument_id;
        Quantity position_size;
        double funding_rate;
        double payment;           // 正数=收到资金，负数=支付资金
        int64_t timestamp;
    };
    
    FundingRateManager();
    
    // 计算资金费率（根据溢价指数和利率）
    double calculateFundingRate(InstrumentID instrument_id, 
                                double premium_index = 0.0,
                                double interest_rate = 0.01);  // 默认利率 0.01%
    
    // 获取当前资金费率
    double getCurrentFundingRate(InstrumentID instrument_id) const;
    
    // 获取资金费率历史
    std::vector<FundingRateRecord> getFundingRateHistory(InstrumentID instrument_id,
                                                         int64_t start_time,
                                                         int64_t end_time) const;
    
    // 执行资金费率结算
    std::vector<FundingSettlement> settleFunding(InstrumentID instrument_id,
                                                 Price mark_price);
    
    // 设置溢价指数（通常是买卖价差）
    void updatePremiumIndex(InstrumentID instrument_id, Price best_bid, Price best_ask,
                           Price mark_price);
    
    // 获取溢价指数
    double getPremiumIndex(InstrumentID instrument_id) const;
    
    // 设置资金费率结算间隔（默认8小时）
    void setSettlementInterval(int64_t interval_seconds) {
        settlement_interval_ = interval_seconds;
    }
    
    // 检查是否需要结算
    bool shouldSettle(InstrumentID instrument_id) const;
    
    // 设置下一个结算时间
    void setNextSettlementTime(InstrumentID instrument_id, int64_t time);
    
    // 获取下一个结算时间
    int64_t getNextSettlementTime(InstrumentID instrument_id) const;
    
    // 设置持仓管理器
    void setPositionManager(class PositionManager* pm) { position_manager_ = pm; }
    
    // 设置账户管理器
    void setAccountManager(class AccountBalanceManager* am) { account_manager_ = am; }
    
private:
    // 计算溢价指数（基于买卖价差）
    double calculatePremiumIndex(Price best_bid, Price best_ask, Price mark_price) const;
    
    mutable std::mutex mutex_;
    
    // 资金费率历史
    std::unordered_map<InstrumentID, std::vector<FundingRateRecord>> funding_rate_history_;
    
    // 当前资金费率
    std::unordered_map<InstrumentID, double> current_funding_rates_;
    
    // 溢价指数
    std::unordered_map<InstrumentID, double> premium_indices_;
    
    // 下一个结算时间
    std::unordered_map<InstrumentID, int64_t> next_settlement_times_;
    
    int64_t settlement_interval_ = 8 * 3600;  // 8小时 = 28800秒
    double interest_rate_ = 0.0001;  // 0.01%
    
    PositionManager* position_manager_ = nullptr;
    AccountBalanceManager* account_manager_ = nullptr;
};

} // namespace perpetual


