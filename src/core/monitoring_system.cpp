#include "core/monitoring_system.h"
#include "core/types.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

namespace perpetual {

MonitoringSystem::MonitoringSystem() {
    // Initialize default metrics
}

void MonitoringSystem::recordCounter(const std::string& name, double value,
                                    const std::unordered_map<std::string, std::string>& labels) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string key = makeMetricKey(name, labels);
    auto it = metrics_.find(key);
    if (it != metrics_.end()) {
        if (it->second.type == COUNTER) {
            it->second.value += value;
        }
    } else {
        Metric metric;
        metric.name = name;
        metric.type = COUNTER;
        metric.value = value;
        metric.labels = labels;
        metric.timestamp = get_current_timestamp() / 1000000000;  // Convert to seconds
        metrics_[key] = metric;
    }
}

void MonitoringSystem::recordGauge(const std::string& name, double value,
                                  const std::unordered_map<std::string, std::string>& labels) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string key = makeMetricKey(name, labels);
    Metric metric;
    metric.name = name;
    metric.type = GAUGE;
    metric.value = value;
    metric.labels = labels;
    metric.timestamp = get_current_timestamp() / 1000000000;
    metrics_[key] = metric;
}

void MonitoringSystem::recordHistogram(const std::string& name, double value,
                                      const std::unordered_map<std::string, std::string>& labels) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // For histogram, we record buckets (simplified implementation)
    std::string key = makeMetricKey(name, labels);
    auto it = metrics_.find(key);
    if (it != metrics_.end() && it->second.type == HISTOGRAM) {
        // Update histogram value (simplified - in production use proper buckets)
        it->second.value = (it->second.value + value) / 2.0;  // Average
    } else {
        Metric metric;
        metric.name = name;
        metric.type = HISTOGRAM;
        metric.value = value;
        metric.labels = labels;
        metric.timestamp = get_current_timestamp() / 1000000000;
        metrics_[key] = metric;
    }
}

void MonitoringSystem::incrementCounter(const std::string& name,
                                       const std::unordered_map<std::string, std::string>& labels,
                                       double amount) {
    recordCounter(name, amount, labels);
}

void MonitoringSystem::setGauge(const std::string& name, double value,
                               const std::unordered_map<std::string, std::string>& labels) {
    recordGauge(name, value, labels);
}

void MonitoringSystem::recordLatency(const std::string& operation, double latency_ms) {
    std::unordered_map<std::string, std::string> labels;
    labels["operation"] = operation;
    recordHistogram("operation_latency_ms", latency_ms, labels);
}

double MonitoringSystem::getMetricValue(const std::string& name,
                                       const std::unordered_map<std::string, std::string>& labels) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string key = makeMetricKey(name, labels);
    auto it = metrics_.find(key);
    if (it != metrics_.end()) {
        return it->second.value;
    }
    return 0.0;
}

std::string MonitoringSystem::getPrometheusMetrics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::stringstream ss;
    
    // Group metrics by name
    std::unordered_map<std::string, std::vector<const Metric*>> metrics_by_name;
    for (const auto& pair : metrics_) {
        metrics_by_name[pair.second.name].push_back(&pair.second);
    }
    
    for (const auto& name_pair : metrics_by_name) {
        const std::string& name = name_pair.first;
        const std::vector<const Metric*>& metrics = name_pair.second;
        
        // Write type and help
        ss << "# TYPE " << name << " ";
        if (!metrics.empty()) {
            switch (metrics[0]->type) {
                case COUNTER:
                    ss << "counter\n";
                    ss << "# HELP " << name << " Counter metric\n";
                    break;
                case GAUGE:
                    ss << "gauge\n";
                    ss << "# HELP " << name << " Gauge metric\n";
                    break;
                case HISTOGRAM:
                    ss << "histogram\n";
                    ss << "# HELP " << name << " Histogram metric\n";
                    break;
                default:
                    ss << "unknown\n";
            }
        }
        
        // Write metric values
        for (const Metric* metric : metrics) {
            ss << name;
            
            // Write labels
            if (!metric->labels.empty()) {
                ss << "{";
                bool first = true;
                for (const auto& label : metric->labels) {
                    if (!first) ss << ",";
                    ss << label.first << "=\"" << label.second << "\"";
                    first = false;
                }
                ss << "}";
            }
            
            ss << " " << std::fixed << std::setprecision(2) << metric->value;
            
            // Write timestamp (in milliseconds)
            ss << " " << (metric->timestamp * 1000) << "\n";
        }
    }
    
    return ss.str();
}

