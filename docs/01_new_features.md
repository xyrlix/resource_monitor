# 资源监视器 - 新增功能文档

## 概述

本文档描述了资源监视器新增的高级功能，包括历史数据记录、配置管理、日志记录等。

---

## 1. 历史数据模块 (History)

### 1.1 功能特性

- **数据存储**: 内存存储，支持最大10000个数据点
- **数据分析**: 统计分析（最小值、最大值、平均值、中位数、标准差）
- **趋势预测**: 基于线性回归的趋势分析和未来预测
- **异常检测**: 自动检测异常值（基于标准差）
- **数据导出**: 支持CSV和JSON格式导出

### 1.2 核心类

#### HistoryPoint
存储单个历史数据点，包含：
- 时间戳
- CPU数据（总体、用户态、内核态、每核使用率）
- 内存数据（总量、已用、可用、缓存、交换空间）
- 磁盘数据（各分区使用率、读写速度）
- 网络数据（各网卡流量、上传下载速度）
- 进程数据（TOP进程列表）

#### MemoryHistoryStorage
内存历史存储实现：
- `save()`: 保存历史数据点
- `query()`: 查询指定时间范围的数据
- `queryRecent()`: 查询最近的N个数据点
- `clear()`: 清除所有历史数据

#### HistoryAnalyzer
历史数据分析器：
- `calculateCpuStatistics()`: CPU统计分析
- `calculateMemoryStatistics()`: 内存统计分析
- `calculateDiskStatistics()`: 磁盘统计分析
- `calculateNetworkStatistics()`: 网络统计分析
- `calculateCpuTrend()`: CPU趋势分析
- `predictCpu()`: CPU预测
- `detectAnomalies()`: 异常检测

#### HistoryExporter
数据导出工具：
- `exportToCSV()`: 导出为CSV格式
- `exportToJSON()`: 导出为JSON格式
- `exportToChartFormat()`: 导出为图表数据格式

### 1.3 使用示例

```cpp
#include "history.h"

// 初始化存储
MemoryHistoryStorage storage(10000);

// 收集数据
HistoryPoint point;
point.timestamp = std::chrono::system_clock::now();
point.cpu_total = 75.5;
// ... 填充其他字段
storage.save(point);

// 分析数据
auto data = storage.queryRecent(100);
HistoryAnalyzer analyzer(data);

auto stats = analyzer.calculateCpuStatistics();
std::cout << "CPU Average: " << stats.avg << "%\n";

auto trend = analyzer.calculateCpuTrend(5);
std::cout << "Trend: " << (trend == Trend::RISING ? "Rising" : "Stable") << "\n";

// 导出数据
HistoryExporter::exportToCSV(data, "history.csv");
HistoryExporter::exportToJSON(data, "history.json");
```

---

## 2. 配置管理模块 (Config)

### 2.1 功能特性

- **配置文件**: 支持INI格式配置文件
- **环境变量**: 支持从环境变量加载配置
- **命令行参数**: 支持命令行参数覆盖
- **配置验证**: 自动验证配置有效性
- **默认配置**: 提供合理的默认值

### 2.2 配置项说明

#### General（通用）
- `update_interval`: 更新间隔（毫秒），默认1000
- `history_max_points`: 历史数据最大点数，默认10000
- `log_file`: 日志文件路径
- `log_level`: 日志级别（DEBUG、INFO、WARNING、ERROR）

#### CPU
- `monitor_cpu`: 是否监控CPU，默认true
- `monitor_per_core`: 是否监控每核，默认true
- `alert_threshold`: CPU告警阈值，默认90%

#### Memory
- `monitor_memory`: 是否监控内存，默认true
- `alert_threshold`: 内存告警阈值，默认90%

#### Disk
- `monitor_disk`: 是否监控磁盘，默认true
- `alert_threshold`: 磁盘告警阈值，默认90%
- `ignore_disks`: 忽略的磁盘列表

#### Network
- `monitor_network`: 是否监控网络，默认true
- `interfaces`: 监控的网卡列表（空表示全部）

#### Process
- `monitor_processes`: 是否监控进程，默认true
- `top_processes_count`: TOP进程数量，默认10

#### Alert
- `enable_alerts`: 是否启用告警，默认true

#### Web
- `port`: Web服务器端口，默认8080
- `bind_address`: 绑定地址，默认0.0.0.0
- `enable_cors`: 是否启用CORS，默认true

#### Console
- `color`: 是否启用彩色输出，默认true
- `refresh`: 是否自动刷新，默认true

#### Export
- `enable_history`: 是否记录历史，默认true
- `format`: 导出格式（json、csv），默认json

### 2.3 使用示例

```cpp
#include "config.h"

// 加载默认配置
auto config = ConfigValidator::getDefault();

// 从文件加载
MonitorConfig::loadFromFile("monitor.conf", config);

// 从环境变量加载
MonitorConfig::loadFromEnv(config);

// 命令行解析
MonitorConfig::parseCommandLine(argc, argv, config);

// 验证配置
std::string error;
if (!ConfigValidator::validate(config, error)) {
    std::cerr << "Invalid config: " << error << std::endl;
    return 1;
}

// 保存配置
MonitorConfig::saveToFile("monitor.conf", config);
```

