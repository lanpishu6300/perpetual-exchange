#include "core/rest_api_server.h"
#include "core/api_gateway.h"
#include "core/auth_manager.h"
#include "core/types.h"
#include <sstream>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>

namespace perpetual {

RESTAPIServer::RESTAPIServer(int port) : port_(port), server_socket_(-1) {
}

RESTAPIServer::~RESTAPIServer() {
    stop();
}

bool RESTAPIServer::start() {
    if (running_) {
        return false;
    }
    
    // Create socket
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ < 0) {
        return false;
    }
    
    // Set socket options
    int opt = 1;
    setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Bind
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);
    
    if (bind(server_socket_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        close(server_socket_);
        return false;
    }
    
    // Listen
    if (listen(server_socket_, 10) < 0) {
        close(server_socket_);
        return false;
    }
    
    running_ = true;
    
    // Start server thread
    server_thread_ = std::thread([this]() {
        while (running_) {
            struct sockaddr_in client_address;
            socklen_t client_len = sizeof(client_address);
            
            int client_socket = accept(server_socket_, (struct sockaddr*)&client_address, &client_len);
            if (client_socket < 0) {
                continue;
            }
            
            // Handle connection in separate thread
            std::thread([this, client_socket]() {
                handleConnection(client_socket);
                close(client_socket);
            }).detach();
        }
    });
    
    return true;
}

void RESTAPIServer::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    if (server_socket_ >= 0) {
        close(server_socket_);
        server_socket_ = -1;
    }
    
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
}

void RESTAPIServer::registerRoute(HTTPMethod method, const std::string& path,
                                  RouteHandler handler, bool require_auth) {
    RouteKey key;
    key.method = method;
    key.path = path;
    
    routes_[key] = handler;
    route_auth_required_[key] = require_auth;
}

void RESTAPIServer::handleConnection(int socket_fd) {
    char buffer[4096] = {0};
    int bytes_read = read(socket_fd, buffer, sizeof(buffer) - 1);
    
    if (bytes_read <= 0) {
        return;
    }
    
    std::string request_str(buffer, bytes_read);
    HTTPRequest request;
    
    if (!parseHTTPRequest(request_str, request)) {
        sendErrorResponse(socket_fd, 400, "Bad Request");
        return;
    }
    
    // Authenticate if required
    UserID user_id = 0;
    if (route_auth_required_[RouteKey{request.method, request.path}]) {
        if (!authenticateRequest(request, user_id)) {
            sendErrorResponse(socket_fd, 401, "Unauthorized");
            return;
        }
        request.user_id = user_id;
    }
    
    // Find route
    RouteKey key{request.method, request.path};
    auto it = routes_.find(key);
    if (it == routes_.end()) {
        sendErrorResponse(socket_fd, 404, "Not Found");
        return;
    }
    
    // Handle request
    HTTPResponse response = it->second(request, user_id);
    
    // Send response
    std::string response_str = buildHTTPResponse(response);
    send(socket_fd, response_str.c_str(), response_str.length(), 0);
}

bool RESTAPIServer::parseHTTPRequest(const std::string& raw_request, HTTPRequest& req) {
    std::istringstream iss(raw_request);
    std::string line;
    
    // Parse first line: METHOD PATH HTTP/VERSION
    if (!std::getline(iss, line)) {
        return false;
    }
    
    size_t first_space = line.find(' ');
    size_t second_space = line.find(' ', first_space + 1);
    
    if (first_space == std::string::npos || second_space == std::string::npos) {
        return false;
    }
    
    std::string method_str = line.substr(0, first_space);
    std::string path_full = line.substr(first_space + 1, second_space - first_space - 1);
    
    // Parse path and query string
    size_t query_pos = path_full.find('?');
    req.path = path_full.substr(0, query_pos);
    if (query_pos != std::string::npos) {
        req.query_string = path_full.substr(query_pos + 1);
    }
    
    // Parse method
    if (method_str == "GET") req.method = GET;
    else if (method_str == "POST") req.method = POST;
    else if (method_str == "PUT") req.method = PUT;
    else if (method_str == "DELETE") req.method = DELETE;
    else if (method_str == "PATCH") req.method = PATCH;
    else return false;
    
    // Parse headers
    while (std::getline(iss, line) && line != "\r") {
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = line.substr(0, colon_pos);
            std::string value = line.substr(colon_pos + 1);
            // Trim whitespace
            value.erase(0, value.find_first_not_of(" \t"));
            req.headers[key] = value;
        }
    }
    
    // Parse body
    std::ostringstream body_ss;
    while (std::getline(iss, line)) {
        body_ss << line << "\n";
    }
    req.body = body_ss.str();
    
    // Get client IP (would need to pass from accept)
    req.client_ip = "0.0.0.0";
    
    return true;
}

