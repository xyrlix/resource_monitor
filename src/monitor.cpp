#include "monitor.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cmath>
#include <vector>
#include <dirent.h>
#include <unistd.h>

#ifdef _WIN32
    #include <psapi.h>
    #include <pdh.h>
    #include <pdhmsg.h>
    #pragma comment(lib, "pdh.lib")
    #include <iphlpapi.h>
    #pragma comment(lib, "iphlpapi.lib")
#endif

namespace monitor {

// ==================== ResourceMonitor 实现 ====================

ResourceMonitor::ResourceMonitor()
    : running_(false) {
}

ResourceMonitor::~ResourceMonitor() {
    stop();
}

bool ResourceMonitor::initialize() {
#ifdef _WIN32
    // Windows初始化
    // 可以在这里初始化PDH计数器
#endif
    return true;
}

bool ResourceMonitor::start(int interval_ms) {
    if (running_) {
        return false;
    }

    running_ = true;
    monitor_thread_ = std::thread([this, interval_ms]() {
        while (running_) {
            updateSystemInfo();
            if (callback_) {
                auto info = getSystemInfo();
                callback_(info);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
        }
    });

    return true;
}

void ResourceMonitor::stop() {
    if (!running_) {
        return;
    }

    running_ = false;
    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }
}

SystemInfo ResourceMonitor::getSystemInfo() {
    std::lock_guard<std::mutex> lock(mutex_);

    SystemInfo info;
    info.cpu = getCPUInfo();
    info.memory = getMemoryInfo();
    info.disks = getDiskInfo();
    info.networks = getNetworkInfo();
    info.top_processes = getTopProcesses(10);
    info.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

#ifdef _WIN32
    // 获取Windows主机名
    char hostname[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(hostname);
    if (GetComputerNameA(hostname, &size)) {
        info.hostname = hostname;
    }
    info.os_name = "Windows";
    info.uptime_seconds = GetTickCount64() / 1000;
#else
    // 获取Linux主机名
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        info.hostname = hostname;
    }
    info.os_name = "Linux";
    
    // 获取运行时间
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        info.uptime_seconds = si.uptime;
    }
#endif

    return info;
}

void ResourceMonitor::setCallback(InfoCallback callback) {
    callback_ = callback;
}

CPUInfo ResourceMonitor::getCPUInfo() {
    std::lock_guard<std::mutex> lock(mutex_);
    
#ifdef _WIN32
    return getCPUInfoWindows();
#else
    return getCPUInfoLinux();
#endif
}

MemoryInfo ResourceMonitor::getMemoryInfo() {
    std::lock_guard<std::mutex> lock(mutex_);
    
#ifdef _WIN32
    return getMemoryInfoWindows();
#else
    return getMemoryInfoLinux();
#endif
}

std::vector<DiskInfo> ResourceMonitor::getDiskInfo() {
    std::lock_guard<std::mutex> lock(mutex_);
    
#ifdef _WIN32
    return getDiskInfoWindows();
#else
    return getDiskInfoLinux();
#endif
}

std::vector<NetworkInfo> ResourceMonitor::getNetworkInfo() {
    std::lock_guard<std::mutex> lock(mutex_);
    
#ifdef _WIN32
    return getNetworkInfoWindows();
#else
    return getNetworkInfoLinux();
#endif
}

std::vector<ProcessInfo> ResourceMonitor::getTopProcesses(int count) {
    std::lock_guard<std::mutex> lock(mutex_);
    
#ifdef _WIN32
    return getTopProcessesWindows(count);
#else
    return getTopProcessesLinux(count);
#endif
}

ProcessInfo ResourceMonitor::getProcessInfo(int pid) {
#ifdef _WIN32
    return getProcessInfoWindows(pid);
#else
    return getProcessInfoLinux(pid);
#endif
}

bool ResourceMonitor::killProcess(int pid) {
#ifdef _WIN32
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProcess == NULL) {
        return false;
    }
    bool result = TerminateProcess(hProcess, 0);
    CloseHandle(hProcess);
    return result;
#else
    return kill(pid, SIGTERM) == 0;
