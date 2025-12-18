#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <memory>
#include <sstream>

namespace perpetual {

// Log levels
enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
    CRITICAL = 4
};

// Simple logger class
class Logger {
public:
    static Logger& getInstance();
    
    void setLogLevel(LogLevel level) { log_level_ = level; }
    LogLevel getLogLevel() const { return log_level_; }
    
    void setOutputFile(const std::string& filename);
    
    // Initialize logger with file and level
    void initialize(const std::string& filename, LogLevel level);
    
    void log(LogLevel level, const std::string& message);
    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    void critical(const std::string& message);
    
private:
    Logger();
    ~Logger();
    
    LogLevel log_level_;
    std::unique_ptr<std::ofstream> log_file_;
    std::mutex log_mutex_;
    
    std::string levelToString(LogLevel level) const;
};

// Convenience macros
#define LOG_DEBUG(msg) perpetual::Logger::getInstance().debug(msg)
#define LOG_INFO(msg) perpetual::Logger::getInstance().info(msg)
#define LOG_WARNING(msg) perpetual::Logger::getInstance().warning(msg)
#define LOG_ERROR(msg) perpetual::Logger::getInstance().error(msg)
#define LOG_CRITICAL(msg) perpetual::Logger::getInstance().critical(msg)

} // namespace perpetual
