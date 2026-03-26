#ifndef RESOURCE_MONITOR_LOGGER_H
#define RESOURCE_MONITOR_LOGGER_H

#include <string>
#include <sstream>
#include <fstream>
#include <mutex>
#include <memory>
#include <chrono>

namespace monitor {

/**
 * @brief 日志级别
 */
enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

/**
 * @brief 日志记录器
 */
class Logger {
public:
    /**
     * @brief 获取单例实例
     */
    static Logger& getInstance();
    
    /**
     * @brief 初始化日志记录器
     */
    void initialize(const std::string& log_file, LogLevel level = LogLevel::INFO);
    
    /**
     * @brief 设置日志级别
     */
    void setLevel(LogLevel level);
    
    /**
     * @brief 记录日志
     */
    void log(LogLevel level, const std::string& message);
    
    /**
     * @brief 快捷方法
     */
    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    
    /**
     * @brief 刷新日志
     */
    void flush();
    
private:
    Logger() = default;
    ~Logger();
    
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    std::string formatMessage(LogLevel level, const std::string& message);
    std::string levelToString(LogLevel level);
    
    std::ofstream file_;
    std::mutex mutex_;
    LogLevel level_;
    bool initialized_;
};

/**
 * @brief 日志流（用于链式调用）
 */
class LogStream {
public:
    LogStream(Logger& logger, LogLevel level);
    ~LogStream();
    
    template<typename T>
    LogStream& operator<<(const T& value) {
        stream_ << value;
        return *this;
    }
    
private:
    Logger& logger_;
    LogLevel level_;
    std::stringstream stream_;
};

// 便捷宏
#define LOG_DEBUG() monitor::LogStream(monitor::Logger::getInstance(), monitor::LogLevel::DEBUG)
#define LOG_INFO() monitor::LogStream(monitor::Logger::getInstance(), monitor::LogLevel::INFO)
#define LOG_WARNING() monitor::LogStream(monitor::Logger::getInstance(), monitor::LogLevel::WARNING)
#define LOG_ERROR() monitor::LogStream(monitor::Logger::getInstance(), monitor::LogLevel::ERROR)

} // namespace monitor

#endif // RESOURCE_MONITOR_LOGGER_H
