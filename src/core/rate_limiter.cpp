#include "core/rate_limiter.h"
#include <algorithm>

namespace perpetual {

RateLimiter::RateLimiter(double rate_per_second, double burst_size)
    : rate_per_second_(rate_per_second), burst_size_(burst_size) {
    default_bucket_.tokens = burst_size;
    default_bucket_.last_update = std::chrono::steady_clock::now();
}

void RateLimiter::refill(TokenBucket& bucket) {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - bucket.last_update).count() / 1000.0;
    
    bucket.tokens = std::min(burst_size_, bucket.tokens + elapsed * rate_per_second_);
    bucket.last_update = now;
}

bool RateLimiter::consume(TokenBucket& bucket, double tokens) {
    refill(bucket);
    if (bucket.tokens >= tokens) {
        bucket.tokens -= tokens;
        return true;
    }
    return false;
}

bool RateLimiter::allow() {
    std::lock_guard<std::mutex> lock(mutex_);
    return consume(default_bucket_);
}

bool RateLimiter::allow(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = user_buckets_.find(key);
    if (it == user_buckets_.end()) {
        TokenBucket bucket;
        bucket.tokens = burst_size_;
        bucket.last_update = std::chrono::steady_clock::now();
        user_buckets_[key] = bucket;
        it = user_buckets_.find(key);
    }
    
    return consume(it->second);
}

void RateLimiter::setRate(double rate_per_second) {
    std::lock_guard<std::mutex> lock(mutex_);
    rate_per_second_ = rate_per_second;
}

void RateLimiter::setBurstSize(double burst_size) {
    std::lock_guard<std::mutex> lock(mutex_);
    burst_size_ = burst_size;
}

} // namespace perpetual