void MonitoringSystem::addAlertRule(const AlertRule& rule) {
    std::lock_guard<std::mutex> lock(mutex_);
    alert_rules_.push_back(rule);
}

std::vector<MonitoringSystem::AlertRule> MonitoringSystem::checkAlerts() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<AlertRule> triggered_alerts;
    
    for (const auto& rule : alert_rules_) {
        // Get metric value
        double value = getMetricValue(rule.metric_name, {});
        
        // Simple condition parser (e.g., "value > 100")
        bool triggered = false;
        if (rule.condition.find(">") != std::string::npos) {
            size_t pos = rule.condition.find(">");
            double threshold = std::stod(rule.condition.substr(pos + 1));
            triggered = value > threshold;
        } else if (rule.condition.find("<") != std::string::npos) {
            size_t pos = rule.condition.find("<");
            double threshold = std::stod(rule.condition.substr(pos + 1));
            triggered = value < threshold;
        } else if (rule.condition.find("==") != std::string::npos) {
            size_t pos = rule.condition.find("==");
            double threshold = std::stod(rule.condition.substr(pos + 2));
            triggered = value == threshold;
        }
        
        if (triggered) {
            auto now = get_current_timestamp() / 1000000000;
            
            auto& state = alert_states_[rule.name];
            if (!state.triggered) {
                state.first_triggered_time = now;
                state.triggered = true;
            }
            state.last_triggered_time = now;
            
            // Check duration requirement
            if (rule.duration_seconds == 0 || 
                (now - state.first_triggered_time) >= rule.duration_seconds) {
                triggered_alerts.push_back(rule);
            }
        } else {
            alert_states_[rule.name].triggered = false;
        }
    }
    
    return triggered_alerts;
}

void MonitoringSystem::recordOrderSubmitted(InstrumentID instrument_id) {
    std::unordered_map<std::string, std::string> labels;
    labels["instrument_id"] = std::to_string(instrument_id);
    incrementCounter("orders_submitted_total", labels);
}

void MonitoringSystem::recordOrderFilled(InstrumentID instrument_id) {
    std::unordered_map<std::string, std::string> labels;
    labels["instrument_id"] = std::to_string(instrument_id);
    incrementCounter("orders_filled_total", labels);
}

void MonitoringSystem::recordOrderCancelled(InstrumentID instrument_id) {
    std::unordered_map<std::string, std::string> labels;
    labels["instrument_id"] = std::to_string(instrument_id);
    incrementCounter("orders_cancelled_total", labels);
}

void MonitoringSystem::recordTrade(InstrumentID instrument_id, Quantity quantity) {
    std::unordered_map<std::string, std::string> labels;
    labels["instrument_id"] = std::to_string(instrument_id);
    double qty_double = static_cast<double>(quantity) / QTY_SCALE;
    recordCounter("trades_volume_total", qty_double, labels);
    incrementCounter("trades_count_total", labels);
}

void MonitoringSystem::recordMatchingLatency(double latency_ns) {
    double latency_ms = latency_ns / 1000000.0;  // Convert to milliseconds
    recordLatency("matching", latency_ms);
}

void MonitoringSystem::recordOrderProcessingLatency(double latency_ns) {
    double latency_ms = latency_ns / 1000000.0;
    recordLatency("order_processing", latency_ms);
}

void MonitoringSystem::recordDatabaseLatency(double latency_ms) {
    recordLatency("database", latency_ms);
}

void MonitoringSystem::recordSystemMetrics() {
    // CPU and memory monitoring (simplified - use proper system calls in production)
    // For now, just record placeholder values
    setGauge("system_cpu_usage_percent", 0.0, {});
    setGauge("system_memory_usage_bytes", 0.0, {});
}

std::string MonitoringSystem::makeMetricKey(const std::string& name,
                                           const std::unordered_map<std::string, std::string>& labels) const {
    std::string key = name;
    if (!labels.empty()) {
        key += "{";
        bool first = true;
        for (const auto& label : labels) {
            if (!first) key += ",";
            key += label.first + "=" + label.second;
            first = false;
        }
        key += "}";
    }
    return key;
}

} // namespace perpetual

