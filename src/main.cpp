#include "monitor.h"
#include "ui.h"
#include <iostream>
#include <string>
#include <cstring>

void printUsage(const char* program) {
    std::cout << "Usage: " << program << " [OPTIONS]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --console, -c    Run in console mode (TUI)\n";
    std::cout << "  --web, -w        Run in web mode (default: 8080)\n";
    std::cout << "  --port, -p PORT  Web server port (default: 8080)\n";
    std::cout << "  --help, -h       Show this help message\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << program << " --console     # Console mode\n";
    std::cout << "  " << program << " --web         # Web mode on port 8080\n";
    std::cout << "  " << program << " -w -p 3000    # Web mode on port 3000\n";
}

int main(int argc, char* argv[]) {
    enum Mode { CONSOLE, WEB };
    Mode mode = CONSOLE;
    int port = 8080;
    
    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--console") == 0 || strcmp(argv[i], "-c") == 0) {
            mode = CONSOLE;
        } else if (strcmp(argv[i], "--web") == 0 || strcmp(argv[i], "-w") == 0) {
            mode = WEB;
        } else if (strcmp(argv[i], "--port") == 0 || strcmp(argv[i], "-p") == 0) {
            if (i + 1 < argc) {
                port = std::atoi(argv[i + 1]);
                i++;
            }
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printUsage(argv[0]);
            return 0;
        } else {
            std::cerr << "Unknown option: " << argv[i] << "\n";
            printUsage(argv[0]);
            return 1;
        }
    }
    
    // 创建监控器
    monitor::ResourceMonitor monitor;
    if (!monitor.initialize()) {
        std::cerr << "Failed to initialize monitor\n";
        return 1;
    }
    
    // 创建UI
    std::unique_ptr<ui::MonitorUI> ui;
    
    if (mode == CONSOLE) {
        ui = std::make_unique<ui::ConsoleUI>(monitor);
        std::cout << "Starting Resource Monitor (Console Mode)...\n";
    } else {
        ui = std::make_unique<ui::WebUI>(monitor, port);
        std::cout << "Starting Resource Monitor (Web Mode)...\n";
    }
    
    if (!ui->initialize()) {
        std::cerr << "Failed to initialize UI\n";
        return 1;
    }
    
    // 运行UI
    ui->run();
    
    std::cout << "\nResource Monitor stopped.\n";
    return 0;
}
