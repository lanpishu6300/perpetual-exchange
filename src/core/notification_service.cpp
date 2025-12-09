#include "core/notification_service.h"
#include "core/types.h"
#include <sstream>
#include <algorithm>

namespace perpetual {

NotificationService::NotificationService() {
    // Initialize default templates
}

bool NotificationService::sendNotification(const Notification& notification) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if callback is registered
    auto it = send_callbacks_.find(notification.type);
    if (it != send_callbacks_.end() && it->second) {
        return it->second(notification);
    }
    
    // Default: just queue the notification
    notification_queue_.push_back(notification);
    return true;
}

void NotificationService::sendBatchNotifications(const std::vector<Notification>& notifications) {
    for (const auto& notification : notifications) {
        sendNotification(notification);
    }
}

void NotificationService::registerTemplate(const NotificationTemplate& template_) {
    std::lock_guard<std::mutex> lock(mutex_);
    templates_[template_.template_id] = template_;
}

void NotificationService::notifyOrderFilled(UserID user_id, uint64_t order_id,
                                           InstrumentID instrument_id, 
                                           Quantity quantity, Price price) {
    Notification notification;
    notification.type = IN_APP;  // Default to in-app notification
    notification.to = std::to_string(user_id);
    notification.template_id = "order_filled";
    notification.priority = NORMAL;
    notification.timestamp = get_current_timestamp() / 1000000000;
    
    notification.variables["order_id"] = std::to_string(order_id);
    notification.variables["instrument_id"] = std::to_string(instrument_id);
    notification.variables["quantity"] = std::to_string(quantity);
    notification.variables["price"] = std::to_string(price);
    
    sendNotification(notification);
}

void NotificationService::notifyOrderCancelled(UserID user_id, uint64_t order_id) {
    Notification notification;
    notification.type = IN_APP;
    notification.to = std::to_string(user_id);
    notification.template_id = "order_cancelled";
    notification.priority = LOW;
    notification.timestamp = get_current_timestamp() / 1000000000;
    
    notification.variables["order_id"] = std::to_string(order_id);
    
    sendNotification(notification);
}

void NotificationService::notifyOrderRejected(UserID user_id, uint64_t order_id, 
                                             const std::string& reason) {
    Notification notification;
    notification.type = IN_APP;
    notification.to = std::to_string(user_id);
    notification.template_id = "order_rejected";
    notification.priority = HIGH;
    notification.timestamp = get_current_timestamp() / 1000000000;
    
    notification.variables["order_id"] = std::to_string(order_id);
    notification.variables["reason"] = reason;
    
    sendNotification(notification);
}

void NotificationService::notifyLiquidation(UserID user_id, InstrumentID instrument_id,
                                           Quantity quantity, Price price) {
    Notification notification;
    notification.type = EMAIL;  // Critical - send email
    notification.to = std::to_string(user_id);
    notification.template_id = "liquidation";
    notification.priority = URGENT;
    notification.timestamp = get_current_timestamp() / 1000000000;
    
    notification.variables["instrument_id"] = std::to_string(instrument_id);
    notification.variables["quantity"] = std::to_string(quantity);
    notification.variables["price"] = std::to_string(price);
    
    sendNotification(notification);
}

void NotificationService::notifyFundingSettlement(UserID user_id, InstrumentID instrument_id,
                                                 double payment) {
    Notification notification;
    notification.type = IN_APP;
    notification.to = std::to_string(user_id);
    notification.template_id = "funding_settlement";
    notification.priority = NORMAL;
    notification.timestamp = get_current_timestamp() / 1000000000;
    
    notification.variables["instrument_id"] = std::to_string(instrument_id);
    notification.variables["payment"] = std::to_string(payment);
    
    sendNotification(notification);
}

void NotificationService::notifyBalanceChange(UserID user_id, double balance, double change) {
    Notification notification;
    notification.type = IN_APP;
    notification.to = std::to_string(user_id);
    notification.template_id = "balance_change";
    notification.priority = change > 0 ? NORMAL : HIGH;
    notification.timestamp = get_current_timestamp() / 1000000000;
    
    notification.variables["balance"] = std::to_string(balance);
    notification.variables["change"] = std::to_string(change);
    
    sendNotification(notification);
}

void NotificationService::setSendCallback(NotificationType type, SendCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    send_callbacks_[type] = cb;
}

std::string NotificationService::renderTemplate(const NotificationTemplate& template_,
                                               const std::unordered_map<std::string, std::string>& variables) {
    std::string result = template_.content;
    
    // Simple template variable replacement
    // Format: {{variable_name}}
    for (const auto& var : variables) {
        std::string placeholder = "{{" + var.first + "}}";
        size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            result.replace(pos, placeholder.length(), var.second);
            pos += var.second.length();
        }
    }
    
    return result;
}

} // namespace perpetual

