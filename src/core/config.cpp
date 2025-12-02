#include "core/config.h"
#include <algorithm>
#include <cctype>

namespace perpetual {

bool Config::loadFromFile(const std::string& config_file) {
    std::ifstream file(config_file);
    if (!file.is_open()) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string line;
    while (std::getline(file, line)) {
        // Remove comments
        size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos) {
            line = line.substr(0, comment_pos);
        }
        
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        if (line.empty()) {
            continue;
        }
        
        // Parse key=value
        size_t eq_pos = line.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = line.substr(0, eq_pos);
            std::string value = line.substr(eq_pos + 1);
            
            // Trim key and value
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            if (!key.empty()) {
                config_[key] = value;
            }
        }
    }
    
    return true;
}

bool Config::loadFromEnv() {
    // Load from environment variables with prefix PERPETUAL_
    // This is a simplified version - in production, use proper env parsing
    return true;
}

std::string Config::getString(const std::string& key, const std::string& default_value) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = config_.find(key);
    if (it != config_.end()) {
        return it->second;
    }
    return default_value;
}

int Config::getInt(const std::string& key, int default_value) const {
    std::string value = getString(key);
    if (value.empty()) {
        return default_value;
    }
    try {
        return std::stoi(value);
    } catch (...) {
        return default_value;
    }
}

double Config::getDouble(const std::string& key, double default_value) const {
    std::string value = getString(key);
    if (value.empty()) {
        return default_value;
    }
    try {
        return std::stod(value);
    } catch (...) {
        return default_value;
    }
}

bool Config::getBool(const std::string& key, bool default_value) const {
    std::string value = getString(key);
    if (value.empty()) {
        return default_value;
    }
    
    std::transform(value.begin(), value.end(), value.begin(), ::tolower);
    return value == "true" || value == "1" || value == "yes";
}

void Config::set(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_[key] = value;
}

bool Config::has(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_.find(key) != config_.end();
}

} // namespace perpetual
