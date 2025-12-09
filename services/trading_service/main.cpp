#include "trading_service.h"
#include <iostream>
#include <signal.h>
#include <unistd.h>

using namespace perpetual::trading;

TradingService* g_service = nullptr;

void signal_handler(int signal) {
    if (g_service) {
        std::cout << "\nShutting down trading service..." << std::endl;
        g_service->stop();
    }
    exit(0);
}

int main(int argc, char* argv[]) {
    std::cout << "==========================================" << std::endl;
    std::cout << "Trading Service - 交易服务" << std::endl;
    std::cout << "==========================================" << std::endl;
    
    // Configuration
    std::string server_address = "0.0.0.0:50052";
    std::string matching_service_address = "localhost:50051";
    
    if (argc > 1) {
        server_address = argv[1];
    }
    if (argc > 2) {
        matching_service_address = argv[2];
    }
    
    std::cout << "Configuration:" << std::endl;
    std::cout << "  Server Address: " << server_address << std::endl;
    std::cout << "  Matching Service: " << matching_service_address << std::endl;
    std::cout << std::endl;
    
    // Create service
    TradingService service(matching_service_address);
    g_service = &service;
    
    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialize
    if (!service.initialize()) {
        std::cerr << "Failed to initialize trading service" << std::endl;
        return 1;
    }
    
    // Start service
    std::cout << "Starting trading service..." << std::endl;
    service.start(server_address);
    
    if (!g_service) {
        std::cerr << "Failed to start trading service" << std::endl;
        return 1;
    }
    
    std::cout << "Trading service is running on " << server_address << std::endl;
    std::cout << "Connected to matching service at " << matching_service_address << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    
    // Wait for shutdown
    while (true) {
        sleep(1);
    }
    
    return 0;
}

