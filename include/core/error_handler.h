#pragma once

#include <exception>
#include <string>
#include <stdexcept>

namespace perpetual {

// Custom exception classes
class ExchangeException : public std::runtime_error {
public:
    ExchangeException(const std::string& message) 
        : std::runtime_error(message) {}
};

class OrderRejectedException : public ExchangeException {
public:
    OrderRejectedException(const std::string& reason) 
        : ExchangeException("Order rejected: " + reason), reason_(reason) {}
    
    const std::string& reason() const { return reason_; }
    
private:
    std::string reason_;
};

class InsufficientBalanceException : public ExchangeException {
public:
    InsufficientBalanceException() 
        : ExchangeException("Insufficient balance") {}
};

class InvalidOrderException : public ExchangeException {
public:
    InvalidOrderException(const std::string& reason) 
        : ExchangeException("Invalid order: " + reason) {}
};

class SystemException : public ExchangeException {
public:
    SystemException(const std::string& message) 
        : ExchangeException("System error: " + message) {}
};

// Error codes
enum class ErrorCode {
    SUCCESS = 0,
    INVALID_ORDER = 1001,
    INSUFFICIENT_BALANCE = 1002,
    INSUFFICIENT_MARGIN = 1003,
    ORDER_REJECTED = 1004,
    SYSTEM_ERROR = 2001,
    DATABASE_ERROR = 2002,
    NETWORK_ERROR = 2003
};

} // namespace perpetual



#include <exception>
#include <string>
#include <stdexcept>

namespace perpetual {

// Custom exception classes
class ExchangeException : public std::runtime_error {
public:
    ExchangeException(const std::string& message) 
        : std::runtime_error(message) {}
};

class OrderRejectedException : public ExchangeException {
public:
    OrderRejectedException(const std::string& reason) 
        : ExchangeException("Order rejected: " + reason), reason_(reason) {}
    
    const std::string& reason() const { return reason_; }
    
private:
    std::string reason_;
};

class InsufficientBalanceException : public ExchangeException {
public:
    InsufficientBalanceException() 
        : ExchangeException("Insufficient balance") {}
};

class InvalidOrderException : public ExchangeException {
public:
    InvalidOrderException(const std::string& reason) 
        : ExchangeException("Invalid order: " + reason) {}
};

class SystemException : public ExchangeException {
public:
    SystemException(const std::string& message) 
        : ExchangeException("System error: " + message) {}
};

// Error codes
enum class ErrorCode {
    SUCCESS = 0,
    INVALID_ORDER = 1001,
    INSUFFICIENT_BALANCE = 1002,
    INSUFFICIENT_MARGIN = 1003,
    ORDER_REJECTED = 1004,
    SYSTEM_ERROR = 2001,
    DATABASE_ERROR = 2002,
    NETWORK_ERROR = 2003
};

} // namespace perpetual


