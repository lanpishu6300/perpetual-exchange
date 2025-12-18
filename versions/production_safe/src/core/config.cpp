#include "core/config.h"
#include <cstdlib>
#include <algorithm>

namespace perpetual {

Config& Config::getInstance() {
    static Config instance;
    return instance;
}

bool Config::loadFromFile(const std::string& filename) {
    // Stub implementation
    return true;
}

void Config::loadFromEnv() {
    // Stub implementation
}

std::string Config::getString(const std::string& key, const std::string& default_value) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = config_.find(key);
    if (it != config_.end()) {
        return it->second;
    }
    const char* env_val = std::getenv(key.c_str());
    if (env_val) {
        return std::string(env_val);
    }
    return default_value;
}

double Config::getDouble(const std::string& key, double default_value) const {
    std::string val = getString(key);
    if (val.empty()) return default_value;
    return std::stod(val);
}

int Config::getInt(const std::string& key, int default_value) const {
    std::string val = getString(key);
    if (val.empty()) return default_value;
    return std::stoi(val);
}

bool Config::getBool(const std::string& key, bool default_value) const {
    std::string val = getString(key);
    if (val.empty()) return default_value;
    std::transform(val.begin(), val.end(), val.begin(), ::tolower);
    return val == "true" || val == "1" || val == "yes";
}

} // namespace perpetual
