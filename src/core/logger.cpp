#include "core/logger.h"
#include <iostream>
#include <ctime>

namespace perpetual {

void Logger::initialize(const std::string& log_file, LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    level_ = level;
    
    if (!log_file.empty()) {
        log_file_ = std::make_unique<std::ofstream>(log_file, std::ios::app);
        if (!log_file_->is_open()) {
            std::cerr << "Failed to open log file: " << log_file << std::endl;
            log_file_.reset();
        }
    }
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARN: return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

std::string Logger::getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < level_) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string log_entry = "[" + getTimestamp() + "] [" + 
                           levelToString(level) + "] " + message;
    
    if (console_output_) {
        std::cerr << log_entry << std::endl;
    }
    
    if (log_file_ && log_file_->is_open()) {
        *log_file_ << log_entry << std::endl;
        log_file_->flush();
    }
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warn(const std::string& message) {
    log(LogLevel::WARN, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void Logger::critical(const std::string& message) {
    log(LogLevel::CRITICAL, message);
}

Logger::~Logger() {
    if (log_file_ && log_file_->is_open()) {
        log_file_->close();
    }
}

} // namespace perpetual


