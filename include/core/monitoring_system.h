#pragma once

#include "types.h"
#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <vector>
#include <atomic>

namespace perpetual {

// 监控系统（Prometheus格式）
class MonitoringSystem {
public:
    // 指标类型
    enum MetricType {
        COUNTER,    // 计数器（只增不减）
        GAUGE,      // 仪表盘（可增可减）
        HISTOGRAM,  // 直方图（分布）
        SUMMARY     // 摘要（分位数）
    };
    
    // 指标
    struct Metric {
        std::string name;
        MetricType type;
        double value = 0.0;
        std::unordered_map<std::string, std::string> labels;
        int64_t timestamp = 0;
    };
    
    // 告警规则
    struct AlertRule {
        std::string name;
        std::string metric_name;
        std::string condition;  // e.g., "value > 100"
        std::string severity;   // "critical", "warning", "info"
        std::string message;
        int duration_seconds = 0;  // 持续多长时间触发
    };
    
    MonitoringSystem();
    
    // 指标记录
    void recordCounter(const std::string& name, double value,
                      const std::unordered_map<std::string, std::string>& labels = {});
    void recordGauge(const std::string& name, double value,
                    const std::unordered_map<std::string, std::string>& labels = {});
    void recordHistogram(const std::string& name, double value,
                        const std::unordered_map<std::string, std::string>& labels = {});
    
    // 增加计数器
    void incrementCounter(const std::string& name,
                         const std::unordered_map<std::string, std::string>& labels = {},
                         double amount = 1.0);
    
    // 设置仪表盘值
    void setGauge(const std::string& name, double value,
                 const std::unordered_map<std::string, std::string>& labels = {});
    
    // 记录延迟
    void recordLatency(const std::string& operation, double latency_ms);
    
    // 获取指标（Prometheus格式）
    std::string getPrometheusMetrics() const;
    
    // 获取特定指标
    double getMetricValue(const std::string& name,
                         const std::unordered_map<std::string, std::string>& labels = {}) const;
    
    // 告警管理
    void addAlertRule(const AlertRule& rule);
    std::vector<AlertRule> checkAlerts();
    
    // 预设指标（交易相关）
    void recordOrderSubmitted(InstrumentID instrument_id);
    void recordOrderFilled(InstrumentID instrument_id);
    void recordOrderCancelled(InstrumentID instrument_id);
    void recordTrade(InstrumentID instrument_id, Quantity quantity);
    void recordLatencyNs(const std::string& operation, double latency_ns);
    
    // 性能指标
    void recordMatchingLatency(double latency_ns);
    void recordOrderProcessingLatency(double latency_ns);
    void recordDatabaseLatency(double latency_ms);
    
    // 系统指标
    void recordSystemMetrics();  // CPU, Memory, etc.
    
private:
    mutable std::mutex mutex_;
    
    // 指标存储
    std::unordered_map<std::string, Metric> metrics_;
    
    // 告警规则
    std::vector<AlertRule> alert_rules_;
    
    // 告警状态
    struct AlertState {
        bool triggered = false;
        int64_t first_triggered_time = 0;
        int64_t last_triggered_time = 0;
    };
    std::unordered_map<std::string, AlertState> alert_states_;
    
    // 生成指标key
    std::string makeMetricKey(const std::string& name,
                             const std::unordered_map<std::string, std::string>& labels) const;
};

} // namespace perpetual


#include "types.h"
#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <vector>
#include <atomic>

namespace perpetual {

// 监控系统（Prometheus格式）
class MonitoringSystem {
public:
    // 指标类型
    enum MetricType {
        COUNTER,    // 计数器（只增不减）
        GAUGE,      // 仪表盘（可增可减）
        HISTOGRAM,  // 直方图（分布）
        SUMMARY     // 摘要（分位数）
    };
    
    // 指标
    struct Metric {
        std::string name;
        MetricType type;
        double value = 0.0;
        std::unordered_map<std::string, std::string> labels;
        int64_t timestamp = 0;
    };
    
    // 告警规则
    struct AlertRule {
        std::string name;
        std::string metric_name;
        std::string condition;  // e.g., "value > 100"
        std::string severity;   // "critical", "warning", "info"
        std::string message;
        int duration_seconds = 0;  // 持续多长时间触发
    };
    
    MonitoringSystem();
    
    // 指标记录
    void recordCounter(const std::string& name, double value,
                      const std::unordered_map<std::string, std::string>& labels = {});
    void recordGauge(const std::string& name, double value,
                    const std::unordered_map<std::string, std::string>& labels = {});
    void recordHistogram(const std::string& name, double value,
                        const std::unordered_map<std::string, std::string>& labels = {});
    
    // 增加计数器
    void incrementCounter(const std::string& name,
                         const std::unordered_map<std::string, std::string>& labels = {},
                         double amount = 1.0);
    
    // 设置仪表盘值
    void setGauge(const std::string& name, double value,
                 const std::unordered_map<std::string, std::string>& labels = {});
    
    // 记录延迟
    void recordLatency(const std::string& operation, double latency_ms);
    
    // 获取指标（Prometheus格式）
    std::string getPrometheusMetrics() const;
    
    // 获取特定指标
    double getMetricValue(const std::string& name,
                         const std::unordered_map<std::string, std::string>& labels = {}) const;
    
    // 告警管理
    void addAlertRule(const AlertRule& rule);
    std::vector<AlertRule> checkAlerts();
    
    // 预设指标（交易相关）
    void recordOrderSubmitted(InstrumentID instrument_id);
    void recordOrderFilled(InstrumentID instrument_id);
    void recordOrderCancelled(InstrumentID instrument_id);
    void recordTrade(InstrumentID instrument_id, Quantity quantity);
    void recordLatencyNs(const std::string& operation, double latency_ns);
    
    // 性能指标
    void recordMatchingLatency(double latency_ns);
    void recordOrderProcessingLatency(double latency_ns);
    void recordDatabaseLatency(double latency_ms);
    
    // 系统指标
    void recordSystemMetrics();  // CPU, Memory, etc.
    
private:
    mutable std::mutex mutex_;
    
    // 指标存储
    std::unordered_map<std::string, Metric> metrics_;
    
    // 告警规则
    std::vector<AlertRule> alert_rules_;
    
    // 告警状态
    struct AlertState {
        bool triggered = false;
        int64_t first_triggered_time = 0;
        int64_t last_triggered_time = 0;
    };
    std::unordered_map<std::string, AlertState> alert_states_;
    
    // 生成指标key
    std::string makeMetricKey(const std::string& name,
                             const std::unordered_map<std::string, std::string>& labels) const;
};

} // namespace perpetual
