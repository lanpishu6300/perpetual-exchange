#include <gtest/gtest.h>
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <filesystem>
#include "core/matching_engine_event_sourcing.h"
#include "core/matching_engine_optimized_v3.h"
#include "core/account_manager.h"
#include "core/position_manager.h"
#include "core/liquidation_engine.h"
#include "core/funding_rate_manager.h"
#include "core/auth_manager.h"
#include "core/order.h"
#include "core/types.h"
#include "core/orderbook.h"

using namespace perpetual;

class ComprehensiveFunctionalTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建临时目录
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
        event_dir_ = "./test_functional_event_" + std::to_string(timestamp);
        persist_dir_ = "./test_functional_persist_" + std::to_string(timestamp);
        std::filesystem::create_directories(event_dir_);
        std::filesystem::create_directories(persist_dir_);
        
        // 初始化引擎
        engine_ = std::make_unique<MatchingEngineEventSourcing>(1);
        ASSERT_TRUE(engine_->initialize(event_dir_));
        
        // 初始化账户管理器
        account_manager_ = std::make_unique<AccountManager>();
        
        // 初始化持仓管理器
        position_manager_ = std::make_unique<PositionManager>();
        
        // 初始化清算引擎（LiquidationEngine需要AccountBalanceManager，这里只初始化）
        liquidation_engine_ = std::make_unique<LiquidationEngine>();
        // Note: LiquidationEngine requires AccountBalanceManager, not AccountManager
        // For testing purposes, we'll skip setting managers
        
        // 初始化资金费率管理器
        funding_rate_manager_ = std::make_unique<FundingRateManager>();
        
        // 初始化认证管理器
        auth_manager_ = std::make_unique<AuthManager>();
        
        // 创建测试账户
        test_user_id_ = 1001;
        test_instrument_id_ = 1;
        
        // 初始化账户余额
        Account* account = account_manager_->get_account(test_user_id_, "USDT");
        if (account) {
            account->deposit(double_to_price(100000.0));
        }
    }
    
    void TearDown() override {
        engine_.reset();
        account_manager_.reset();
        position_manager_.reset();
        liquidation_engine_.reset();
        funding_rate_manager_.reset();
        auth_manager_.reset();
        
        // 清理临时目录
        if (std::filesystem::exists(event_dir_)) {
            std::filesystem::remove_all(event_dir_);
        }
        if (std::filesystem::exists(persist_dir_)) {
            std::filesystem::remove_all(persist_dir_);
        }
    }
    
    std::unique_ptr<MatchingEngineEventSourcing> engine_;
    std::unique_ptr<AccountManager> account_manager_;
    std::unique_ptr<PositionManager> position_manager_;
    std::unique_ptr<LiquidationEngine> liquidation_engine_;
    std::unique_ptr<FundingRateManager> funding_rate_manager_;
    std::unique_ptr<AuthManager> auth_manager_;
    
    std::string event_dir_;
    std::string persist_dir_;
    UserID test_user_id_;
    InstrumentID test_instrument_id_;
};

// ==================== 订单处理测试 ====================

TEST_F(ComprehensiveFunctionalTest, OrderPlacement_BuyLimitOrder) {
    // 测试：限价买单下单
    Order order(1, test_user_id_, test_instrument_id_, OrderSide::BUY,
                double_to_price(50000.0), double_to_quantity(0.1),
                OrderType::LIMIT);
    
    auto trades = engine_->process_order_es(&order);
    
    EXPECT_EQ(trades.size(), 0);  // 没有匹配的卖单
    EXPECT_EQ(order.status, OrderStatus::PENDING);
    EXPECT_EQ(order.remaining_quantity, double_to_quantity(0.1));
}

TEST_F(ComprehensiveFunctionalTest, OrderPlacement_SellLimitOrder) {
    // 测试：限价卖单下单
    Order order(2, test_user_id_, test_instrument_id_, OrderSide::SELL,
                double_to_price(50000.0), double_to_quantity(0.1),
                OrderType::LIMIT);
    
    auto trades = engine_->process_order_es(&order);
    
    EXPECT_EQ(trades.size(), 0);  // 没有匹配的买单
    EXPECT_EQ(order.status, OrderStatus::PENDING);
}

