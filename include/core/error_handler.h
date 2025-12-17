#pragma once

#include <string>
#include <exception>

namespace perpetual {

enum class ErrorCode {
    SUCCESS = 0,
    INVALID_ORDER = 1,
    INSUFFICIENT_BALANCE = 2,
    RATE_LIMIT_EXCEEDED = 3,
    SYSTEM_ERROR = 4
};

class SystemException : public std::exception {
public:
    SystemException(const std::string& msg) : message_(msg) {}
    const char* what() const noexcept override { return message_.c_str(); }
private:
    std::string message_;
};

class InvalidOrderException : public std::exception {
public:
    InvalidOrderException(const std::string& msg) : message_(msg) {}
    const char* what() const noexcept override { return message_.c_str(); }
private:
    std::string message_;
};

class OrderRejectedException : public std::exception {
public:
    OrderRejectedException(const std::string& msg) : message_(msg) {}
    const char* what() const noexcept override { return message_.c_str(); }
private:
    std::string message_;
};

class InsufficientBalanceException : public std::exception {
public:
    InsufficientBalanceException(const std::string& msg) : message_(msg) {}
    const char* what() const noexcept override { return message_.c_str(); }
private:
    std::string message_;
};

} // namespace perpetual
