#include "core/rate_limiter.h"
#include <algorithm>

namespace perpetual {

RateLimiter::RateLimiter(double rate, double burst)
    : rate_(rate), burst_(burst), tokens_(burst) {
    last_check_ = std::chrono::steady_clock::now();
}

RateLimiter::~RateLimiter() {
}

bool RateLimiter::checkLimit() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_check_).count() / 1000.0;
    
    tokens_ = std::min(burst_, tokens_ + elapsed * rate_);
    last_check_ = now;
    
    if (tokens_ >= 1.0) {
        tokens_ -= 1.0;
        return true;
    }
    return false;
}

bool RateLimiter::checkLimit(UserID user_id) {
    std::lock_guard<std::mutex> lock(user_mutex_);
    auto now = std::chrono::steady_clock::now();
    auto& user_data = user_tokens_[user_id];
    
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - user_data.first).count() / 1000.0;
    user_data.second = std::min(200.0, user_data.second + elapsed * 100.0);
    user_data.first = now;
    
    if (user_data.second >= 1.0) {
        user_data.second -= 1.0;
        return true;
    }
    return false;
}

} // namespace perpetual
