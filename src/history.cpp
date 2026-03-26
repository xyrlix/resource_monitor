#include "history.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <mutex>

namespace monitor {

//==============================================================================
// MemoryHistoryStorage 实现
//==============================================================================

MemoryHistoryStorage::MemoryHistoryStorage(size_t max_points)
    : max_points_(max_points) {
}

bool MemoryHistoryStorage::save(const HistoryPoint& point) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    history_.push_back(point);
    
    // 如果超过最大数量，删除最旧的
    if (history_.size() > max_points_) {
        history_.erase(history_.begin());
    }
    
    return true;
}

std::vector<HistoryPoint> MemoryHistoryStorage::query(
    const std::chrono::system_clock::time_point& start_time,
    const std::chrono::system_clock::time_point& end_time
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<HistoryPoint> result;
    
    for (const auto& point : history_) {
        if (point.timestamp >= start_time && point.timestamp <= end_time) {
            result.push_back(point);
        }
    }
    
    return result;
}

std::vector<HistoryPoint> MemoryHistoryStorage::queryRecent(size_t count) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (count >= history_.size()) {
        return history_;
    }
    
    return std::vector<HistoryPoint>(
        history_.end() - count,
        history_.end()
    );
}

bool MemoryHistoryStorage::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    history_.clear();
    return true;
}

//==============================================================================
// HistoryAnalyzer 实现
//==============================================================================

HistoryAnalyzer::HistoryAnalyzer(const std::vector<HistoryPoint>& data)
    : data_(data) {
}

HistoryAnalyzer::Statistics HistoryAnalyzer::calculateCpuStatistics() const {
    if (data_.empty()) {
        return {0, 0, 0, 0, 0};
    }
    
    std::vector<double> cpu_values;
    cpu_values.reserve(data_.size());
    
    for (const auto& point : data_) {
        cpu_values.push_back(point.cpu_total);
    }
    
    double min = *std::min_element(cpu_values.begin(), cpu_values.end());
    double max = *std::max_element(cpu_values.begin(), cpu_values.end());
    double avg = calculateAverage(cpu_values);
    double median = calculateMedian(cpu_values);
    double std_dev = calculateStdDev(cpu_values, avg);
    
    return {min, max, avg, median, std_dev};
}

HistoryAnalyzer::Statistics HistoryAnalyzer::calculateMemoryStatistics() const {
    if (data_.empty()) {
        return {0, 0, 0, 0, 0};
    }
    
    std::vector<double> mem_values;
    mem_values.reserve(data_.size());
    
    for (const auto& point : data_) {
        double usage = (point.memory_used / point.memory_total) * 100.0;
        mem_values.push_back(usage);
    }
    
    double min = *std::min_element(mem_values.begin(), mem_values.end());
    double max = *std::max_element(mem_values.begin(), mem_values.end());
    double avg = calculateAverage(mem_values);
    double median = calculateMedian(mem_values);
    double std_dev = calculateStdDev(mem_values, avg);
    
    return {min, max, avg, median, std_dev};
}

HistoryAnalyzer::Statistics HistoryAnalyzer::calculateDiskStatistics(
    const std::string& mount_point
) const {
    if (data_.empty()) {
        return {0, 0, 0, 0, 0};
    }
    
    std::vector<double> usage_values;
    
    for (const auto& point : data_) {
        for (const auto& disk : point.disks) {
            if (disk.mount_point == mount_point) {
                double usage = (disk.used / disk.total) * 100.0;
                usage_values.push_back(usage);
                break;
            }
        }
    }
    
    if (usage_values.empty()) {
        return {0, 0, 0, 0, 0};
    }
    
    double min = *std::min_element(usage_values.begin(), usage_values.end());
    double max = *std::max_element(usage_values.begin(), usage_values.end());
    double avg = calculateAverage(usage_values);
    double median = calculateMedian(usage_values);
    double std_dev = calculateStdDev(usage_values, avg);
    
    return {min, max, avg, median, std_dev};
}

