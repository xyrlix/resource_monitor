#include "logger.h"
#include <iostream>
#include <iomanip>
#include <ctime>

namespace monitor {

//==============================================================================
// Logger 实现
//==============================================================================

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::initialize(const std::string& log_file, LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (file_.is_open()) {
        file_.close();
    }
    
    level_ = level;
    initialized_ = true;
    
    if (!log_file.empty()) {
        file_.open(log_file, std::ios::app);
        if (!file_.is_open()) {
            std::cerr << "Failed to open log file: " << log_file << std::endl;
        }
    }
}

void Logger::setLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    level_ = level;
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < level_ || !initialized_) {
        return;
    }
    
    std::string formatted = formatMessage(level, message);
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 输出到控制台
    if (level == LogLevel::ERROR) {
        std::cerr << formatted << std::endl;
    } else {
        std::cout << formatted << std::endl;
    }
    
    // 输出到文件
    if (file_.is_open()) {
        file_ << formatted << std::endl;
        file_.flush();
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

void Logger::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.is_open()) {
        file_.flush();
    }
    std::cout.flush();
    std::cerr.flush();
}

Logger::~Logger() {
    if (file_.is_open()) {
        file_.close();
    }
}

std::string Logger::formatMessage(LogLevel level, const std::string& message) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << " [" << levelToString(level) << "] ";
    ss << message;
    
    return ss.str();
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARN";
        case LogLevel::ERROR:   return "ERROR";
        default: return "UNKNOWN";
    }
}

//==============================================================================
// LogStream 实现
//==============================================================================

LogStream::LogStream(Logger& logger, LogLevel level)
    : logger_(logger), level_(level) {
}

LogStream::~LogStream() {
    logger_.log(level_, stream_.str());
}

} // namespace monitor
