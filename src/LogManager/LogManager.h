#ifndef LOG_MANAGER_H
#define LOG_MANAGER_H

#include <string>
#include <vector>
#include <mutex>
#include <fstream>

class LogManager {
public:
    enum class LogType { UserAction, DeviceAction, System };
    
    struct LogEntry {
        LogType type;
        int userId;
        int deviceId;
        std::string content;
        std::string timestamp;
    };

    // 单例访问方法
    static LogManager& GetInstance();
    
    // 日志记录接口
    void logAction(LogType type, int userId, int deviceId, const std::string& content);
    
    // 日志持久化
    void saveLogsToFile(const std::string& filename);
    
    // 禁止拷贝和赋值
    LogManager(const LogManager&) = delete;
    void operator=(const LogManager&) = delete;

private:
    LogManager() = default;  // 私有构造函数
    ~LogManager() = default;
    
    std::string formatLogEntry(const LogEntry& entry) const;
    std::string logTypeToString(LogType type) const;
    
    std::vector<LogEntry> logs_;
    mutable std::mutex logMutex_;
};

#endif // LOG_MANAGER_H