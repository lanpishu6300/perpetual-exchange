#include "core/order_validator.h"
#include "core/types.h"
#include "core/order.h"
#include "core/error_handler.h"

namespace perpetual {

OrderValidator::OrderValidator() {
}

OrderValidator::ValidationResult OrderValidator::validate(const Order* order) const {
    ValidationResult result;
    
    if (!order) {
        result.reason = "Order is null";
        result.error_code = ErrorCode::INVALID_ORDER;
        return result;
    }
    
    // Basic validation
    if (order->price <= 0 || order->quantity <= 0) {
        result.reason = "Invalid price or quantity";
        result.error_code = ErrorCode::INVALID_ORDER;
        return result;
    }
    
    if (order->order_id == 0) {
        result.reason = "Invalid order ID";
        result.error_code = ErrorCode::INVALID_ORDER;
        return result;
    }
    
    if (order->user_id == 0) {
        result.reason = "Invalid user ID";
        result.error_code = ErrorCode::INVALID_ORDER;
        return result;
    }
    
    // Validate order side
    if (order->side != OrderSide::BUY && order->side != OrderSide::SELL) {
        result.reason = "Invalid order side";
        result.error_code = ErrorCode::INVALID_ORDER;
        return result;
    }
    
    // Price and quantity range validation
    if (order->price < min_price_ || order->price > max_price_) {
        result.reason = "Price out of range";
        result.error_code = ErrorCode::INVALID_ORDER;
        return result;
    }
    
    if (order->quantity < min_quantity_ || order->quantity > max_quantity_) {
        result.reason = "Quantity out of range";
        result.error_code = ErrorCode::INVALID_ORDER;
        return result;
    }
    
    result.valid = true;
    result.error_code = ErrorCode::SUCCESS;
    return result;
}

bool OrderValidator::isValidPriceTick(Price price) const {
    return (price % price_tick_) == 0;
}

bool OrderValidator::isValidQuantityStep(Quantity quantity) const {
    return (quantity % quantity_step_) == 0;
}

} // namespace perpetual