#endif
}

void ResourceMonitor::updateSystemInfo() {
    // 更新系统信息
    auto info = getSystemInfo();
    
    // 更新上次数据
    std::lock_guard<std::mutex> lock(mutex_);
    last_cpu_info_ = info.cpu;
    last_disk_info_ = info.disks;
    last_network_info_ = info.networks;
}

// ==================== Windows 实现 ====================

#ifdef _WIN32

CPUInfo ResourceMonitor::getCPUInfoWindows() {
    CPUInfo info;

    // 获取CPU使用率
    static ULARGE_INTEGER last_cpu_time = {0};
    static ULARGE_INTEGER last_idle_time = {0};
    static FILETIME last_system_time = {0};

    ULARGE_INTEGER now_cpu_time, now_idle_time;
    FILETIME idle_time, kernel_time, user_time;

    if (GetSystemTimes(&idle_time, &kernel_time, &user_time)) {
        now_cpu_time.LowPart = kernel_time.dwLowDateTime + user_time.dwLowDateTime;
        now_cpu_time.HighPart = kernel_time.dwHighDateTime + user_time.dwHighDateTime;
        now_idle_time.LowPart = idle_time.dwLowDateTime;
        now_idle_time.HighPart = idle_time.dwHighDateTime;

        if (last_cpu_time.QuadPart != 0) {
            ULONGLONG cpu_diff = now_cpu_time.QuadPart - last_cpu_time.QuadPart;
            ULONGLONG idle_diff = now_idle_time.QuadPart - last_idle_time.QuadPart;

            if (cpu_diff > 0) {
                info.usage_percent = 100.0 - (100.0 * idle_diff / cpu_diff);
                info.user_percent = 100.0 * (user_time.dwLowDateTime - last_system_time.dwLowDateTime) / cpu_diff;
                info.system_percent = 100.0 * (kernel_time.dwLowDateTime - last_system_time.dwLowDateTime) / cpu_diff;
                info.idle_percent = 100.0 * idle_diff / cpu_diff;
            }
        }

        last_cpu_time = now_cpu_time;
        last_idle_time = now_idle_time;
        last_system_time = kernel_time;
    }

    // 获取CPU核心数
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    info.core_count = si.dwNumberOfProcessors;

    // 获取CPU信息
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, 
        "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 
        0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        
        char buffer[256];
        DWORD size = sizeof(buffer);
        if (RegQueryValueExA(hKey, "ProcessorNameString", NULL, NULL, 
            (LPBYTE)buffer, &size) == ERROR_SUCCESS) {
            info.model_name = buffer;
        }
        
        RegCloseKey(hKey);
    }

    return info;
}

MemoryInfo ResourceMonitor::getMemoryInfoWindows() {
    MemoryInfo info;

    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    GlobalMemoryStatusEx(&ms);

    info.total_mb = ms.ullTotalPhys / (1024 * 1024);
    info.used_mb = (ms.ullTotalPhys - ms.ullAvailPhys) / (1024 * 1024);
    info.free_mb = ms.ullAvailPhys / (1024 * 1024);
    info.usage_percent = ms.dwMemoryLoad;

    info.swap_total_mb = ms.ullTotalPageFile / (1024 * 1024);
    info.swap_used_mb = (ms.ullTotalPageFile - ms.ullAvailPageFile) / (1024 * 1024);
    info.swap_usage_percent = (ms.ullTotalPageFile > 0) ? 
        (100.0 * info.swap_used_mb / info.swap_total_mb) : 0;

    return info;
}

