#include "core/order_validator.h"
#include "core/error_handler.h"

namespace perpetual {

OrderValidator::OrderValidator() {
    auto& config = Config::getInstance();
    
    // Load validation rules from config
    if (config.has("validation.min_price")) {
        min_price_ = double_to_price(config.getDouble("validation.min_price", 0.0001));
    }
    if (config.has("validation.max_price")) {
        max_price_ = double_to_price(config.getDouble("validation.max_price", 1000000.0));
    }
    if (config.has("validation.min_quantity")) {
        min_quantity_ = double_to_quantity(config.getDouble("validation.min_quantity", 0.0001));
    }
    if (config.has("validation.max_quantity")) {
        max_quantity_ = double_to_quantity(config.getDouble("validation.max_quantity", 1000000.0));
    }
    if (config.has("validation.price_tick")) {
        price_tick_ = double_to_price(config.getDouble("validation.price_tick", 0.01));
    }
    if (config.has("validation.quantity_step")) {
        quantity_step_ = double_to_quantity(config.getDouble("validation.quantity_step", 0.0001));
    }
}

OrderValidator::ValidationResult OrderValidator::validate(const Order* order) const {
    ValidationResult result;
    
    if (!order) {
        result.reason = "Order is null";
        result.error_code = ErrorCode::INVALID_ORDER;
        return result;
    }
    
    // Check order ID
    if (order->order_id == 0) {
        result.reason = "Invalid order ID";
        result.error_code = ErrorCode::INVALID_ORDER;
        return result;
    }
    
    // Check user ID
    if (order->user_id == 0) {
        result.reason = "Invalid user ID";
        result.error_code = ErrorCode::INVALID_ORDER;
        return result;
    }
    
    // Check instrument ID
    if (order->instrument_id == 0) {
        result.reason = "Invalid instrument ID";
        result.error_code = ErrorCode::INVALID_ORDER;
        return result;
    }
    
    // Check quantity
    if (order->quantity <= 0) {
        result.reason = "Quantity must be positive";
        result.error_code = ErrorCode::INVALID_ORDER;
        return result;
    }
    
    if (order->quantity < min_quantity_) {
        result.reason = "Quantity below minimum: " + std::to_string(quantity_to_double(min_quantity_));
        result.error_code = ErrorCode::INVALID_ORDER;
        return result;
    }
    
    if (order->quantity > max_quantity_) {
        result.reason = "Quantity exceeds maximum: " + std::to_string(quantity_to_double(max_quantity_));
        result.error_code = ErrorCode::INVALID_ORDER;
        return result;
    }
    
    // Check quantity step
    if (!isValidQuantityStep(order->quantity)) {
        result.reason = "Quantity must be multiple of step: " + std::to_string(quantity_to_double(quantity_step_));
        result.error_code = ErrorCode::INVALID_ORDER;
        return result;
    }
    
    // Check price for limit orders
    if (order->order_type == OrderType::LIMIT) {
        if (order->price <= 0) {
            result.reason = "Price must be positive for limit orders";
            result.error_code = ErrorCode::INVALID_ORDER;
            return result;
        }
        
        if (order->price < min_price_) {
            result.reason = "Price below minimum: " + std::to_string(price_to_double(min_price_));
            result.error_code = ErrorCode::INVALID_ORDER;
            return result;
        }
        
        if (order->price > max_price_) {
            result.reason = "Price exceeds maximum: " + std::to_string(price_to_double(max_price_));
            result.error_code = ErrorCode::INVALID_ORDER;
            return result;
        }
        
        // Check price tick
        if (!isValidPriceTick(order->price)) {
            result.reason = "Price must be multiple of tick: " + std::to_string(price_to_double(price_tick_));
            result.error_code = ErrorCode::INVALID_ORDER;
            return result;
        }
    }
    
    // All validations passed
    result.valid = true;
    result.error_code = ErrorCode::SUCCESS;
    return result;
}

bool OrderValidator::isValidPriceTick(Price price) const {
    if (price_tick_ == 0) return true;
    return (price % price_tick_) == 0;
}

bool OrderValidator::isValidQuantityStep(Quantity quantity) const {
    if (quantity_step_ == 0) return true;
    return (quantity % quantity_step_) == 0;
}

} // namespace perpetual


