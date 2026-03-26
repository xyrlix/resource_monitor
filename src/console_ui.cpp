#include "ui.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <thread>
#include <chrono>
#include <sstream>

#ifdef _WIN32
    #include <windows.h>
    #include <conio.h>
#else
    #include <unistd.h>
    #include <termios.h>
#endif

namespace ui {

// ==================== ConsoleUI 实现 ====================

ConsoleUI::ConsoleUI(monitor::ResourceMonitor& monitor)
    : monitor_(monitor), running_(false) {
}

bool ConsoleUI::initialize() {
    return true;
}

bool ConsoleUI::start() {
    if (running_) {
        return false;
    }
    
    running_ = true;
    
    // 设置回调
    monitor_.setCallback([this](const monitor::SystemInfo& info) {
        update(info);
    });
    
    // 启动监控
    if (!monitor_.start(1000)) {
        running_ = false;
        return false;
    }
    
    return true;
}

void ConsoleUI::stop() {
    running_ = false;
    monitor_.stop();
}

void ConsoleUI::update(const monitor::SystemInfo& info) {
    if (!running_) {
        return;
    }
    
    clearScreen();
    
    printHeader();
    std::cout << "\n";
    
    printCPU(info.cpu);
    std::cout << "\n";
    
    printMemory(info.memory);
    std::cout << "\n";
    
    printDisk(info.disks);
    std::cout << "\n";
    
    printNetwork(info.networks);
    std::cout << "\n";
    
    printProcesses(info.top_processes);
    
    std::cout << "\nPress 'q' to quit...\n";
    std::cout.flush();
}

void ConsoleUI::run() {
    if (!start()) {
        return;
    }
    
    while (running_) {
#ifdef _WIN32
        if (_kbhit()) {
            int ch = _getch();
            if (ch == 'q' || ch == 'Q') {
                break;
            }
        }
#else
        // Linux非阻塞输入
        struct termios oldt, newt;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        
        fd_set readfds;
        struct timeval tv;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 100ms
        
        if (select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv) > 0) {
            char ch;
            if (read(STDIN_FILENO, &ch, 1) > 0 && (ch == 'q' || ch == 'Q')) {
                tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                break;
            }
        }
        
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    stop();
}

void ConsoleUI::printHeader() {
    std::cout << "========================================\n";
    std::cout << "      System Resource Monitor         \n";
    std::cout << "========================================\n";
    std::cout << "Hostname: " << monitor_.getSystemInfo().hostname << "\n";
    std::cout << "OS: " << monitor_.getSystemInfo().os_name << "\n";
    std::cout << "Uptime: " << monitor_.getSystemInfo().uptime_seconds / 3600 << "h\n";
}

void ConsoleUI::printCPU(const monitor::CPUInfo& cpu) {
    std::cout << "CPU Usage:\n";
    std::cout << "  Model: " << cpu.model_name << "\n";
    std::cout << "  Cores: " << cpu.core_count << "\n";
    std::cout << "  Total: ";
    drawProgressBar(cpu.usage_percent);
    std::cout << " " << std::fixed << std::setprecision(1) << cpu.usage_percent << "%\n";
    std::cout << "  User:  " << cpu.user_percent << "%\n";
    std::cout << "  System: " << cpu.system_percent << "%\n";
    std::cout << "  Idle:  " << cpu.idle_percent << "%\n";
    
    if (!cpu.per_core_usage.empty()) {
        std::cout << "  Per Core:\n";
        for (size_t i = 0; i < cpu.per_core_usage.size(); i++) {
            std::cout << "    Core " << i << ": ";
            drawProgressBar(cpu.per_core_usage[i], 10);
            std::cout << " " << cpu.per_core_usage[i] << "%\n";
        }
    }
}

void ConsoleUI::printMemory(const monitor::MemoryInfo& memory) {
    std::cout << "Memory Usage:\n";
    std::cout << "  Total: " << memory.total_mb << " MB\n";
    std::cout << "  Used:  " << memory.used_mb << " MB\n";
    std::cout << "  Free:  " << memory.free_mb << " MB\n";
    std::cout << "  Usage: ";
    drawProgressBar(memory.usage_percent);
    std::cout << " " << memory.usage_percent << "%\n";
    
    if (memory.swap_total_mb > 0) {
        std::cout << "  Swap: " << memory.swap_used_mb << " / " 
                 << memory.swap_total_mb << " MB (";
        drawProgressBar(memory.swap_usage_percent, 10);
        std::cout << " " << memory.swap_usage_percent << "%)\n";
    }
}

void ConsoleUI::printDisk(const std::vector<monitor::DiskInfo>& disks) {
    std::cout << "Disk Usage:\n";
    
    for (const auto& disk : disks) {
        std::cout << "  " << disk.mount_point << " (" << disk.file_system << "):\n";
        std::cout << "    Total: " << std::fixed << std::setprecision(2) 
                 << disk.total_gb << " GB\n";
        std::cout << "    Used:  " << disk.used_gb << " GB\n";
        std::cout << "    Free:  " << disk.free_gb << " GB\n";
        std::cout << "    Usage: ";
        drawProgressBar(disk.usage_percent, 15);
        std::cout << " " << disk.usage_percent << "%\n";
        
        if (disk.read_speed_mb_s > 0 || disk.write_speed_mb_s > 0) {
            std::cout << "    R/W:   " << disk.read_speed_mb_s << " / " 
                     << disk.write_speed_mb_s << " MB/s\n";
        }
    }
}

void ConsoleUI::printNetwork(const std::vector<monitor::NetworkInfo>& networks) {
    std::cout << "Network:\n";
    
    for (const auto& net : networks) {
        std::cout << "  " << net.interface << ":\n";
        std::cout << "    Down: " << std::fixed << std::setprecision(2) 
                 << net.speed_in_mb_s << " MB/s\n";
        std::cout << "    Up:   " << net.speed_out_mb_s << " MB/s\n";
        std::cout << "    Rx:   " << net.bytes_received / (1024 * 1024) << " MB\n";
        std::cout << "    Tx:   " << net.bytes_sent / (1024 * 1024) << " MB\n";
    }
}

void ConsoleUI::printProcesses(const std::vector<monitor::ProcessInfo>& processes) {
    std::cout << "Top Processes:\n";
    std::cout << "  PID   %CPU  %MEM  Name\n";
    std::cout << "  " << std::string(40, '-') << "\n";
    
    for (const auto& proc : processes) {
        std::cout << "  " << std::setw(5) << proc.pid << "  ";
        std::cout << std::fixed << std::setprecision(1);
        std::cout << std::setw(4) << proc.cpu_percent << "%  ";
        std::cout << std::setw(4) << proc.memory_percent << "%  ";
        std::cout << proc.name << "\n";
    }
}

void ConsoleUI::drawProgressBar(double percent, int width) {
    int filled = static_cast<int>(percent * width / 100.0);
    
    std::cout << "[";
    for (int i = 0; i < width; i++) {
        if (i < filled) {
            std::cout << "█";
        } else {
            std::cout << " ";
        }
    }
    std::cout << "]";
}

void ConsoleUI::clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void ConsoleUI::moveCursor(int row, int col) {
#ifdef _WIN32
    COORD coord;
    coord.X = col;
    coord.Y = row;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
#else
    std::cout << "\033[" << row << ";" << col << "H";
#endif
}

// ==================== WebUI 实现 ====================

WebUI::WebUI(monitor::ResourceMonitor& monitor, int port)
    : monitor_(monitor), port_(port), running_(false)
#ifdef _WIN32
    , server_socket_(INVALID_SOCKET)
#else
    , server_socket_(-1)
#endif
{
}

bool WebUI::initialize() {
    return true;
}

bool WebUI::start() {
    if (running_) {
        return false;
    }
    
    // 设置回调
    monitor_.setCallback([this](const monitor::SystemInfo& info) {
        std::lock_guard<std::mutex> lock(info_mutex_);
        last_info_ = info;
    });
    
    // 启动监控
    if (!monitor_.start(1000)) {
        return false;
    }
    
    // 启动HTTP服务器
    startHTTPServer();
    
    running_ = true;
    return true;
}

void WebUI::stop() {
    running_ = false;
    monitor_.stop();
    stopHTTPServer();
}

void WebUI::update(const monitor::SystemInfo& info) {
    std::lock_guard<std::mutex> lock(info_mutex_);
    last_info_ = info;
}

void WebUI::run() {
    if (!start()) {
        return;
    }
    
    std::cout << "Web UI started at http://localhost:" << port_ << "\n";
    std::cout << "Press Enter to stop...\n";
    
    std::string line;
    std::getline(std::cin, line);
    
    stop();
}

std::string WebUI::generateHTML(const monitor::SystemInfo& info) {
    std::ostringstream html;
    
    html << R"(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>System Resource Monitor</title>
    <style>)";
    