std::vector<DiskInfo> ResourceMonitor::getDiskInfoWindows() {
    std::vector<DiskInfo> disks;

    // 获取所有逻辑驱动器
    DWORD drives = GetLogicalDrives();
    char drive[] = "A:\\";

    for (int i = 0; i < 26; i++) {
        if (drives & (1 << i)) {
            drive[0] = 'A' + i;

            UINT type = GetDriveTypeA(drive);
            if (type == DRIVE_FIXED || type == DRIVE_REMOVABLE) {
                DiskInfo info;
                info.mount_point = drive;

                ULARGE_INTEGER free_bytes, total_bytes, total_free;
                if (GetDiskFreeSpaceExA(drive, &free_bytes, &total_bytes, &total_free)) {
                    info.total_gb = total_bytes.QuadPart / (1024.0 * 1024 * 1024);
                    info.used_gb = (total_bytes.QuadPart - total_free.QuadPart) / (1024.0 * 1024 * 1024);
                    info.free_gb = total_free.QuadPart / (1024.0 * 1024 * 1024);
                    info.usage_percent = (total_bytes.QuadPart > 0) ? 
                        (100.0 * info.used_gb / info.total_gb) : 0;
                }

                char fs_name[256];
                if (GetVolumeInformationA(drive, NULL, 0, NULL, NULL, NULL, 
                    fs_name, sizeof(fs_name))) {
                    info.file_system = fs_name;
                }

                disks.push_back(info);
            }
        }
    }

    return disks;
}

std::vector<NetworkInfo> ResourceMonitor::getNetworkInfoWindows() {
    std::vector<NetworkInfo> networks;

    PMIB_IFTABLE2 if_table;
    ULONG size = 0;

    if (GetIfTable2Ex(MibIfTableRaw, &if_table) == NO_ERROR) {
        for (ULONG i = 0; i < if_table->NumEntries; i++) {
            PMIB_IF_ROW2 row = &if_table->Table[i];

            // 跳过回环和未连接的接口
            if (row->Type == IF_TYPE_SOFTWARE_LOOPBACK || row->OperStatus != IfOperStatusUp) {
                continue;
            }

            NetworkInfo info;
            char if_name[256];
            WideCharToMultiByte(CP_UTF8, 0, row->Description, -1, 
                if_name, sizeof(if_name), NULL, NULL);
            info.interface = if_name;
            
            info.bytes_received = row->InOctets;
            info.bytes_sent = row->OutOctets;
            info.packets_received = row->InUcastPkts;
            info.packets_sent = row->OutUcastPkts;

            networks.push_back(info);
        }

        FreeMibTable(if_table);
    }

    return networks;
}

std::vector<ProcessInfo> ResourceMonitor::getTopProcessesWindows(int count) {
    std::vector<ProcessInfo> processes;
    
    DWORD aProcesses[1024], cbNeeded;
    
    if (EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded)) {
        int cProcesses = cbNeeded / sizeof(DWORD);
        
        std::vector<std::pair<double, ProcessInfo>> temp_processes;
        
        for (int i = 0; i < cProcesses; i++) {
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 
                FALSE, aProcesses[i]);
            if (hProcess != NULL) {
                ProcessInfo info = getProcessInfoWindows(aProcesses[i]);
                temp_processes.push_back({info.cpu_percent, info});
                CloseHandle(hProcess);
            }
        }
        
        // 按CPU使用率排序
        std::sort(temp_processes.begin(), temp_processes.end(), 
            [](const auto& a, const auto& b) {
                return a.first > b.first;
            });
        
        // 取前N个
        for (int i = 0; i < std::min((int)temp_processes.size(), count); i++) {
            processes.push_back(temp_processes[i].second);
        }
    }
    
    return processes;
}

ProcessInfo ResourceMonitor::getProcessInfoWindows(int pid) {
    ProcessInfo info;
    info.pid = pid;
    
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProcess == NULL) {
        return info;
    }
    
    // 获取进程名称
    char name[MAX_PATH];
    if (GetModuleBaseNameA(hProcess, NULL, name, sizeof(name))) {
        info.name = name;
    }
    
    // 获取内存使用
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
        info.memory_mb = pmc.WorkingSetSize / (1024 * 1024);
    }
    
    // 获取创建时间
    FILETIME creation_time, exit_time, kernel_time, user_time;
    if (GetProcessTimes(hProcess, &creation_time, &exit_time, &kernel_time, &user_time)) {
        ULARGE_INTEGER uli;
        uli.LowPart = creation_time.dwLowDateTime;
        uli.HighPart = creation_time.dwHighDateTime;
        info.uptime_seconds = (GetTickCount64() - (uli.QuadPart / 10000)) / 1000;
    }
    
    CloseHandle(hProcess);
    return info;
}

