#include "core/auth_manager.h"
#include "core/config.h"
#include "core/logger.h"
#include "core/types.h"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>

namespace perpetual {

namespace {
// Simple base64 encoding (for demo - use proper library in production)
std::string base64_encode(const std::string& data) {
    const char* base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string encoded;
    int val = 0, valb = -6;
    for (unsigned char c : data) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            encoded.push_back(base64_chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) encoded.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
    while (encoded.size() % 4) encoded.push_back('=');
    return encoded;
}

std::string base64_decode(const std::string& encoded) {
    const char* base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string decoded;
    int val = 0, valb = -8;
    for (char c : encoded) {
        if (c == '=') break;
        const char* pos = strchr(base64_chars, c);
        if (!pos) continue;
        val = (val << 6) + (pos - base64_chars);
        valb += 6;
        if (valb >= 0) {
            decoded.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return decoded;
}

} // anonymous namespace

// JWT Manager Implementation (simplified - use proper JWT library in production)
std::string perpetual::JWTManager::generateToken(const TokenPayload& payload, const std::string& secret) {
    // Simplified JWT: header.payload.signature
    std::stringstream claims;
    claims << payload.user_id << "|" << payload.username << "|" << payload.exp;
    
    std::string encoded_header = base64_encode("{\"typ\":\"JWT\",\"alg\":\"HS256\"}");
    std::string encoded_claims = base64_encode(claims.str());
    
    // Create signature
    std::string data = encoded_header + "." + encoded_claims;
    unsigned char* hmac_result;
    unsigned int hmac_len;
    hmac_result = HMAC(EVP_sha256(), secret.c_str(), secret.length(),
                      reinterpret_cast<const unsigned char*>(data.c_str()),
                      data.length(), nullptr, &hmac_len);
    
    std::string signature = base64_encode(std::string(reinterpret_cast<char*>(hmac_result), hmac_len));
    
    return encoded_header + "." + encoded_claims + "." + signature;
}

bool perpetual::JWTManager::verifyToken(const std::string& token, const std::string& secret, TokenPayload& payload) {
    std::vector<std::string> parts;
    std::stringstream ss(token);
    std::string item;
    
    while (std::getline(ss, item, '.')) {
        parts.push_back(item);
    }
    
    if (parts.size() != 3) {
        return false;
    }
    
    // Verify signature
    std::string data = parts[0] + "." + parts[1];
    unsigned char* hmac_result;
    unsigned int hmac_len;
    hmac_result = HMAC(EVP_sha256(), secret.c_str(), secret.length(),
                      reinterpret_cast<const unsigned char*>(data.c_str()),
                      data.length(), nullptr, &hmac_len);
    
    std::string expected_signature = base64_encode(std::string(reinterpret_cast<char*>(hmac_result), hmac_len));
    
    if (parts[2] != expected_signature) {
        return false;
    }
    
    // Decode claims (simplified format: user_id|username|exp)
    std::string claims_str = base64_decode(parts[1]);
    std::stringstream ss_claims(claims_str);
    std::string token_user_id, token_username, token_exp;
    
    if (!std::getline(ss_claims, token_user_id, '|') ||
        !std::getline(ss_claims, token_username, '|') ||
        !std::getline(ss_claims, token_exp, '|')) {
        return false;
    }
    
    payload.user_id = std::stoull(token_user_id);
    payload.username = token_username;
    payload.exp = std::stoll(token_exp);
    
    // Check expiration
    if (isTokenExpired(payload)) {
        return false;
    }
    
    return true;
}

bool perpetual::JWTManager::isTokenExpired(const TokenPayload& payload) {
    auto now = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;
    return payload.exp < now;
}

// API Key Manager Implementation
std::pair<std::string, std::string> APIKeyManager::generateKeyPair() {
    unsigned char api_key_bytes[32];
    unsigned char api_secret_bytes[64];
    
    RAND_bytes(api_key_bytes, sizeof(api_key_bytes));
    RAND_bytes(api_secret_bytes, sizeof(api_secret_bytes));
    
    std::string api_key = base64_encode(std::string(reinterpret_cast<char*>(api_key_bytes), sizeof(api_key_bytes)));
    std::string api_secret = base64_encode(std::string(reinterpret_cast<char*>(api_secret_bytes), sizeof(api_secret_bytes)));
    
    // Format: remove special chars, add prefix
    api_key = "pk_" + api_key.substr(0, 32);
    api_secret = "sk_" + api_secret.substr(0, 64);
    
    return {api_key, api_secret};
}

std::string APIKeyManager::createAPIKey(UserID user_id, const std::string& name,
                                       const std::vector<std::string>& permissions,
                                       const std::string& ip_whitelist) {
    auto [api_key, api_secret] = generateKeyPair();
    
    APIKey key;
    key.api_key = api_key;
    key.api_secret = hashSecret(api_secret);
    key.user_id = user_id;
    key.name = name;
    key.permissions = permissions;
    key.is_active = true;
    key.ip_whitelist = ip_whitelist;
    key.created_at = get_current_timestamp();
    key.last_used_at = 0;
    
    std::lock_guard<std::mutex> lock(keys_mutex_);
    api_keys_[api_key] = key;
    key_to_secret_[api_key] = api_secret;  // Store plain secret temporarily
    
    return api_secret;  // Return secret to user (only shown once)
}

bool APIKeyManager::verifySignature(const std::string& api_key, const std::string& signature,
                                   const std::string& timestamp, const std::string& method,
                                   const std::string& path, const std::string& body) {
    std::lock_guard<std::mutex> lock(keys_mutex_);
    auto it = key_to_secret_.find(api_key);
    if (it == key_to_secret_.end()) {
        return false;
    }
    
    auto key_it = api_keys_.find(api_key);
    if (key_it == api_keys_.end() || !key_it->second.is_active) {
        return false;
    }
    
    std::string api_secret = it->second;
    
    // Create message: timestamp + method + path + body
    std::string message = timestamp + method + path + body;
    
    // Calculate HMAC-SHA256
    unsigned char* hmac_result;
    unsigned int hmac_len;
    hmac_result = HMAC(EVP_sha256(), api_secret.c_str(), api_secret.length(),
                      reinterpret_cast<const unsigned char*>(message.c_str()),
                      message.length(), nullptr, &hmac_len);
    
    std::string expected_signature = base64_encode(std::string(reinterpret_cast<char*>(hmac_result), hmac_len));
    
    // Update last used time
    key_it->second.last_used_at = get_current_timestamp();
    
    return signature == expected_signature;
}

const APIKeyManager::APIKey* APIKeyManager::getAPIKey(const std::string& api_key) const {
    std::lock_guard<std::mutex> lock(keys_mutex_);
    auto it = api_keys_.find(api_key);
    if (it == api_keys_.end()) {
        return nullptr;
    }
    return &it->second;
}

bool APIKeyManager::revokeAPIKey(const std::string& api_key) {
    std::lock_guard<std::mutex> lock(keys_mutex_);
    auto it = api_keys_.find(api_key);
    if (it == api_keys_.end()) {
        return false;
    }
    
    it->second.is_active = false;
    key_to_secret_.erase(api_key);
    return true;
}

bool APIKeyManager::checkIPWhitelist(const std::string& api_key, const std::string& ip) const {
    std::lock_guard<std::mutex> lock(keys_mutex_);
    auto it = api_keys_.find(api_key);
    if (it == api_keys_.end()) {
        return false;
    }
    
    if (it->second.ip_whitelist.empty()) {
        return true;  // No whitelist means allow all
    }
    
    // Simple check (comma-separated IPs)
    std::stringstream ss(it->second.ip_whitelist);
    std::string item;
    while (std::getline(ss, item, ',')) {
        if (item == ip) {
            return true;
        }
    }
    
    return false;
}

std::string APIKeyManager::hashSecret(const std::string& secret) const {
    // For API keys, we store a hash of the secret
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, secret.c_str(), secret.length());
    SHA256_Final(hash, &sha256);
    
    return base64_encode(std::string(reinterpret_cast<char*>(hash), SHA256_DIGEST_LENGTH));
}

// Auth Manager Implementation
AuthManager::AuthManager() {
    // Initialize default admin user
    User admin;
    admin.user_id = 1;
    admin.username = "admin";
    admin.email = "admin@exchange.com";
    admin.password_hash = hashPassword("admin123");  // Change in production!
    admin.roles = {"admin"};
    admin.is_active = true;
    admin.is_verified = true;
    admin.created_at = get_current_timestamp();
    admin.last_login_at = 0;
    
    std::lock_guard<std::mutex> lock(users_mutex_);
    users_[admin.user_id] = admin;
    username_to_id_[admin.username] = admin.user_id;
    email_to_id_[admin.email] = admin.user_id;
    next_user_id_ = 1000000;
}

bool AuthManager::registerUser(const std::string& username, const std::string& email,
                               const std::string& password, std::string& error_msg) {
    std::lock_guard<std::mutex> lock(users_mutex_);
    
    // Check if username exists
    if (username_to_id_.find(username) != username_to_id_.end()) {
        error_msg = "Username already exists";
        return false;
    }
    
    // Check if email exists
    if (email_to_id_.find(email) != email_to_id_.end()) {
        error_msg = "Email already exists";
        return false;
    }
    
    // Validate password strength
    if (password.length() < 8) {
        error_msg = "Password must be at least 8 characters";
        return false;
    }
    
    // Create new user
    User user;
    user.user_id = next_user_id_++;
    user.username = username;
    user.email = email;
    user.password_hash = hashPassword(password);
    user.roles = {"user"};  // Default role
    user.is_active = true;
    user.is_verified = false;  // Requires email verification
    user.created_at = get_current_timestamp();
    user.last_login_at = 0;
    
    users_[user.user_id] = user;
    username_to_id_[username] = user.user_id;
    email_to_id_[email] = user.user_id;
    
    return true;
}

bool AuthManager::login(const std::string& username_or_email, const std::string& password,
                       std::string& token, std::string& error_msg) {
    std::lock_guard<std::mutex> lock(users_mutex_);
    
    UserID user_id = 0;
    if (username_to_id_.find(username_or_email) != username_to_id_.end()) {
        user_id = username_to_id_[username_or_email];
    } else if (email_to_id_.find(username_or_email) != email_to_id_.end()) {
        user_id = email_to_id_[username_or_email];
    } else {
        error_msg = "Invalid username or email";
        return false;
    }
    
    auto it = users_.find(user_id);
    if (it == users_.end()) {
        error_msg = "User not found";
        return false;
    }
    
    User& user = it->second;
    
    if (!user.is_active) {
        error_msg = "Account is disabled";
        return false;
    }
    
    if (!verifyPassword(password, user.password_hash)) {
        error_msg = "Invalid password";
        return false;
    }
    
    // Generate JWT token
    JWTManager::TokenPayload payload;
    payload.user_id = user.user_id;
    payload.username = user.username;
    payload.roles = user.roles;
    auto now = std::chrono::system_clock::now();
    auto exp_time = now + std::chrono::hours(24);  // 24 hours expiration
    payload.exp = std::chrono::duration_cast<std::chrono::seconds>(
        exp_time.time_since_epoch()).count();
    
    token = jwt_manager_.generateToken(payload, jwt_secret_);
    user.last_login_at = get_current_timestamp();
    
    return true;
}

bool AuthManager::verifyToken(const std::string& token, UserID& user_id, std::vector<std::string>& roles) {
    JWTManager::TokenPayload payload;
    if (!jwt_manager_.verifyToken(token, jwt_secret_, payload)) {
        return false;
    }
    
    user_id = payload.user_id;
    roles = payload.roles;
    return true;
}

const AuthManager::User* AuthManager::getUser(UserID user_id) const {
    std::lock_guard<std::mutex> lock(users_mutex_);
    auto it = users_.find(user_id);
    if (it == users_.end()) {
        return nullptr;
    }
    return &it->second;
}

bool AuthManager::changePassword(UserID user_id, const std::string& old_password,
                                const std::string& new_password, std::string& error_msg) {
    std::lock_guard<std::mutex> lock(users_mutex_);
    auto it = users_.find(user_id);
    if (it == users_.end()) {
        error_msg = "User not found";
        return false;
    }
    
    User& user = it->second;
    if (!verifyPassword(old_password, user.password_hash)) {
        error_msg = "Invalid old password";
        return false;
    }
    
    if (new_password.length() < 8) {
        error_msg = "New password must be at least 8 characters";
        return false;
    }
    
    user.password_hash = hashPassword(new_password);
    return true;
}

bool AuthManager::hasPermission(UserID user_id, const std::string& permission) const {
    std::lock_guard<std::mutex> lock(users_mutex_);
    auto it = users_.find(user_id);
    if (it == users_.end()) {
        return false;
    }
    
    const User& user = it->second;
    
    // Admin has all permissions
    for (const auto& role : user.roles) {
        if (role == "admin") {
            return true;
        }
    }
    
    // Check role-based permissions (simplified)
    // In production, use a proper permission system
    return false;
}

std::string AuthManager::hashPassword(const std::string& password) {
    // Use bcrypt or scrypt in production
    // For now, use SHA256 (not secure, just for demo)
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, password.c_str(), password.length());
    SHA256_Final(hash, &sha256);
    
    return base64_encode(std::string(reinterpret_cast<char*>(hash), SHA256_DIGEST_LENGTH));
}

bool AuthManager::verifyPassword(const std::string& password, const std::string& hash) {
    std::string computed_hash = hashPassword(password);
    return computed_hash == hash;
}

} // namespace perpetual
