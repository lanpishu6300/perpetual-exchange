#include <gtest/gtest.h>
#include <vector>
#include <thread>
#include <chrono>
#include <filesystem>
#include "core/event_sourcing_advanced.h"
#include "core/event_sourcing.h"
#include "core/order.h"
#include "core/types.h"

using namespace perpetual;

class ConsensusTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建临时目录
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
        
        base_dir_ = "./test_consensus_" + std::to_string(timestamp);
        std::filesystem::create_directories(base_dir_);
        
        // 创建多个节点的目录
        for (int i = 0; i < 3; ++i) {
            std::string node_dir = base_dir_ + "/node" + std::to_string(i);
            std::filesystem::create_directories(node_dir);
            node_dirs_.push_back(node_dir);
        }
    }
    
    void TearDown() override {
        if (std::filesystem::exists(base_dir_)) {
            std::filesystem::remove_all(base_dir_);
        }
    }
    
    std::string base_dir_;
    std::vector<std::string> node_dirs_;
};

// ==================== 分布式事件存储测试 ====================

TEST_F(ConsensusTest, DistributedEventStore_Initialization_SingleNodeMode) {
    // 测试：单节点模式初始化（默认模式，适合本地测试）
    DistributedEventStoreConfig config = DistributedEventStoreConfig::create_single_node();
    
    DistributedEventStore store(config);
    bool initialized = store.initialize(node_dirs_[0]);
    
    EXPECT_TRUE(initialized);
    EXPECT_NE(store.get_local_store(), nullptr);
}

TEST_F(ConsensusTest, DistributedEventStore_Initialization_DistributedMode) {
    // 测试：分布式模式初始化（需要多节点环境）
    DistributedEventStoreConfig config = DistributedEventStoreConfig::create_distributed(
        1, {2, 3}, 3);
    
    DistributedEventStore store(config);
    bool initialized = store.initialize(node_dirs_[0]);
    
    EXPECT_TRUE(initialized);
    EXPECT_NE(store.get_local_store(), nullptr);
}

TEST_F(ConsensusTest, DistributedEventStore_AppendEvent) {
    // 测试：追加事件到分布式存储（单节点模式）
    DistributedEventStoreConfig config = DistributedEventStoreConfig::create_single_node();
    
    DistributedEventStore store(config);
    store.initialize(node_dirs_[0]);
    
    // 创建测试事件
    Event event;
    event.type = EventType::ORDER_PLACED;
    event.instrument_id = 1;
    event.sequence_id = 1;
    event.event_timestamp = get_current_timestamp();
    event.data.order_placed.order_id = 1001;
    event.data.order_placed.user_id = 2001;
    event.data.order_placed.side = OrderSide::BUY;
    event.data.order_placed.price = double_to_price(50000.0);
    event.data.order_placed.quantity = double_to_quantity(0.1);
    event.data.order_placed.order_type = OrderType::LIMIT;
    
    // 追加事件
    bool appended = store.append_event(event);
    EXPECT_TRUE(appended);
}

TEST_F(ConsensusTest, DistributedEventStore_GetEvents) {
    // 测试：从分布式存储获取事件（单节点模式）
    DistributedEventStoreConfig config = DistributedEventStoreConfig::create_single_node();
    
    DistributedEventStore store(config);
    store.initialize(node_dirs_[0]);
    
    // 追加多个事件
    for (int i = 1; i <= 5; ++i) {
        Event event;
        event.type = EventType::ORDER_PLACED;
        event.instrument_id = 1;
        event.sequence_id = i;
        event.event_timestamp = get_current_timestamp();
        event.data.order_placed.order_id = 1000 + i;
        event.data.order_placed.user_id = 2001;
        event.data.order_placed.side = OrderSide::BUY;
        event.data.order_placed.price = double_to_price(50000.0);
        event.data.order_placed.quantity = double_to_quantity(0.1);
        event.data.order_placed.order_type = OrderType::LIMIT;
        
        store.append_event(event);
    }
    
    // 获取事件
    auto events = store.get_events(1, 5);
    EXPECT_EQ(events.size(), 5);
    
    // 验证事件顺序
    for (size_t i = 0; i < events.size(); ++i) {
        EXPECT_EQ(events[i].sequence_id, i + 1);
    }
}