std::string RESTAPIServer::buildHTTPResponse(const HTTPResponse& resp) {
    std::ostringstream ss;
    
    ss << "HTTP/1.1 " << resp.status_code << " ";
    
    switch (resp.status_code) {
        case 200: ss << "OK"; break;
        case 400: ss << "Bad Request"; break;
        case 401: ss << "Unauthorized"; break;
        case 403: ss << "Forbidden"; break;
        case 404: ss << "Not Found"; break;
        case 500: ss << "Internal Server Error"; break;
        default: ss << "OK"; break;
    }
    
    ss << "\r\n";
    ss << "Content-Type: application/json\r\n";
    ss << "Content-Length: " << resp.body.length() << "\r\n";
    
    for (const auto& header : resp.headers) {
        ss << header.first << ": " << header.second << "\r\n";
    }
    
    ss << "\r\n";
    ss << resp.body;
    
    return ss.str();
}

bool RESTAPIServer::authenticateRequest(const HTTPRequest& req, UserID& user_id) {
    if (!auth_manager_) {
        return false;
    }
    
    // Check Authorization header
    auto auth_it = req.headers.find("Authorization");
    if (auth_it != req.headers.end()) {
        std::string auth_header = auth_it->second;
        if (auth_header.find("Bearer ") == 0) {
            std::string token = auth_header.substr(7);
            std::vector<std::string> roles;
            if (auth_manager_->verifyToken(token, user_id, roles)) {
                return true;
            }
        }
    }
    
    return false;
}

void RESTAPIServer::sendErrorResponse(int socket_fd, int status_code, const std::string& message) {
    HTTPResponse resp;
    resp.status_code = status_code;
    resp.body = R"({"error": ")" + message + R"("})";
    
    std::string response_str = buildHTTPResponse(resp);
    send(socket_fd, response_str.c_str(), response_str.length(), 0);
}

// API endpoint implementations (simplified)

RESTAPIServer::HTTPResponse RESTAPIServer::handleRegister(const HTTPRequest& req) {
    HTTPResponse resp;
    // Parse request body and register user
    resp.status_code = 200;
    resp.body = R"({"message": "User registered"})";
    return resp;
}

RESTAPIServer::HTTPResponse RESTAPIServer::handleLogin(const HTTPRequest& req) {
    HTTPResponse resp;
    // Parse credentials and login
    resp.status_code = 200;
    resp.body = R"({"token": "jwt_token_here"})";
    return resp;
}

RESTAPIServer::HTTPResponse RESTAPIServer::handleGetProfile(const HTTPRequest& req, UserID user_id) {
    HTTPResponse resp;
    resp.status_code = 200;
    resp.body = R"({"user_id": )" + std::to_string(user_id) + R"(})";
    return resp;
}

RESTAPIServer::HTTPResponse RESTAPIServer::handleSubmitOrder(const HTTPRequest& req, UserID user_id) {
    HTTPResponse resp;
    resp.status_code = 200;
    resp.body = R"({"order_id": 123456})";
    return resp;
}

RESTAPIServer::HTTPResponse RESTAPIServer::handleCancelOrder(const HTTPRequest& req, UserID user_id) {
    HTTPResponse resp;
    resp.status_code = 200;
    resp.body = R"({"success": true})";
    return resp;
}

RESTAPIServer::HTTPResponse RESTAPIServer::handleGetOrder(const HTTPRequest& req, UserID user_id) {
    HTTPResponse resp;
    resp.status_code = 200;
    resp.body = R"({"order": {...}})";
    return resp;
}

RESTAPIServer::HTTPResponse RESTAPIServer::handleGetOrders(const HTTPRequest& req, UserID user_id) {
    HTTPResponse resp;
    resp.status_code = 200;
    resp.body = R"({"orders": []})";
    return resp;
}

RESTAPIServer::HTTPResponse RESTAPIServer::handleGetAccount(const HTTPRequest& req, UserID user_id) {
    HTTPResponse resp;
    resp.status_code = 200;
    resp.body = R"({"balance": 10000.0})";
    return resp;
}

RESTAPIServer::HTTPResponse RESTAPIServer::handleGetBalanceHistory(const HTTPRequest& req, UserID user_id) {
    HTTPResponse resp;
    resp.status_code = 200;
    resp.body = R"({"history": []})";
    return resp;
}