### 2.4 配置文件示例

```ini
[general]
update_interval=1000
history_max_points=10000
log_file=/var/log/monitor.log
log_level=INFO

[cpu]
monitor_cpu=true
monitor_per_core=true
alert_threshold=90.0

[memory]
monitor_memory=true
alert_threshold=90.0

[web]
port=8080
bind_address=0.0.0.0
enable_cors=true
```

---

## 3. 日志记录模块 (Logger)

### 3.1 功能特性

- **多级别日志**: DEBUG、INFO、WARNING、ERROR
- **文件输出**: 支持输出到文件
- **控制台输出**: 支持输出到控制台
- **线程安全**: 使用互斥锁保证线程安全
- **便捷宏**: 提供便捷的日志宏

### 3.2 使用示例

```cpp
#include "logger.h"

// 初始化日志
Logger::getInstance().initialize("monitor.log", LogLevel::INFO);

// 记录日志
LOG_INFO() << "Monitor started";
LOG_WARNING() << "High CPU usage: " << cpu_usage << "%";
LOG_ERROR() << "Failed to read disk: " << error_message;

// 调试日志（仅在DEBUG级别显示）
LOG_DEBUG() << "Processing PID: " << pid;

// 刷新日志
Logger::getInstance().flush();
```

### 3.3 日志格式

```
2024-03-22 16:30:45 [INFO] Monitor started
2024-03-22 16:30:46 [WARNING] High CPU usage: 85.5%
2024-03-22 16:30:47 [ERROR] Failed to read disk: Permission denied
```

---

## 4. 完整使用示例

### 4.1 启动时加载配置

```cpp
#include "monitor.h"
#include "config.h"
#include "logger.h"

int main(int argc, char* argv[]) {
    // 加载配置
    auto config = ConfigValidator::getDefault();
    MonitorConfig::loadFromFile("monitor.conf", config);
    MonitorConfig::parseCommandLine(argc, argv, config);
    
    // 验证配置
    std::string error;
    if (!ConfigValidator::validate(config, error)) {
        std::cerr << "Invalid config: " << error << std::endl;
        return 1;
    }
    
    // 初始化日志
    Logger::getInstance().initialize(config.log_file,
                                       parseLogLevel(config.log_level));
    
    // 初始化监控器
    ResourceMonitor monitor;
    if (!monitor.initialize()) {
        LOG_ERROR() << "Failed to initialize monitor";
        return 1;
    }
    
    // 启动监控
    monitor.start(config.update_interval_ms);
    
    // 运行UI...
    
    return 0;
}
```

### 4.2 历史数据收集和分析

```cpp
#include "monitor.h"
#include "history.h"

// 初始化
ResourceMonitor monitor;
monitor.initialize();
monitor.start(1000);

MemoryHistoryStorage storage(10000);

// 持续收集数据
while (running) {
    SystemInfo info = monitor.getSystemInfo();
    
    HistoryPoint point;
    // ... 填充数据
    storage.save(point);
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

// 分析
auto data = storage.queryRecent(3600); // 最近1小时
HistoryAnalyzer analyzer(data);

auto stats = analyzer.calculateCpuStatistics();
std::cout << "CPU avg: " << stats.avg << "%\n";

auto anomalies = analyzer.detectAnomalies(2.0);
for (const auto& anomaly : anomalies) {
    std::cout << "Anomaly: " << anomaly.description << "\n";
}
```

---

## 5. 示例程序

### 5.1 history_demo
演示历史数据收集、分析和导出：
```bash
./history_demo
```

功能：
- 收集30秒的系统数据
- 计算CPU和内存的统计信息
- 分析趋势和预测
- 检测异常
- 导出为CSV和JSON

### 5.2 config_demo
演示配置管理功能：
```bash
./config_demo
```

功能：
- 生成默认配置
- 修改配置参数
- 保存到文件
- 从文件加载
- 验证配置有效性
- 添加告警规则

---

## 6. 性能考虑

### 6.1 内存使用
- 每个HistoryPoint约1-2KB（取决于磁盘/网络数量）
- 10000个点约10-20MB内存

### 6.2 CPU开销
- 数据收集：< 0.1%
- 数据分析：O(n)，1000个点约1-2ms
- 导出：O(n)，1000个点约5-10ms

### 6.3 优化建议
- 根据需求调整`history_max_points`
- 定期清理旧数据
- 避免频繁导出大文件

---

## 7. 未来计划

- [ ] 支持数据库存储（SQLite、MySQL）
- [ ] 支持更多图表格式（Prometheus、Grafana）
- [ ] 支持实时数据流推送（WebSocket）
- [ ] 支持分布式监控
- [ ] 支持自定义分析脚本
