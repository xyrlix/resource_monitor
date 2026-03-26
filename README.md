# 跨平台系统资源监视器

一个轻量级的跨平台系统资源监视器，支持Windows和Linux，提供实时系统监控和可视化界面。

## 功能特性

### 核心监控
- ✅ **CPU监控**: 总体使用率、每核使用率、用户态/内核态/空闲比例
- ✅ **内存监控**: 总内存、已用、可用、缓存、交换空间使用情况
- ✅ **磁盘监控**: 各分区使用率、读写速度、IOPS
- ✅ **网络监控**: 各网卡流量、上传/下载速度
- ✅ **进程监控**: TOP进程列表（按CPU/内存排序）

### 高级功能
- ✅ **历史数据记录**: 支持数据存储、统计分析、趋势预测
- ✅ **数据分析**: 统计（均值、中位数、标准差）、异常检测
- ✅ **数据导出**: 支持CSV、JSON格式导出
- ✅ **配置管理**: INI配置文件、环境变量、命令行参数
- ✅ **日志记录**: 多级别日志、文件和控制台输出
- ✅ **告警系统**: 支持自定义告警规则和执行命令

### 界面和部署
- ✅ **跨平台**: 支持Windows和Linux
- ✅ **两种UI模式**: 控制台TUI和Web界面
- ✅ **实时更新**: 可配置刷新间隔
- ✅ **轻量级**: CPU占用 < 1%，内存占用 ~10 MB

## 截图

### 控制台模式
```
========================================
      System Resource Monitor
========================================
Hostname: MyPC
OS: Windows
Uptime: 5h

CPU Usage:
  Model: Intel(R) Core(TM) i7-9700K CPU @ 3.60GHz
  Cores: 8
  Total: [████████████░░░░░] 75.2%
  User:  45.3%
  System: 29.9%
  Idle:  24.8%
  Per Core:
    Core 0: [██████████░░░] 82.5%
    Core 1: [████████░░░░░] 70.2%
    ...

Memory Usage:
  Total: 16384 MB
  Used:  12288 MB
  Free:  4096 MB
  Usage: [████████████░░] 75.0%

Disk Usage:
  C:\ (NTFS):
    Total: 500.00 GB
    Used:  350.00 GB
    Free:  150.00 GB
    Usage: [██████████░░░] 70.0%
  D:\ (NTFS):
    ...

Network:
  eth0:
    Down: 5.23 MB/s
    Up:   1.15 MB/s
    Rx:   1024 MB
    Tx:   256 MB

Top Processes:
  PID   %CPU  %MEM  Name
  ----------------------------------------
   1234  45.2%  8.5%  chrome.exe
   5678  23.1%  5.2%  node.exe
   ...

Press 'q' to quit...
```

### Web界面
访问 `http://localhost:8080` 查看美观的Web界面，包含：
- 渐变色进度条
- 实时数据更新（5秒自动刷新）
- 响应式设计
- 表格化进程列表

## 编译

### Windows (MSVC)
```bash
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
```

### Linux (GCC)
```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

### macOS (Clang)
```bash
mkdir build
cd build
cmake ..
make -j$(sysctl -n hw.ncpu)
```

## 使用方法

### 控制台模式
```bash
./resource_monitor --console
# 或
./resource_monitor -c
```

### Web模式
```bash
./resource_monitor --web
# 或
./resource_monitor -w

# 指定端口
./resource_monitor -w --port 3000
# 或
./resource_monitor -w -p 3000
```

### 查看帮助
```bash
./resource_monitor --help
# 或
./resource_monitor -h
```

## 命令行选项

| 选项 | 说明 |
|------|------|
| `--console`, `-c` | 控制台模式 |
| `--web`, `-w` | Web模式 |
| `--port`, `-p PORT` | Web服务器端口（默认8080） |
| `--help`, `-h` | 显示帮助信息 |

## 告警功能示例

```cpp
#include "monitor.h"
#include "ui.h"

