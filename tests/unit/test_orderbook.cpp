#include <gtest/gtest.h>
#include "core/orderbook.h"
#include "core/order.h"
#include "core/types.h"
#include <vector>
#include <memory>

using namespace perpetual;

class OrderBookTest : public ::testing::Test {
protected:
    void SetUp() override {
        instrument_id_ = 1;
        orderbook_ = std::make_unique<OrderBook>(instrument_id_);
        user_id_ = 1000000;
        order_id_counter_ = 1;
    }
    
    Order* createOrder(OrderSide side, Price price, Quantity quantity) {
        Order* order = new Order(
            order_id_counter_++,
            user_id_,
            instrument_id_,
            side,
            price,
            quantity,
            OrderType::LIMIT
        );
        return order;
    }
    
    void TearDown() override {
        // Cleanup orders if needed
    }
    
    std::unique_ptr<OrderBook> orderbook_;
    InstrumentID instrument_id_;
    UserID user_id_;
    OrderID order_id_counter_;
};

// Test basic order insertion
TEST_F(OrderBookTest, BasicOrderInsertion) {
    Order* buy_order = createOrder(OrderSide::BUY, double_to_price(50000.0), double_to_quantity(0.1));
    bool result = orderbook_->insert_order(buy_order);
    EXPECT_TRUE(result);
    
    Order* sell_order = createOrder(OrderSide::SELL, double_to_price(50100.0), double_to_quantity(0.1));
    result = orderbook_->insert_order(sell_order);
    EXPECT_TRUE(result);
    
    // Check best bid and ask
    EXPECT_EQ(orderbook_->best_bid(), double_to_price(50000.0));
    EXPECT_EQ(orderbook_->best_ask(), double_to_price(50100.0));
}

// Test best bid and ask
TEST_F(OrderBookTest, BestBidAndAsk) {
    // Initially empty
    EXPECT_EQ(orderbook_->best_bid(), 0);
    EXPECT_EQ(orderbook_->best_ask(), 0);
    
    // Insert buy orders at different prices
    Order* buy1 = createOrder(OrderSide::BUY, double_to_price(50000.0), double_to_quantity(0.1));
    Order* buy2 = createOrder(OrderSide::BUY, double_to_price(50100.0), double_to_quantity(0.1));
    Order* buy3 = createOrder(OrderSide::BUY, double_to_price(49900.0), double_to_quantity(0.1));
    
    orderbook_->insert_order(buy1);
    orderbook_->insert_order(buy2);
    orderbook_->insert_order(buy3);
    
    // Best bid should be highest (50100.0)
    EXPECT_EQ(orderbook_->best_bid(), double_to_price(50100.0));
    
    // Insert sell orders
    Order* sell1 = createOrder(OrderSide::SELL, double_to_price(50200.0), double_to_quantity(0.1));
    Order* sell2 = createOrder(OrderSide::SELL, double_to_price(50300.0), double_to_quantity(0.1));
    Order* sell3 = createOrder(OrderSide::SELL, double_to_price(50150.0), double_to_quantity(0.1));
    
    orderbook_->insert_order(sell1);
    orderbook_->insert_order(sell2);
    orderbook_->insert_order(sell3);
    
    // Best ask should be lowest (50150.0)
    EXPECT_EQ(orderbook_->best_ask(), double_to_price(50150.0));
}

// Test spread calculation
TEST_F(OrderBookTest, SpreadCalculation) {
    // No spread when empty
    EXPECT_EQ(orderbook_->spread(), 0);
    
    // Insert buy and sell orders
    Order* buy = createOrder(OrderSide::BUY, double_to_price(50000.0), double_to_quantity(0.1));
    Order* sell = createOrder(OrderSide::SELL, double_to_price(50100.0), double_to_quantity(0.1));
    
    orderbook_->insert_order(buy);
    orderbook_->insert_order(sell);
    
    Price spread = orderbook_->spread();
    EXPECT_EQ(spread, double_to_price(100.0)); // 50100 - 50000
}