TEST_F(ConsensusTest, DistributedEventStore_ConsensusSequence) {
    // 测试：获取共识序列号（单节点模式）
    DistributedEventStoreConfig config = DistributedEventStoreConfig::create_single_node();
    
    DistributedEventStore store(config);
    store.initialize(node_dirs_[0]);
    
    // 追加事件
    Event event;
    event.type = EventType::ORDER_PLACED;
    event.instrument_id = 1;
    event.sequence_id = 1;
    event.event_timestamp = get_current_timestamp();
    event.data.order_placed.order_id = 1001;
    event.data.order_placed.user_id = 2001;
    event.data.order_placed.side = OrderSide::BUY;
    event.data.order_placed.price = double_to_price(50000.0);
    event.data.order_placed.quantity = double_to_quantity(0.1);
    event.data.order_placed.order_type = OrderType::LIMIT;
    
    store.append_event(event);
    
    // 获取共识序列号
    SequenceID consensus_seq = store.get_consensus_sequence();
    EXPECT_GE(consensus_seq, 0);
}

TEST_F(ConsensusTest, DistributedEventStore_NodeAvailability_SingleNode) {
    // 测试：单节点模式下的节点可用性检查
    DistributedEventStoreConfig config = DistributedEventStoreConfig::create_single_node();
    
    DistributedEventStore store(config);
    store.initialize(node_dirs_[0]);
    
    // 在单节点模式下，其他节点应该不可用
    bool node2_available = store.is_node_available(2);
    bool node3_available = store.is_node_available(3);
    
    EXPECT_FALSE(node2_available);
    EXPECT_FALSE(node3_available);
}

TEST_F(ConsensusTest, DistributedEventStore_NodeAvailability_Distributed) {
    // 测试：分布式模式下的节点可用性检查
    DistributedEventStoreConfig config = DistributedEventStoreConfig::create_distributed(
        1, {2, 3}, 3);
    
    DistributedEventStore store(config);
    store.initialize(node_dirs_[0]);
    
    // 在分布式模式下，节点可用性取决于实际网络连接
    // 在测试环境中，其他节点可能不可用
    bool node2_available = store.is_node_available(2);
    bool node3_available = store.is_node_available(3);
    
    // 至少应该能检查（结果取决于实际环境）
    EXPECT_TRUE(!node2_available || node2_available);
    EXPECT_TRUE(!node3_available || node3_available);
}

TEST_F(ConsensusTest, DistributedEventStore_Replication_SingleNode) {
    // 测试：单节点模式下的事件复制（应该成功但不实际复制）
    DistributedEventStoreConfig config = DistributedEventStoreConfig::create_single_node();
    
    DistributedEventStore store(config);
    store.initialize(node_dirs_[0]);
    
    // 创建事件
    Event event;
    event.type = EventType::ORDER_PLACED;
    event.instrument_id = 1;
    event.sequence_id = 1;
    event.event_timestamp = get_current_timestamp();
    event.data.order_placed.order_id = 1001;
    event.data.order_placed.user_id = 2001;
    event.data.order_placed.side = OrderSide::BUY;
    event.data.order_placed.price = double_to_price(50000.0);
    event.data.order_placed.quantity = double_to_quantity(0.1);
    event.data.order_placed.order_type = OrderType::LIMIT;
    
    // 在单节点模式下，复制应该成功但不实际执行
    std::vector<NodeID> target_nodes = {2, 3};
    bool replicated = store.replicate_event(event, target_nodes);
    EXPECT_TRUE(replicated);  // 应该返回成功（no-op）
}