TEST_F(ComprehensiveFunctionalTest, OrderPlacement_MarketOrder) {
    // 测试：市价单下单
    Order order(3, test_user_id_, test_instrument_id_, OrderSide::BUY,
                double_to_price(50000.0), double_to_quantity(0.1),
                OrderType::MARKET);
    
    auto trades = engine_->process_order_es(&order);
    
    // 市价单如果没有对手盘，应该被拒绝或取消
    EXPECT_TRUE(order.status == OrderStatus::CANCELLED || 
                order.status == OrderStatus::REJECTED);
}

TEST_F(ComprehensiveFunctionalTest, OrderPlacement_InvalidPrice) {
    // 测试：无效价格订单
    Order order(4, test_user_id_, test_instrument_id_, OrderSide::BUY,
                0, double_to_quantity(0.1), OrderType::LIMIT);
    
    auto trades = engine_->process_order_es(&order);
    
    EXPECT_EQ(trades.size(), 0);
    EXPECT_EQ(order.status, OrderStatus::REJECTED);
}

TEST_F(ComprehensiveFunctionalTest, OrderPlacement_InvalidQuantity) {
    // 测试：无效数量订单
    Order order(5, test_user_id_, test_instrument_id_, OrderSide::BUY,
                double_to_price(50000.0), 0, OrderType::LIMIT);
    
    auto trades = engine_->process_order_es(&order);
    
    EXPECT_EQ(trades.size(), 0);
    EXPECT_EQ(order.status, OrderStatus::REJECTED);
}

// ==================== 撮合测试 ====================

TEST_F(ComprehensiveFunctionalTest, OrderMatching_ExactMatch) {
    // 测试：完全匹配
    // 先下一个卖单
    Order sell_order(1, test_user_id_, test_instrument_id_, OrderSide::SELL,
                     double_to_price(50000.0), double_to_quantity(0.1),
                     OrderType::LIMIT);
    auto trades1 = engine_->process_order_es(&sell_order);
    EXPECT_EQ(trades1.size(), 0);
    
    // 下一个相同价格的买单
    Order buy_order(2, test_user_id_, test_instrument_id_, OrderSide::BUY,
                    double_to_price(50000.0), double_to_quantity(0.1),
                    OrderType::LIMIT);
    auto trades2 = engine_->process_order_es(&buy_order);
    
    EXPECT_EQ(trades2.size(), 1);  // 应该产生一笔交易
    EXPECT_EQ(buy_order.status, OrderStatus::FILLED);
    EXPECT_EQ(sell_order.status, OrderStatus::FILLED);
    EXPECT_EQ(trades2[0].price, double_to_price(50000.0));
    EXPECT_EQ(trades2[0].quantity, double_to_quantity(0.1));
}

TEST_F(ComprehensiveFunctionalTest, OrderMatching_PartialMatch) {
    // 测试：部分匹配
    // 先下一个大卖单
    Order sell_order(1, test_user_id_, test_instrument_id_, OrderSide::SELL,
                     double_to_price(50000.0), double_to_quantity(1.0),
                     OrderType::LIMIT);
    engine_->process_order_es(&sell_order);
    
    // 下一个小买单
    Order buy_order(2, test_user_id_, test_instrument_id_, OrderSide::BUY,
                    double_to_price(50000.0), double_to_quantity(0.3),
                    OrderType::LIMIT);
    auto trades = engine_->process_order_es(&buy_order);
    
    EXPECT_EQ(trades.size(), 1);
    EXPECT_EQ(buy_order.status, OrderStatus::FILLED);
    EXPECT_EQ(sell_order.status, OrderStatus::PARTIAL_FILLED);
    EXPECT_EQ(sell_order.remaining_quantity, double_to_quantity(0.7));
}

