#include "core/health_check.h"
#include <mutex>

namespace perpetual {

void HealthChecker::start() {
    start_time_ = std::chrono::steady_clock::now();
    status_ = HealthStatus::HEALTHY;
    status_message_ = "System is healthy";
}

void HealthChecker::stop() {
    setUnhealthy("System is shutting down");
}

HealthInfo HealthChecker::getHealth() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    HealthInfo info;
    info.status = status_.load();
    info.message = status_message_;
    
    auto now = std::chrono::steady_clock::now();
    info.uptime = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - start_time_);
    
    info.total_orders = total_orders_.load();
    info.total_trades = total_trades_.load();
    info.avg_latency_us = avg_latency_us_.load();
    
    return info;
}

void HealthChecker::setDegraded(const std::string& reason) {
    std::lock_guard<std::mutex> lock(mutex_);
    status_ = HealthStatus::DEGRADED;
    status_message_ = "Degraded: " + reason;
}

void HealthChecker::setUnhealthy(const std::string& reason) {
    std::lock_guard<std::mutex> lock(mutex_);
    status_ = HealthStatus::UNHEALTHY;
    status_message_ = "Unhealthy: " + reason;
}

void HealthChecker::updateMetrics(uint64_t orders, uint64_t trades, double avg_latency_us) {
    total_orders_ = orders;
    total_trades_ = trades;
    avg_latency_us_ = avg_latency_us;
}

} // namespace perpetual