#endif // _WIN32

// ==================== Linux 实现 ====================

#ifndef _WIN32

CPUInfo ResourceMonitor::getCPUInfoLinux() {
    CPUInfo info;

    // 读取/proc/stat
    std::string stat = readFile("/proc/stat");
    if (stat.empty()) {
        return info;
    }

    // 解析第一行 (总CPU)
    std::istringstream iss(stat);
    std::string cpu;
    long user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice;
    
    iss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal >> guest >> guest_nice;
    
    long total = user + nice + system + idle + iowait + irq + softirq + steal;
    long idle_total = idle + iowait;
    
    static long last_total = 0;
    static long last_idle = 0;
    
    if (last_total > 0) {
        long total_diff = total - last_total;
        long idle_diff = idle_total - last_idle;
        
        if (total_diff > 0) {
            info.usage_percent = 100.0 * (1.0 - (double)idle_diff / total_diff);
            info.user_percent = 100.0 * (double)(user - last_total) / total_diff;
            info.system_percent = 100.0 * (double)(system - last_total) / total_diff;
            info.idle_percent = 100.0 * (double)idle_diff / total_diff;
        }
    }
    
    last_total = total;
    last_idle = idle_total;
    
    // 解析每核CPU
    std::istringstream iss2(stat);
    std::string line;
    int core = 0;
    while (std::getline(iss2, line)) {
        if (line.substr(0, 4) == "cpu ") continue;
        if (line.substr(0, 3) != "cpu") break;
        
        std::istringstream line_iss(line);
        line_iss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq;
        
        long c_total = user + nice + system + idle + iowait + irq + softirq;
        long c_idle = idle + iowait;
        
        if (c_total > 0) {
            info.per_core_usage.push_back(100.0 * (1.0 - (double)c_idle / c_total));
        }
        
        core++;
    }
    
    info.core_count = core;
    
    // 读取/proc/cpuinfo获取CPU型号
    std::string cpuinfo = readFile("/proc/cpuinfo");
    size_t pos = cpuinfo.find("model name");
    if (pos != std::string::npos) {
        pos = cpuinfo.find(':', pos);
        if (pos != std::string::npos) {
            pos = cpuinfo.find_first_not_of(" \t", pos + 1);
            size_t end = cpuinfo.find('\n', pos);
            info.model_name = cpuinfo.substr(pos, end - pos);
        }
    }
    
    return info;
}

MemoryInfo ResourceMonitor::getMemoryInfoLinux() {
    MemoryInfo info;
    
    std::string meminfo = readFile("/proc/meminfo");
    if (meminfo.empty()) {
        return info;
    }
    
    std::istringstream iss(meminfo);
    std::string line;
    
    while (std::getline(iss, line)) {
        std::istringstream line_iss(line);
        std::string key;
        long value;
        
        line_iss >> key >> value;
        
        if (key == "MemTotal:") {
            info.total_mb = value / 1024;
        } else if (key == "MemAvailable:") {
            info.free_mb = value / 1024;
        } else if (key == "MemFree:") {
            info.free_mb = value / 1024;
        } else if (key == "Cached:") {
            info.cached_mb = value / 1024;
        } else if (key == "SwapTotal:") {
            info.swap_total_mb = value / 1024;
        } else if (key == "SwapFree:") {
            info.swap_used_mb = (info.swap_total_mb - value / 1024);
        }
    }
    
    info.used_mb = info.total_mb - info.free_mb;
    info.usage_percent = (info.total_mb > 0) ? (100.0 * info.used_mb / info.total_mb) : 0;
    info.swap_usage_percent = (info.swap_total_mb > 0) ? 
        (100.0 * info.swap_used_mb / info.swap_total_mb) : 0;
    
    return info;
}

