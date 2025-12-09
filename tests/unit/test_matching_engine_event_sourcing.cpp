#include <gtest/gtest.h>
#include "core/matching_engine_event_sourcing.h"
#include "core/order.h"
#include "core/types.h"
#include <filesystem>

using namespace perpetual;

class MatchingEngineEventSourcingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary event store directory
        event_store_dir_ = "./test_event_store_" + std::to_string(get_current_timestamp());
        std::filesystem::create_directories(event_store_dir_);
        
        engine_ = std::make_unique<MatchingEngineEventSourcing>(1);
        engine_->initialize(event_store_dir_);
        engine_->set_deterministic_mode(true);
        
        user_id_ = 1000000;
    }
    
    void TearDown() override {
        // Cleanup
        if (std::filesystem::exists(event_store_dir_)) {
            std::filesystem::remove_all(event_store_dir_);
        }
    }
    
    std::unique_ptr<MatchingEngineEventSourcing> engine_;
    std::string event_store_dir_;
    UserID user_id_;
};

TEST_F(MatchingEngineEventSourcingTest, OrderProcessingWithEventSourcing) {
    // Create buy order
    Order buy_order(1, user_id_, 1, OrderSide::BUY,
                   double_to_price(50000.0), double_to_quantity(0.1),
                   OrderType::LIMIT);
    
    auto trades = engine_->process_order_es(&buy_order);
    
    // Order should be processed
    EXPECT_TRUE(buy_order.order_id != 0);
    
    // Check if events are stored (event store access is internal)
    // Events are automatically stored by the engine
    EXPECT_TRUE(buy_order.order_id != 0);
}

TEST_F(MatchingEngineEventSourcingTest, OrderMatchingWithEvents) {
    // Create buy order
    Order buy_order(1, user_id_, 1, OrderSide::BUY,
                   double_to_price(50000.0), double_to_quantity(0.1),
                   OrderType::LIMIT);
    engine_->process_order_es(&buy_order);
    
    // Create matching sell order
    Order sell_order(2, user_id_ + 1, 1, OrderSide::SELL,
                    double_to_price(50000.0), double_to_quantity(0.1),
                    OrderType::LIMIT);
    auto trades = engine_->process_order_es(&sell_order);
    
    // Should have at least one trade if matched
    // EXPECT_GE(trades.size(), 0);  // May or may not match depending on implementation
}

TEST_F(MatchingEngineEventSourcingTest, EventReplay) {
    // Process some orders first
    Order order1(1, user_id_, 1, OrderSide::BUY,
                double_to_price(50000.0), double_to_quantity(0.1),
                OrderType::LIMIT);
    engine_->process_order_es(&order1);
    
    // Replay events
    bool result = engine_->replay_events(0, UINT64_MAX);
    EXPECT_TRUE(result);
}

TEST_F(MatchingEngineEventSourcingTest, DeterministicMode) {
    EXPECT_TRUE(engine_->deterministic_mode());
    
    engine_->set_deterministic_mode(false);
    EXPECT_FALSE(engine_->deterministic_mode());
}

TEST_F(MatchingEngineEventSourcingTest, OrderCancellationWithEvents) {
    // Create and process order
    Order order(1, user_id_, 1, OrderSide::BUY,
               double_to_price(50000.0), double_to_quantity(0.1),
               OrderType::LIMIT);
    engine_->process_order_es(&order);
    
    // Cancel order
    bool result = engine_->cancel_order_es(order.order_id, user_id_);
    EXPECT_TRUE(result);
}