    html << generateCSS();
    
    html << R"(</style>
    <script>)";
    
    html << generateJavaScript();
    
    html << R"(</script>
</head>
<body>
    <div class="container">
        <h1>System Resource Monitor</h1>
        <div class="info">
            <span>Hostname: )" << info.hostname << R"(</span>
            <span>OS: )" << info.os_name << R"(</span>
            <span>Uptime: )" << (info.uptime_seconds / 3600) << R"(h</span>
        </div>
        
        <div class="section">
            <h2>CPU Usage</h2>
            <div class="metric">
                <span class="label">Total</span>
                <div class="progress">
                    <div class="bar" id="cpu-total" style="width: )" 
                     << info.cpu.usage_percent << R"(%"></div>
                </div>
                <span class="value" id="cpu-value">)" 
                 << std::fixed << std::setprecision(1) << info.cpu.usage_percent << R"(%</span>
            </div>
            <div class="detail">
                <span>User: )" << info.cpu.user_percent << R"(%</span>
                <span>System: )" << info.cpu.system_percent << R"(%</span>
                <span>Idle: )" << info.cpu.idle_percent << R"(%</span>
            </div>
        </div>
        
        <div class="section">
            <h2>Memory Usage</h2>
            <div class="metric">
                <span class="label">RAM</span>
                <div class="progress">
                    <div class="bar" id="mem-ram" style="width: )" 
                     << info.memory.usage_percent << R"(%"></div>
                </div>
                <span class="value" id="mem-value">)" 
                 << std::fixed << std::setprecision(1) << info.memory.usage_percent 
                 << R"%</span>
            </div>
            <div class="detail">
                <span>Used: )" << info.memory.used_mb << R"( MB</span>
                <span>Free: )" << info.memory.free_mb << R"( MB</span>
                <span>Total: )" << info.memory.total_mb << R"( MB</span>
            </div>
        </div>
        
        <div class="section">
            <h2>Disk Usage</h2>)";
    
    for (const auto& disk : info.disks) {
        html << R"(
            <div class="disk">
                <span class="label">)" << disk.mount_point << R"(</span>
                <div class="progress">
                    <div class="bar" style="width: )" << disk.usage_percent 
                     << R"(%"></div>
                </div>
                <span class="value">)" << std::fixed << std::setprecision(1) 
                 << disk.usage_percent << R"%</span>
                <span class="detail">)" << disk.used_gb << " / " << disk.total_gb 
                 << R"( GB</span>
            </div>)";
    }
    
    html << R"(
        </div>
        
        <div class="section">
            <h2>Top Processes</h2>
            <table>
                <thead>
                    <tr>
                        <th>PID</th>
                        <th>%CPU</th>
                        <th>%MEM</th>
                        <th>Name</th>
                    </tr>
                </thead>
                <tbody>)";
    
    for (const auto& proc : info.top_processes) {
        html << R"(
                    <tr>
                        <td>)" << proc.pid << R"(</td>
                        <td>)" << std::fixed << std::setprecision(1) 
                         << proc.cpu_percent << R"(</td>
                        <td>)" << std::fixed << std::setprecision(1) 
                         << proc.memory_percent << R"(</td>
                        <td>)" << proc.name << R"(</td>
                    </tr>)";
    }
    
    html << R"(
                </tbody>
            </table>
        </div>
        
        <div class="footer">
            Last updated: <span id="timestamp">)" << info.timestamp 
             << R"(</span>
        </div>
    </div>
