#pragma once

#include "types.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <memory>
#include <openssl/evp.h>
#include <openssl/hmac.h>

namespace perpetual {

// JWT Token管理
class JWTManager {
public:
    struct TokenPayload {
        UserID user_id;
        std::string username;
        std::vector<std::string> roles;
        int64_t exp;  // Expiration time
    };
    
    static std::string generateToken(const TokenPayload& payload, const std::string& secret);
    static bool verifyToken(const std::string& token, const std::string& secret, TokenPayload& payload);
    static bool isTokenExpired(const TokenPayload& payload);
};

// API密钥管理
class APIKeyManager {
public:
    struct APIKey {
        std::string api_key;
        std::string api_secret;
        UserID user_id;
        std::string name;
        std::vector<std::string> permissions;
        bool is_active;
        int64_t created_at;
        int64_t last_used_at;
        std::string ip_whitelist;  // Comma-separated IPs
    };
    
    // Generate new API key pair
    static std::pair<std::string, std::string> generateKeyPair();
    
    // Create API key
    std::string createAPIKey(UserID user_id, const std::string& name, 
                            const std::vector<std::string>& permissions,
                            const std::string& ip_whitelist = "");
    
    // Verify API key signature
    bool verifySignature(const std::string& api_key, const std::string& signature,
                        const std::string& timestamp, const std::string& method,
                        const std::string& path, const std::string& body);
    
    // Get API key info
    const APIKey* getAPIKey(const std::string& api_key) const;
    
    // Revoke API key
    bool revokeAPIKey(const std::string& api_key);
    
    // Check IP whitelist
    bool checkIPWhitelist(const std::string& api_key, const std::string& ip) const;
    
private:
    mutable std::mutex keys_mutex_;
    std::unordered_map<std::string, APIKey> api_keys_;
    std::unordered_map<std::string, std::string> key_to_secret_;  // API key -> secret
    
    std::string hashSecret(const std::string& secret) const;
};

// 用户认证管理器
class AuthManager {
public:
    AuthManager();
    struct User {
        UserID user_id;
        std::string username;
        std::string email;
        std::string password_hash;
        std::vector<std::string> roles;
        bool is_active;
        bool is_verified;
        int64_t created_at;
        int64_t last_login_at;
    };
    
    // Register new user
    bool registerUser(const std::string& username, const std::string& email,
                     const std::string& password, std::string& error_msg);
    
    // Login
    bool login(const std::string& username_or_email, const std::string& password,
              std::string& token, std::string& error_msg);
    
    // Verify JWT token
    bool verifyToken(const std::string& token, UserID& user_id, std::vector<std::string>& roles);
    
    // Get user by ID
    const User* getUser(UserID user_id) const;
    
    // Change password
    bool changePassword(UserID user_id, const std::string& old_password,
                       const std::string& new_password, std::string& error_msg);
    
    // Check permission
    bool hasPermission(UserID user_id, const std::string& permission) const;
    
    // Generate password hash
    static std::string hashPassword(const std::string& password);
    
    // Verify password
    static bool verifyPassword(const std::string& password, const std::string& hash);
    
private:
    mutable std::mutex users_mutex_;
    std::unordered_map<UserID, User> users_;
    std::unordered_map<std::string, UserID> username_to_id_;
    std::unordered_map<std::string, UserID> email_to_id_;
    
    UserID next_user_id_{1000000};
    std::string jwt_secret_ = "change-this-secret-in-production";
    
    APIKeyManager api_key_manager_;
    JWTManager jwt_manager_;
};

} // namespace perpetual

