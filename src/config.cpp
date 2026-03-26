#include "config.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <regex>

namespace monitor {

//==============================================================================
// MonitorConfig 实现
//==============================================================================

bool MonitorConfig::loadFromFile(const std::string& filename, MonitorConfig& config) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    int current_section = -1;
    
    while (std::getline(file, line)) {
        // 去除空白和注释
        line = std::regex_replace(line, std::regex("^\\s+|\\s+$"), "");
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // Section
        if (line[0] == '[' && line.back() == ']') {
            std::string section = line.substr(1, line.length() - 2);
            
            if (section == "general") {
                current_section = 0;
            } else if (section == "cpu") {
                current_section = 1;
            } else if (section == "memory") {
                current_section = 2;
            } else if (section == "disk") {
                current_section = 3;
            } else if (section == "network") {
                current_section = 4;
            } else if (section == "process") {
                current_section = 5;
            } else if (section == "alert") {
                current_section = 6;
            } else if (section == "web") {
                current_section = 7;
            } else if (section == "console") {
                current_section = 8;
            } else if (section == "export") {
                current_section = 9;
            }
            continue;
        }
        
        // Key-Value
        size_t pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }
        
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        
        // Trim
        key = std::regex_replace(key, std::regex("^\\s+|\\s+$"), "");
        value = std::regex_replace(value, std::regex("^\\s+|\\s+$"), "");
        
        // Parse based on section
        switch (current_section) {
            case 0: // general
                if (key == "update_interval") config.update_interval_ms = std::stoul(value);
                else if (key == "history_max_points") config.history_max_points = std::stoul(value);
                else if (key == "log_file") config.log_file = value;
                else if (key == "log_level") config.log_level = value;
                break;
                
            case 1: // cpu
                if (key == "monitor_cpu") config.monitor_cpu = (value == "true" || value == "1");
                else if (key == "monitor_per_core") config.monitor_per_core = (value == "true" || value == "1");
                else if (key == "alert_threshold") config.cpu_alert_threshold = std::stod(value);
                break;
                
            case 2: // memory
                if (key == "monitor_memory") config.monitor_memory = (value == "true" || value == "1");
                else if (key == "alert_threshold") config.memory_alert_threshold = std::stod(value);
                break;
                
            case 3: // disk
                if (key == "monitor_disk") config.monitor_disk = (value == "true" || value == "1");
                else if (key == "alert_threshold") config.disk_alert_threshold = std::stod(value);
                else if (key == "ignore_disks") {
                    // TODO: Parse comma-separated list
                }
                break;
                
            case 4: // network
                if (key == "monitor_network") config.monitor_network = (value == "true" || value == "1");
                // TODO: Parse interface list
                break;
                
            case 5: // process
                if (key == "monitor_processes") config.monitor_processes = (value == "true" || value == "1");
                else if (key == "top_processes_count") config.top_processes_count = std::stoul(value);
                break;
                
            case 6: // alert
                if (key == "enable_alerts") config.enable_alerts = (value == "true" || value == "1");
                break;
                
            case 7: // web
                if (key == "port") config.web_port = std::stoul(value);
                else if (key == "bind_address") config.web_bind_address = value;
                else if (key == "enable_cors") config.web_enable_cors = (value == "true" || value == "1");
                break;
                
            case 8: // console
                if (key == "color") config.console_color = (value == "true" || value == "1");
                else if (key == "refresh") config.console_refresh = (value == "true" || value == "1");
                break;
                
            case 9: // export
                if (key == "enable_history") config.enable_history = (value == "true" || value == "1");
                else if (key == "format") config.history_format = value;
                break;
        }
    }
    
    file.close();
    return true;
}