// Test order removal
TEST_F(OrderBookTest, OrderRemoval) {
    Order* buy1 = createOrder(OrderSide::BUY, double_to_price(50000.0), double_to_quantity(0.1));
    Order* buy2 = createOrder(OrderSide::BUY, double_to_price(50100.0), double_to_quantity(0.1));
    
    orderbook_->insert_order(buy1);
    orderbook_->insert_order(buy2);
    
    EXPECT_EQ(orderbook_->best_bid(), double_to_price(50100.0));
    
    // Remove best bid
    bool result = orderbook_->remove_order(buy2);
    EXPECT_TRUE(result);
    EXPECT_EQ(orderbook_->best_bid(), double_to_price(50000.0));
    
    // Remove remaining order
    result = orderbook_->remove_order(buy1);
    EXPECT_TRUE(result);
    EXPECT_EQ(orderbook_->best_bid(), 0);
    EXPECT_TRUE(orderbook_->empty());
}

// Test empty check
TEST_F(OrderBookTest, EmptyCheck) {
    EXPECT_TRUE(orderbook_->empty());
    
    Order* buy = createOrder(OrderSide::BUY, double_to_price(50000.0), double_to_quantity(0.1));
    orderbook_->insert_order(buy);
    EXPECT_FALSE(orderbook_->empty());
    
    orderbook_->remove_order(buy);
    EXPECT_TRUE(orderbook_->empty());
}

// Test can match
TEST_F(OrderBookTest, CanMatch) {
    // Insert buy order
    Order* buy = createOrder(OrderSide::BUY, double_to_price(50000.0), double_to_quantity(0.1));
    orderbook_->insert_order(buy);
    
    // Sell order at same or lower price can match
    Order* sell_match = createOrder(OrderSide::SELL, double_to_price(50000.0), double_to_quantity(0.1));
    EXPECT_TRUE(orderbook_->can_match(sell_match));
    
    // Sell order at higher price cannot match
    Order* sell_no_match = createOrder(OrderSide::SELL, double_to_price(50100.0), double_to_quantity(0.1));
    EXPECT_FALSE(orderbook_->can_match(sell_no_match));
    
    // Insert sell order
    Order* sell = createOrder(OrderSide::SELL, double_to_price(50100.0), double_to_quantity(0.1));
    orderbook_->insert_order(sell);
    
    // Buy order at same or higher price can match
    Order* buy_match = createOrder(OrderSide::BUY, double_to_price(50100.0), double_to_quantity(0.1));
    EXPECT_TRUE(orderbook_->can_match(buy_match));
    
    // Buy order at lower price cannot match
    Order* buy_no_match = createOrder(OrderSide::BUY, double_to_price(50000.0), double_to_quantity(0.1));
    EXPECT_FALSE(orderbook_->can_match(buy_no_match));
}

// Test multiple orders at same price level
TEST_F(OrderBookTest, MultipleOrdersSamePrice) {
    Price price = double_to_price(50000.0);
    
    // Insert multiple buy orders at same price
    Order* buy1 = createOrder(OrderSide::BUY, price, double_to_quantity(0.1));
    Order* buy2 = createOrder(OrderSide::BUY, price, double_to_quantity(0.2));
    Order* buy3 = createOrder(OrderSide::BUY, price, double_to_quantity(0.3));
    
    orderbook_->insert_order(buy1);
    orderbook_->insert_order(buy2);
    orderbook_->insert_order(buy3);
    
    // Best bid should still be the same price
    EXPECT_EQ(orderbook_->best_bid(), price);
    
    // Best order should be the first one inserted
    EXPECT_EQ(orderbook_->bids().best_order(), buy1);
    
    // Remove first order
    orderbook_->remove_order(buy1);
    EXPECT_EQ(orderbook_->bids().best_order(), buy2);
    
    // Remove second order
    orderbook_->remove_order(buy2);
    EXPECT_EQ(orderbook_->bids().best_order(), buy3);
}