#include "core/types.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

namespace perpetual {

MonitoringSystem::MonitoringSystem() {
    // Initialize default metrics
}

void MonitoringSystem::recordCounter(const std::string& name, double value,
                                    const std::unordered_map<std::string, std::string>& labels) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string key = makeMetricKey(name, labels);
    auto it = metrics_.find(key);
    if (it != metrics_.end()) {
        if (it->second.type == COUNTER) {
            it->second.value += value;
        }
    } else {
        Metric metric;
        metric.name = name;
        metric.type = COUNTER;
        metric.value = value;
        metric.labels = labels;
        metric.timestamp = get_current_timestamp() / 1000000000;  // Convert to seconds
        metrics_[key] = metric;
    }
}

void MonitoringSystem::recordGauge(const std::string& name, double value,
                                  const std::unordered_map<std::string, std::string>& labels) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string key = makeMetricKey(name, labels);
    Metric metric;
    metric.name = name;
    metric.type = GAUGE;
    metric.value = value;
    metric.labels = labels;
    metric.timestamp = get_current_timestamp() / 1000000000;
    metrics_[key] = metric;
}

void MonitoringSystem::recordHistogram(const std::string& name, double value,
                                      const std::unordered_map<std::string, std::string>& labels) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // For histogram, we record buckets (simplified implementation)
    std::string key = makeMetricKey(name, labels);
    auto it = metrics_.find(key);
    if (it != metrics_.end() && it->second.type == HISTOGRAM) {
        // Update histogram value (simplified - in production use proper buckets)
        it->second.value = (it->second.value + value) / 2.0;  // Average
    } else {
        Metric metric;
        metric.name = name;
        metric.type = HISTOGRAM;
        metric.value = value;
        metric.labels = labels;
        metric.timestamp = get_current_timestamp() / 1000000000;
        metrics_[key] = metric;
    }
}

void MonitoringSystem::incrementCounter(const std::string& name,
                                       const std::unordered_map<std::string, std::string>& labels,
                                       double amount) {
    recordCounter(name, amount, labels);
}

void MonitoringSystem::setGauge(const std::string& name, double value,
                               const std::unordered_map<std::string, std::string>& labels) {
    recordGauge(name, value, labels);
}

void MonitoringSystem::recordLatency(const std::string& operation, double latency_ms) {
    std::unordered_map<std::string, std::string> labels;
    labels["operation"] = operation;
    recordHistogram("operation_latency_ms", latency_ms, labels);
}

double MonitoringSystem::getMetricValue(const std::string& name,
                                       const std::unordered_map<std::string, std::string>& labels) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string key = makeMetricKey(name, labels);
    auto it = metrics_.find(key);
    if (it != metrics_.end()) {
        return it->second.value;
    }
    return 0.0;
}

std::string MonitoringSystem::getPrometheusMetrics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::stringstream ss;
    
    // Group metrics by name
    std::unordered_map<std::string, std::vector<const Metric*>> metrics_by_name;
    for (const auto& pair : metrics_) {
        metrics_by_name[pair.second.name].push_back(&pair.second);
    }
    
    for (const auto& name_pair : metrics_by_name) {
        const std::string& name = name_pair.first;
        const std::vector<const Metric*>& metrics = name_pair.second;
        
        // Write type and help
        ss << "# TYPE " << name << " ";
        if (!metrics.empty()) {
            switch (metrics[0]->type) {
                case COUNTER:
                    ss << "counter\n";
                    ss << "# HELP " << name << " Counter metric\n";
                    break;
                case GAUGE:
                    ss << "gauge\n";
                    ss << "# HELP " << name << " Gauge metric\n";
                    break;
                case HISTOGRAM:
                    ss << "histogram\n";
                    ss << "# HELP " << name << " Histogram metric\n";
                    break;
                default:
                    ss << "unknown\n";
            }
        }
        
        // Write metric values
        for (const Metric* metric : metrics) {
            ss << name;
            
            // Write labels
            if (!metric->labels.empty()) {
                ss << "{";
                bool first = true;
                for (const auto& label : metric->labels) {
                    if (!first) ss << ",";
                    ss << label.first << "=\"" << label.second << "\"";
                    first = false;
                }
                ss << "}";
            }
            
            ss << " " << std::fixed << std::setprecision(2) << metric->value;
            
            // Write timestamp (in milliseconds)
            ss << " " << (metric->timestamp * 1000) << "\n";
        }
    }
    
    return ss.str();
}

