#pragma once

#include <string>
#include <fstream>
#include <memory>
#include <mutex>
#include <sstream>
#include <chrono>
#include <iomanip>

namespace perpetual {

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    CRITICAL = 4
};

class Logger {
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }
    
    void initialize(const std::string& log_file, LogLevel level = LogLevel::INFO);
    void setLevel(LogLevel level) { level_ = level; }
    
    void debug(const std::string& message);
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);
    void critical(const std::string& message);
    
    void log(LogLevel level, const std::string& message);
    
    ~Logger();
    
private:
    Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    std::string levelToString(LogLevel level);
    std::string getTimestamp();
    
    std::unique_ptr<std::ofstream> log_file_;
    LogLevel level_ = LogLevel::INFO;
    std::mutex mutex_;
    bool console_output_ = true;
};

// Convenience macros
#define LOG_DEBUG(msg) perpetual::Logger::getInstance().debug(msg)
#define LOG_INFO(msg) perpetual::Logger::getInstance().info(msg)
#define LOG_WARN(msg) perpetual::Logger::getInstance().warn(msg)
#define LOG_ERROR(msg) perpetual::Logger::getInstance().error(msg)
#define LOG_CRITICAL(msg) perpetual::Logger::getInstance().critical(msg)

} // namespace perpetual



#include <string>
#include <fstream>
#include <memory>
#include <mutex>
#include <sstream>
#include <chrono>
#include <iomanip>

namespace perpetual {

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    CRITICAL = 4
};

class Logger {
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }
    
    void initialize(const std::string& log_file, LogLevel level = LogLevel::INFO);
    void setLevel(LogLevel level) { level_ = level; }
    
    void debug(const std::string& message);
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);
    void critical(const std::string& message);
    
    void log(LogLevel level, const std::string& message);
    
    ~Logger();
    
private:
    Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    std::string levelToString(LogLevel level);
    std::string getTimestamp();
    
    std::unique_ptr<std::ofstream> log_file_;
    LogLevel level_ = LogLevel::INFO;
    std::mutex mutex_;
    bool console_output_ = true;
};

// Convenience macros
#define LOG_DEBUG(msg) perpetual::Logger::getInstance().debug(msg)
#define LOG_INFO(msg) perpetual::Logger::getInstance().info(msg)
#define LOG_WARN(msg) perpetual::Logger::getInstance().warn(msg)
#define LOG_ERROR(msg) perpetual::Logger::getInstance().error(msg)
#define LOG_CRITICAL(msg) perpetual::Logger::getInstance().critical(msg)

} // namespace perpetual