// Test price level ordering (bids descending, asks ascending)
TEST_F(OrderBookTest, PriceLevelOrdering) {
    // Insert bids in random order
    Order* buy1 = createOrder(OrderSide::BUY, double_to_price(50000.0), double_to_quantity(0.1));
    Order* buy2 = createOrder(OrderSide::BUY, double_to_price(50200.0), double_to_quantity(0.1));
    Order* buy3 = createOrder(OrderSide::BUY, double_to_price(50100.0), double_to_quantity(0.1));
    
    orderbook_->insert_order(buy1);
    orderbook_->insert_order(buy2);
    orderbook_->insert_order(buy3);
    
    // Best bid should be highest (50200.0)
    EXPECT_EQ(orderbook_->best_bid(), double_to_price(50200.0));
    
    // Remove best bid
    orderbook_->remove_order(buy2);
    EXPECT_EQ(orderbook_->best_bid(), double_to_price(50100.0));
    
    // Remove next best bid
    orderbook_->remove_order(buy3);
    EXPECT_EQ(orderbook_->best_bid(), double_to_price(50000.0));
    
    // Insert asks in random order
    Order* sell1 = createOrder(OrderSide::SELL, double_to_price(50300.0), double_to_quantity(0.1));
    Order* sell2 = createOrder(OrderSide::SELL, double_to_price(50100.0), double_to_quantity(0.1));
    Order* sell3 = createOrder(OrderSide::SELL, double_to_price(50200.0), double_to_quantity(0.1));
    
    orderbook_->insert_order(sell1);
    orderbook_->insert_order(sell2);
    orderbook_->insert_order(sell3);
    
    // Best ask should be lowest (50100.0)
    EXPECT_EQ(orderbook_->best_ask(), double_to_price(50100.0));
    
    // Remove best ask
    orderbook_->remove_order(sell2);
    EXPECT_EQ(orderbook_->best_ask(), double_to_price(50200.0));
}

// Test find order
TEST_F(OrderBookTest, FindOrder) {
    Order* buy = createOrder(OrderSide::BUY, double_to_price(50000.0), double_to_quantity(0.1));
    orderbook_->insert_order(buy);
    
    // Find existing order
    Order* found = orderbook_->bids().find_order(buy->order_id);
    EXPECT_NE(found, nullptr);
    EXPECT_EQ(found->order_id, buy->order_id);
    
    // Find non-existent order
    Order* not_found = orderbook_->bids().find_order(99999);
    EXPECT_EQ(not_found, nullptr);
}

// Test order book side size
TEST_F(OrderBookTest, OrderBookSideSize) {
    EXPECT_EQ(orderbook_->bids().size(), 0);
    EXPECT_EQ(orderbook_->asks().size(), 0);
    
    // Insert buy orders
    Order* buy1 = createOrder(OrderSide::BUY, double_to_price(50000.0), double_to_quantity(0.1));
    Order* buy2 = createOrder(OrderSide::BUY, double_to_price(50100.0), double_to_quantity(0.1));
    orderbook_->insert_order(buy1);
    orderbook_->insert_order(buy2);
    
    EXPECT_EQ(orderbook_->bids().size(), 2);
    EXPECT_EQ(orderbook_->asks().size(), 0);
    
    // Insert sell orders
    Order* sell1 = createOrder(OrderSide::SELL, double_to_price(50200.0), double_to_quantity(0.1));
    orderbook_->insert_order(sell1);
    
    EXPECT_EQ(orderbook_->bids().size(), 2);
    EXPECT_EQ(orderbook_->asks().size(), 1);
    
    // Remove order
    orderbook_->remove_order(buy1);
    EXPECT_EQ(orderbook_->bids().size(), 1);
}

// Test edge cases
TEST_F(OrderBookTest, EdgeCases) {
    // Test invalid order insertion (nullptr)
    bool result = orderbook_->insert_order(nullptr);
    EXPECT_FALSE(result);
    
    // Test removing non-existent order
    Order* fake_order = createOrder(OrderSide::BUY, double_to_price(50000.0), double_to_quantity(0.1));
    result = orderbook_->remove_order(fake_order);
    EXPECT_FALSE(result);
    delete fake_order;
    
    // Test removing order twice
    Order* buy = createOrder(OrderSide::BUY, double_to_price(50000.0), double_to_quantity(0.1));
    orderbook_->insert_order(buy);
    result = orderbook_->remove_order(buy);
    EXPECT_TRUE(result);
    result = orderbook_->remove_order(buy);
    EXPECT_FALSE(result);
}

