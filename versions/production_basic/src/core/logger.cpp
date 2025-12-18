#include "core/logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace perpetual {

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() : log_level_(LogLevel::INFO) {
}

Logger::~Logger() {
    if (log_file_ && log_file_->is_open()) {
        log_file_->close();
    }
}

void Logger::setOutputFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    if (log_file_ && log_file_->is_open()) {
        log_file_->close();
    }
    if (!filename.empty()) {
        log_file_ = std::make_unique<std::ofstream>(filename, std::ios::app);
    }
}

void Logger::initialize(const std::string& filename, LogLevel level) {
    log_level_ = level;
    setOutputFile(filename);
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < log_level_) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(log_mutex_);
    
    // Get current time
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();
    ss << " [" << levelToString(level) << "] " << message;
    
    std::string log_line = ss.str();
    
    // Output to file if available
    if (log_file_ && log_file_->is_open()) {
        *log_file_ << log_line << std::endl;
        log_file_->flush();
    } else {
        // Output to stdout
        std::cout << log_line << std::endl;
    }
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warning(const std::string& message) {
    log(LogLevel::WARNING, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void Logger::critical(const std::string& message) {
    log(LogLevel::CRITICAL, message);
}

std::string Logger::levelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

} // namespace perpetual

