#pragma once

#include "types.h"
#include "auth_manager.h"
#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <functional>
#include <memory>

namespace perpetual {

// API网关
class APIGateway {
public:
    // 请求上下文
    struct RequestContext {
        std::string method;           // HTTP method
        std::string path;             // Request path
        std::string query_string;     // Query string
        std::string body;             // Request body
        std::string client_ip;        // Client IP
        std::string user_agent;       // User agent
        std::unordered_map<std::string, std::string> headers;  // HTTP headers
        UserID user_id = 0;           // Authenticated user ID
        std::vector<std::string> roles; // User roles
        bool authenticated = false;   // Is authenticated
        int64_t timestamp = 0;        // Request timestamp
    };
    
    // 路由规则
    struct RouteRule {
        std::string pattern;          // Path pattern
        std::string service;          // Target service
        std::string method;           // HTTP method
        bool require_auth;            // Require authentication
        std::vector<std::string> required_permissions; // Required permissions
        int rate_limit = 0;           // Rate limit (requests per second)
    };
    
    // 响应
    struct Response {
        int status_code = 200;
        std::string body;
        std::unordered_map<std::string, std::string> headers;
    };
    
    APIGateway();
    
    // 添加路由规则
    void addRoute(const RouteRule& rule);
    
    // 处理请求
    Response handleRequest(const RequestContext& context);
    
    // 认证中间件
    bool authenticate(RequestContext& context);
    
    // 授权检查
    bool authorize(const RequestContext& context, const RouteRule& rule);
    
    // 限流检查
    bool checkRateLimit(const RequestContext& context, const RouteRule& rule);
    
    // 路由匹配
    RouteRule* matchRoute(const std::string& method, const std::string& path);
    
    // 设置认证管理器
    void setAuthManager(AuthManager* am) { auth_manager_ = am; }
    
    // 记录请求日志
    void logRequest(const RequestContext& context, const Response& response);
    
private:
    // 解析JWT token
    bool parseJWTToken(const std::string& token, UserID& user_id, 
                      std::vector<std::string>& roles);
    
    // 验证API key
    bool verifyAPIKey(const std::string& api_key, const std::string& signature,
                     const std::string& timestamp, const std::string& method,
                     const std::string& path, const std::string& body,
                     UserID& user_id, std::vector<std::string>& permissions);
    
    // 解析API key（从headers等）
    bool parseAPIKey(const std::string& api_key, const std::string& signature,
                     const std::string& timestamp, const std::string& method,
                     const std::string& path, const std::string& body,
                     UserID& user_id, std::vector<std::string>& roles);
    
    mutable std::mutex mutex_;
    
    std::vector<RouteRule> routes_;
    
    // 限流器：IP -> 请求计数
    struct RateLimitEntry {
        int count = 0;
        int64_t reset_time = 0;
    };
    std::unordered_map<std::string, RateLimitEntry> rate_limit_map_;
    
    AuthManager* auth_manager_ = nullptr;
};

} // namespace perpetual


#include "types.h"
#include "auth_manager.h"
#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <functional>
#include <memory>

namespace perpetual {

// API网关
class APIGateway {
public:
    // 请求上下文
    struct RequestContext {
        std::string method;           // HTTP method
        std::string path;             // Request path
        std::string query_string;     // Query string
        std::string body;             // Request body
        std::string client_ip;        // Client IP
        std::string user_agent;       // User agent
        std::unordered_map<std::string, std::string> headers;  // HTTP headers
        UserID user_id = 0;           // Authenticated user ID
        std::vector<std::string> roles; // User roles
        bool authenticated = false;   // Is authenticated
        int64_t timestamp = 0;        // Request timestamp
    };
    
    // 路由规则
    struct RouteRule {
        std::string pattern;          // Path pattern
        std::string service;          // Target service
        std::string method;           // HTTP method
        bool require_auth;            // Require authentication
        std::vector<std::string> required_permissions; // Required permissions
        int rate_limit = 0;           // Rate limit (requests per second)
    };
    
    // 响应
    struct Response {
        int status_code = 200;
        std::string body;
        std::unordered_map<std::string, std::string> headers;
    };
    
    APIGateway();
    
    // 添加路由规则
    void addRoute(const RouteRule& rule);
    
    // 处理请求
    Response handleRequest(const RequestContext& context);
    
    // 认证中间件
    bool authenticate(RequestContext& context);
    
    // 授权检查
    bool authorize(const RequestContext& context, const RouteRule& rule);
    
    // 限流检查
    bool checkRateLimit(const RequestContext& context, const RouteRule& rule);
    
    // 路由匹配
    RouteRule* matchRoute(const std::string& method, const std::string& path);
    
    // 设置认证管理器
    void setAuthManager(AuthManager* am) { auth_manager_ = am; }
    
    // 记录请求日志
    void logRequest(const RequestContext& context, const Response& response);
    
private:
    // 解析JWT token
    bool parseJWTToken(const std::string& token, UserID& user_id, 
                      std::vector<std::string>& roles);
    
    // 验证API key
    bool verifyAPIKey(const std::string& api_key, const std::string& signature,
                     const std::string& timestamp, const std::string& method,
                     const std::string& path, const std::string& body,
                     UserID& user_id, std::vector<std::string>& permissions);
    
    // 解析API key（从headers等）
    bool parseAPIKey(const std::string& api_key, const std::string& signature,
                     const std::string& timestamp, const std::string& method,
                     const std::string& path, const std::string& body,
                     UserID& user_id, std::vector<std::string>& roles);
    
    mutable std::mutex mutex_;
    
    std::vector<RouteRule> routes_;
    
    // 限流器：IP -> 请求计数
    struct RateLimitEntry {
        int count = 0;
        int64_t reset_time = 0;
    };
    std::unordered_map<std::string, RateLimitEntry> rate_limit_map_;
    
    AuthManager* auth_manager_ = nullptr;
};

} // namespace perpetual