void MonitoringSystem::addAlertRule(const AlertRule& rule) {
    std::lock_guard<std::mutex> lock(mutex_);
    alert_rules_.push_back(rule);
}

std::vector<MonitoringSystem::AlertRule> MonitoringSystem::checkAlerts() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<AlertRule> triggered_alerts;
    
    for (const auto& rule : alert_rules_) {
        // Get metric value
        double value = getMetricValue(rule.metric_name, {});
        
        // Simple condition parser (e.g., "value > 100")
        bool triggered = false;
        if (rule.condition.find(">") != std::string::npos) {
            size_t pos = rule.condition.find(">");
            double threshold = std::stod(rule.condition.substr(pos + 1));
            triggered = value > threshold;
        } else if (rule.condition.find("<") != std::string::npos) {
            size_t pos = rule.condition.find("<");
            double threshold = std::stod(rule.condition.substr(pos + 1));
            triggered = value < threshold;
        } else if (rule.condition.find("==") != std::string::npos) {
            size_t pos = rule.condition.find("==");
            double threshold = std::stod(rule.condition.substr(pos + 2));
            triggered = value == threshold;
        }
        
        if (triggered) {
            auto now = get_current_timestamp() / 1000000000;
            
            auto& state = alert_states_[rule.name];
            if (!state.triggered) {
                state.first_triggered_time = now;
                state.triggered = true;
            }
            state.last_triggered_time = now;
            
            // Check duration requirement
            if (rule.duration_seconds == 0 || 
                (now - state.first_triggered_time) >= rule.duration_seconds) {
                triggered_alerts.push_back(rule);
            }
        } else {
            alert_states_[rule.name].triggered = false;
        }
    }
    
    return triggered_alerts;
}

void MonitoringSystem::recordOrderSubmitted(InstrumentID instrument_id) {
    std::unordered_map<std::string, std::string> labels;
    labels["instrument_id"] = std::to_string(instrument_id);
    incrementCounter("orders_submitted_total", labels);
}

void MonitoringSystem::recordOrderFilled(InstrumentID instrument_id) {
    std::unordered_map<std::string, std::string> labels;
    labels["instrument_id"] = std::to_string(instrument_id);
    incrementCounter("orders_filled_total", labels);
}

void MonitoringSystem::recordOrderCancelled(InstrumentID instrument_id) {
    std::unordered_map<std::string, std::string> labels;
    labels["instrument_id"] = std::to_string(instrument_id);
    incrementCounter("orders_cancelled_total", labels);
}

void MonitoringSystem::recordTrade(InstrumentID instrument_id, Quantity quantity) {
    std::unordered_map<std::string, std::string> labels;
    labels["instrument_id"] = std::to_string(instrument_id);
    double qty_double = static_cast<double>(quantity) / QTY_SCALE;
    recordCounter("trades_volume_total", qty_double, labels);
    incrementCounter("trades_count_total", labels);
}

void MonitoringSystem::recordMatchingLatency(double latency_ns) {
    double latency_ms = latency_ns / 1000000.0;  // Convert to milliseconds
    recordLatency("matching", latency_ms);
}

void MonitoringSystem::recordOrderProcessingLatency(double latency_ns) {
    double latency_ms = latency_ns / 1000000.0;
    recordLatency("order_processing", latency_ms);
}

void MonitoringSystem::recordDatabaseLatency(double latency_ms) {
    recordLatency("database", latency_ms);
}

void MonitoringSystem::recordSystemMetrics() {
    // CPU and memory monitoring (simplified - use proper system calls in production)
    // For now, just record placeholder values
    setGauge("system_cpu_usage_percent", 0.0, {});
    setGauge("system_memory_usage_bytes", 0.0, {});
}

std::string MonitoringSystem::makeMetricKey(const std::string& name,
                                           const std::unordered_map<std::string, std::string>& labels) const {
    std::string key = name;
    if (!labels.empty()) {
        key += "{";
        bool first = true;
        for (const auto& label : labels) {
            if (!first) key += ",";
            key += label.first + "=" + label.second;
            first = false;
        }
        key += "}";
    }
    return key;
}

} // namespace perpetual

