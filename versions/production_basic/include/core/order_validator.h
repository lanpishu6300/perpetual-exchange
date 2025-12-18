#pragma once

#include "core/order.h"
#include "core/types.h"
#include "config.h"
#include "core/error_handler.h"
#include <string>
#include <unordered_map>
#include <mutex>

namespace perpetual {

// Comprehensive order validation
class OrderValidator {
public:
    OrderValidator();
    
    struct ValidationResult {
        bool valid = false;
        std::string reason;
        ErrorCode error_code = ErrorCode::SUCCESS;
    };
    
    // Validate order before processing
    ValidationResult validate(const Order* order) const;
    
    // Set validation rules
    void setMinPrice(Price min_price) { min_price_ = min_price; }
    void setMaxPrice(Price max_price) { max_price_ = max_price; }
    void setMinQuantity(Quantity min_qty) { min_quantity_ = min_qty; }
    void setMaxQuantity(Quantity max_qty) { max_quantity_ = max_qty; }
    void setPriceTick(Price tick) { price_tick_ = tick; }
    void setQuantityStep(Quantity step) { quantity_step_ = step; }
    
    // Price precision validation
    bool isValidPriceTick(Price price) const;
    
    // Quantity precision validation
    bool isValidQuantityStep(Quantity quantity) const;
    
private:
    Price min_price_ = 1;
    Price max_price_ = 1000000000000LL; // 1M with scale
    Quantity min_quantity_ = 1;
    Quantity max_quantity_ = 1000000000000LL;
    Price price_tick_ = 1; // Minimum price increment
    Quantity quantity_step_ = 1; // Minimum quantity increment
};

} // namespace perpetual