</body>
</html>)";
    
    return html.str();
}

std::string WebUI::generateCSS() {
    return R"(
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #1e3c72 0%, #2a5298 100%);
            min-height: 100vh;
            padding: 20px;
            color: #fff;
        }
        .container {
            max-width: 1200px;
            margin: 0 auto;
        }
        h1 {
            text-align: center;
            margin-bottom: 20px;
            font-size: 2.5em;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
        }
        .info {
            display: flex;
            justify-content: space-around;
            background: rgba(255,255,255,0.1);
            padding: 15px;
            border-radius: 10px;
            margin-bottom: 20px;
        }
        .info span {
            font-size: 1.1em;
        }
        .section {
            background: rgba(255,255,255,0.1);
            padding: 20px;
            border-radius: 10px;
            margin-bottom: 20px;
            backdrop-filter: blur(10px);
        }
        .section h2 {
            margin-bottom: 15px;
            font-size: 1.5em;
        }
        .metric {
            margin-bottom: 10px;
        }
        .label {
            display: inline-block;
            width: 80px;
            font-weight: bold;
        }
        .progress {
            display: inline-block;
            width: 300px;
            height: 25px;
            background: rgba(0,0,0,0.3);
            border-radius: 12px;
            overflow: hidden;
            margin: 0 10px;
        }
        .bar {
            height: 100%;
            background: linear-gradient(90deg, #00ff87, #60efff);
            transition: width 0.5s ease;
            border-radius: 12px;
        }
        .value {
            display: inline-block;
            width: 60px;
            text-align: right;
        }
        .detail {
            margin-left: 90px;
            margin-top: 5px;
            font-size: 0.9em;
            opacity: 0.8;
        }
        .detail span {
            margin-right: 20px;
        }
        .disk {
            margin-bottom: 10px;
        }
        table {
            width: 100%;
            border-collapse: collapse;
            margin-top: 10px;
        }
        th, td {
            padding: 10px;
            text-align: left;
            border-bottom: 1px solid rgba(255,255,255,0.2);
        }
        th {
            background: rgba(255,255,255,0.2);
            font-weight: bold;
        }
        .footer {
            text-align: center;
            margin-top: 20px;
            opacity: 0.7;
        }
    )";
}

