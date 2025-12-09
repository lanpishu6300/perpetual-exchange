#include <gtest/gtest.h>
#include "core/auth_manager.h"
#include "core/api_gateway.h"
#include "core/rest_api_server.h"
#include "core/monitoring_system.h"
#include <string>

using namespace perpetual;

class APIIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        auth_manager_ = std::make_unique<AuthManager>();
        api_gateway_ = std::make_unique<APIGateway>(auth_manager_.get());
        monitoring_system_ = std::make_unique<MonitoringSystem>();
    }
    
    std::unique_ptr<AuthManager> auth_manager_;
    std::unique_ptr<APIGateway> api_gateway_;
    std::unique_ptr<MonitoringSystem> monitoring_system_;
};

// 测试用户注册和登录
TEST_F(APIIntegrationTest, UserRegistrationAndLogin) {
    std::string error_msg;
    
    // 注册用户
    bool registered = auth_manager_->registerUser(
        "testuser", "test@example.com", "password123", error_msg);
    EXPECT_TRUE(registered) << error_msg;
    
    // 登录
    std::string token;
    bool logged_in = auth_manager_->login("testuser", "password123", token, error_msg);
    EXPECT_TRUE(logged_in) << error_msg;
    EXPECT_FALSE(token.empty());
}

// 测试API密钥管理
TEST_F(APIIntegrationTest, APIKeyManagement) {
    std::string error_msg;
    
    // 注册用户
    auth_manager_->registerUser("testuser2", "test2@example.com", "password123", error_msg);
    
    // 创建API密钥
    std::string api_key, api_secret;
    bool created = auth_manager_->createAPIKey(1001, api_key, api_secret, error_msg);
    EXPECT_TRUE(created) << error_msg;
    EXPECT_FALSE(api_key.empty());
    EXPECT_FALSE(api_secret.empty());
    
    // 验证API密钥
    UserID user_id;
    bool verified = auth_manager_->verifyAPIKey(api_key, api_secret, user_id, error_msg);
    EXPECT_TRUE(verified) << error_msg;
    EXPECT_EQ(user_id, 1001);
}

// 测试API网关认证
TEST_F(APIIntegrationTest, APIGatewayAuthentication) {
    std::string error_msg;
    
    // 注册并登录
    auth_manager_->registerUser("testuser3", "test3@example.com", "password123", error_msg);
    std::string token;
    auth_manager_->login("testuser3", "password123", token, error_msg);
    
    // 创建请求上下文
    APIGateway::RequestContext context;
    context.headers["Authorization"] = "Bearer " + token;
    context.path = "/api/v1/orders";
    context.method = "POST";
    
    // 认证
    bool authenticated = api_gateway_->authenticate(context);
    EXPECT_TRUE(authenticated);
}

// 测试监控系统
TEST_F(APIIntegrationTest, MonitoringSystem) {
    // 记录指标
    monitoring_system_->recordMetric("orders.processed", 100);
    monitoring_system_->recordMetric("trades.executed", 50);
    
    // 记录延迟
    monitoring_system_->recordLatency("order.processing", 1000);  // 1ms
    
    // 获取统计
    auto stats = monitoring_system_->getStatistics();
    EXPECT_GE(stats.metrics.size(), 2);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

