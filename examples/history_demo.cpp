#include "monitor.h"
#include "history.h"
#include "ui.h"
#include "config.h"
#include "logger.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace monitor;

int main() {
    // 初始化日志
    Logger::getInstance().initialize("history_demo.log", LogLevel::INFO);
    LOG_INFO() << "History Demo Started";
    
    // 初始化监控器
    ResourceMonitor monitor;
    if (!monitor.initialize()) {
        LOG_ERROR() << "Failed to initialize monitor";
        return 1;
    }
    
    // 初始化历史存储
    MemoryHistoryStorage storage(10000);
    
    // 启动监控
    monitor.start(1000);
    
    LOG_INFO() << "Collecting data for 30 seconds...";
    
    // 收集30秒数据
    for (int i = 0; i < 30; ++i) {
        SystemInfo info = monitor.getSystemInfo();
        
        HistoryPoint point;
        point.timestamp = std::chrono::system_clock::now();
        point.cpu_total = info.cpu.total_usage;
        point.cpu_user = info.cpu.user_usage;
        point.cpu_system = info.cpu.system_usage;
        point.memory_total = info.memory.total;
        point.memory_used = info.memory.used;
        point.memory_available = info.memory.available;
        
        // Disk
        for (const auto& disk : info.disks) {
            HistoryPoint::DiskPoint dp;
            dp.mount_point = disk.mount_point;
            dp.total = disk.total;
            dp.used = disk.used;
            dp.free = disk.free;
            point.disks.push_back(dp);
        }
        
        // Network
        for (const auto& net : info.networks) {
            HistoryPoint::NetworkPoint np;
            np.interface = net.interface;
            np.rx_speed = net.rx_speed;
            np.tx_speed = net.tx_speed;
            np.total_rx = net.total_rx;
            np.total_tx = net.total_tx;
            point.networks.push_back(np);
        }
        
        storage.save(point);
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    LOG_INFO() << "Data collection complete. Analyzing...";
    
    // 获取最近的数据
    auto recent_data = storage.queryRecent(30);
    
    // 分析
    HistoryAnalyzer analyzer(recent_data);
    
    // CPU统计
    auto cpu_stats = analyzer.calculateCpuStatistics();
    std::cout << "\n=== CPU Statistics ===\n";
    std::cout << "Min:   " << cpu_stats.min << "%\n";
    std::cout << "Max:   " << cpu_stats.max << "%\n";
    std::cout << "Avg:   " << cpu_stats.avg << "%\n";
    std::cout << "Std:   " << cpu_stats.std_dev << "%\n";
    
    // 内存统计
    auto mem_stats = analyzer.calculateMemoryStatistics();
    std::cout << "\n=== Memory Statistics ===\n";
    std::cout << "Min:   " << mem_stats.min << "%\n";
    std::cout << "Max:   " << mem_stats.max << "%\n";
    std::cout << "Avg:   " << mem_stats.avg << "%\n";
    
    // 趋势
    auto cpu_trend = analyzer.calculateCpuTrend(5);
    std::cout << "\n=== Trends ===\n";
    std::cout << "CPU Trend: ";
    switch (cpu_trend) {
        case HistoryAnalyzer::Trend::RISING:
            std::cout << "RISING\n";
            break;
        case HistoryAnalyzer::Trend::FALLING:
            std::cout << "FALLING\n";
            break;
        case HistoryAnalyzer::Trend::STABLE:
            std::cout << "STABLE\n";
            break;
    }
    
    // 预测
    auto cpu_pred = analyzer.predictCpu(5);
    std::cout << "\n=== Predictions ===\n";
    std::cout << "CPU (5 min): " << cpu_pred.predicted_value
              << "% (confidence: " << cpu_pred.confidence << ")\n";
    std::cout << "Reason: " << cpu_pred.explanation << "\n";
    
    // 异常检测
    auto anomalies = analyzer.detectAnomalies(2.0);
    if (!anomalies.empty()) {
        std::cout << "\n=== Anomalies Detected ===\n";
        for (const auto& anomaly : anomalies) {
            std::cout << anomaly.description
                      << " (deviation: " << anomaly.deviation << ")\n";
        }
    }
    
    // 导出
    std::cout << "\n=== Exporting Data ===\n";
    
    if (HistoryExporter::exportToCSV(recent_data, "history.csv")) {
        std::cout << "CSV exported to history.csv\n";
    }
    
    if (HistoryExporter::exportToJSON(recent_data, "history.json")) {
        std::cout << "JSON exported to history.json\n";
    }
    
    // 图表数据
    std::string chart_data = HistoryExporter::exportToChartFormat(recent_data, "cpu");
    std::cout << "Chart data length: " << chart_data.length() << " bytes\n";
    
    monitor.stop();
    LOG_INFO() << "History Demo Completed";
    
    std::cout << "\nPress Enter to exit...";
    std::cin.get();
    
    return 0;
}
