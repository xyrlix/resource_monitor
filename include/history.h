#ifndef RESOURCE_MONITOR_HISTORY_H
#define RESOURCE_MONITOR_HISTORY_H

#include <string>
#include <vector>
#include <chrono>
#include <memory>

namespace monitor {

/**
 * @brief 系统指标历史数据点
 */
struct HistoryPoint {
    std::chrono::system_clock::time_point timestamp;
    
    // CPU数据
    double cpu_total;
    double cpu_user;
    double cpu_system;
    std::vector<double> cpu_per_core;
    
    // 内存数据
    double memory_total;
    double memory_used;
    double memory_available;
    double memory_cache;
    double memory_swap_used;
    
    // 磁盘数据
    struct DiskPoint {
        std::string mount_point;
        double total;
        double used;
        double free;
        double read_speed;
        double write_speed;
    };
    std::vector<DiskPoint> disks;
    
    // 网络数据
    struct NetworkPoint {
        std::string interface;
        double rx_speed;
        double tx_speed;
        double total_rx;
        double total_tx;
    };
    std::vector<NetworkPoint> networks;
    
    // 进程数据（可选）
    struct ProcessPoint {
        uint32_t pid;
        std::string name;
        double cpu_usage;
        double memory_usage;
    };
    std::vector<ProcessPoint> top_processes;
};

/**
 * @brief 历史数据存储接口
 */
class IHistoryStorage {
public:
    virtual ~IHistoryStorage() = default;
    
    /**
     * @brief 保存历史数据点
     */
    virtual bool save(const HistoryPoint& point) = 0;
    
    /**
     * @brief 查询历史数据
     * @param start_time 开始时间
     * @param end_time 结束时间
     * @return 历史数据点列表
     */
    virtual std::vector<HistoryPoint> query(
        const std::chrono::system_clock::time_point& start_time,
        const std::chrono::system_clock::time_point& end_time
    ) = 0;
    
    /**
     * @brief 查询最近N个数据点
     */
    virtual std::vector<HistoryPoint> queryRecent(size_t count) = 0;
    
    /**
     * @brief 清除历史数据
     */
    virtual bool clear() = 0;
};

/**
 * @brief 内存历史存储
 */
class MemoryHistoryStorage : public IHistoryStorage {
public:
    explicit MemoryHistoryStorage(size_t max_points = 10000);
    
    bool save(const HistoryPoint& point) override;
    std::vector<HistoryPoint> query(
        const std::chrono::system_clock::time_point& start_time,
        const std::chrono::system_clock::time_point& end_time
    ) override;
    std::vector<HistoryPoint> queryRecent(size_t count) override;
    bool clear() override;
    
    size_t size() const { return history_.size(); }
    size_t maxPoints() const { return max_points_; }
    
private:
    std::vector<HistoryPoint> history_;
    size_t max_points_;
    std::mutex mutex_;
};

/**
 * @brief 历史数据分析
 */
class HistoryAnalyzer {
public:
    explicit HistoryAnalyzer(const std::vector<HistoryPoint>& data);
    
    /**
     * @brief 计算平均值
     */
    struct Statistics {
        double min;
        double max;
        double avg;
        double median;
        double std_dev;
    };
    
    Statistics calculateCpuStatistics() const;
    Statistics calculateMemoryStatistics() const;
    Statistics calculateDiskStatistics(const std::string& mount_point) const;
    Statistics calculateNetworkStatistics(const std::string& interface) const;
    
    /**
     * @brief 计算趋势（上升/下降/稳定）
     */
    enum class Trend { RISING, FALLING, STABLE };
    Trend calculateCpuTrend(int minutes = 5) const;
    Trend calculateMemoryTrend(int minutes = 5) const;
    
    /**
     * @brief 预测
     */
    struct Prediction {
        double predicted_value;
        double confidence;
        std::string explanation;
    };
    Prediction predictCpu(int minutes_ahead = 5) const;
    Prediction predictMemory(int minutes_ahead = 5) const;
    
    /**
     * @brief 异常检测
     */
    struct Anomaly {
        std::chrono::system_clock::time_point timestamp;
        double value;
        double deviation;
        std::string description;
    };
    std::vector<Anomaly> detectAnomalies(double threshold = 2.0) const;
    
private:
    const std::vector<HistoryPoint>& data_;
    
    double calculateAverage(const std::vector<double>& values) const;
    double calculateStdDev(const std::vector<double>& values, double avg) const;
    double calculateMedian(std::vector<double> values) const;
    std::vector<double> getCpuValues(int minutes) const;
    std::vector<double> getMemoryValues(int minutes) const;
};

/**
 * @brief 历史数据导出
 */
class HistoryExporter {
public:
    /**
     * @brief 导出为CSV
     */
    static bool exportToCSV(
        const std::vector<HistoryPoint>& data,
        const std::string& filename
    );
    
    /**
     * @brief 导出为JSON
     */
    static bool exportToJSON(
        const std::vector<HistoryPoint>& data,
        const std::string& filename
    );
    
    /**
     * @brief 导出为图表数据（用于前端绘制）
     */
    static std::string exportToChartFormat(
        const std::vector<HistoryPoint>& data,
        const std::string& metric
    );
};

} // namespace monitor

#endif // RESOURCE_MONITOR_HISTORY_H
