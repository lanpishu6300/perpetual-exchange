#pragma once

#include "types.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <functional>
#include <memory>

namespace perpetual {

// 通知服务
class NotificationService {
public:
    // 通知类型
    enum NotificationType {
        EMAIL,
        SMS,
        PUSH,
        IN_APP
    };
    
    // 通知优先级
    enum Priority {
        LOW,
        NORMAL,
        HIGH,
        URGENT
    };
    
    // 通知模板
    struct NotificationTemplate {
        std::string template_id;
        NotificationType type;
        std::string subject;
        std::string content;
        std::vector<std::string> variables;  // 模板变量
    };
    
    // 通知
    struct Notification {
        NotificationType type;
        std::string to;              // Email/Phone/UserID
        std::string template_id;
        std::unordered_map<std::string, std::string> variables;
        Priority priority = NORMAL;
        int64_t timestamp = 0;
    };
    
    NotificationService();
    
    // 发送通知
    bool sendNotification(const Notification& notification);
    
    // 批量发送
    void sendBatchNotifications(const std::vector<Notification>& notifications);
    
    // 注册通知模板
    void registerTemplate(const NotificationTemplate& notification_template);
    
    // 发送订单通知
    void notifyOrderFilled(UserID user_id, uint64_t order_id, 
                          InstrumentID instrument_id, Quantity quantity, Price price);
    
    void notifyOrderCancelled(UserID user_id, uint64_t order_id);
    
    void notifyOrderRejected(UserID user_id, uint64_t order_id, const std::string& reason);
    
    // 发送清算通知
    void notifyLiquidation(UserID user_id, InstrumentID instrument_id,
                          Quantity quantity, Price price);
    
    // 发送资金费率结算通知
    void notifyFundingSettlement(UserID user_id, InstrumentID instrument_id,
                                double payment);
    
    // 发送账户通知
    void notifyBalanceChange(UserID user_id, double balance, double change);
    
    // 设置回调函数（用于实际发送）
    using SendCallback = std::function<bool(const Notification&)>;
    void setSendCallback(NotificationType type, SendCallback cb);
    
private:
    // 渲染模板
    std::string renderTemplate(const NotificationTemplate& notification_template,
                              const std::unordered_map<std::string, std::string>& variables);
    
    mutable std::mutex mutex_;
    
    std::unordered_map<std::string, NotificationTemplate> templates_;
    std::unordered_map<NotificationType, SendCallback> send_callbacks_;
    
    // 通知队列
    std::vector<Notification> notification_queue_;
};

} // namespace perpetual


#include "types.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <functional>
#include <memory>

namespace perpetual {

// 通知服务
class NotificationService {
public:
    // 通知类型
    enum NotificationType {
        EMAIL,
        SMS,
        PUSH,
        IN_APP
    };
    
    // 通知优先级
    enum Priority {
        LOW,
        NORMAL,
        HIGH,
        URGENT
    };
    
    // 通知模板
    struct NotificationTemplate {
        std::string template_id;
        NotificationType type;
        std::string subject;
        std::string content;
        std::vector<std::string> variables;  // 模板变量
    };
    
    // 通知
    struct Notification {
        NotificationType type;
        std::string to;              // Email/Phone/UserID
        std::string template_id;
        std::unordered_map<std::string, std::string> variables;
        Priority priority = NORMAL;
        int64_t timestamp = 0;
    };
    
    NotificationService();
    
    // 发送通知
    bool sendNotification(const Notification& notification);
    
    // 批量发送
    void sendBatchNotifications(const std::vector<Notification>& notifications);
    
    // 注册通知模板
    void registerTemplate(const NotificationTemplate& notification_template);
    
    // 发送订单通知
    void notifyOrderFilled(UserID user_id, uint64_t order_id, 
                          InstrumentID instrument_id, Quantity quantity, Price price);
    
    void notifyOrderCancelled(UserID user_id, uint64_t order_id);
    
    void notifyOrderRejected(UserID user_id, uint64_t order_id, const std::string& reason);
    
    // 发送清算通知
    void notifyLiquidation(UserID user_id, InstrumentID instrument_id,
                          Quantity quantity, Price price);
    
    // 发送资金费率结算通知
    void notifyFundingSettlement(UserID user_id, InstrumentID instrument_id,
                                double payment);
    
    // 发送账户通知
    void notifyBalanceChange(UserID user_id, double balance, double change);
    
    // 设置回调函数（用于实际发送）
    using SendCallback = std::function<bool(const Notification&)>;
    void setSendCallback(NotificationType type, SendCallback cb);
    
private:
    // 渲染模板
    std::string renderTemplate(const NotificationTemplate& notification_template,
                              const std::unordered_map<std::string, std::string>& variables);
    
    mutable std::mutex mutex_;
    
    std::unordered_map<std::string, NotificationTemplate> templates_;
    std::unordered_map<NotificationType, SendCallback> send_callbacks_;
    
    // 通知队列
    std::vector<Notification> notification_queue_;
};

} // namespace perpetual
