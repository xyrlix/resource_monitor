#pragma once

#include "monitor.h"
#include <string>
#include <vector>
#include <memory>

namespace ui {

/**
 * 可视化界面基类
 */
class MonitorUI {
public:
    virtual ~MonitorUI() = default;
    
    /**
     * 初始化UI
     */
    virtual bool initialize() = 0;
    
    /**
     * 启动UI
     */
    virtual bool start() = 0;
    
    /**
     * 停止UI
     */
    virtual void stop() = 0;
    
    /**
     * 更新系统信息
     */
    virtual void update(const monitor::SystemInfo& info) = 0;
    
    /**
     * 运行主循环
     */
    virtual void run() = 0;
};

/**
 * 控制台UI（TUI）
 */
class ConsoleUI : public MonitorUI {
public:
    ConsoleUI(monitor::ResourceMonitor& monitor);
    
    bool initialize() override;
    bool start() override;
    void stop() override;
    void update(const monitor::SystemInfo& info) override;
    void run() override;
    
private:
    void printHeader();
    void printCPU(const monitor::CPUInfo& cpu);
    void printMemory(const monitor::MemoryInfo& memory);
    void printDisk(const std::vector<monitor::DiskInfo>& disks);
    void printNetwork(const std::vector<monitor::NetworkInfo>& networks);
    void printProcesses(const std::vector<monitor::ProcessInfo>& processes);
    
    void drawProgressBar(double percent, int width = 20);
    void clearScreen();
    void moveCursor(int row, int col);
    
    monitor::ResourceMonitor& monitor_;
    bool running_;
};

/**
 * Web UI（HTML/JavaScript）
 */
class WebUI : public MonitorUI {
public:
    WebUI(monitor::ResourceMonitor& monitor, int port = 8080);
    
    bool initialize() override;
    bool start() override;
    void stop() override;
    void update(const monitor::SystemInfo& info) override;
    void run() override;
    
private:
    std::string generateHTML(const monitor::SystemInfo& info);
    std::string generateCSS();
    std::string generateJavaScript();
    
    void startHTTPServer();
    void stopHTTPServer();
    
    monitor::ResourceMonitor& monitor_;
    int port_;
    bool running_;
    monitor::SystemInfo last_info_;
    std::mutex info_mutex_;
    
#ifdef _WIN32
    SOCKET server_socket_;
#else
    int server_socket_;
#endif
};

} // namespace ui