bool MonitorConfig::saveToFile(const std::string& filename, const MonitorConfig& config) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    file << "# Resource Monitor Configuration\n\n";
    
    file << "[general]\n";
    file << "update_interval=" << config.update_interval_ms << "\n";
    file << "history_max_points=" << config.history_max_points << "\n";
    file << "log_file=" << config.log_file << "\n";
    file << "log_level=" << config.log_level << "\n\n";
    
    file << "[cpu]\n";
    file << "monitor_cpu=" << (config.monitor_cpu ? "true" : "false") << "\n";
    file << "monitor_per_core=" << (config.monitor_per_core ? "true" : "false") << "\n";
    file << "alert_threshold=" << config.cpu_alert_threshold << "\n\n";
    
    file << "[memory]\n";
    file << "monitor_memory=" << (config.monitor_memory ? "true" : "false") << "\n";
    file << "alert_threshold=" << config.memory_alert_threshold << "\n\n";
    
    file << "[disk]\n";
    file << "monitor_disk=" << (config.monitor_disk ? "true" : "false") << "\n";
    file << "alert_threshold=" << config.disk_alert_threshold << "\n\n";
    
    file << "[network]\n";
    file << "monitor_network=" << (config.monitor_network ? "true" : "false") << "\n\n";
    
    file << "[process]\n";
    file << "monitor_processes=" << (config.monitor_processes ? "true" : "false") << "\n";
    file << "top_processes_count=" << config.top_processes_count << "\n\n";
    
    file << "[alert]\n";
    file << "enable_alerts=" << (config.enable_alerts ? "true" : "false") << "\n\n";
    
    file << "[web]\n";
    file << "port=" << config.web_port << "\n";
    file << "bind_address=" << config.web_bind_address << "\n";
    file << "enable_cors=" << (config.web_enable_cors ? "true" : "false") << "\n\n";
    
    file << "[console]\n";
    file << "color=" << (config.console_color ? "true" : "false") << "\n";
    file << "refresh=" << (config.console_refresh ? "true" : "false") << "\n\n";
    
    file << "[export]\n";
    file << "enable_history=" << (config.enable_history ? "true" : "false") << "\n";
    file << "format=" << config.history_format << "\n";
    
    file.close();
    return true;
}

void MonitorConfig::loadFromEnv(MonitorConfig& config) {
    // Check environment variables
    if (const char* env = std::getenv("MONITOR_UPDATE_INTERVAL")) {
        config.update_interval_ms = std::stoul(env);
    }
    if (const char* env = std::getenv("MONITOR_PORT")) {
        config.web_port = std::stoul(env);
    }
    if (const char* env = std::getenv("MONITOR_LOG_FILE")) {
        config.log_file = env;
    }
    if (const char* env = std::getenv("MONITOR_LOG_LEVEL")) {
        config.log_level = env;
    }
}

void MonitorConfig::parseCommandLine(int argc, char* argv[], MonitorConfig& config) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            // Print help (handled by main)
            continue;
        }
        
        if (arg == "-c" || arg == "--console") {
            // Console mode (handled by main)
            continue;
        }
        
        if (arg == "-w" || arg == "--web") {
            // Web mode (handled by main)
            continue;
        }
        
        if (arg == "-p" || arg == "--port") {
            if (i + 1 < argc) {
                config.web_port = std::stoul(argv[++i]);
            }
            continue;
        }
        
        if (arg == "-i" || arg == "--interval") {
            if (i + 1 < argc) {
                config.update_interval_ms = std::stoul(argv[++i]);
            }
            continue;
        }
        
        if (arg == "--no-color") {
            config.console_color = false;
            continue;
        }
        
        if (arg == "--config") {
            if (i + 1 < argc) {
                loadFromFile(argv[++i], config);
            }
            continue;
        }
        
        if (arg == "--log-file") {
            if (i + 1 < argc) {
                config.log_file = argv[++i];
            }
            continue;
        }
    }
}

//==============================================================================
// ConfigValidator 实现
//==============================================================================

bool ConfigValidator::validate(const MonitorConfig& config, std::string& error) {
    // Validate intervals
    if (config.update_interval_ms < 100) {
        error = "Update interval must be at least 100ms";
        return false;
    }
    
    if (config.update_interval_ms > 60000) {
        error = "Update interval must be at most 60000ms";
        return false;
    }
    
    // Validate thresholds
    if (!validateThreshold(config.cpu_alert_threshold)) {
        error = "CPU alert threshold must be between 0 and 100";
        return false;
    }
    
    if (!validateThreshold(config.memory_alert_threshold)) {
        error = "Memory alert threshold must be between 0 and 100";
        return false;
    }
    
    if (!validateThreshold(config.disk_alert_threshold)) {
        error = "Disk alert threshold must be between 0 and 100";
        return false;
    }
    
    // Validate port
    if (!validatePort(config.web_port)) {
        error = "Web port must be between 1024 and 65535";
        return false;
    }
    
    // Validate history
    if (config.history_max_points < 100) {
        error = "History max points must be at least 100";
        return false;
    }
    
    if (config.history_max_points > 1000000) {
        error = "History max points must be at most 1000000";
        return false;
    }
    
    // Validate log level
    if (config.log_level != "DEBUG" && config.log_level != "INFO" &&
        config.log_level != "WARNING" && config.log_level != "ERROR") {
        error = "Log level must be DEBUG, INFO, WARNING, or ERROR";
        return false;
    }
    
    return true;
}

MonitorConfig ConfigValidator::getDefault() {
    MonitorConfig config;
    return config;
}

bool ConfigValidator::validateThreshold(double threshold) {
    return threshold >= 0.0 && threshold <= 100.0;
}

bool ConfigValidator::validatePort(uint32_t port) {
    return port >= 1024 && port <= 65535;
}

} // namespace monitor