HistoryAnalyzer::Statistics HistoryAnalyzer::calculateNetworkStatistics(
    const std::string& interface
) const {
    if (data_.empty()) {
        return {0, 0, 0, 0, 0};
    }
    
    std::vector<double> rx_speeds;
    std::vector<double> tx_speeds;
    
    for (const auto& point : data_) {
        for (const auto& net : point.networks) {
            if (net.interface == interface) {
                rx_speeds.push_back(net.rx_speed);
                tx_speeds.push_back(net.tx_speed);
                break;
            }
        }
    }
    
    if (rx_speeds.empty() || tx_speeds.empty()) {
        return {0, 0, 0, 0, 0};
    }
    
    // 合并上传和下载速度
    std::vector<double> all_speeds;
    all_speeds.reserve(rx_speeds.size() + tx_speeds.size());
    all_speeds.insert(all_speeds.end(), rx_speeds.begin(), rx_speeds.end());
    all_speeds.insert(all_speeds.end(), tx_speeds.begin(), tx_speeds.end());
    
    double min = *std::min_element(all_speeds.begin(), all_speeds.end());
    double max = *std::max_element(all_speeds.begin(), all_speeds.end());
    double avg = calculateAverage(all_speeds);
    double median = calculateMedian(all_speeds);
    double std_dev = calculateStdDev(all_speeds, avg);
    
    return {min, max, avg, median, std_dev};
}

HistoryAnalyzer::Trend HistoryAnalyzer::calculateCpuTrend(int minutes) const {
    auto values = getCpuValues(minutes);
    if (values.size() < 3) {
        return Trend::STABLE;
    }
    
    // 简单线性回归
    double sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;
    size_t n = values.size();
    
    for (size_t i = 0; i < n; ++i) {
        sum_x += i;
        sum_y += values[i];
        sum_xy += i * values[i];
        sum_x2 += i * i;
    }
    
    double slope = (n * sum_xy - sum_x * sum_y) / (n * sum_x2 - sum_x * sum_x);
    
    if (slope > 0.5) {
        return Trend::RISING;
    } else if (slope < -0.5) {
        return Trend::FALLING;
    }
    
    return Trend::STABLE;
}

HistoryAnalyzer::Trend HistoryAnalyzer::calculateMemoryTrend(int minutes) const {
    auto values = getMemoryValues(minutes);
    if (values.size() < 3) {
        return Trend::STABLE;
    }
    
    double sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;
    size_t n = values.size();
    
    for (size_t i = 0; i < n; ++i) {
        sum_x += i;
        sum_y += values[i];
        sum_xy += i * values[i];
        sum_x2 += i * i;
    }
    
    double slope = (n * sum_xy - sum_x * sum_y) / (n * sum_x2 - sum_x * sum_x);
    
    if (slope > 0.5) {
        return Trend::RISING;
    } else if (slope < -0.5) {
        return Trend::FALLING;
    }
    
    return Trend::STABLE;
}

HistoryAnalyzer::Prediction HistoryAnalyzer::predictCpu(int minutes_ahead) const {
    auto values = getCpuValues(10); // 使用最近10分钟的数据预测
    
    if (values.size() < 3) {
        return {0, 0, "Not enough data"};
    }
    
    // 简单线性回归预测
    double sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;
    size_t n = values.size();
    
    for (size_t i = 0; i < n; ++i) {
        sum_x += i;
        sum_y += values[i];
        sum_xy += i * values[i];
        sum_x2 += i * i;
    }
    
    double slope = (n * sum_xy - sum_x * sum_y) / (n * sum_x2 - sum_x * sum_x);
    double intercept = (sum_y - slope * sum_x) / n;
    
    double predicted = intercept + slope * (n + minutes_ahead - 1);
    
    // 计算置信度（基于斜率和数据稳定性）
    auto stats = calculateCpuStatistics();
    double confidence = std::max(0.0, 1.0 - stats.std_dev / 20.0);
    
    std::string explanation;
    if (std::abs(slope) < 0.1) {
        explanation = "CPU usage is stable";
    } else if (slope > 0) {
        explanation = "CPU usage is trending upward";
    } else {
        explanation = "CPU usage is trending downward";
    }
    
    return {predicted, confidence, explanation};
}

HistoryAnalyzer::Prediction HistoryAnalyzer::predictMemory(int minutes_ahead) const {
    auto values = getMemoryValues(10);
    
    if (values.size() < 3) {
        return {0, 0, "Not enough data"};
    }
    
    double sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;
    size_t n = values.size();
    
    for (size_t i = 0; i < n; ++i) {
        sum_x += i;
        sum_y += values[i];
        sum_xy += i * values[i];
        sum_x2 += i * i;
    }
    
    double slope = (n * sum_xy - sum_x * sum_y) / (n * sum_x2 - sum_x * sum_x);
    double intercept = (sum_y - slope * sum_x) / n;
    
    double predicted = intercept + slope * (n + minutes_ahead - 1);
    
    auto stats = calculateMemoryStatistics();
    double confidence = std::max(0.0, 1.0 - stats.std_dev / 20.0);
    
    std::string explanation;
    if (std::abs(slope) < 0.1) {
        explanation = "Memory usage is stable";
    } else if (slope > 0) {
        explanation = "Memory usage is trending upward";
    } else {
        explanation = "Memory usage is trending downward";
    }
    
    return {predicted, confidence, explanation};
}

