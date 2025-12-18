#pragma once

#include <string>
#include <unordered_map>
#include <mutex>

namespace perpetual {

class Config {
public:
    static Config& getInstance();
    
    bool loadFromFile(const std::string& filename);
    void loadFromEnv();
    
    std::string getString(const std::string& key, const std::string& default_value = "") const;
    double getDouble(const std::string& key, double default_value = 0.0) const;
    int getInt(const std::string& key, int default_value = 0) const;
    bool getBool(const std::string& key, bool default_value = false) const;
    
private:
    Config() = default;
    std::unordered_map<std::string, std::string> config_;
    mutable std::mutex mutex_;
};

} // namespace perpetual