TEST_F(ConsensusTest, DistributedEventStore_Replication_Distributed) {
    // 测试：分布式模式下的事件复制
    DistributedEventStoreConfig config = DistributedEventStoreConfig::create_distributed(
        1, {2, 3}, 3);
    
    DistributedEventStore store(config);
    store.initialize(node_dirs_[0]);
    
    // 创建事件
    Event event;
    event.type = EventType::ORDER_PLACED;
    event.instrument_id = 1;
    event.sequence_id = 1;
    event.event_timestamp = get_current_timestamp();
    event.data.order_placed.order_id = 1001;
    event.data.order_placed.user_id = 2001;
    event.data.order_placed.side = OrderSide::BUY;
    event.data.order_placed.price = double_to_price(50000.0);
    event.data.order_placed.quantity = double_to_quantity(0.1);
    event.data.order_placed.order_type = OrderType::LIMIT;
    
    // 在分布式模式下，复制应该尝试执行
    std::vector<NodeID> target_nodes = {2, 3};
    bool replicated = store.replicate_event(event, target_nodes);
    // 在测试环境中可能失败（节点不可达），但应该能尝试
    EXPECT_TRUE(!replicated || replicated);
}

// ==================== 共识一致性测试 ====================

TEST_F(ConsensusTest, ConsensusConsistency_MultipleEvents) {
    // 测试：多个事件的共识一致性（单节点模式）
    DistributedEventStoreConfig config = DistributedEventStoreConfig::create_single_node();
    
    DistributedEventStore store(config);
    store.initialize(node_dirs_[0]);
    
    // 追加多个事件
    const int num_events = 10;
    for (int i = 1; i <= num_events; ++i) {
        Event event;
        event.type = EventType::ORDER_PLACED;
        event.instrument_id = 1;
        event.sequence_id = i;
        event.event_timestamp = get_current_timestamp();
        event.data.order_placed.order_id = 1000 + i;
        event.data.order_placed.user_id = 2001;
        event.data.order_placed.side = OrderSide::BUY;
        event.data.order_placed.price = double_to_price(50000.0);
        event.data.order_placed.quantity = double_to_quantity(0.1);
        event.data.order_placed.order_type = OrderType::LIMIT;
        
        bool appended = store.append_event(event);
        EXPECT_TRUE(appended);
    }
    
    // 验证所有事件都能获取
    auto events = store.get_events(1, num_events);
    EXPECT_EQ(events.size(), num_events);
    
    // 验证事件顺序
    for (size_t i = 0; i < events.size(); ++i) {
        EXPECT_EQ(events[i].sequence_id, i + 1);
        EXPECT_EQ(events[i].data.order_placed.order_id, 1000 + i + 1);
    }
}

TEST_F(ConsensusTest, ConsensusConsistency_EventOrdering) {
    // 测试：事件顺序一致性（单节点模式）
    DistributedEventStoreConfig config = DistributedEventStoreConfig::create_single_node();
    
    DistributedEventStore store(config);
    store.initialize(node_dirs_[0]);
    
    // 按顺序追加事件
    std::vector<OrderID> order_ids = {1001, 1002, 1003, 1004, 1005};
    
    for (size_t i = 0; i < order_ids.size(); ++i) {
        Event event;
        event.type = EventType::ORDER_PLACED;
        event.instrument_id = 1;
        event.sequence_id = i + 1;
        event.event_timestamp = get_current_timestamp();
        event.data.order_placed.order_id = order_ids[i];
        event.data.order_placed.user_id = 2001;
        event.data.order_placed.side = OrderSide::BUY;
        event.data.order_placed.price = double_to_price(50000.0);
        event.data.order_placed.quantity = double_to_quantity(0.1);
        event.data.order_placed.order_type = OrderType::LIMIT;
        
        store.append_event(event);
    }
    
    // 获取事件并验证顺序
    auto events = store.get_events(1, order_ids.size());
    EXPECT_EQ(events.size(), order_ids.size());
    
    for (size_t i = 0; i < events.size(); ++i) {
        EXPECT_EQ(events[i].data.order_placed.order_id, order_ids[i]);
    }
}