int main() {
    monitor::ResourceMonitor monitor;
    monitor.initialize();
    
    // 创建告警管理器
    monitor::AlertManager alert_manager(monitor);
    
    // 添加CPU告警
    monitor::AlertRule cpu_alert;
    cpu_alert.name = "High CPU";
    cpu_alert.type = monitor::AlertRule::CPU;
    cpu_alert.threshold = 80.0;
    cpu_alert.enabled = true;
    cpu_alert.callback = []() {
        std::cout << "\n[ALERT] High CPU usage detected!\n";
    };
    alert_manager.addAlert(cpu_alert);
    
    // 启动监控和告警
    monitor.start(1000);
    alert_manager.start();
    
    // 运行UI
    ui::ConsoleUI ui(monitor);
    ui.run();
    
    return 0;
}
```

## 技术细节

### Windows实现
- 使用 `GetSystemTimes()` 获取CPU使用率
- 使用 `GlobalMemoryStatusEx()` 获取内存信息
- 使用 `GetDiskFreeSpaceEx()` 获取磁盘信息
- 使用 `GetIfTable2()` 获取网络信息
- 使用 `EnumProcesses()` 获取进程列表

### Linux实现
- 读取 `/proc/stat` 获取CPU信息
- 读取 `/proc/meminfo` 获取内存信息
- 读取 `/proc/mounts` 和 `statvfs()` 获取磁盘信息
- 读取 `/proc/net/dev` 获取网络信息
- 读取 `/proc/[pid]/` 获取进程信息

## 系统要求

### Windows
- Windows 7 或更高版本
- Visual Studio 2017 或更高版本
- MSVC 编译器

### Linux
- 任意主流Linux发行版
- GCC 7.0 或更高版本
- glibc 2.17 或更高版本

### macOS
- macOS 10.13 或更高版本
- Xcode 命令行工具
- Clang 5.0 或更高版本

## 性能

- CPU占用: < 1%
- 内存占用: ~10 MB
- 更新频率: 可配置（默认1秒）
- 支持监控核心数: 无限制

## 高级功能

### 历史数据分析
```cpp
#include "history.h"

// 收集历史数据
MemoryHistoryStorage storage(10000);
// ... 收集数据

// 分析数据
HistoryAnalyzer analyzer(data);
auto stats = analyzer.calculateCpuStatistics();
auto trend = analyzer.calculateCpuTrend(5);
auto anomalies = analyzer.detectAnomalies(2.0);

// 导出数据
HistoryExporter::exportToCSV(data, "history.csv");
HistoryExporter::exportToJSON(data, "history.json");
```

### 配置管理
```bash
# 创建配置文件
./config_demo

# 使用配置文件启动
./resource_monitor --config monitor.conf

# 查看配置示例
cat monitor.conf
```

### 日志记录
```cpp
#include "logger.h"

// 初始化日志
Logger::getInstance().initialize("monitor.log", LogLevel::INFO);

// 记录日志
LOG_INFO() << "Monitor started";
LOG_WARNING() << "High CPU usage";
LOG_ERROR() << "Failed to read disk";
```

## 示例程序

### history_demo
演示历史数据收集、分析和导出：
```bash
./history_demo
```

### config_demo
演示配置管理功能：
```bash
./config_demo
```

## 技术文档

- [新功能详细文档](docs/01_new_features.md) - 历史、配置、日志模块详细说明

## 待实现功能

- [ ] 图表绘制（集成ECharts/Chart.js）
- [ ] 远程监控支持（WebSocket推送）
- [ ] 数据库存储（SQLite、MySQL）
- [ ] 更多告警规则（基于趋势、阈值组合）
- [ ] macOS支持（已完成代码，待测试）
- [ ] 分布式监控（Agent-Server模式）

## 许可证

MIT License

## 作者

IM System Project

## 贡献

欢迎提交Issue和Pull Request！
