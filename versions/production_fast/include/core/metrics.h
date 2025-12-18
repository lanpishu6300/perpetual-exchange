#pragma once

#include <atomic>
#include <chrono>
#include <string>
#include <unordered_map>
#include <mutex>
#include <vector>

namespace perpetual {

// Metrics collector for production monitoring
class Metrics {
public:
    static Metrics& getInstance() {
        static Metrics instance;
        return instance;
    }
    
    // Counter metrics
    void incrementCounter(const std::string& name, int64_t value = 1);
    void decrementCounter(const std::string& name, int64_t value = 1);
    int64_t getCounter(const std::string& name) const;
    
    // Gauge metrics
    void setGauge(const std::string& name, double value);
    double getGauge(const std::string& name) const;
    
    // Histogram metrics (for latency tracking)
    void recordHistogram(const std::string& name, double value);
    
    // Timer helper
    class Timer {
    public:
        Timer(const std::string& metric_name) 
            : metric_name_(metric_name), start_(std::chrono::high_resolution_clock::now()) {}
        
        ~Timer() {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start_);
            Metrics::getInstance().recordHistogram(metric_name_, duration.count());
        }
        
    private:
        std::string metric_name_;
        std::chrono::high_resolution_clock::time_point start_;
    };
    
    // Get all metrics as string (for Prometheus format)
    std::string getPrometheusFormat() const;
    
    // Reset all metrics
    void reset();
    
private:
    Metrics() = default;
    
    std::unordered_map<std::string, std::atomic<int64_t>> counters_;
    std::unordered_map<std::string, std::atomic<double>> gauges_;
    std::unordered_map<std::string, std::vector<double>> histograms_;
    mutable std::mutex mutex_;
};

#define METRICS_TIMER(name) perpetual::Metrics::Timer _timer(name)

} // namespace perpetual

#include <atomic>
#include <chrono>
#include <string>
#include <unordered_map>
#include <mutex>
#include <vector>

namespace perpetual {

// Metrics collector for production monitoring
} // namespace perpetual
