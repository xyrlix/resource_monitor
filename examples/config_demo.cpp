#include "monitor.h"
#include "config.h"
#include "logger.h"
#include <iostream>

using namespace monitor;

int main() {
    // 初始化日志
    Logger::getInstance().initialize("config_demo.log", LogLevel::DEBUG);
    LOG_INFO() << "Config Demo Started";
    
    // 获取默认配置
    auto config = ConfigValidator::getDefault();
    LOG_INFO() << "Loaded default configuration";
    
    // 修改配置
    config.update_interval_ms = 2000;
    config.web_port = 9000;
    config.cpu_alert_threshold = 85.0;
    config.log_level = "DEBUG";
    config.console_color = true;
    
    // 验证配置
    std::string error;
    if (!ConfigValidator::validate(config, error)) {
        LOG_ERROR() << "Invalid configuration: " << error;
        return 1;
    }
    
    LOG_INFO() << "Configuration validated";
    
    // 保存到文件
    if (MonitorConfig::saveToFile("monitor.conf", config)) {
        LOG_INFO() << "Configuration saved to monitor.conf";
        std::cout << "Configuration saved to monitor.conf\n";
    } else {
        LOG_ERROR() << "Failed to save configuration";
        return 1;
    }
    
    // 从文件加载
    MonitorConfig loaded_config;
    if (MonitorConfig::loadFromFile("monitor.conf", loaded_config)) {
        LOG_INFO() << "Configuration loaded from monitor.conf";
        
        std::cout << "\n=== Loaded Configuration ===\n";
        std::cout << "Update Interval: " << loaded_config.update_interval_ms << "ms\n";
        std::cout << "Web Port: " << loaded_config.web_port << "\n";
        std::cout << "CPU Alert Threshold: " << loaded_config.cpu_alert_threshold << "%\n";
        std::cout << "Memory Alert Threshold: " << loaded_config.memory_alert_threshold << "%\n";
        std::cout << "Log Level: " << loaded_config.log_level << "\n";
        std::cout << "Console Color: " << (loaded_config.console_color ? "Enabled" : "Disabled") << "\n";
        std::cout << "History Max Points: " << loaded_config.history_max_points << "\n";
        std::cout << "Log File: " << loaded_config.log_file << "\n";
    }
    
    // 环境变量示例
    std::cout << "\n=== Environment Variables ===\n";
    std::cout << "Set MONITOR_PORT to override port\n";
    std::cout << "Set MONITOR_UPDATE_INTERVAL to override interval\n";
    std::cout << "Set MONITOR_LOG_FILE to set log file path\n";
    std::cout << "Set MONITOR_LOG_LEVEL to set log level\n";
    
    // 命令行参数示例
    std::cout << "\n=== Command Line Options ===\n";
    std::cout << "--config <file>    Load configuration from file\n";
    std::cout << "--port <port>      Set web server port\n";
    std::cout << "--interval <ms>    Set update interval\n";
    std::cout << "--no-color         Disable console colors\n";
    std::cout << "--log-file <file>  Set log file path\n";
    
    // 添加告警规则
    std::cout << "\n=== Custom Alert Rules ===\n";
    
    AlertRule cpu_alert;
    cpu_alert.type = AlertRule::CPU;
    cpu_alert.name = "High CPU Alert";
    cpu_alert.threshold = 90.0;
    cpu_alert.condition = ">";
    cpu_alert.enabled = true;
    cpu_alert.cooldown_seconds = 300;
    cpu_alert.command = "echo 'High CPU usage detected!' | mail -s 'Alert' admin@example.com";
    
    AlertRule mem_alert;
    mem_alert.type = AlertRule::MEMORY;
    mem_alert.name = "High Memory Alert";
    mem_alert.threshold = 85.0;
    mem_alert.condition = ">";
    mem_alert.enabled = true;
    mem_alert.cooldown_seconds = 300;
    
    config.alert_rules.push_back(cpu_alert);
    config.alert_rules.push_back(mem_alert);
    
    std::cout << "Added " << config.alert_rules.size() << " alert rules\n";
    
    // 保存更新后的配置
    if (MonitorConfig::saveToFile("monitor_with_alerts.conf", config)) {
        std::cout << "Configuration with alerts saved to monitor_with_alerts.conf\n";
    }
    
    LOG_INFO() << "Config Demo Completed";
    
    return 0;
}