RESTAPIServer::HTTPResponse RESTAPIServer::handleGetPositions(const HTTPRequest& req, UserID user_id) {
    HTTPResponse resp;
    resp.status_code = 200;
    resp.body = R"({"positions": []})";
    return resp;
}

RESTAPIServer::HTTPResponse RESTAPIServer::handleGetPositionHistory(const HTTPRequest& req, UserID user_id) {
    HTTPResponse resp;
    resp.status_code = 200;
    resp.body = R"({"history": []})";
    return resp;
}

RESTAPIServer::HTTPResponse RESTAPIServer::handleGetOrderBook(const HTTPRequest& req) {
    HTTPResponse resp;
    resp.status_code = 200;
    resp.body = R"({"bids": [], "asks": []})";
    return resp;
}

RESTAPIServer::HTTPResponse RESTAPIServer::handleGetTrades(const HTTPRequest& req) {
    HTTPResponse resp;
    resp.status_code = 200;
    resp.body = R"({"trades": []})";
    return resp;
}

RESTAPIServer::HTTPResponse RESTAPIServer::handleGetKLine(const HTTPRequest& req) {
    HTTPResponse resp;
    resp.status_code = 200;
    resp.body = R"({"klines": []})";
    return resp;
}

RESTAPIServer::HTTPResponse RESTAPIServer::handleGetTicker24H(const HTTPRequest& req) {
    HTTPResponse resp;
    resp.status_code = 200;
    resp.body = R"({"ticker": {...}})";
    return resp;
}

RESTAPIServer::HTTPResponse RESTAPIServer::handleCreateAPIKey(const HTTPRequest& req, UserID user_id) {
    HTTPResponse resp;
    resp.status_code = 200;
    resp.body = R"({"api_key": "pk_...", "api_secret": "sk_..."})";
    return resp;
}

RESTAPIServer::HTTPResponse RESTAPIServer::handleGetAPIKeys(const HTTPRequest& req, UserID user_id) {
    HTTPResponse resp;
    resp.status_code = 200;
    resp.body = R"({"api_keys": []})";
    return resp;
}

RESTAPIServer::HTTPResponse RESTAPIServer::handleRevokeAPIKey(const HTTPRequest& req, UserID user_id) {
    HTTPResponse resp;
    resp.status_code = 200;
    resp.body = R"({"success": true})";
    return resp;
}

} // namespace perpetual


#include "core/auth_manager.h"
#include "core/types.h"
#include <sstream>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>

namespace perpetual {

RESTAPIServer::RESTAPIServer(int port) : port_(port), server_socket_(-1) {
}

RESTAPIServer::~RESTAPIServer() {
    stop();
}

bool RESTAPIServer::start() {
    if (running_) {
        return false;
    }
    
    // Create socket
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ < 0) {
        return false;
    }
    
    // Set socket options
    int opt = 1;
    setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Bind
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);
    
    if (bind(server_socket_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        close(server_socket_);
        return false;
    }
    
    // Listen
    if (listen(server_socket_, 10) < 0) {
        close(server_socket_);
        return false;
    }
    
    running_ = true;
    
    // Start server thread
    server_thread_ = std::thread([this]() {
        while (running_) {
            struct sockaddr_in client_address;
            socklen_t client_len = sizeof(client_address);
            
            int client_socket = accept(server_socket_, (struct sockaddr*)&client_address, &client_len);
            if (client_socket < 0) {
                continue;
            }
            
            // Handle connection in separate thread
            std::thread([this, client_socket]() {
                handleConnection(client_socket);
                close(client_socket);
            }).detach();
        }
    });
    
    return true;
}

void RESTAPIServer::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    if (server_socket_ >= 0) {
        close(server_socket_);
        server_socket_ = -1;
    }
    
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
}

void RESTAPIServer::registerRoute(HTTPMethod method, const std::string& path,
                                  RouteHandler handler, bool require_auth) {
    RouteKey key;
    key.method = method;
    key.path = path;
    
    routes_[key] = handler;
    route_auth_required_[key] = require_auth;
}

