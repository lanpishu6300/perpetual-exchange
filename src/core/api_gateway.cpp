#include "core/api_gateway.h"
#include "core/auth_manager.h"
#include "core/types.h"
#include <algorithm>
#include <regex>
#include <map>

namespace perpetual {

APIGateway::APIGateway() {
    // Initialize default routes
}

void APIGateway::addRoute(const RouteRule& rule) {
    std::lock_guard<std::mutex> lock(mutex_);
    routes_.push_back(rule);
}

APIGateway::Response APIGateway::handleRequest(const RequestContext& context) {
    Response response;
    
    // Match route
    RouteRule* route = matchRoute(context.method, context.path);
    if (!route) {
        response.status_code = 404;
        response.body = R"({"error": "Route not found"})";
        return response;
    }
    
    // Check authentication
    if (route->require_auth) {
        if (!authenticate(const_cast<RequestContext&>(context))) {
            response.status_code = 401;
            response.body = R"({"error": "Unauthorized"})";
            return response;
        }
    }
    
    // Check authorization
    if (!authorize(context, *route)) {
        response.status_code = 403;
        response.body = R"({"error": "Forbidden"})";
        return response;
    }
    
    // Check rate limit
    if (!checkRateLimit(context, *route)) {
        response.status_code = 429;
        response.body = R"({"error": "Rate limit exceeded"})";
        return response;
    }
    
    // Forward to service (simplified - in production would make actual RPC call)
    response.status_code = 200;
    response.body = R"({"message": "Request forwarded to service"})";
    
    // Log request
    logRequest(context, response);
    
    return response;
}

bool APIGateway::authenticate(RequestContext& context) {
    if (!auth_manager_) {
        return false;
    }
    
    // Check for JWT token in Authorization header
    // Note: RequestContext needs headers field - simplified implementation
    // In production, extract from HTTP headers properly
    
    // Simplified: check for token in headers map (would need to populate from HTTP request)
    std::map<std::string, std::string> headers_map;  // Would come from HTTP request parsing
    
    // Check headers from context (would be populated from HTTP request parsing)
    auto auth_header_it = context.headers.find("Authorization");
    if (auth_header_it != context.headers.end()) {
        std::string auth_header = auth_header_it->second;
        if (auth_header.find("Bearer ") == 0) {
            std::string token = auth_header.substr(7);
            std::vector<std::string> roles;
            if (auth_manager_->verifyToken(token, context.user_id, roles)) {
                context.authenticated = true;
                context.roles = roles;
                return true;
            }
        }
    }
    
    // Check for API key
    auto api_key_it = context.headers.find("X-API-Key");
    auto signature_it = context.headers.find("X-Signature");
    auto timestamp_it = context.headers.find("X-Timestamp");
    
    if (api_key_it != context.headers.end() && 
        signature_it != context.headers.end() &&
        timestamp_it != context.headers.end()) {
        
        std::string api_key = api_key_it->second;
        std::string signature = signature_it->second;
        std::string timestamp = timestamp_it->second;
        
        if (parseAPIKey(api_key, signature, timestamp, context.method, 
                       context.path, context.body, context.user_id, context.roles)) {
            context.authenticated = true;
            return true;
        }
    }
    
    return false;
}

bool APIGateway::authorize(const RequestContext& context, const RouteRule& rule) {
    if (rule.required_permissions.empty()) {
        return true;  // No permissions required
    }
    
    if (!context.authenticated) {
        return false;
    }
    
    // Check if user has required permissions
    // Simplified - in production use proper permission checking
    for (const auto& required_perm : rule.required_permissions) {
        if (!auth_manager_->hasPermission(context.user_id, required_perm)) {
            // Check if user has admin role
            bool is_admin = std::find(context.roles.begin(), context.roles.end(), "admin") 
                           != context.roles.end();
            if (!is_admin) {
                return false;
            }
        }
    }
    
    return true;
}

bool APIGateway::checkRateLimit(const RequestContext& context, const RouteRule& rule) {
    if (rule.rate_limit <= 0) {
        return true;  // No rate limit
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Use IP or user ID as key
    std::string key = context.client_ip;
    if (context.user_id != 0) {
        key = "user_" + std::to_string(context.user_id);
    }
    
    auto now = get_current_timestamp() / 1000000000;  // Convert to seconds
    auto it = rate_limit_map_.find(key);
    
    if (it == rate_limit_map_.end()) {
        // First request
        RateLimitEntry entry;
        entry.count = 1;
        entry.reset_time = now + 1;  // Reset every second
        rate_limit_map_[key] = entry;
        return true;
    }
    
    RateLimitEntry& entry = it->second;
    
    // Reset if time window passed
    if (now >= entry.reset_time) {
        entry.count = 1;
        entry.reset_time = now + 1;
        return true;
    }
    
    // Check limit
    if (entry.count >= rule.rate_limit) {
        return false;
    }
    
    entry.count++;
    return true;
}

APIGateway::RouteRule* APIGateway::matchRoute(const std::string& method, const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Convert method string to RouteRule method (simplified)
    std::string route_method = method;
    
    for (auto& route : routes_) {
        // Simple string comparison for method (in production, use enum)
        if (route.method != route_method) {
            continue;
        }
        
        // Simple pattern matching (convert to regex in production)
        if (route.pattern == path) {
            return &route;
        }
        
        // Support wildcard patterns
        if (!route.pattern.empty() && route.pattern.back() == '*' && 
            path.find(route.pattern.substr(0, route.pattern.length() - 1)) == 0) {
            return &route;
        }
    }
    
    return nullptr;
}

void APIGateway::logRequest(const RequestContext& context, const Response& response) {
    // In production, use proper logging framework
    // For now, just store in memory or write to log file
    int64_t timestamp = get_current_timestamp() / 1000000000;
    
    // Log format: timestamp method path status_code user_id client_ip
    // Would use Logger in production
}

bool APIGateway::parseJWTToken(const std::string& token, UserID& user_id,
                               std::vector<std::string>& roles) {
    if (!auth_manager_) {
        return false;
    }
    return auth_manager_->verifyToken(token, user_id, roles);
}

bool APIGateway::verifyAPIKey(const std::string& api_key, const std::string& signature,
                              const std::string& timestamp, const std::string& method,
                              const std::string& path, const std::string& body,
                              UserID& user_id, std::vector<std::string>& permissions) {
    if (!auth_manager_) {
        return false;
    }
    
    // Get API key manager from auth manager (would need to expose this)
    // For now, simplified verification
    return false;  // Would use APIKeyManager in production
}

bool APIGateway::parseAPIKey(const std::string& api_key, const std::string& signature,
                             const std::string& timestamp, const std::string& method,
                             const std::string& path, const std::string& body,
                             UserID& user_id, std::vector<std::string>& roles) {
    // Simplified API key parsing
    // In production, use APIKeyManager to verify signature
    return false;
}

} // namespace perpetual