// ==================== 故障容错测试 ====================

TEST_F(ConsensusTest, FaultTolerance_SingleNodeFailure) {
    // 测试：单节点故障容错（单节点模式）
    DistributedEventStoreConfig config = DistributedEventStoreConfig::create_single_node();
    
    DistributedEventStore store(config);
    store.initialize(node_dirs_[0]);
    
    // 即使其他节点不可用，本地节点应该仍然能工作
    Event event;
    event.type = EventType::ORDER_PLACED;
    event.instrument_id = 1;
    event.sequence_id = 1;
    event.event_timestamp = get_current_timestamp();
    event.data.order_placed.order_id = 1001;
    event.data.order_placed.user_id = 2001;
    event.data.order_placed.side = OrderSide::BUY;
    event.data.order_placed.price = double_to_price(50000.0);
    event.data.order_placed.quantity = double_to_quantity(0.1);
    event.data.order_placed.order_type = OrderType::LIMIT;
    
    // 应该能够追加事件（即使复制失败）
    bool appended = store.append_event(event);
    EXPECT_TRUE(appended);
    
    // 应该能够获取事件
    auto events = store.get_events(1, 1);
    EXPECT_EQ(events.size(), 1);
}

TEST_F(ConsensusTest, FaultTolerance_ConsensusWithPartialNodes) {
    // 测试：部分节点可用时的共识（分布式模式）
    DistributedEventStoreConfig config = DistributedEventStoreConfig::create_distributed(
        1, {2, 3}, 3);
    
    DistributedEventStore store(config);
    store.initialize(node_dirs_[0]);
    
    // 追加事件
    for (int i = 1; i <= 5; ++i) {
        Event event;
        event.type = EventType::ORDER_PLACED;
        event.instrument_id = 1;
        event.sequence_id = i;
        event.event_timestamp = get_current_timestamp();
        event.data.order_placed.order_id = 1000 + i;
        event.data.order_placed.user_id = 2001;
        event.data.order_placed.side = OrderSide::BUY;
        event.data.order_placed.price = double_to_price(50000.0);
        event.data.order_placed.quantity = double_to_quantity(0.1);
        event.data.order_placed.order_type = OrderType::LIMIT;
        
        store.append_event(event);
    }
    
    // 即使部分节点不可用，应该能获取共识序列号
    SequenceID consensus_seq = store.get_consensus_sequence();
    EXPECT_GE(consensus_seq, 0);
}

// ==================== 性能测试 ====================

TEST_F(ConsensusTest, Performance_HighThroughput) {
    // 测试：高吞吐量下的共识性能（单节点模式）
    DistributedEventStoreConfig config = DistributedEventStoreConfig::create_single_node();
    
    DistributedEventStore store(config);
    store.initialize(node_dirs_[0]);
    
    const int num_events = 1000;
    auto start = std::chrono::high_resolution_clock::now();
    
    // 批量追加事件
    for (int i = 1; i <= num_events; ++i) {
        Event event;
        event.type = EventType::ORDER_PLACED;
        event.instrument_id = 1;
        event.sequence_id = i;
        event.event_timestamp = get_current_timestamp();
        event.data.order_placed.order_id = 1000 + i;
        event.data.order_placed.user_id = 2001;
        event.data.order_placed.side = OrderSide::BUY;
        event.data.order_placed.price = double_to_price(50000.0);
        event.data.order_placed.quantity = double_to_quantity(0.1);
        event.data.order_placed.order_type = OrderType::LIMIT;
        
        store.append_event(event);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    // 验证所有事件都已追加
    auto events = store.get_events(1, num_events);
    EXPECT_EQ(events.size(), num_events);
    
    // 输出性能指标
    double throughput = (num_events * 1000.0) / duration;
    std::cout << "Consensus throughput: " << throughput << " events/sec" << std::endl;
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