void RESTAPIServer::handleConnection(int socket_fd) {
    char buffer[4096] = {0};
    int bytes_read = read(socket_fd, buffer, sizeof(buffer) - 1);
    
    if (bytes_read <= 0) {
        return;
    }
    
    std::string request_str(buffer, bytes_read);
    HTTPRequest request;
    
    if (!parseHTTPRequest(request_str, request)) {
        sendErrorResponse(socket_fd, 400, "Bad Request");
        return;
    }
    
    // Authenticate if required
    UserID user_id = 0;
    if (route_auth_required_[RouteKey{request.method, request.path}]) {
        if (!authenticateRequest(request, user_id)) {
            sendErrorResponse(socket_fd, 401, "Unauthorized");
            return;
        }
        request.user_id = user_id;
    }
    
    // Find route
    RouteKey key{request.method, request.path};
    auto it = routes_.find(key);
    if (it == routes_.end()) {
        sendErrorResponse(socket_fd, 404, "Not Found");
        return;
    }
    
    // Handle request
    HTTPResponse response = it->second(request, user_id);
    
    // Send response
    std::string response_str = buildHTTPResponse(response);
    send(socket_fd, response_str.c_str(), response_str.length(), 0);
}

bool RESTAPIServer::parseHTTPRequest(const std::string& raw_request, HTTPRequest& req) {
    std::istringstream iss(raw_request);
    std::string line;
    
    // Parse first line: METHOD PATH HTTP/VERSION
    if (!std::getline(iss, line)) {
        return false;
    }
    
    size_t first_space = line.find(' ');
    size_t second_space = line.find(' ', first_space + 1);
    
    if (first_space == std::string::npos || second_space == std::string::npos) {
        return false;
    }
    
    std::string method_str = line.substr(0, first_space);
    std::string path_full = line.substr(first_space + 1, second_space - first_space - 1);
    
    // Parse path and query string
    size_t query_pos = path_full.find('?');
    req.path = path_full.substr(0, query_pos);
    if (query_pos != std::string::npos) {
        req.query_string = path_full.substr(query_pos + 1);
    }
    
    // Parse method
    if (method_str == "GET") req.method = GET;
    else if (method_str == "POST") req.method = POST;
    else if (method_str == "PUT") req.method = PUT;
    else if (method_str == "DELETE") req.method = DELETE;
    else if (method_str == "PATCH") req.method = PATCH;
    else return false;
    
    // Parse headers
    while (std::getline(iss, line) && line != "\r") {
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = line.substr(0, colon_pos);
            std::string value = line.substr(colon_pos + 1);
            // Trim whitespace
            value.erase(0, value.find_first_not_of(" \t"));
            req.headers[key] = value;
        }
    }
    
    // Parse body
    std::ostringstream body_ss;
    while (std::getline(iss, line)) {
        body_ss << line << "\n";
    }
    req.body = body_ss.str();
    
    // Get client IP (would need to pass from accept)
    req.client_ip = "0.0.0.0";
    
    return true;
}

std::string RESTAPIServer::buildHTTPResponse(const HTTPResponse& resp) {
    std::ostringstream ss;
    
    ss << "HTTP/1.1 " << resp.status_code << " ";
    
    switch (resp.status_code) {
        case 200: ss << "OK"; break;
        case 400: ss << "Bad Request"; break;
        case 401: ss << "Unauthorized"; break;
        case 403: ss << "Forbidden"; break;
        case 404: ss << "Not Found"; break;
        case 500: ss << "Internal Server Error"; break;
        default: ss << "OK"; break;
    }
    
    ss << "\r\n";
    ss << "Content-Type: application/json\r\n";
    ss << "Content-Length: " << resp.body.length() << "\r\n";
    
    for (const auto& header : resp.headers) {
        ss << header.first << ": " << header.second << "\r\n";
    }
    
    ss << "\r\n";
    ss << resp.body;
    
    return ss.str();
}

bool RESTAPIServer::authenticateRequest(const HTTPRequest& req, UserID& user_id) {
    if (!auth_manager_) {
        return false;
    }
    
    // Check Authorization header
    auto auth_it = req.headers.find("Authorization");
    if (auth_it != req.headers.end()) {
        std::string auth_header = auth_it->second;
        if (auth_header.find("Bearer ") == 0) {
            std::string token = auth_header.substr(7);
            std::vector<std::string> roles;
            if (auth_manager_->verifyToken(token, user_id, roles)) {
                return true;
            }
        }
    }
    
    return false;
}

void RESTAPIServer::sendErrorResponse(int socket_fd, int status_code, const std::string& message) {
    HTTPResponse resp;
    resp.status_code = status_code;
    resp.body = R"({"error": ")" + message + R"("})";
    
    std::string response_str = buildHTTPResponse(resp);
    send(socket_fd, response_str.c_str(), response_str.length(), 0);
}

// API endpoint implementations (simplified)

RESTAPIServer::HTTPResponse RESTAPIServer::handleRegister(const HTTPRequest& req) {
    HTTPResponse resp;
    // Parse request body and register user
    resp.status_code = 200;
    resp.body = R"({"message": "User registered"})";
    return resp;
}

