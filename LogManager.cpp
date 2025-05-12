#include "LogManager.h"
#include <ctime>
#include <iomanip>

LogManager& LogManager::GetInstance() {
    static LogManager instance;
    return instance;
}

void LogManager::logAction(LogType type, int userId, int deviceId, const std::string& content) {
    std::lock_guard<std::mutex> lock(logMutex_);
    
    // 生成时间戳
    time_t now = time(nullptr);
    tm* tm = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm);
    
    // 添加日志条目
    logs_.push_back({
        type,
        userId,
        deviceId,
        content,
        timestamp
    });
}

void LogManager::saveLogsToFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(logMutex_);
    std::ofstream file(filename, std::ios::trunc);
    
    if (!file.is_open()) {
        throw std::runtime_error("无法打开日志文件: " + filename);
    }
    
    for (const auto& entry : logs_) {
        file << formatLogEntry(entry) << "\n";
    }
}

std::string LogManager::formatLogEntry(const LogEntry& entry) const {
    std::stringstream ss;
    ss << "[" << entry.timestamp << "] "
       << std::left << std::setw(12) << logTypeToString(entry.type)
       << " User:" << std::setw(4) << entry.userId
       << " Device:" << std::setw(4) << entry.deviceId
       << " | " << entry.content;
    return ss.str();
}

std::string LogManager::logTypeToString(LogType type) const {
    switch (type) {
        case LogType::UserAction:   return "USER_ACTION";
        case LogType::DeviceAction: return "DEVICE_ACTION";
        case LogType::System:       return "SYSTEM";
        default:                    return "UNKNOWN";
    }
}