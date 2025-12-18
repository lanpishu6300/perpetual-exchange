#include "core/matching_engine_production.h"
#include "core/config.h"
#include "core/logger.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <signal.h>

using namespace perpetual;

// Global instance for graceful shutdown
ProductionMatchingEngine* g_matching_engine = nullptr;

void signal_handler(int signal) {
    std::cout << "\nShutting down production server..." << std::endl;
    exit(0);
}

int main(int argc, char* argv[]) {
    std::cout << "==========================================" << std::endl;
    std::cout << "Perpetual Exchange - Production Server" << std::endl;
    std::cout << "==========================================" << std::endl;
    
    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Load configuration
    std::string config_file = "config.ini";
    if (argc > 1) {
        config_file = argv[1];
    }
    
    Config& config = Config::getInstance();
    if (!config.loadFromFile(config_file)) {
        std::cerr << "Failed to load configuration from " << config_file << std::endl;
        return 1;
    }
    
    // Initialize Logger
    Logger::getInstance().setLogLevel(LogLevel::INFO);
    std::string log_file = config.getString("log.file", "logs/production_server.log");
    Logger::getInstance().setOutputFile(log_file);
    
    std::cout << "Initializing production matching engine..." << std::endl;
    
    // Initialize Matching Engine
    InstrumentID instrument_id = config.getInt("instrument.id", 1);
    ProductionMatchingEngine matching_engine(instrument_id);
    g_matching_engine = &matching_engine;
    
    std::cout << "Production server initialized successfully!" << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    
    // Main loop
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return 0;
}

