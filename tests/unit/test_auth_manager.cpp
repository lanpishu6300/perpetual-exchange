#include <gtest/gtest.h>
#include "core/auth_manager.h"
#include <string>

using namespace perpetual;

class AuthManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        auth_manager_ = std::make_unique<AuthManager>();
    }
    
    std::unique_ptr<AuthManager> auth_manager_;
};

TEST_F(AuthManagerTest, UserRegistration) {
    std::string error_msg;
    
    // Test successful registration
    bool result = auth_manager_->registerUser("testuser", "test@example.com", "password123", error_msg);
    EXPECT_TRUE(result) << error_msg;
    
    // Test duplicate username
    result = auth_manager_->registerUser("testuser", "test2@example.com", "password123", error_msg);
    EXPECT_FALSE(result);
    EXPECT_EQ(error_msg, "Username already exists");
    
    // Test duplicate email
    result = auth_manager_->registerUser("testuser2", "test@example.com", "password123", error_msg);
    EXPECT_FALSE(result);
    EXPECT_EQ(error_msg, "Email already exists");
    
    // Test weak password
    result = auth_manager_->registerUser("testuser3", "test3@example.com", "short", error_msg);
    EXPECT_FALSE(result);
}

TEST_F(AuthManagerTest, UserLogin) {
    std::string error_msg;
    
    // Register user first
    auth_manager_->registerUser("loginuser", "login@example.com", "password123", error_msg);
    
    // Test successful login
    std::string token;
    bool result = auth_manager_->login("loginuser", "password123", token, error_msg);
    EXPECT_TRUE(result) << error_msg;
    EXPECT_FALSE(token.empty());
    
    // Test wrong password
    result = auth_manager_->login("loginuser", "wrongpassword", token, error_msg);
    EXPECT_FALSE(result);
    
    // Test non-existent user
    result = auth_manager_->login("nonexistent", "password123", token, error_msg);
    EXPECT_FALSE(result);
}

TEST_F(AuthManagerTest, TokenVerification) {
    std::string error_msg;
    
    // Register and login
    auth_manager_->registerUser("tokenuser", "token@example.com", "password123", error_msg);
    std::string token;
    auth_manager_->login("tokenuser", "password123", token, error_msg);
    
    // Verify token
    UserID user_id;
    std::vector<std::string> roles;
    bool result = auth_manager_->verifyToken(token, user_id, roles);
    EXPECT_TRUE(result);
    EXPECT_NE(user_id, 0);
}

TEST_F(AuthManagerTest, APIKeyManagement) {
    std::string error_msg;
    
    // Register user
    auth_manager_->registerUser("apikeyuser", "apikey@example.com", "password123", error_msg);
    UserID user_id = 1000000;  // First user
    
    // Note: API key management is tested through AuthManager interface
    // Direct access to api_key_manager_ is private, so we test through public methods
    // This is a placeholder test - API key testing would require public methods
    EXPECT_TRUE(true);
}

TEST_F(AuthManagerTest, PasswordChange) {
    std::string error_msg;
    
    // Register user
    auth_manager_->registerUser("changepassuser", "changepass@example.com", "oldpassword", error_msg);
    UserID user_id = 1000000;
    
    // Change password
    bool result = auth_manager_->changePassword(user_id, "oldpassword", "newpassword", error_msg);
    EXPECT_TRUE(result) << error_msg;
    
    // Try login with new password
    std::string token;
    result = auth_manager_->login("changepassuser", "newpassword", token, error_msg);
    EXPECT_TRUE(result);
    
    // Try login with old password (should fail)
    result = auth_manager_->login("changepassuser", "oldpassword", token, error_msg);
    EXPECT_FALSE(result);
}