RESTAPIServer::HTTPResponse RESTAPIServer::handleLogin(const HTTPRequest& req) {
    HTTPResponse resp;
    // Parse credentials and login
    resp.status_code = 200;
    resp.body = R"({"token": "jwt_token_here"})";
    return resp;
}

RESTAPIServer::HTTPResponse RESTAPIServer::handleGetProfile(const HTTPRequest& req, UserID user_id) {
    HTTPResponse resp;
    resp.status_code = 200;
    resp.body = R"({"user_id": )" + std::to_string(user_id) + R"(})";
    return resp;
}

RESTAPIServer::HTTPResponse RESTAPIServer::handleSubmitOrder(const HTTPRequest& req, UserID user_id) {
    HTTPResponse resp;
    resp.status_code = 200;
    resp.body = R"({"order_id": 123456})";
    return resp;
}

RESTAPIServer::HTTPResponse RESTAPIServer::handleCancelOrder(const HTTPRequest& req, UserID user_id) {
    HTTPResponse resp;
    resp.status_code = 200;
    resp.body = R"({"success": true})";
    return resp;
}

RESTAPIServer::HTTPResponse RESTAPIServer::handleGetOrder(const HTTPRequest& req, UserID user_id) {
    HTTPResponse resp;
    resp.status_code = 200;
    resp.body = R"({"order": {...}})";
    return resp;
}

RESTAPIServer::HTTPResponse RESTAPIServer::handleGetOrders(const HTTPRequest& req, UserID user_id) {
    HTTPResponse resp;
    resp.status_code = 200;
    resp.body = R"({"orders": []})";
    return resp;
}

RESTAPIServer::HTTPResponse RESTAPIServer::handleGetAccount(const HTTPRequest& req, UserID user_id) {
    HTTPResponse resp;
    resp.status_code = 200;
    resp.body = R"({"balance": 10000.0})";
    return resp;
}

RESTAPIServer::HTTPResponse RESTAPIServer::handleGetBalanceHistory(const HTTPRequest& req, UserID user_id) {
    HTTPResponse resp;
    resp.status_code = 200;
    resp.body = R"({"history": []})";
    return resp;
}

RESTAPIServer::HTTPResponse RESTAPIServer::handleGetPositions(const HTTPRequest& req, UserID user_id) {
    HTTPResponse resp;
    resp.status_code = 200;
    resp.body = R"({"positions": []})";
    return resp;
}

RESTAPIServer::HTTPResponse RESTAPIServer::handleGetPositionHistory(const HTTPRequest& req, UserID user_id) {
    HTTPResponse resp;
    resp.status_code = 200;
    resp.body = R"({"history": []})";
    return resp;
}

RESTAPIServer::HTTPResponse RESTAPIServer::handleGetOrderBook(const HTTPRequest& req) {
    HTTPResponse resp;
    resp.status_code = 200;
    resp.body = R"({"bids": [], "asks": []})";
    return resp;
}

RESTAPIServer::HTTPResponse RESTAPIServer::handleGetTrades(const HTTPRequest& req) {
    HTTPResponse resp;
    resp.status_code = 200;
    resp.body = R"({"trades": []})";
    return resp;
}

RESTAPIServer::HTTPResponse RESTAPIServer::handleGetKLine(const HTTPRequest& req) {
    HTTPResponse resp;
    resp.status_code = 200;
    resp.body = R"({"klines": []})";
    return resp;
}

RESTAPIServer::HTTPResponse RESTAPIServer::handleGetTicker24H(const HTTPRequest& req) {
    HTTPResponse resp;
    resp.status_code = 200;
    resp.body = R"({"ticker": {...}})";
    return resp;
}

RESTAPIServer::HTTPResponse RESTAPIServer::handleCreateAPIKey(const HTTPRequest& req, UserID user_id) {
    HTTPResponse resp;
    resp.status_code = 200;
    resp.body = R"({"api_key": "pk_...", "api_secret": "sk_..."})";
    return resp;
}

RESTAPIServer::HTTPResponse RESTAPIServer::handleGetAPIKeys(const HTTPRequest& req, UserID user_id) {
    HTTPResponse resp;
    resp.status_code = 200;
    resp.body = R"({"api_keys": []})";
    return resp;
}

RESTAPIServer::HTTPResponse RESTAPIServer::handleRevokeAPIKey(const HTTPRequest& req, UserID user_id) {
    HTTPResponse resp;
    resp.status_code = 200;
    resp.body = R"({"success": true})";
    return resp;
}

} // namespace perpetual
