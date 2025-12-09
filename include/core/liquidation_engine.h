#pragma once

#include "types.h"
#include "position.h"
#include "account.h"
#include "order.h"
#include <unordered_map>
#include <mutex>
#include <vector>
#include <functional>

namespace perpetual {

// 强制平仓引擎
class LiquidationEngine {
public:
    // 风险度计算
    struct RiskLevel {
        double risk_ratio = 0.0;      // 风险度 = 维持保证金率 / 保证金率
        double margin_ratio = 0.0;    // 保证金率 = 保证金 / 持仓价值
        double maintenance_margin = 0.0;  // 维持保证金
        double position_value = 0.0;   // 持仓价值
        double available_balance = 0.0; // 可用余额
        bool is_liquidatable = false;  // 是否可清算
    };
    
    // 清算结果
    struct LiquidationResult {
        bool success = false;
        UserID user_id = 0;
        InstrumentID instrument_id = 0;
        Quantity liquidated_quantity = 0;
        Price liquidation_price = 0;
        std::vector<Trade> trades;
        double insurance_fund_used = 0.0;
        std::string error_message;
    };
    
    // 清算策略
    enum LiquidationStrategy {
        FULL_LIQUIDATION,      // 全部清算
        PARTIAL_LIQUIDATION,   // 部分清算
        GRADUAL_LIQUIDATION    // 逐步清算
    };
    
    LiquidationEngine();
    
    // 计算用户风险度
    RiskLevel calculateRiskLevel(UserID user_id, InstrumentID instrument_id,
                                 Price current_price) const;
    
    // 检查是否需要强制平仓
    bool shouldLiquidate(UserID user_id, InstrumentID instrument_id,
                        Price current_price) const;
    
    // 执行强制平仓
    LiquidationResult liquidate(UserID user_id, InstrumentID instrument_id,
                               Price current_price,
                               LiquidationStrategy strategy = PARTIAL_LIQUIDATION);
    
    // 批量检查所有持仓
    std::vector<UserID> checkAllPositions(Price current_price);
    
    // 设置清算参数
    void setMaintenanceMarginRatio(double ratio) { maintenance_margin_ratio_ = ratio; }
    void setLiquidationMarginRatio(double ratio) { liquidation_margin_ratio_ = ratio; }
    void setInsuranceFundBalance(double balance) { insurance_fund_balance_ = balance; }
    
    // 获取保险基金余额
    double getInsuranceFundBalance() const { return insurance_fund_balance_; }
    
    // 设置回调函数（用于撮合）
    using MatchCallback = std::function<std::vector<Trade>(Order* order)>;
    void setMatchCallback(MatchCallback cb) { match_callback_ = cb; }
    
    // 设置持仓管理器
    void setPositionManager(class PositionManager* pm) { position_manager_ = pm; }
    
    // 设置账户管理器
    void setAccountManager(class AccountBalanceManager* am) { account_manager_ = am; }
    
private:
    // 计算清算价格
    Price calculateLiquidationPrice(UserID user_id, InstrumentID instrument_id,
                                   Quantity position_size) const;
    
    // 创建强制平仓订单
    std::unique_ptr<Order> createLiquidationOrder(UserID user_id, InstrumentID instrument_id,
                                                  Quantity quantity, OrderSide side);
    
    // 计算维持保证金
    double calculateMaintenanceMargin(Quantity position_size, Price entry_price,
                                     Price current_price, double leverage) const;
    
    double maintenance_margin_ratio_ = 0.5;  // 维持保证金率 0.5%
    double liquidation_margin_ratio_ = 0.4;  // 清算保证金率 0.4%
    double insurance_fund_balance_ = 0.0;
    
    PositionManager* position_manager_ = nullptr;
    AccountBalanceManager* account_manager_ = nullptr;
    MatchCallback match_callback_;
    
    mutable std::mutex mutex_;
};

} // namespace perpetual


#include "types.h"
#include "position.h"
#include "account.h"
#include "order.h"
#include <unordered_map>
#include <mutex>
#include <vector>
#include <functional>

namespace perpetual {

// 强制平仓引擎
class LiquidationEngine {
public:
    // 风险度计算
    struct RiskLevel {
        double risk_ratio = 0.0;      // 风险度 = 维持保证金率 / 保证金率
        double margin_ratio = 0.0;    // 保证金率 = 保证金 / 持仓价值
        double maintenance_margin = 0.0;  // 维持保证金
        double position_value = 0.0;   // 持仓价值
        double available_balance = 0.0; // 可用余额
        bool is_liquidatable = false;  // 是否可清算
    };
    
    // 清算结果
    struct LiquidationResult {
        bool success = false;
        UserID user_id = 0;
        InstrumentID instrument_id = 0;
        Quantity liquidated_quantity = 0;
        Price liquidation_price = 0;
        std::vector<Trade> trades;
        double insurance_fund_used = 0.0;
        std::string error_message;
    };
    
    // 清算策略
    enum LiquidationStrategy {
        FULL_LIQUIDATION,      // 全部清算
        PARTIAL_LIQUIDATION,   // 部分清算
        GRADUAL_LIQUIDATION    // 逐步清算
    };
    
    LiquidationEngine();
    
    // 计算用户风险度
    RiskLevel calculateRiskLevel(UserID user_id, InstrumentID instrument_id,
                                 Price current_price) const;
    
    // 检查是否需要强制平仓
    bool shouldLiquidate(UserID user_id, InstrumentID instrument_id,
                        Price current_price) const;
    
    // 执行强制平仓
    LiquidationResult liquidate(UserID user_id, InstrumentID instrument_id,
                               Price current_price,
                               LiquidationStrategy strategy = PARTIAL_LIQUIDATION);
    
    // 批量检查所有持仓
    std::vector<UserID> checkAllPositions(Price current_price);
    
    // 设置清算参数
    void setMaintenanceMarginRatio(double ratio) { maintenance_margin_ratio_ = ratio; }
    void setLiquidationMarginRatio(double ratio) { liquidation_margin_ratio_ = ratio; }
    void setInsuranceFundBalance(double balance) { insurance_fund_balance_ = balance; }
    
    // 获取保险基金余额
    double getInsuranceFundBalance() const { return insurance_fund_balance_; }
    
    // 设置回调函数（用于撮合）
    using MatchCallback = std::function<std::vector<Trade>(Order* order)>;
    void setMatchCallback(MatchCallback cb) { match_callback_ = cb; }
    
    // 设置持仓管理器
    void setPositionManager(class PositionManager* pm) { position_manager_ = pm; }
    
    // 设置账户管理器
    void setAccountManager(class AccountBalanceManager* am) { account_manager_ = am; }
    
private:
    // 计算清算价格
    Price calculateLiquidationPrice(UserID user_id, InstrumentID instrument_id,
                                   Quantity position_size) const;
    
    // 创建强制平仓订单
    std::unique_ptr<Order> createLiquidationOrder(UserID user_id, InstrumentID instrument_id,
                                                  Quantity quantity, OrderSide side);
    
    // 计算维持保证金
    double calculateMaintenanceMargin(Quantity position_size, Price entry_price,
                                     Price current_price, double leverage) const;
    
    double maintenance_margin_ratio_ = 0.5;  // 维持保证金率 0.5%
    double liquidation_margin_ratio_ = 0.4;  // 清算保证金率 0.4%
    double insurance_fund_balance_ = 0.0;
    
    PositionManager* position_manager_ = nullptr;
    AccountBalanceManager* account_manager_ = nullptr;
    MatchCallback match_callback_;
    
    mutable std::mutex mutex_;
};

} // namespace perpetual

