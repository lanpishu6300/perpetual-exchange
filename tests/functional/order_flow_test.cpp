#include <gtest/gtest.h>
#include <vector>
#include "core/matching_engine_event_sourcing.h"
#include "core/order.h"
#include "core/types.h"
#include <filesystem>

using namespace perpetual;

class OrderFlowTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
        event_dir_ = "./test_orderflow_" + std::to_string(timestamp);
        std::filesystem::create_directories(event_dir_);
        
        engine_ = std::make_unique<MatchingEngineEventSourcing>(1);
        ASSERT_TRUE(engine_->initialize(event_dir_));
    }
    
    void TearDown() override {
        if (std::filesystem::exists(event_dir_)) {
            std::filesystem::remove_all(event_dir_);
        }
    }
    
    std::unique_ptr<MatchingEngineEventSourcing> engine_;
    std::string event_dir_;
    UserID user_id_ = 1001;
    InstrumentID instrument_id_ = 1;
};

// 测试完整的订单流程：下单 -> 部分成交 -> 完全成交
TEST_F(OrderFlowTest, CompleteOrderFlow_PartialThenFullFill) {
    // Step 1: 下一个大卖单
    Order sell_order(1, user_id_, instrument_id_, OrderSide::SELL,
                     double_to_price(50000.0), double_to_quantity(1.0),
                     OrderType::LIMIT);
    auto trades1 = engine_->process_order_es(&sell_order);
    EXPECT_EQ(trades1.size(), 0);
    EXPECT_EQ(sell_order.status, OrderStatus::PENDING);
    
    // Step 2: 下一个小买单，部分成交
    Order buy_order1(2, user_id_, instrument_id_, OrderSide::BUY,
                      double_to_price(50000.0), double_to_quantity(0.3),
                      OrderType::LIMIT);
    auto trades2 = engine_->process_order_es(&buy_order1);
    EXPECT_EQ(trades2.size(), 1);
    EXPECT_EQ(buy_order1.status, OrderStatus::FILLED);
    EXPECT_EQ(sell_order.status, OrderStatus::PARTIAL_FILLED);
    EXPECT_EQ(sell_order.remaining_quantity, double_to_quantity(0.7));
    
    // Step 3: 再下一个买单，完全成交卖单
    Order buy_order2(3, user_id_, instrument_id_, OrderSide::BUY,
                      double_to_price(50000.0), double_to_quantity(0.7),
                      OrderType::LIMIT);
    auto trades3 = engine_->process_order_es(&buy_order2);
    EXPECT_EQ(trades3.size(), 1);
    EXPECT_EQ(buy_order2.status, OrderStatus::FILLED);
    EXPECT_EQ(sell_order.status, OrderStatus::FILLED);
}

// 测试订单取消流程
TEST_F(OrderFlowTest, OrderCancellationFlow) {
    // Step 1: 下单
    Order order(1, user_id_, instrument_id_, OrderSide::BUY,
                double_to_price(50000.0), double_to_quantity(0.1),
                OrderType::LIMIT);
    engine_->process_order_es(&order);
    EXPECT_EQ(order.status, OrderStatus::PENDING);
    
    // Step 2: 取消订单
    bool cancelled = engine_->cancel_order_es(order.order_id, user_id_);
    EXPECT_TRUE(cancelled);
    
    // Step 3: 尝试再次取消（应该失败）
    bool cancelled_again = engine_->cancel_order_es(order.order_id, user_id_);
    EXPECT_FALSE(cancelled_again);
}

// 测试价格改进流程
TEST_F(OrderFlowTest, PriceImprovementFlow) {
    // Step 1: 下一个低价格卖单
    Order sell_order(1, user_id_, instrument_id_, OrderSide::SELL,
                     double_to_price(49900.0), double_to_quantity(0.1),
                     OrderType::LIMIT);
    engine_->process_order_es(&sell_order);
    
    // Step 2: 下一个高价格买单，应该以卖单价格成交（价格改进）
    Order buy_order(2, user_id_, instrument_id_, OrderSide::BUY,
                    double_to_price(50000.0), double_to_quantity(0.1),
                    OrderType::LIMIT);
    auto trades = engine_->process_order_es(&buy_order);
    
    EXPECT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].price, double_to_price(49900.0));  // 以卖单价格成交
}

// 测试订单簿深度
TEST_F(OrderFlowTest, OrderBookDepth) {
    // 下多个不同价格的订单
    std::vector<Order> buy_orders;
    std::vector<Order> sell_orders;
    
    // 买单：价格从高到低
    for (int i = 0; i < 5; ++i) {
        Order order(100 + i, user_id_, instrument_id_, OrderSide::BUY,
                    double_to_price(50000.0 - i * 10), double_to_quantity(0.1),
                    OrderType::LIMIT);
        engine_->process_order_es(&order);
        buy_orders.push_back(order);
    }
    
    // 卖单：价格从低到高
    for (int i = 0; i < 5; ++i) {
        Order order(200 + i, user_id_, instrument_id_, OrderSide::SELL,
                    double_to_price(50000.0 + i * 10), double_to_quantity(0.1),
                    OrderType::LIMIT);
        engine_->process_order_es(&order);
        sell_orders.push_back(order);
    }
    
    // 验证订单簿深度
    // 这里可以添加订单簿查询的测试
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