std::vector<DiskInfo> ResourceMonitor::getDiskInfoLinux() {
    std::vector<DiskInfo> disks;
    
    // 读取/proc/mounts
    std::string mounts = readFile("/proc/mounts");
    if (mounts.empty()) {
        return disks;
    }
    
    std::istringstream iss(mounts);
    std::string line;
    
    while (std::getline(iss, line)) {
        std::istringstream line_iss(line);
        std::string device, mount_point, type;
        
        line_iss >> device >> mount_point >> type;
        
        // 跳过特殊文件系统
        if (type != "ext4" && type != "xfs" && type != "btrfs" && type != "ntfs") {
            continue;
        }
        
        DiskInfo info;
        info.mount_point = mount_point;
        info.file_system = type;
        
        struct statvfs stat;
        if (statvfs(mount_point.c_str(), &stat) == 0) {
            unsigned long long total = stat.f_blocks * stat.f_frsize;
            unsigned long long free = stat.f_bavail * stat.f_frsize;
            unsigned long long used = total - free;
            
            info.total_gb = total / (1024.0 * 1024 * 1024);
            info.used_gb = used / (1024.0 * 1024 * 1024);
            info.free_gb = free / (1024.0 * 1024 * 1024);
            info.usage_percent = (total > 0) ? (100.0 * used / total) : 0;
        }
        
        // 读取磁盘IO统计
        std::string diskstat = readFile("/proc/diskstats");
        // 解析逻辑...
        
        disks.push_back(info);
    }
    
    return disks;
}

std::vector<NetworkInfo> ResourceMonitor::getNetworkInfoLinux() {
    std::vector<NetworkInfo> networks;
    
    std::string net_dev = readFile("/proc/net/dev");
    if (net_dev.empty()) {
        return networks;
    }
    
    std::istringstream iss(net_dev);
    std::string line;
    
    // 跳过前两行标题
    std::getline(iss, line);
    std::getline(iss, line);
    
    while (std::getline(iss, line)) {
        std::istringstream line_iss(line);
        std::string interface;
        long rx_bytes, rx_packets, tx_bytes, tx_packets;
        
        // 跳过冒号
        size_t pos = line.find(':');
        if (pos == std::string::npos) continue;
        
        interface = line.substr(0, pos);
        
        // 移除空格
        interface.erase(std::remove_if(interface.begin(), interface.end(), ::isspace), interface.end());
        
        // 跳过lo
        if (interface == "lo") continue;
        
        // 解析数据
        line_iss.seekg(pos + 1);
        line_iss >> rx_bytes >> rx_packets;
        line_iss.ignore(8); // 跳过一些字段
        line_iss >> tx_bytes >> tx_packets;
        
        NetworkInfo info;
        info.interface = interface;
        info.bytes_received = rx_bytes;
        info.bytes_sent = tx_bytes;
        info.packets_received = rx_packets;
        info.packets_sent = tx_packets;
        
        networks.push_back(info);
    }
    
    return networks;
}

std::vector<ProcessInfo> ResourceMonitor::getTopProcessesLinux(int count) {
    std::vector<ProcessInfo> processes;
    
    // 遍历/proc目录
    DIR* dir = opendir("/proc");
    if (dir == NULL) {
        return processes;
    }
    
    struct dirent* entry;
    std::vector<std::pair<double, ProcessInfo>> temp_processes;
    
    while ((entry = readdir(dir)) != NULL) {
        // 检查是否是数字（PID）
        if (!isdigit(entry->d_name[0])) continue;
        
        int pid = atoi(entry->d_name);
        ProcessInfo info = getProcessInfoLinux(pid);
        
        if (!info.name.empty()) {
            temp_processes.push_back({info.cpu_percent, info});
        }
    }
    
    closedir(dir);
    
    // 按CPU使用率排序
    std::sort(temp_processes.begin(), temp_processes.end(), 
        [](const auto& a, const auto& b) {
            return a.first > b.first;
        });
    
    // 取前N个
    for (int i = 0; i < std::min((int)temp_processes.size(), count); i++) {
        processes.push_back(temp_processes[i].second);
    }
    
    return processes;
}

