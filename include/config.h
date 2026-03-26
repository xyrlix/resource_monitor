#ifndef RESOURCE_MONITOR_CONFIG_H
#define RESOURCE_MONITOR_CONFIG_H

#include <string>
#include <vector>
#include <map>
#include <cstdint>

namespace monitor {

/**
 * @brief 告警规则配置
 */
struct AlertRule {
    enum Type {
        CPU, MEMORY, DISK, NETWORK, PROCESS
    };
    
    Type type;
    std::string name;
    double threshold;
    std::string condition;  // ">", "<", "==", "!="
    bool enabled;
    uint32_t cooldown_seconds;
    std::string command;    // 触发时执行的命令
    
    // 进程监控专用
    std::string process_name;
    bool check_process_count;
    uint32_t process_threshold;
};

/**
 * @brief 监控配置
 */
struct MonitorConfig {
    // 通用设置
    uint32_t update_interval_ms = 1000;
    uint32_t history_max_points = 10000;
    std::string log_file;
    std::string log_level = "INFO";
    
    // CPU设置
    bool monitor_cpu = true;
    bool monitor_per_core = true;
    double cpu_alert_threshold = 90.0;
    
    // 内存设置
    bool monitor_memory = true;
    double memory_alert_threshold = 90.0;
    
    // 磁盘设置
    bool monitor_disk = true;
    double disk_alert_threshold = 90.0;
    std::vector<std::string> ignore_disks;
    
    // 网络设置
    bool monitor_network = true;
    std::vector<std::string> network_interfaces;  // 空表示监控所有
    
    // 进程设置
    bool monitor_processes = true;
    uint32_t top_processes_count = 10;
    
    // 告警设置
    bool enable_alerts = true;
    std::vector<AlertRule> alert_rules;
    
    // Web设置
    uint32_t web_port = 8080;
    std::string web_bind_address = "0.0.0.0";
    bool web_enable_cors = true;
    
    // 控制台设置
    bool console_color = true;
    bool console_refresh = true;
    
    // 导出设置
    bool enable_history = true;
    std::string history_format = "json";  // json, csv
    
    /**
     * @brief 从配置文件加载
     */
    static bool loadFromFile(const std::string& filename, MonitorConfig& config);
    
    /**
     * @brief 保存到配置文件
     */
    static bool saveToFile(const std::string& filename, const MonitorConfig& config);
    
    /**
     * @brief 从环境变量加载
     */
    static void loadFromEnv(MonitorConfig& config);
    
    /**
     * @brief 命令行解析
     */
    static void parseCommandLine(int argc, char* argv[], MonitorConfig& config);
};

/**
 * @brief 配置验证器
 */
class ConfigValidator {
public:
    /**
     * @brief 验证配置是否有效
     */
    static bool validate(const MonitorConfig& config, std::string& error);
    
    /**
     * @brief 获取默认配置
     */
    static MonitorConfig getDefault();
    
private:
    static bool validateThreshold(double threshold);
    static bool validatePort(uint32_t port);
};

} // namespace monitor

#endif // RESOURCE_MONITOR_CONFIG_H
