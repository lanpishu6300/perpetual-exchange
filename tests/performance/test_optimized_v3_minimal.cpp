#include <iostream>
#include <chrono>
#include "core/matching_engine_optimized_v3.h"
#include "core/order.h"
#include "core/types.h"
#include <filesystem>

using namespace perpetual;

// 最小化测试 - 只验证基本功能，不进行性能测试
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Minimal Test: MatchingEngineOptimizedV3" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // 创建临时目录
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    std::string event_dir = "./test_v3_minimal_event_" + std::to_string(timestamp);
    std::string persist_dir = "./test_v3_minimal_persist_" + std::to_string(timestamp);
    std::filesystem::create_directories(event_dir);
    std::filesystem::create_directories(persist_dir);
    
    std::cout << "1. Creating engine..." << std::endl;
    MatchingEngineOptimizedV3 engine(1);
    
    std::cout << "2. Initializing engine..." << std::endl;
    if (!engine.initialize(event_dir, persist_dir)) {
        std::cerr << "❌ Failed to initialize engine!" << std::endl;
        return 1;
    }
    std::cout << "   ✅ Engine initialized" << std::endl;
    
    std::cout << "3. Starting engine..." << std::endl;
    engine.start();
    std::cout << "   ✅ Engine started" << std::endl;
    
    std::cout << "4. Processing test orders..." << std::endl;
    
    // 测试1: 买单
    Order buy_order(1, 1001, 1, OrderSide::BUY, 
                   double_to_price(50000.0), double_to_quantity(0.1),
                   OrderType::LIMIT);
    std::cout << "   Processing buy order..." << std::endl;
    auto trades1 = engine.process_order_es(&buy_order);
    std::cout << "   ✅ Buy order processed, trades: " << trades1.size() << std::endl;
    
    // 测试2: 卖单（应该匹配）
    Order sell_order(2, 1002, 1, OrderSide::SELL, 
                    double_to_price(50000.0), double_to_quantity(0.1),
                    OrderType::LIMIT);
    std::cout << "   Processing sell order..." << std::endl;
    auto trades2 = engine.process_order_es(&sell_order);
    std::cout << "   ✅ Sell order processed, trades: " << trades2.size() << std::endl;
    
    // 测试3: 另一个买单
    Order buy_order2(3, 1003, 1, OrderSide::BUY, 
                    double_to_price(50001.0), double_to_quantity(0.2),
                    OrderType::LIMIT);
    std::cout << "   Processing buy order 2..." << std::endl;
    auto trades3 = engine.process_order_es(&buy_order2);
    std::cout << "   ✅ Buy order 2 processed, trades: " << trades3.size() << std::endl;
    
    std::cout << "5. Getting statistics..." << std::endl;
    auto stats = engine.getStatistics();
    std::cout << "   Orders processed: " << stats.orders_processed << std::endl;
    std::cout << "   Trades executed: " << stats.trades_executed << std::endl;
    std::cout << "   ✅ Statistics retrieved" << std::endl;
    
    std::cout << "6. Stopping engine..." << std::endl;
    engine.stop();
    std::cout << "   ✅ Engine stopped" << std::endl;
    
    // 清理
    std::cout << "7. Cleaning up..." << std::endl;
    if (std::filesystem::exists(event_dir)) {
        std::filesystem::remove_all(event_dir);
    }
    if (std::filesystem::exists(persist_dir)) {
        std::filesystem::remove_all(persist_dir);
    }
    std::cout << "   ✅ Cleanup complete" << std::endl;
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "✅ Minimal test completed successfully!" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}

