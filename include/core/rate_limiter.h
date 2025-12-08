#pragma once

#include <chrono>
#include <unordered_map>
#include <mutex>
#include <atomic>

namespace perpetual {

// Token bucket rate limiter
class RateLimiter {
public:
    RateLimiter(double rate_per_second, double burst_size);
    
    // Check if request is allowed
    bool allow();
    
    // Check if request is allowed for specific key (per-user rate limiting)
    bool allow(const std::string& key);
    
    void setRate(double rate_per_second);
    void setBurstSize(double burst_size);
    
private:
    struct TokenBucket {
        double tokens;
        std::chrono::steady_clock::time_point last_update;
    };
    
    void refill(TokenBucket& bucket);
    bool consume(TokenBucket& bucket, double tokens = 1.0);
    
    double rate_per_second_;
    double burst_size_;
    std::mutex mutex_;
    TokenBucket default_bucket_;
    std::unordered_map<std::string, TokenBucket> user_buckets_;
};

// Rate limit configuration
struct RateLimitConfig {
    double orders_per_second = 1000.0;
    double burst_size = 2000.0;
    double per_user_orders_per_second = 100.0;
    double per_user_burst_size = 200.0;
};

} // namespace perpetual