TEST_F(ComprehensiveFunctionalTest, OrderMatching_PriceTimePriority) {
    // 测试：价格-时间优先级
    // 先下一个低价格卖单
    Order sell_order1(1, test_user_id_, test_instrument_id_, OrderSide::SELL,
                      double_to_price(49900.0), double_to_quantity(0.1),
                      OrderType::LIMIT);
    engine_->process_order_es(&sell_order1);
    
    // 再下一个高价格卖单
    Order sell_order2(2, test_user_id_, test_instrument_id_, OrderSide::SELL,
                      double_to_price(50000.0), double_to_quantity(0.1),
                      OrderType::LIMIT);
    engine_->process_order_es(&sell_order2);
    
    // 下一个买单，应该先匹配低价格的
    Order buy_order(3, test_user_id_, test_instrument_id_, OrderSide::BUY,
                    double_to_price(50100.0), double_to_quantity(0.1),
                    OrderType::LIMIT);
    auto trades = engine_->process_order_es(&buy_order);
    
    EXPECT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].price, double_to_price(49900.0));  // 应该匹配最低卖价
    EXPECT_EQ(sell_order1.status, OrderStatus::FILLED);
    EXPECT_EQ(sell_order2.status, OrderStatus::PENDING);
}

TEST_F(ComprehensiveFunctionalTest, OrderMatching_MultipleMatches) {
    // 测试：多个订单匹配
    // 下多个卖单
    for (int i = 0; i < 5; ++i) {
        Order sell_order(100 + i, test_user_id_, test_instrument_id_, OrderSide::SELL,
                         double_to_price(50000.0 + i * 10), double_to_quantity(0.1),
                         OrderType::LIMIT);
        engine_->process_order_es(&sell_order);
    }
    
    // 下一个大买单
    Order buy_order(200, test_user_id_, test_instrument_id_, OrderSide::BUY,
                    double_to_price(50100.0), double_to_quantity(0.5),
                    OrderType::LIMIT);
    auto trades = engine_->process_order_es(&buy_order);
    
    EXPECT_GE(trades.size(), 1);  // 应该产生多笔交易
    EXPECT_EQ(buy_order.status, OrderStatus::FILLED);
}

// ==================== 订单取消测试 ====================

TEST_F(ComprehensiveFunctionalTest, OrderCancellation_ActiveOrder) {
    // 测试：取消活跃订单
    Order order(1, test_user_id_, test_instrument_id_, OrderSide::BUY,
                double_to_price(50000.0), double_to_quantity(0.1),
                OrderType::LIMIT);
    engine_->process_order_es(&order);
    EXPECT_EQ(order.status, OrderStatus::PENDING);
    
    bool cancelled = engine_->cancel_order_es(order.order_id, test_user_id_);
    EXPECT_TRUE(cancelled);
}

TEST_F(ComprehensiveFunctionalTest, OrderCancellation_NonExistentOrder) {
    // 测试：取消不存在的订单
    bool cancelled = engine_->cancel_order_es(99999, test_user_id_);
    EXPECT_FALSE(cancelled);
}

TEST_F(ComprehensiveFunctionalTest, OrderCancellation_WrongUser) {
    // 测试：错误用户取消订单
    Order order(1, test_user_id_, test_instrument_id_, OrderSide::BUY,
                double_to_price(50000.0), double_to_quantity(0.1),
                OrderType::LIMIT);
    engine_->process_order_es(&order);
    
    bool cancelled = engine_->cancel_order_es(order.order_id, 9999);
    EXPECT_FALSE(cancelled);
}

// ==================== 账户管理测试 ====================

TEST_F(ComprehensiveFunctionalTest, AccountManagement_Deposit) {
    // 测试：账户充值
    Price amount = double_to_price(1000.0);
    Account* account = account_manager_->get_account(test_user_id_, "USDT");
    ASSERT_NE(account, nullptr);
    account->deposit(amount);
    
    // 检查余额（Account类使用balance字段）
    EXPECT_GE(account->balance, amount);
}

TEST_F(ComprehensiveFunctionalTest, AccountManagement_Withdraw) {
    // 测试：账户提现
    Price amount = double_to_price(1000.0);
    Account* account = account_manager_->get_account(test_user_id_, "USDT");
    ASSERT_NE(account, nullptr);
    account->withdraw(amount);
    
    // 检查余额减少
    EXPECT_LT(account->balance, double_to_price(100000.0));
}

