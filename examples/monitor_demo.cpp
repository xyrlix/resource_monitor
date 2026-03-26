#include "../include/monitor.h"
#include "../include/ui.h"
#include <iostream>
#include <chrono>
#include <thread>

using namespace monitor;
using namespace ui;

// 告警回调函数
void onHighCPU() {
    std::cout << "\n⚠️  [ALERT] CPU usage is above 80%!\n";
}

void onHighMemory() {
    std::cout << "\n⚠️  [ALERT] Memory usage is above 90%!\n";
}

int main() {
    std::cout << "========================================\n";
    std::cout << "   Resource Monitor Demo\n";
    std::cout << "========================================\n\n";
    
    // 创建监控器
    ResourceMonitor monitor;
    if (!monitor.initialize()) {
        std::cerr << "Failed to initialize monitor!\n";
        return 1;
    }
    
    std::cout << "Monitor initialized successfully.\n\n";
    
    // 获取并显示系统信息
    auto info = monitor.getSystemInfo();
    
    std::cout << "System Information:\n";
    std::cout << "  Hostname: " << info.hostname << "\n";
    std::cout << "  OS: " << info.os_name << " " << info.os_version << "\n";
    std::cout << "  Architecture: " << info.arch << "\n";
    std::cout << "  Uptime: " << info.uptime_seconds / 3600 << "h\n\n";
    
    // CPU信息
    std::cout << "CPU Info:\n";
    std::cout << "  Model: " << info.cpu.model_name << "\n";
    std::cout << "  Cores: " << info.cpu.core_count << "\n";
    std::cout << "  Usage: " << info.cpu.usage_percent << "%\n";
    std::cout << "  User: " << info.cpu.user_percent << "%\n";
    std::cout << "  System: " << info.cpu.system_percent << "%\n\n";
    
    // 内存信息
    std::cout << "Memory Info:\n";
    std::cout << "  Total: " << info.memory.total_mb << " MB\n";
    std::cout << "  Used: " << info.memory.used_mb << " MB\n";
    std::cout << "  Free: " << info.memory.free_mb << " MB\n";
    std::cout << "  Usage: " << info.memory.usage_percent << "%\n\n";
    
    // 磁盘信息
    std::cout << "Disk Info:\n";
    for (const auto& disk : info.disks) {
        std::cout << "  " << disk.mount_point << ":\n";
        std::cout << "    Total: " << disk.total_gb << " GB\n";
        std::cout << "    Used: " << disk.used_gb << " GB\n";
        std::cout << "    Free: " << disk.free_gb << " GB\n";
        std::cout << "    Usage: " << disk.usage_percent << "%\n";
    }
    std::cout << "\n";
    
    // 网络信息
    std::cout << "Network Info:\n";
    for (const auto& net : info.networks) {
        std::cout << "  " << net.interface << ":\n";
        std::cout << "    RX: " << net.bytes_received / (1024 * 1024) << " MB\n";
        std::cout << "    TX: " << net.bytes_sent / (1024 * 1024) << " MB\n";
    }
    std::cout << "\n";
    
    // TOP进程
    std::cout << "Top 5 Processes:\n";
    for (const auto& proc : info.top_processes) {
        std::cout << "  PID: " << proc.pid << ", CPU: " << proc.cpu_percent 
                  << "%, MEM: " << proc.memory_mb << " MB, Name: " << proc.name << "\n";
    }
    std::cout << "\n";
    
    // 设置回调
    monitor.setCallback([](const SystemInfo& info) {
        std::cout << "\rCPU: " << std::fixed << std::setprecision(1) 
                  << info.cpu.usage_percent << "% | "
                  << "Memory: " << info.memory.usage_percent << "% | "
                  << "Processes: " << info.top_processes.size() 
                  << std::flush;
    });
    
    // 创建告警管理器
    AlertManager alert_manager(monitor);
    
    AlertRule cpu_alert;
    cpu_alert.name = "High CPU";
    cpu_alert.type = AlertRule::CPU;
    cpu_alert.threshold = 80.0;
    cpu_alert.enabled = true;
    cpu_alert.callback = onHighCPU;
    alert_manager.addAlert(cpu_alert);
    
    AlertRule memory_alert;
    memory_alert.name = "High Memory";
    memory_alert.type = AlertRule::MEMORY;
    memory_alert.threshold = 90.0;
    memory_alert.enabled = true;
    memory_alert.callback = onHighMemory;
    alert_manager.addAlert(memory_alert);
    
    std::cout << "Starting monitor (Press Enter to stop)...\n\n";
    
    // 启动监控
    monitor.start(1000);
    alert_manager.start();
    
    // 等待用户输入
    std::cin.get();
    
    // 停止监控
    alert_manager.stop();
    monitor.stop();
    
    std::cout << "\n\nMonitor stopped.\n";
    
    return 0;
}
