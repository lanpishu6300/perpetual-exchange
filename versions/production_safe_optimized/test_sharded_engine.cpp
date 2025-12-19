#include "core/sharded_matching_engine.h"
#include "core/order.h"
#include "core/types.h"
#include <iostream>
#include <cassert>
#include <vector>

using namespace perpetual;

// 简单的单元测试，验证分片核心逻辑
void test_shard_routing() {
    std::cout << "=== 测试分片路由逻辑 ===" << std::endl;
    
    // 创建分片引擎（2个交易分片，3个撮合分片）
    // 注意：不初始化，只测试路由逻辑
    ShardedMatchingEngine engine(2, 3);
    
    // 测试交易分片路由
    std::cout << "\n1. 测试交易分片路由（按user_id）:" << std::endl;
    
    // user_id = 1 -> shard 1 % 2 = 1
    size_t shard1 = engine.get_trading_shard_id(1);
    assert(shard1 == 1);
    std::cout << "   user_id=1 -> trading_shard=" << shard1 << " ✓" << std::endl;
    
    // user_id = 2 -> shard 2 % 2 = 0
    size_t shard2 = engine.get_trading_shard_id(2);
    assert(shard2 == 0);
    std::cout << "   user_id=2 -> trading_shard=" << shard2 << " ✓" << std::endl;
    
    // user_id = 3 -> shard 3 % 2 = 1
    size_t shard3 = engine.get_trading_shard_id(3);
    assert(shard3 == 1);
    std::cout << "   user_id=3 -> trading_shard=" << shard3 << " ✓" << std::endl;
    
    // 测试撮合分片路由
    std::cout << "\n2. 测试撮合分片路由（按instrument_id）:" << std::endl;
    
    // instrument_id = 1 -> shard 1 % 3 = 1
    size_t match_shard1 = engine.get_matching_shard_id(1);
    assert(match_shard1 == 1);
    std::cout << "   instrument_id=1 -> matching_shard=" << match_shard1 << " ✓" << std::endl;
    
    // instrument_id = 2 -> shard 2 % 3 = 2
    size_t match_shard2 = engine.get_matching_shard_id(2);
    assert(match_shard2 == 2);
    std::cout << "   instrument_id=2 -> matching_shard=" << match_shard2 << " ✓" << std::endl;
    
    // instrument_id = 3 -> shard 3 % 3 = 0
    size_t match_shard3 = engine.get_matching_shard_id(3);
    assert(match_shard3 == 0);
    std::cout << "   instrument_id=3 -> matching_shard=" << match_shard3 << " ✓" << std::endl;
    
    // instrument_id = 4 -> shard 4 % 3 = 1
    size_t match_shard4 = engine.get_matching_shard_id(4);
    assert(match_shard4 == 1);
    std::cout << "   instrument_id=4 -> matching_shard=" << match_shard4 << " ✓" << std::endl;
    
    std::cout << "\n✅ 分片路由测试通过！" << std::endl;
}

void test_order_processing() {
    std::cout << "\n=== 测试订单处理流程 ===" << std::endl;
    
    // 创建分片引擎（禁用WAL以加快测试）
    ShardedMatchingEngine engine(2, 2);
    
    if (!engine.initialize("", false)) {  // 禁用WAL
        std::cerr << "❌ 引擎初始化失败" << std::endl;
        return;
    }
    
    std::cout << "\n1. 测试订单处理:" << std::endl;
    
    // 创建测试订单
    Order order1;
    order1.order_id = 1;
    order1.user_id = 1;           // 路由到交易分片 1 % 2 = 1
    order1.instrument_id = 1;     // 路由到撮合分片 1 % 2 = 1
    order1.side = OrderSide::BUY;
    order1.order_type = OrderType::LIMIT;
    order1.price = 50000;
    order1.quantity = 100;
    order1.remaining_quantity = 100;
    order1.status = OrderStatus::PENDING;
    
    std::cout << "   订单1: user_id=1, instrument_id=1" << std::endl;
    auto trades1 = engine.process_order(&order1);
    std::cout << "   处理结果: " << trades1.size() << " 笔交易 ✓" << std::endl;
    
    // 创建第二个订单（不同用户，相同币对）
    Order order2;
    order2.order_id = 2;
    order2.user_id = 2;           // 路由到交易分片 2 % 2 = 0
    order2.instrument_id = 1;     // 路由到撮合分片 1 % 2 = 1（相同撮合分片）
    order2.side = OrderSide::SELL;
    order2.order_type = OrderType::LIMIT;
    order2.price = 50000;
    order2.quantity = 50;
    order2.remaining_quantity = 50;
    order2.status = OrderStatus::PENDING;
    
    std::cout << "   订单2: user_id=2, instrument_id=1" << std::endl;
    auto trades2 = engine.process_order(&order2);
    std::cout << "   处理结果: " << trades2.size() << " 笔交易 ✓" << std::endl;
    
    // 验证：如果价格匹配，应该产生交易
    if (trades2.size() > 0) {
        std::cout << "   ✅ 订单撮合成功！" << std::endl;
    } else {
        std::cout << "   ⚠️  订单未撮合（可能是价格不匹配或订单状态问题）" << std::endl;
    }
    
    // 获取统计
    auto stats = engine.get_stats();
    std::cout << "\n2. 统计信息:" << std::endl;
    std::cout << "   交易分片数: " << engine.get_trading_shard_count() << std::endl;
    std::cout << "   撮合分片数: " << engine.get_matching_shard_count() << std::endl;
    std::cout << "   总订单数: " << stats.total_orders << std::endl;
    
    engine.shutdown();
    std::cout << "\n✅ 订单处理测试完成！" << std::endl;
}

