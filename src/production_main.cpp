#include "core/matching_engine_production.h"
#include "core/logger.h"
#include "core/config.h"
#include "core/health_check.h"
#include <iostream>
#include <csignal>
#include <thread>
#include <chrono>

using namespace perpetual;

ProductionMatchingEngine* g_engine = nullptr;

void signalHandler(int signal) {
    if (g_engine) {
        Logger::getInstance().info("Received signal " + std::to_string(signal) + ", shutting down...");
        g_engine->shutdown();
    }
    exit(0);
}

void healthCheckThread() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        
        if (g_engine) {
            auto health = g_engine->getHealth();
            if (health.status == HealthStatus::UNHEALTHY) {
                Logger::getInstance().error("Health check failed: " + health.message);
            }
        }
    }
}

int main(int argc, char* argv[]) {
    // Setup signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // Initialize logger first
    Logger::getInstance().initialize("", LogLevel::INFO);
    LOG_INFO("Starting Perpetual Exchange Production Server...");
    
    // Load configuration
    std::string config_file = "config.ini";
    if (argc > 1) {
        config_file = argv[1];
    }
    
    // Create and initialize engine
    InstrumentID instrument_id = 1;
    ProductionMatchingEngine engine(instrument_id);
    g_engine = &engine;
    
    if (!engine.initialize(config_file)) {
        LOG_CRITICAL("Failed to initialize engine");
        return 1;
    }
    
    // Start health check thread
    std::thread health_thread(healthCheckThread);
    health_thread.detach();
    
    LOG_INFO("Production server started successfully");
    LOG_INFO("Press Ctrl+C to shutdown");
    
    // Main loop (in production, this would handle network requests)
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // Check health
        auto health = engine.getHealth();
        if (health.status == HealthStatus::UNHEALTHY) {
            LOG_CRITICAL("System unhealthy, shutting down");
            break;
        }
    }
    
    engine.shutdown();
    LOG_INFO("Server stopped");
    
    return 0;
}