std::vector<HistoryAnalyzer::Anomaly> HistoryAnalyzer::detectAnomalies(
    double threshold
) const {
    std::vector<Anomaly> anomalies;
    
    if (data_.size() < 10) {
        return anomalies;
    }
    
    // 检测CPU异常
    std::vector<double> cpu_values;
    for (const auto& point : data_) {
        cpu_values.push_back(point.cpu_total);
    }
    
    double cpu_avg = calculateAverage(cpu_values);
    double cpu_std = calculateStdDev(cpu_values, cpu_avg);
    
    for (const auto& point : data_) {
        double deviation = std::abs(point.cpu_total - cpu_avg) / cpu_std;
        
        if (deviation > threshold) {
            Anomaly anomaly;
            anomaly.timestamp = point.timestamp;
            anomaly.value = point.cpu_total;
            anomaly.deviation = deviation;
            anomaly.description = "CPU usage anomaly: " + 
                                 std::to_string(point.cpu_total) + "%";
            anomalies.push_back(anomaly);
        }
    }
    
    // 检测内存异常
    std::vector<double> mem_values;
    for (const auto& point : data_) {
        mem_values.push_back(point.memory_used / point.memory_total * 100.0);
    }
    
    double mem_avg = calculateAverage(mem_values);
    double mem_std = calculateStdDev(mem_values, mem_avg);
    
    for (const auto& point : data_) {
        double usage = point.memory_used / point.memory_total * 100.0;
        double deviation = std::abs(usage - mem_avg) / mem_std;
        
        if (deviation > threshold) {
            Anomaly anomaly;
            anomaly.timestamp = point.timestamp;
            anomaly.value = usage;
            anomaly.deviation = deviation;
            anomaly.description = "Memory usage anomaly: " + 
                                 std::to_string(usage) + "%";
            anomalies.push_back(anomaly);
        }
    }
    
    // 按时间排序
    std::sort(anomalies.begin(), anomalies.end(),
        [](const Anomaly& a, const Anomaly& b) {
            return a.timestamp < b.timestamp;
        });
    
    return anomalies;
}

double HistoryAnalyzer::calculateAverage(const std::vector<double>& values) const {
    if (values.empty()) {
        return 0.0;
    }
    return std::accumulate(values.begin(), values.end(), 0.0) / values.size();
}

double HistoryAnalyzer::calculateStdDev(
    const std::vector<double>& values, double avg
) const {
    if (values.size() < 2) {
        return 0.0;
    }
    
    double sum_sq = 0.0;
    for (double val : values) {
        double diff = val - avg;
        sum_sq += diff * diff;
    }
    
    return std::sqrt(sum_sq / (values.size() - 1));
}

double HistoryAnalyzer::calculateMedian(std::vector<double> values) const {
    if (values.empty()) {
        return 0.0;
    }
    
    std::sort(values.begin(), values.end());
    size_t n = values.size();
    
    if (n % 2 == 0) {
        return (values[n/2 - 1] + values[n/2]) / 2.0;
    } else {
        return values[n/2];
    }
}

std::vector<double> HistoryAnalyzer::getCpuValues(int minutes) const {
    std::vector<double> values;
    
    auto now = std::chrono::system_clock::now();
    auto start = now - std::chrono::minutes(minutes);
    
    for (const auto& point : data_) {
        if (point.timestamp >= start && point.timestamp <= now) {
            values.push_back(point.cpu_total);
        }
    }
    
    return values;
}

std::vector<double> HistoryAnalyzer::getMemoryValues(int minutes) const {
    std::vector<double> values;
    
    auto now = std::chrono::system_clock::now();
    auto start = now - std::chrono::minutes(minutes);
    
    for (const auto& point : data_) {
        if (point.timestamp >= start && point.timestamp <= now) {
            values.push_back(point.memory_used / point.memory_total * 100.0);
        }
    }
    
    return values;
}

