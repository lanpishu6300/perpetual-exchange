#include "core/metrics.h"
#include <mutex>
#include <unordered_map>
#include <sstream>
#include <iomanip>

namespace perpetual {

void Metrics::incrementCounter(const std::string& name, int64_t value) {
    std::lock_guard<std::mutex> lock(mutex_);
    counters_[name] += value;
}

void Metrics::decrementCounter(const std::string& name, int64_t value) {
    std::lock_guard<std::mutex> lock(mutex_);
    counters_[name] -= value;
}

int64_t Metrics::getCounter(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = counters_.find(name);
    return (it != counters_.end()) ? it->second.load() : 0;
}

void Metrics::setGauge(const std::string& name, double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    gauges_[name].store(value);
}

double Metrics::getGauge(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = gauges_.find(name);
    return (it != gauges_.end()) ? it->second.load() : 0.0;
}

void Metrics::recordHistogram(const std::string& name, double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    histograms_[name].push_back(value);
    // Keep only last 1000 values
    if (histograms_[name].size() > 1000) {
        histograms_[name].erase(histograms_[name].begin());
    }
}

void Metrics::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    counters_.clear();
    gauges_.clear();
    histograms_.clear();
}

std::string Metrics::getPrometheusFormat() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;
    
    // Counters
    for (const auto& [name, value] : counters_) {
        oss << "# TYPE " << name << " counter\n";
        oss << name << " " << value.load() << "\n";
    }
    
    // Gauges
    for (const auto& [name, value] : gauges_) {
        oss << "# TYPE " << name << " gauge\n";
        oss << name << " " << std::fixed << std::setprecision(2) << value.load() << "\n";
    }
    
    return oss.str();
}

} // namespace perpetual

