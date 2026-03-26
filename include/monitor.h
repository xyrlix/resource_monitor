#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/sysinfo.h>
    #include <sys/statvfs.h>
#endif

namespace monitor {

// ==================== 系统信息结构体 ====================

/**
 * CPU信息
 */
struct CPUInfo {
    double usage_percent;           // CPU使用率 (0-100)
    double user_percent;            // 用户态使用率
    double system_percent;          // 内核态使用率
    double idle_percent;            // 空闲率
    int core_count;                 // CPU核心数
    std::vector<double> per_core_usage; // 每核使用率
    int64_t frequency_mhz;          // CPU频率
    std::string model_name;         // CPU型号
};

/**
 * 内存信息
 */
struct MemoryInfo {
    int64_t total_mb;              // 总内存 (MB)
    int64_t used_mb;               // 已用内存 (MB)
    int64_t free_mb;               // 可用内存 (MB)
    int64_t cached_mb;             // 缓存 (MB, 仅Linux)
    double usage_percent;           // 使用率 (0-100)
    double swap_total_mb;           // 交换空间总量 (MB)
    double swap_used_mb;           // 交换空间已用 (MB)
    double swap_usage_percent;     // 交换空间使用率
};

/**
 * 磁盘信息
 */
struct DiskInfo {
    std::string mount_point;       // 挂载点
    std::string file_system;       // 文件系统类型
    int64_t total_gb;              // 总容量 (GB)
    int64_t used_gb;               // 已用 (GB)
    int64_t free_gb;               // 可用 (GB)
    double usage_percent;          // 使用率 (0-100)
    double read_speed_mb_s;        // 读取速度 (MB/s)
    double write_speed_mb_s;       // 写入速度 (MB/s)
    int64_t read_bytes_total;      // 总读取字节数
    int64_t write_bytes_total;     // 总写入字节数
    double iops_in;                // 读取IOPS
    double iops_out;               // 写入IOPS
};

/**
 * 网络信息
 */
struct NetworkInfo {
    std::string interface;         // 网卡名称
    int64_t bytes_received;        // 接收字节数
    int64_t bytes_sent;            // 发送字节数
    double speed_in_mb_s;         // 下载速度 (MB/s)
    double speed_out_mb_s;        // 上传速度 (MB/s)
    int64_t packets_received;      // 接收包数
    int64_t packets_sent;         // 发送包数
    double packets_in_per_sec;    // 接收包/秒
    double packets_out_per_sec;   // 发送包/秒
};

/**
 * 进程信息
 */
struct ProcessInfo {
    int pid;                      // 进程ID
    std::string name;             // 进程名称
    std::string user;             // 用户名
    double cpu_percent;           // CPU使用率
    int64_t memory_mb;            // 内存使用 (MB)
    double memory_percent;         // 内存使用率
    std::string state;            // 进程状态
    int64_t uptime_seconds;       // 运行时间 (秒)
    int64_t threads;              // 线程数
    int64_t read_bytes;           // 读取字节数
    int64_t write_bytes;          // 写入字节数
};

/**
 * 系统信息汇总
 */
struct SystemInfo {
    CPUInfo cpu;
    MemoryInfo memory;
    std::vector<DiskInfo> disks;
    std::vector<NetworkInfo> networks;
    std::vector<ProcessInfo> top_processes;
    double uptime_seconds;        // 系统运行时间 (秒)
    std::string hostname;         // 主机名
    std::string os_name;          // 操作系统名称
    std::string os_version;       // 操作系统版本
    std::string arch;             // 架构 (x86_64, arm64等)
    int64_t timestamp;            // 时间戳
};

// ==================== 监控回调 ====================

using InfoCallback = std::function<void(const SystemInfo&)>;

// ==================== 资源监视器 ====================

class ResourceMonitor {
public:
    ResourceMonitor();
    ~ResourceMonitor();

    /**
     * 初始化监视器
     */
    bool initialize();

    /**
     * 启动监控（定期获取系统信息）
     * @param interval_ms 更新间隔（毫秒）
     */
    bool start(int interval_ms = 1000);

    /**
     * 停止监控
     */
    void stop();

    /**
     * 获取当前系统信息
     */
    SystemInfo getSystemInfo();

    /**
     * 设置数据更新回调
     */
    void setCallback(InfoCallback callback);

    /**
     * 获取CPU信息
     */
    CPUInfo getCPUInfo();

    /**
     * 获取内存信息
     */
    MemoryInfo getMemoryInfo();

    /**
     * 获取磁盘信息列表
     */
    std::vector<DiskInfo> getDiskInfo();

    /**
     * 获取网络信息列表
     */
    std::vector<NetworkInfo> getNetworkInfo();

    /**
     * 获取TOP进程列表
     * @param count 返回进程数量
     */
    std::vector<ProcessInfo> getTopProcesses(int count = 10);

    /**
     * 获取指定进程信息
     */
    ProcessInfo getProcessInfo(int pid);

    /**
     * 杀死进程
     */
    bool killProcess(int pid);

    /**
     * 检查是否正在运行
     */
    bool isRunning() const { return running_; }

private:
    // Windows特定实现
#ifdef _WIN32
    CPUInfo getCPUInfoWindows();
    MemoryInfo getMemoryInfoWindows();
    std::vector<DiskInfo> getDiskInfoWindows();
    std::vector<NetworkInfo> getNetworkInfoWindows();
    std::vector<ProcessInfo> getTopProcessesWindows(int count);
    ProcessInfo getProcessInfoWindows(int pid);
#else
    // Linux特定实现
    CPUInfo getCPUInfoLinux();
    MemoryInfo getMemoryInfoLinux();
    std::vector<DiskInfo> getDiskInfoLinux();
    std::vector<NetworkInfo> getNetworkInfoLinux();
    std::vector<ProcessInfo> getTopProcessesLinux(int count);
    ProcessInfo getProcessInfoLinux(int pid);
#endif

    // 辅助函数
    void updateSystemInfo();
    std::string readFile(const std::string& path);
    std::vector<std::string> splitString(const std::string& str, char delimiter);

private:
    std::atomic<bool> running_;
    std::thread monitor_thread_;
    InfoCallback callback_;

    // 上次数据（用于计算速度）
    CPUInfo last_cpu_info_;
    std::vector<DiskInfo> last_disk_info_;
    std::vector<NetworkInfo> last_network_info_;

    // 互斥锁
    mutable std::mutex mutex_;
};

// ==================== 警告配置 ====================

struct AlertRule {
    std::string name;
    enum Type { CPU, MEMORY, DISK, NETWORK } type;
    double threshold;              // 阈值（百分比）
    std::function<void()> callback; // 触发回调
    bool enabled;
};

class AlertManager {
public:
    AlertManager(ResourceMonitor& monitor);
    ~AlertManager();

    void addAlert(const AlertRule& rule);
    void removeAlert(const std::string& name);
    void start();
    void stop();

private:
    void checkAlerts();
    std::vector<AlertRule> rules_;
    ResourceMonitor& monitor_;
    std::atomic<bool> running_;
    std::thread alert_thread_;
};

} // namespace monitor