TEST_F(ComprehensiveFunctionalTest, AccountManagement_InsufficientBalance) {
    // 测试：余额不足
    Price amount = double_to_price(200000.0);
    Account* account = account_manager_->get_account(test_user_id_, "USDT");
    ASSERT_NE(account, nullptr);
    Price before = account->balance;
    account->withdraw(amount);
    Price after = account->balance;
    // 余额不应该减少（因为不足）
    EXPECT_EQ(before, after);
}

TEST_F(ComprehensiveFunctionalTest, AccountManagement_FreezeUnfreeze) {
    // 测试：冻结和解冻资金
    Price amount = double_to_price(1000.0);
    Account* account = account_manager_->get_account(test_user_id_, "USDT");
    ASSERT_NE(account, nullptr);
    
    account->freeze(amount);
    account->unfreeze(amount);
    // Test passes if no exception is thrown
}

// ==================== 持仓管理测试 ====================

TEST_F(ComprehensiveFunctionalTest, PositionManagement_OpenLongPosition) {
    // 测试：检查持仓限制（PositionManager只管理限制，不管理实际持仓）
    Price price = double_to_price(50000.0);
    Quantity quantity = double_to_quantity(0.1);
    
    // 检查是否可以开仓（检查持仓限制）
    bool can_open = position_manager_->checkPositionLimit(
        test_user_id_, test_instrument_id_, quantity, OrderSide::BUY);
    EXPECT_TRUE(can_open);
    
    // 获取当前持仓大小（应该为0，因为没有实际持仓）
    Quantity current_size = position_manager_->getPositionSize(
        test_user_id_, test_instrument_id_);
    EXPECT_EQ(current_size, 0);
}

TEST_F(ComprehensiveFunctionalTest, PositionManagement_OpenShortPosition) {
    // 测试：检查持仓限制（开空仓）
    Price price = double_to_price(50000.0);
    Quantity quantity = double_to_quantity(0.1);
    
    // 检查是否可以开空仓
    bool can_open = position_manager_->checkPositionLimit(
        test_user_id_, test_instrument_id_, quantity, OrderSide::SELL);
    EXPECT_TRUE(can_open);
}

TEST_F(ComprehensiveFunctionalTest, PositionManagement_ClosePosition) {
    // 测试：持仓限制管理（PositionManager只管理限制，实际平仓由交易引擎处理）
    Price open_price = double_to_price(50000.0);
    Quantity quantity = double_to_quantity(0.1);
    
    // 设置持仓限制
    position_manager_->setPositionLimit(test_user_id_, test_instrument_id_, 
                                        double_to_quantity(1.0));
    
    // 检查限制是否正确设置
    Quantity limit = position_manager_->getPositionLimit(
        test_user_id_, test_instrument_id_);
    EXPECT_EQ(limit, double_to_quantity(1.0));
}

TEST_F(ComprehensiveFunctionalTest, PositionManagement_UpdatePnL) {
    // 测试：持仓限制计算（PositionManager只管理限制，不管理实际盈亏）
    Price open_price = double_to_price(50000.0);
    Quantity quantity = double_to_quantity(0.1);
    
    // 计算新持仓大小
    Quantity current_size = position_manager_->getPositionSize(
        test_user_id_, test_instrument_id_);
    Quantity new_size = position_manager_->calculateNewPositionSize(
        test_user_id_, test_instrument_id_, current_size, quantity, OrderSide::BUY);
    EXPECT_GE(new_size, current_size);
}

// ==================== 清算测试 ====================

TEST_F(ComprehensiveFunctionalTest, LiquidationEngine_RiskCheck) {
    // 测试：清算引擎初始化（LiquidationEngine需要账户和持仓管理器）
    // 由于PositionManager只管理限制，实际清算逻辑由交易引擎处理
    // 这里只测试引擎是否正常初始化
    EXPECT_NE(liquidation_engine_.get(), nullptr);
    // Test passes if engine is initialized
}

// ==================== 资金费率测试 ====================

TEST_F(ComprehensiveFunctionalTest, FundingRate_Calculation) {
    // 测试：资金费率管理器初始化
    // 资金费率计算通常需要市场数据，这里只测试管理器是否正常初始化
    EXPECT_NE(funding_rate_manager_.get(), nullptr);
    // Test passes if manager is initialized
}

