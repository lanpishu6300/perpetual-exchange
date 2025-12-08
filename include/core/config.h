#pragma once

#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <mutex>

namespace perpetual {

class Config {
public:
    static Config& getInstance() {
        static Config instance;
        return instance;
    }
    
    bool loadFromFile(const std::string& config_file);
    bool loadFromEnv();
    
    std::string getString(const std::string& key, const std::string& default_value = "") const;
    int getInt(const std::string& key, int default_value = 0) const;
    double getDouble(const std::string& key, double default_value = 0.0) const;
    bool getBool(const std::string& key, bool default_value = false) const;
    
    void set(const std::string& key, const std::string& value);
    
    bool has(const std::string& key) const;
    
private:
    Config() = default;
    std::unordered_map<std::string, std::string> config_;
    mutable std::mutex mutex_;
};

// Configuration keys
namespace ConfigKeys {
    constexpr const char* LOG_LEVEL = "log.level";
    constexpr const char* LOG_FILE = "log.file";
    constexpr const char* MATCHING_THREADS = "matching.threads";
    constexpr const char* MAX_ORDERS_PER_USER = "limits.max_orders_per_user";
    constexpr const char* MAX_POSITION_SIZE = "limits.max_position_size";
    constexpr const char* ENABLE_PERSISTENCE = "persistence.enabled";
    constexpr const char* DB_PATH = "persistence.db_path";
    constexpr const char* ENABLE_METRICS = "metrics.enabled";
    constexpr const char* METRICS_PORT = "metrics.port";
}

} // namespace perpetual