//==============================================================================
// HistoryExporter 实现
//==============================================================================

bool HistoryExporter::exportToCSV(
    const std::vector<HistoryPoint>& data,
    const std::string& filename
) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    // CSV Header
    file << "timestamp,cpu_total,cpu_user,cpu_system,";
    file << "memory_total,memory_used,memory_available,";
    file << "disk_used_pct,net_rx_speed,net_tx_speed\n";
    
    // Data rows
    for (const auto& point : data) {
        // Convert timestamp to seconds
        auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            point.timestamp.time_since_epoch()
        ).count();
        
        file << timestamp << ",";
        file << point.cpu_total << ",";
        file << point.cpu_user << ",";
        file << point.cpu_system << ",";
        file << point.memory_total << ",";
        file << point.memory_used << ",";
        file << point.memory_available << ",";
        
        // First disk usage percentage
        if (!point.disks.empty()) {
            double usage = (point.disks[0].used / point.disks[0].total) * 100.0;
            file << usage;
        }
        file << ",";
        
        // First network interface
        if (!point.networks.empty()) {
            file << point.networks[0].rx_speed << ",";
            file << point.networks[0].tx_speed;
        }
        
        file << "\n";
    }
    
    file.close();
    return true;
}

bool HistoryExporter::exportToJSON(
    const std::vector<HistoryPoint>& data,
    const std::string& filename
) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    file << "[\n";
    
    for (size_t i = 0; i < data.size(); ++i) {
        const auto& point = data[i];
        
        auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            point.timestamp.time_since_epoch()
        ).count();
        
        file << "  {\n";
        file << "    \"timestamp\": " << timestamp << ",\n";
        file << "    \"cpu\": {\n";
        file << "      \"total\": " << point.cpu_total << ",\n";
        file << "      \"user\": " << point.cpu_user << ",\n";
        file << "      \"system\": " << point.cpu_system << "\n";
        file << "    },\n";
        file << "    \"memory\": {\n";
        file << "      \"total\": " << point.memory_total << ",\n";
        file << "      \"used\": " << point.memory_used << ",\n";
        file << "      \"available\": " << point.memory_available << "\n";
        file << "    },\n";
        
        file << "    \"disks\": [\n";
        for (size_t j = 0; j < point.disks.size(); ++j) {
            const auto& disk = point.disks[j];
            file << "      {\n";
            file << "        \"mount_point\": \"" << disk.mount_point << "\",\n";
            file << "        \"total\": " << disk.total << ",\n";
            file << "        \"used\": " << disk.used << ",\n";
            file << "        \"free\": " << disk.free << "\n";
            file << "      }";
            if (j < point.disks.size() - 1) {
                file << ",";
            }
            file << "\n";
        }
        file << "    ],\n";
        
        file << "    \"networks\": [\n";
        for (size_t j = 0; j < point.networks.size(); ++j) {
            const auto& net = point.networks[j];
            file << "      {\n";
            file << "        \"interface\": \"" << net.interface << "\",\n";
            file << "        \"rx_speed\": " << net.rx_speed << ",\n";
            file << "        \"tx_speed\": " << net.tx_speed << "\n";
            file << "      }";
            if (j < point.networks.size() - 1) {
                file << ",";
            }
            file << "\n";
        }
        file << "    ]\n";
        
        file << "  }";
        if (i < data.size() - 1) {
            file << ",";
        }
        file << "\n";
    }
    
    file << "]\n";
    file.close();
    return true;
}

std::string HistoryExporter::exportToChartFormat(
    const std::vector<HistoryPoint>& data,
    const std::string& metric
) {
    std::stringstream ss;
    
    ss << "[";
    
    for (size_t i = 0; i < data.size(); ++i) {
        const auto& point = data[i];
        
        auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            point.timestamp.time_since_epoch()
        ).count();
        
        double value = 0.0;
        
        if (metric == "cpu") {
            value = point.cpu_total;
        } else if (metric == "memory") {
            value = (point.memory_used / point.memory_total) * 100.0;
        } else if (metric == "network") {
            if (!point.networks.empty()) {
                value = point.networks[0].rx_speed + point.networks[0].tx_speed;
            }
        }
        
        ss << "{\"time\":" << timestamp << ",\"value\":" << value << "}";
        
        if (i < data.size() - 1) {
            ss << ",";
        }
    }
    
    ss << "]";
    
    return ss.str();
}

} // namespace monitor
