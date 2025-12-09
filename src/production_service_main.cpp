#include "core/auth_manager.h"
#include "core/liquidation_engine.h"
#include "core/funding_rate_manager.h"
#include "core/market_data_service.h"
#include "core/api_gateway.h"
#include "core/notification_service.h"
#include "core/database_manager.h"
#include "core/monitoring_system.h"
#include "core/rest_api_server.h"
#include "core/matching_engine_event_sourcing.h"  // 使用Event Sourcing版本
#include "core/event_sourcing.h"
#include "core/deterministic_calculator.h"
#include "core/account_manager.h"
#include "core/position_manager.h"
#include "core/logger.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <signal.h>

using namespace perpetual;

// Global instances for graceful shutdown
RESTAPIServer* g_api_server = nullptr;
MatchingEngineEventSourcing* g_matching_engine = nullptr;
MonitoringSystem* g_monitoring = nullptr;

void signal_handler(int signal) {
    std::cout << "\nShutting down production service..." << std::endl;
    if (g_api_server) {
        g_api_server->stop();
    }
    exit(0);
}

int main(int argc, char* argv[]) {
    std::cout << "==========================================" << std::endl;
    std::cout << "Perpetual Exchange - Production Service" << std::endl;
    std::cout << "==========================================" << std::endl;
    
    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialize Logger
    Logger::getInstance().setLevel(LogLevel::INFO);
    Logger::getInstance().setOutputFile("logs/production_service.log");
    
    std::cout << "Initializing components..." << std::endl;
    
    // 1. Initialize Authentication Manager
    std::cout << "  [1/9] Initializing Auth Manager..." << std::endl;
    AuthManager auth_manager;
    
    // 2. Initialize Monitoring System
    std::cout << "  [2/9] Initializing Monitoring System..." << std::endl;
    MonitoringSystem monitoring;
    g_monitoring = &monitoring;
    
    // 3. Initialize Database Manager
    std::cout << "  [3/9] Initializing Database Manager..." << std::endl;
    DatabaseManager db_manager(DatabaseManager::SQLITE, "data/exchange.db");
    if (!db_manager.connect()) {
        std::cerr << "Failed to connect to database" << std::endl;
        return 1;
    }
    db_manager.createIndexes();
    
    // 4. Initialize Account Manager
    std::cout << "  [4/9] Initializing Account Manager..." << std::endl;
    AccountBalanceManager account_manager;
    
    // 5. Initialize Position Manager
    std::cout << "  [5/9] Initializing Position Manager..." << std::endl;
    PositionManager position_manager;
    
    // 6. Initialize Matching Engine with Event Sourcing
    std::cout << "  [6/9] Initializing Matching Engine (Event Sourcing + Deterministic Calculation)..." << std::endl;
    MatchingEngineEventSourcing matching_engine(1);  // Instrument ID = 1
    if (!matching_engine.initialize("data/event_store")) {
        std::cerr << "Failed to initialize event store" << std::endl;
        return 1;
    }
    matching_engine.set_deterministic_mode(true);  // 启用确定性计算模式
    g_matching_engine = &matching_engine;
    
    std::cout << "    ✅ Event Sourcing enabled" << std::endl;
    std::cout << "    ✅ Deterministic Calculation enabled" << std::endl;
    
    // 7. Initialize Liquidation Engine
    std::cout << "  [7/9] Initializing Liquidation Engine..." << std::endl;
    LiquidationEngine liquidation_engine;
    liquidation_engine.setPositionManager(&position_manager);
    liquidation_engine.setAccountManager(&account_manager);
    
    // 8. Initialize Funding Rate Manager
    std::cout << "  [8/9] Initializing Funding Rate Manager..." << std::endl;
    FundingRateManager funding_rate_manager;
    funding_rate_manager.setPositionManager(&position_manager);
    funding_rate_manager.setAccountManager(&account_manager);
    
    // 9. Initialize Market Data Service
    std::cout << "  [9/9] Initializing Market Data Service..." << std::endl;
    MarketDataService market_data_service;
    
    // Initialize Notification Service
    std::cout << "Initializing Notification Service..." << std::endl;
    NotificationService notification_service;
    
    // Initialize API Gateway
    std::cout << "Initializing API Gateway..." << std::endl;
    APIGateway api_gateway;
    api_gateway.setAuthManager(&auth_manager);
    
    // Initialize REST API Server
    std::cout << "Initializing REST API Server..." << std::endl;
    RESTAPIServer api_server(8080);
    api_server.setAuthManager(&auth_manager);
    api_server.setAPIGateway(&api_gateway);
    
    // Register API routes
    api_server.registerRoute(RESTAPIServer::POST, "/api/v1/users/register",
                            [&api_server](const RESTAPIServer::HTTPRequest& req, UserID user_id) {
                                return api_server.handleRegister(req);
                            }, false);
    
    api_server.registerRoute(RESTAPIServer::POST, "/api/v1/users/login",
                            [&api_server](const RESTAPIServer::HTTPRequest& req, UserID user_id) {
                                return api_server.handleLogin(req);
                            }, false);
    
    api_server.registerRoute(RESTAPIServer::GET, "/api/v1/account",
                            [&api_server](const RESTAPIServer::HTTPRequest& req, UserID user_id) {
                                return api_server.handleGetAccount(req, user_id);
                            }, true);
    
    api_server.registerRoute(RESTAPIServer::POST, "/api/v1/orders",
                            [&api_server](const RESTAPIServer::HTTPRequest& req, UserID user_id) {
                                return api_server.handleSubmitOrder(req, user_id);
                            }, true);
    
    api_server.registerRoute(RESTAPIServer::DELETE, "/api/v1/orders/*",
                            [&api_server](const RESTAPIServer::HTTPRequest& req, UserID user_id) {
                                return api_server.handleCancelOrder(req, user_id);
                            }, true);
    
    api_server.registerRoute(RESTAPIServer::GET, "/api/v1/market/depth",
                            [&api_server](const RESTAPIServer::HTTPRequest& req) {
                                return api_server.handleGetOrderBook(req);
                            }, false);
    
    g_api_server = &api_server;
    
    // Start API server
    std::cout << "Starting REST API server on port 8080..." << std::endl;
    if (!api_server.start()) {
        std::cerr << "Failed to start API server" << std::endl;
        return 1;
    }
    
    std::cout << "==========================================" << std::endl;
    std::cout << "Production Service Started Successfully!" << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << "API Server: http://localhost:8080" << std::endl;
    std::cout << "Monitoring: http://localhost:8080/metrics" << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    std::cout << "==========================================" << std::endl;
    
    // Main loop
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // Periodic tasks
        static int counter = 0;
        counter++;
        
        // Update system metrics every 10 seconds
        if (counter % 10 == 0) {
            monitoring.recordSystemMetrics();
        }
        
        // Check funding rate settlement every 60 seconds
        if (counter % 60 == 0) {
            InstrumentID instrument_id = 1;
            Price current_price = 50000000000LL;  // 50.0
            if (funding_rate_manager.shouldSettle(instrument_id)) {
                funding_rate_manager.settleFunding(instrument_id, current_price);
            }
        }
        
        // Check liquidations every 5 seconds
        if (counter % 5 == 0) {
            Price current_price = 50000000000LL;
            auto users_to_liquidate = liquidation_engine.checkAllPositions(current_price);
            for (UserID user_id : users_to_liquidate) {
                InstrumentID instrument_id = 1;
                liquidation_engine.liquidate(user_id, instrument_id, current_price);
            }
        }
    }
    
    return 0;
}

