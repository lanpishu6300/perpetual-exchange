#pragma once

#include "core/types.h"
#include "api_gateway.h"
#include <string>
#include <functional>
#include <unordered_map>
#include <memory>
#include <thread>
#include <atomic>

namespace perpetual {

// RESTful API服务器
class RESTAPIServer {
public:
    // HTTP方法
    enum HTTPMethod {
        GET,
        POST,
        PUT,
        DELETE,
        PATCH
    };
    
    // HTTP请求
    struct HTTPRequest {
        HTTPMethod method;
        std::string path;
        std::unordered_map<std::string, std::string> headers;
        std::string body;
        std::unordered_map<std::string, std::string> query_params;
        std::string query_string;
        std::string client_ip;
        UserID user_id = 0;  // Set after authentication
    };
    
    // HTTP响应
    struct HTTPResponse {
        int status_code = 200;
        std::unordered_map<std::string, std::string> headers;
        std::string body;
    };
    
    // 路由处理器
    using RouteHandler = std::function<HTTPResponse(const HTTPRequest&, UserID user_id)>;
    
    RESTAPIServer(int port = 8080);
    ~RESTAPIServer();
    
    // 启动服务器
    bool start();
    void stop();
    bool isRunning() const { return running_; }
    
    // 注册路由
    void registerRoute(HTTPMethod method, const std::string& path, 
                      RouteHandler handler, bool require_auth = true);
    
    // 设置认证管理器
    void setAuthManager(class AuthManager* am) { auth_manager_ = am; }
    
    // 设置API网关
    void setAPIGateway(APIGateway* gateway) { api_gateway_ = gateway; }
    
    // API端点实现
    
    // 用户相关
    HTTPResponse handleRegister(const HTTPRequest& req);
    HTTPResponse handleLogin(const HTTPRequest& req);
    HTTPResponse handleGetProfile(const HTTPRequest& req, UserID user_id);
    HTTPResponse handleUpdateProfile(const HTTPRequest& req, UserID user_id);
    
    // 订单相关
    HTTPResponse handleSubmitOrder(const HTTPRequest& req, UserID user_id);
    HTTPResponse handleCancelOrder(const HTTPRequest& req, UserID user_id);
    HTTPResponse handleGetOrder(const HTTPRequest& req, UserID user_id);
    HTTPResponse handleGetOrders(const HTTPRequest& req, UserID user_id);
    
    // 账户相关
    HTTPResponse handleGetAccount(const HTTPRequest& req, UserID user_id);
    HTTPResponse handleGetBalanceHistory(const HTTPRequest& req, UserID user_id);
    
    // 持仓相关
    HTTPResponse handleGetPositions(const HTTPRequest& req, UserID user_id);
    HTTPResponse handleGetPositionHistory(const HTTPRequest& req, UserID user_id);
    
    // 市场数据相关
    HTTPResponse handleGetOrderBook(const HTTPRequest& req);
    HTTPResponse handleGetTrades(const HTTPRequest& req);
    HTTPResponse handleGetKLine(const HTTPRequest& req);
    HTTPResponse handleGetTicker24H(const HTTPRequest& req);
    
    // API密钥相关
    HTTPResponse handleCreateAPIKey(const HTTPRequest& req, UserID user_id);
    HTTPResponse handleGetAPIKeys(const HTTPRequest& req, UserID user_id);
    HTTPResponse handleRevokeAPIKey(const HTTPRequest& req, UserID user_id);
    
private:
    // 处理HTTP请求
    void handleConnection(int socket_fd);
    
    // 解析HTTP请求
    bool parseHTTPRequest(const std::string& raw_request, HTTPRequest& req);
    
    // 构建HTTP响应
    std::string buildHTTPResponse(const HTTPResponse& resp);
    
    // 认证检查
    bool authenticateRequest(const HTTPRequest& req, UserID& user_id);
    
    // 发送错误响应
    void sendErrorResponse(int socket_fd, int status_code, const std::string& message);
    
    int port_;
    int server_socket_;
    std::atomic<bool> running_{false};
    std::thread server_thread_;
    
    // 路由表
    struct RouteKey {
        HTTPMethod method;
        std::string path;
        
        bool operator==(const RouteKey& other) const {
            return method == other.method && path == other.path;
        }
    };
    
    struct RouteKeyHash {
        size_t operator()(const RouteKey& key) const {
            return std::hash<int>()(static_cast<int>(key.method)) ^
                   std::hash<std::string>()(key.path);
        }
    };
    
    std::unordered_map<RouteKey, RouteHandler, RouteKeyHash> routes_;
    std::unordered_map<RouteKey, bool, RouteKeyHash> route_auth_required_;
    
    AuthManager* auth_manager_ = nullptr;
    APIGateway* api_gateway_ = nullptr;
};

} // namespace perpetual