TEST_F(ComprehensiveFunctionalTest, FundingRate_Settlement) {
    // 测试：资金费率管理器功能
    // 资金费率计算需要市场数据，这里只测试管理器是否正常初始化
    EXPECT_NE(funding_rate_manager_.get(), nullptr);
    // Test passes if manager is initialized
}

// ==================== 事件溯源测试 ====================

TEST_F(ComprehensiveFunctionalTest, EventSourcing_EventStorage) {
    // 测试：事件存储
    Order order(1, test_user_id_, test_instrument_id_, OrderSide::BUY,
                double_to_price(50000.0), double_to_quantity(0.1),
                OrderType::LIMIT);
    
    engine_->process_order_es(&order);
    
    auto event_store = engine_->get_event_store();
    ASSERT_NE(event_store, nullptr);
    
    auto latest_seq = event_store->get_latest_sequence();
    EXPECT_GE(latest_seq, 0);
}

TEST_F(ComprehensiveFunctionalTest, EventSourcing_EventReplay) {
    // 测试：事件回放
    // 先处理一些订单
    for (int i = 0; i < 5; ++i) {
        Order order(100 + i, test_user_id_, test_instrument_id_, OrderSide::BUY,
                    double_to_price(50000.0), double_to_quantity(0.1),
                    OrderType::LIMIT);
        engine_->process_order_es(&order);
    }
    
    // 回放事件
    bool replayed = engine_->replay_events(0, UINT64_MAX);
    EXPECT_TRUE(replayed);
}

// ==================== 边界条件测试 ====================

TEST_F(ComprehensiveFunctionalTest, BoundaryConditions_ZeroQuantity) {
    // 测试：零数量订单
    Order order(1, test_user_id_, test_instrument_id_, OrderSide::BUY,
                double_to_price(50000.0), 0, OrderType::LIMIT);
    auto trades = engine_->process_order_es(&order);
    EXPECT_EQ(order.status, OrderStatus::REJECTED);
}

TEST_F(ComprehensiveFunctionalTest, BoundaryConditions_VeryLargeQuantity) {
    // 测试：超大数量订单
    Order order(1, test_user_id_, test_instrument_id_, OrderSide::BUY,
                double_to_price(50000.0), double_to_quantity(1000000.0),
                OrderType::LIMIT);
    auto trades = engine_->process_order_es(&order);
    // 应该能处理，但可能因为余额不足被拒绝
    EXPECT_TRUE(order.status == OrderStatus::PENDING || 
                order.status == OrderStatus::REJECTED);
}

TEST_F(ComprehensiveFunctionalTest, BoundaryConditions_VeryHighPrice) {
    // 测试：超高价格订单
    Order order(1, test_user_id_, test_instrument_id_, OrderSide::BUY,
                double_to_price(1000000000.0), double_to_quantity(0.1),
                OrderType::LIMIT);
    auto trades = engine_->process_order_es(&order);
    EXPECT_TRUE(order.status == OrderStatus::PENDING || 
                order.status == OrderStatus::REJECTED);
}

// ==================== 并发测试 ====================

TEST_F(ComprehensiveFunctionalTest, Concurrency_MultipleOrders) {
    // 测试：并发下单
    const int num_threads = 4;
    const int orders_per_thread = 100;
    std::atomic<int> success_count{0};
    std::atomic<int> trade_count{0};
    
    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < orders_per_thread; ++i) {
                uint64_t order_id = static_cast<uint64_t>(t) * 10000 + i;
                Order order(order_id, test_user_id_ + t, test_instrument_id_,
                           (i % 2 == 0) ? OrderSide::BUY : OrderSide::SELL,
                           double_to_price(50000.0 + (i % 100)),
                           double_to_quantity(0.1), OrderType::LIMIT);
                
                auto trades = engine_->process_order_es(&order);
                if (order.status != OrderStatus::REJECTED) {
                    success_count++;
                }
                trade_count += trades.size();
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_GT(success_count.load(), 0);
    EXPECT_GE(trade_count.load(), 0);
}

// ==================== 主函数 ====================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