std::string WebUI::generateJavaScript() {
    return R"(
        setInterval(function() {
            location.reload();
        }, 5000);
    )";
}

void WebUI::startHTTPServer() {
#ifdef _WIN32
    // Windows socket初始化
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        return;
    }
    
    server_socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket_ == INVALID_SOCKET) {
        return;
    }
    
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port_);
    
    if (bind(server_socket_, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        closesocket(server_socket_);
        return;
    }
    
    if (listen(server_socket_, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(server_socket_);
        return;
    }
#else
    // Linux socket
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ < 0) {
        return;
    }
    
    int opt = 1;
    setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port_);
    
    if (bind(server_socket_, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(server_socket_);
        return;
    }
    
    if (listen(server_socket_, 5) < 0) {
        close(server_socket_);
        return;
    }
#endif
    
    // 启动服务器线程
    std::thread([this]() {
        while (running_) {
#ifdef _WIN32
            sockaddr_in client_addr;
            int client_len = sizeof(client_addr);
            SOCKET client_socket = accept(server_socket_, (sockaddr*)&client_addr, &client_len);
            
            if (client_socket == INVALID_SOCKET) {
                continue;
            }
            
            char buffer[1024];
            recv(client_socket, buffer, sizeof(buffer), 0);
            
            // 发送HTML
            std::lock_guard<std::mutex> lock(info_mutex_);
            std::string html = generateHTML(last_info_);
            
            std::string response = "HTTP/1.1 200 OK\r\n"
                                  "Content-Type: text/html\r\n"
                                  "Content-Length: " + std::to_string(html.length()) + "\r\n"
                                  "Connection: close\r\n\r\n" + html;
            
            send(client_socket, response.c_str(), response.length(), 0);
            closesocket(client_socket);
#else
            sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_socket = accept(server_socket_, (sockaddr*)&client_addr, &client_len);
            
            if (client_socket < 0) {
                continue;
            }
            
            char buffer[1024];
            read(client_socket, buffer, sizeof(buffer));
            
            // 发送HTML
            std::lock_guard<std::mutex> lock(info_mutex_);
            std::string html = generateHTML(last_info_);
            
            std::string response = "HTTP/1.1 200 OK\r\n"
                                  "Content-Type: text/html\r\n"
                                  "Content-Length: " + std::to_string(html.length()) + "\r\n"
                                  "Connection: close\r\n\r\n" + html;
            
            write(client_socket, response.c_str(), response.length());
            close(client_socket);
#endif
        }
    }).detach();
}

void WebUI::stopHTTPServer() {
#ifdef _WIN32
    if (server_socket_ != INVALID_SOCKET) {
        closesocket(server_socket_);
        server_socket_ = INVALID_SOCKET;
    }
    WSACleanup();
#else
    if (server_socket_ >= 0) {
        close(server_socket_);
        server_socket_ = -1;
    }
#endif
}

} // namespace ui