ProcessInfo ResourceMonitor::getProcessInfoLinux(int pid) {
    ProcessInfo info;
    info.pid = pid;
    
    // 读取/proc/[pid]/stat
    std::string stat_path = "/proc/" + std::to_string(pid) + "/stat";
    std::string stat = readFile(stat_path);
    
    if (!stat.empty()) {
        std::istringstream iss(stat);
        std::string temp;
        
        iss >> info.pid >> temp; // pid和name
        
        // 提取名称（去掉括号）
        size_t start = temp.find('(');
        size_t end = temp.find(')');
        if (start != std::string::npos && end != std::string::npos) {
            info.name = temp.substr(start + 1, end - start - 1);
        }
        
        // �过一些字段
        for (int i = 0; i < 11; i++) iss >> temp;
        
        // utime和stime
        long utime, stime;
        iss >> utime >> stime;
        
        // 获取系统总CPU时间
        static long last_total_cpu = 0;
        static long last_process_cpu = 0;
        
        std::string cpu_stat = readFile("/proc/stat");
        std::istringstream cpu_iss(cpu_stat);
        cpu_iss >> temp;
        long user, nice, system, idle;
        cpu_iss >> user >> nice >> system >> idle;
        long total_cpu = user + nice + system + idle;
        
        if (last_total_cpu > 0) {
            long total_diff = total_cpu - last_total_cpu;
            long process_diff = (utime + stime) - last_process_cpu;
            
            if (total_diff > 0) {
                info.cpu_percent = 100.0 * process_diff / total_diff;
            }
        }
        
        last_total_cpu = total_cpu;
        last_process_cpu = utime + stime;
        
        // 状态
        char state;
        iss >> state;
        info.state = state;
    }
    
    // 读取/proc/[pid]/statm获取内存
    std::string statm_path = "/proc/" + std::to_string(pid) + "/statm";
    std::string statm = readFile(statm_path);
    
    if (!statm.empty()) {
        std::istringstream iss(statm);
        long resident;
        iss >> resident; // 跳过size
        iss >> resident;
        info.memory_mb = (resident * getpagesize()) / (1024 * 1024);
    }
    
    // 获取用户
    struct stat st;
    if (stat(stat_path.c_str(), &st) == 0) {
        struct passwd* pw = getpwuid(st.st_uid);
        if (pw) {
            info.user = pw->pw_name;
        }
    }
    
    return info;
}

#endif // !_WIN32

// ==================== 辅助函数 ====================

std::string ResourceMonitor::readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::vector<std::string> ResourceMonitor::splitString(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    
    return tokens;
}

// ==================== AlertManager 实现 ====================

AlertManager::AlertManager(ResourceMonitor& monitor)
    : monitor_(monitor), running_(false) {
}

AlertManager::~AlertManager() {
    stop();
}

void AlertManager::addAlert(const AlertRule& rule) {
    rules_.push_back(rule);
}

void AlertManager::removeAlert(const std::string& name) {
    rules_.erase(
        std::remove_if(rules_.begin(), rules_.end(),
            [&name](const AlertRule& r) { return r.name == name; }),
        rules_.end());
}

void AlertManager::start() {
    if (running_) return;
    
    running_ = true;
    alert_thread_ = std::thread([this]() {
        while (running_) {
            checkAlerts();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });
}

void AlertManager::stop() {
    if (!running_) return;
    
    running_ = false;
    if (alert_thread_.joinable()) {
        alert_thread_.join();
    }
}

void AlertManager::checkAlerts() {
    auto info = monitor_.getSystemInfo();
    
    for (auto& rule : rules_) {
        if (!rule.enabled) continue;
        
        bool triggered = false;
        
        switch (rule.type) {
            case AlertRule::CPU:
                triggered = info.cpu.usage_percent >= rule.threshold;
                break;
            case AlertRule::MEMORY:
                triggered = info.memory.usage_percent >= rule.threshold;
                break;
            case AlertRule::DISK:
                for (const auto& disk : info.disks) {
                    if (disk.usage_percent >= rule.threshold) {
                        triggered = true;
                        break;
                    }
                }
                break;
            case AlertRule::NETWORK:
                // 网络告警逻辑
                break;
        }
        
        if (triggered && rule.callback) {
            rule.callback();
        }
    }
}

} // namespace monitor