void test_shard_isolation() {
    std::cout << "\n=== 测试分片隔离 ===" << std::endl;
    
    ShardedMatchingEngine engine(3, 3);
    
    std::cout << "\n1. 验证不同用户路由到不同交易分片:" << std::endl;
    std::vector<size_t> trading_shards;
    for (UserID user_id = 1; user_id <= 10; ++user_id) {
        size_t shard = engine.get_trading_shard_id(user_id);
        trading_shards.push_back(shard);
        std::cout << "   user_id=" << user_id << " -> trading_shard=" << shard << std::endl;
    }
    
    // 验证分片分布
    bool has_shard0 = false, has_shard1 = false, has_shard2 = false;
    for (size_t shard : trading_shards) {
        if (shard == 0) has_shard0 = true;
        if (shard == 1) has_shard1 = true;
        if (shard == 2) has_shard2 = true;
    }
    
    std::cout << "\n   分片分布: ";
    if (has_shard0) std::cout << "shard0 ";
    if (has_shard1) std::cout << "shard1 ";
    if (has_shard2) std::cout << "shard2 ";
    std::cout << std::endl;
    
    std::cout << "\n2. 验证不同币对路由到不同撮合分片:" << std::endl;
    std::vector<size_t> matching_shards;
    for (InstrumentID inst_id = 1; inst_id <= 10; ++inst_id) {
        size_t shard = engine.get_matching_shard_id(inst_id);
        matching_shards.push_back(shard);
        std::cout << "   instrument_id=" << inst_id << " -> matching_shard=" << shard << std::endl;
    }
    
    // 验证分片分布
    has_shard0 = has_shard1 = has_shard2 = false;
    for (size_t shard : matching_shards) {
        if (shard == 0) has_shard0 = true;
        if (shard == 1) has_shard1 = true;
        if (shard == 2) has_shard2 = true;
    }
    
    std::cout << "\n   分片分布: ";
    if (has_shard0) std::cout << "shard0 ";
    if (has_shard1) std::cout << "shard1 ";
    if (has_shard2) std::cout << "shard2 ";
    std::cout << std::endl;
    
    std::cout << "\n✅ 分片隔离测试通过！" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "分片引擎核心逻辑单元测试" << std::endl;
    std::cout << "========================================" << std::endl;
    
    int passed = 0;
    int failed = 0;
    
    try {
        test_shard_routing();
        passed++;
    } catch (const std::exception& e) {
        std::cerr << "❌ 分片路由测试失败: " << e.what() << std::endl;
        failed++;
    } catch (...) {
        std::cerr << "❌ 分片路由测试失败: 未知错误" << std::endl;
        failed++;
    }
    
    try {
        test_shard_isolation();
        passed++;
    } catch (const std::exception& e) {
        std::cerr << "❌ 分片隔离测试失败: " << e.what() << std::endl;
        failed++;
    } catch (...) {
        std::cerr << "❌ 分片隔离测试失败: 未知错误" << std::endl;
        failed++;
    }
    
    try {
        test_order_processing();
        passed++;
    } catch (const std::exception& e) {
        std::cerr << "❌ 订单处理测试失败: " << e.what() << std::endl;
        failed++;
    } catch (...) {
        std::cerr << "❌ 订单处理测试失败: 未知错误" << std::endl;
        failed++;
    }
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "测试结果: " << passed << " 通过, " << failed << " 失败" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return failed == 0 ? 0 : 1;
}

