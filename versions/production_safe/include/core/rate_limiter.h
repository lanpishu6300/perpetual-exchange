#pragma once

#include "core/types.h"
#include <chrono>
#include <unordered_map>
#include <mutex>

namespace perpetual {

struct RateLimitConfig {
    double orders_per_second = 1000.0;
    double burst_size = 2000.0;
    double per_user_orders_per_second = 100.0;
    double per_user_burst_size = 200.0;
};

class RateLimiter {
public:
    RateLimiter(double rate, double burst);
    ~RateLimiter();
    
    bool checkLimit();
    bool checkLimit(UserID user_id);
    
private:
    double rate_;
    double burst_;
    std::chrono::steady_clock::time_point last_check_;
    double tokens_;
    std::mutex mutex_;
    
    std::unordered_map<UserID, std::pair<std::chrono::steady_clock::time_point, double>> user_tokens_;
    std::mutex user_mutex_;
};

} // namespace perpetual
