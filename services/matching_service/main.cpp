#include "matching_service.h"
#include <iostream>
#include <signal.h>
#include <unistd.h>

using namespace perpetual::matching;

MatchingService* g_service = nullptr;

void signal_handler(int signal) {
    if (g_service) {
        std::cout << "\nShutting down matching service..." << std::endl;
        g_service->stop();
    }
    exit(0);
}

int main(int argc, char* argv[]) {
    std::cout << "==========================================" << std::endl;
    std::cout << "Matching Service - 撮合服务" << std::endl;
    std::cout << "==========================================" << std::endl;
    
    // Configuration
    InstrumentID instrument_id = 1;
    std::string server_address = "0.0.0.0:50051";
    std::string event_store_dir = "./event_store";
    
    if (argc > 1) {
        instrument_id = std::stoul(argv[1]);
    }
    if (argc > 2) {
        server_address = argv[2];
    }
    if (argc > 3) {
        event_store_dir = argv[3];
    }
    
    std::cout << "Configuration:" << std::endl;
    std::cout << "  Instrument ID: " << instrument_id << std::endl;
    std::cout << "  Server Address: " << server_address << std::endl;
    std::cout << "  Event Store: " << event_store_dir << std::endl;
    std::cout << std::endl;
    
    // Create service
    MatchingService service(instrument_id, event_store_dir);
    g_service = &service;
    
    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialize
    if (!service.initialize()) {
        std::cerr << "Failed to initialize matching service" << std::endl;
        return 1;
    }
    
    // Start service
    std::cout << "Starting matching service..." << std::endl;
    service.start(server_address);
    
    if (!g_service) {
        std::cerr << "Failed to start matching service" << std::endl;
        return 1;
    }
    
    std::cout << "Matching service is running on " << server_address << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    
    // Wait for shutdown
    while (true) {
        sleep(1);
    }
    
    return 0;
}

