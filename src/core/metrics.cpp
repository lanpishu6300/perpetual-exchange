#include "core/metrics.h"
#include <sstream>
#include <algorithm>
#include <numeric>

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
    if (it != counters_.end()) {
        return it->second.load();
    }
    return 0;
}

void Metrics::setGauge(const std::string& name, double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    gauges_[name] = value;
}

double Metrics::getGauge(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = gauges_.find(name);
    if (it != gauges_.end()) {
        return it->second.load();
    }
    return 0.0;
}

void Metrics::recordHistogram(const std::string& name, double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    histograms_[name].push_back(value);
    
    // Keep only last 1000 values to prevent memory growth
    if (histograms_[name].size() > 1000) {
        histograms_[name].erase(histograms_[name].begin());
    }
}

std::string Metrics::getPrometheusFormat() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::stringstream ss;
    
    // Counters
    for (const auto& [name, value] : counters_) {
        ss << "# TYPE " << name << " counter\n";
        ss << name << " " << value.load() << "\n";
    }
    
    // Gauges
    for (const auto& [name, value] : gauges_) {
        ss << "# TYPE " << name << " gauge\n";
        ss << name << " " << value.load() << "\n";
    }
    
    // Histograms (simplified - show count, sum, avg)
    for (const auto& [name, values] : histograms_) {
        if (!values.empty()) {
            double sum = std::accumulate(values.begin(), values.end(), 0.0);
            double avg = sum / values.size();
            double min = *std::min_element(values.begin(), values.end());
            double max = *std::max_element(values.begin(), values.end());
            
            ss << "# TYPE " << name << "_count counter\n";
            ss << name << "_count " << values.size() << "\n";
            ss << "# TYPE " << name << "_sum counter\n";
            ss << name << "_sum " << sum << "\n";
            ss << "# TYPE " << name << "_avg gauge\n";
            ss << name << "_avg " << avg << "\n";
            ss << "# TYPE " << name << "_min gauge\n";
            ss << name << "_min " << min << "\n";
            ss << "# TYPE " << name << "_max gauge\n";
            ss << name << "_max " << max << "\n";
        }
    }
    
    return ss.str();
}

void Metrics::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    counters_.clear();
    gauges_.clear();
    histograms_.clear();
}

} // namespace perpetual


#include <sstream>
#include <algorithm>
#include <numeric>

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
    if (it != counters_.end()) {
        return it->second.load();
    }
    return 0;
}

void Metrics::setGauge(const std::string& name, double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    gauges_[name] = value;
}

double Metrics::getGauge(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = gauges_.find(name);
    if (it != gauges_.end()) {
        return it->second.load();
    }
    return 0.0;
}

void Metrics::recordHistogram(const std::string& name, double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    histograms_[name].push_back(value);
    
    // Keep only last 1000 values to prevent memory growth
    if (histograms_[name].size() > 1000) {
        histograms_[name].erase(histograms_[name].begin());
    }
}

std::string Metrics::getPrometheusFormat() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::stringstream ss;
    
    // Counters
    for (const auto& [name, value] : counters_) {
        ss << "# TYPE " << name << " counter\n";
        ss << name << " " << value.load() << "\n";
    }
    
    // Gauges
    for (const auto& [name, value] : gauges_) {
        ss << "# TYPE " << name << " gauge\n";
        ss << name << " " << value.load() << "\n";
    }
    
    // Histograms (simplified - show count, sum, avg)
    for (const auto& [name, values] : histograms_) {
        if (!values.empty()) {
            double sum = std::accumulate(values.begin(), values.end(), 0.0);
            double avg = sum / values.size();
            double min = *std::min_element(values.begin(), values.end());
            double max = *std::max_element(values.begin(), values.end());
            
            ss << "# TYPE " << name << "_count counter\n";
            ss << name << "_count " << values.size() << "\n";
            ss << "# TYPE " << name << "_sum counter\n";
            ss << name << "_sum " << sum << "\n";
            ss << "# TYPE " << name << "_avg gauge\n";
            ss << name << "_avg " << avg << "\n";
            ss << "# TYPE " << name << "_min gauge\n";
            ss << name << "_min " << min << "\n";
            ss << "# TYPE " << name << "_max gauge\n";
            ss << name << "_max " << max << "\n";
        }
    }
    
    return ss.str();
}

void Metrics::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    counters_.clear();
    gauges_.clear();
    histograms_.clear();
}

} // namespace perpetual