#include "core/types.h"
#include "api_gateway.h"
#include <string>
#include <functional>
#include <unordered_map>
#include <memory>
#include <thread>
#include <atomic>

namespace perpetual {

// RESTful API服务器
class RESTAPIServer {
public:
    // HTTP方法
    enum HTTPMethod {
        GET,
        POST,
        PUT,
        DELETE,
        PATCH
    };
    
    // HTTP请求
    struct HTTPRequest {
        HTTPMethod method;
        std::string path;
        std::unordered_map<std::string, std::string> headers;
        std::string body;
        std::unordered_map<std::string, std::string> query_params;
        std::string query_string;
        std::string client_ip;
        UserID user_id = 0;  // Set after authentication
    };
    
    // HTTP响应
    struct HTTPResponse {
        int status_code = 200;
        std::unordered_map<std::string, std::string> headers;
        std::string body;
    };
    
    // 路由处理器
    using RouteHandler = std::function<HTTPResponse(const HTTPRequest&, UserID user_id)>;
    
    RESTAPIServer(int port = 8080);
    ~RESTAPIServer();
    
    // 启动服务器
    bool start();
    void stop();
    bool isRunning() const { return running_; }
    
    // 注册路由
    void registerRoute(HTTPMethod method, const std::string& path, 
                      RouteHandler handler, bool require_auth = true);
    
    // 设置认证管理器
    void setAuthManager(class AuthManager* am) { auth_manager_ = am; }
    
    // 设置API网关
    void setAPIGateway(APIGateway* gateway) { api_gateway_ = gateway; }
    
    // API端点实现
    
    // 用户相关
    HTTPResponse handleRegister(const HTTPRequest& req);
    HTTPResponse handleLogin(const HTTPRequest& req);
    HTTPResponse handleGetProfile(const HTTPRequest& req, UserID user_id);
    HTTPResponse handleUpdateProfile(const HTTPRequest& req, UserID user_id);
    
    // 订单相关
    HTTPResponse handleSubmitOrder(const HTTPRequest& req, UserID user_id);
    HTTPResponse handleCancelOrder(const HTTPRequest& req, UserID user_id);
    HTTPResponse handleGetOrder(const HTTPRequest& req, UserID user_id);
    HTTPResponse handleGetOrders(const HTTPRequest& req, UserID user_id);
    
    // 账户相关
    HTTPResponse handleGetAccount(const HTTPRequest& req, UserID user_id);
    HTTPResponse handleGetBalanceHistory(const HTTPRequest& req, UserID user_id);
    
    // 持仓相关
    HTTPResponse handleGetPositions(const HTTPRequest& req, UserID user_id);
    HTTPResponse handleGetPositionHistory(const HTTPRequest& req, UserID user_id);
    
    // 市场数据相关
    HTTPResponse handleGetOrderBook(const HTTPRequest& req);
    HTTPResponse handleGetTrades(const HTTPRequest& req);
    HTTPResponse handleGetKLine(const HTTPRequest& req);
    HTTPResponse handleGetTicker24H(const HTTPRequest& req);
    
    // API密钥相关
    HTTPResponse handleCreateAPIKey(const HTTPRequest& req, UserID user_id);
    HTTPResponse handleGetAPIKeys(const HTTPRequest& req, UserID user_id);
    HTTPResponse handleRevokeAPIKey(const HTTPRequest& req, UserID user_id);
    
private:
    // 处理HTTP请求
    void handleConnection(int socket_fd);
    
    // 解析HTTP请求
    bool parseHTTPRequest(const std::string& raw_request, HTTPRequest& req);
    
    // 构建HTTP响应
    std::string buildHTTPResponse(const HTTPResponse& resp);
    
    // 认证检查
    bool authenticateRequest(const HTTPRequest& req, UserID& user_id);
    
    // 发送错误响应
    void sendErrorResponse(int socket_fd, int status_code, const std::string& message);
    
    int port_;
    int server_socket_;
    std::atomic<bool> running_{false};
    std::thread server_thread_;
    
    // 路由表
    struct RouteKey {
        HTTPMethod method;
        std::string path;
        
        bool operator==(const RouteKey& other) const {
            return method == other.method && path == other.path;
        }
    };
    
    struct RouteKeyHash {
        size_t operator()(const RouteKey& key) const {
            return std::hash<int>()(static_cast<int>(key.method)) ^
                   std::hash<std::string>()(key.path);
        }
    };
    
    std::unordered_map<RouteKey, RouteHandler, RouteKeyHash> routes_;
    std::unordered_map<RouteKey, bool, RouteKeyHash> route_auth_required_;
    
    AuthManager* auth_manager_ = nullptr;
    APIGateway* api_gateway_ = nullptr;
};

} // namespace perpetual
