#ifndef LOG_MANAGER_H
#define LOG_MANAGER_H

#include <string>
#include <queue>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <fstream>
#include <chrono>
#include <ctime>
#include <filesystem>

namespace fs = std::filesystem;

class LogManager {
public:
    enum class LogType {
        USER_ACTION,
        DEVICE_OPERATION,
        SYSTEM_EVENT,
        ERROR_LOG
    };

    static LogManager& getInstance();
    
    void init(const std::string& logDir = "logs", 
             size_t maxFileSize = 10'485'760,  // 10MB
             int maxBackups = 5);
    
    void log(LogType type, 
            const std::string& message, 
            int userId = -1, 
            int deviceId = -1);
    
    void flush();
    void shutdown();

private:
    LogManager() = default;
    ~LogManager();
    
    struct LogEntry {
        std::time_t timestamp;
        LogType type;
        std::string message;
        int userId;
        int deviceId;
    };

    std::queue<LogEntry> logQueue_;
    std::mutex queueMutex_;
    std::condition_variable cv_;
    std::atomic<bool> running_{false};
    
    fs::path logDir_;
    std::ofstream logFile_;
    size_t currentFileSize_;
    size_t maxFileSize_;
    int maxBackups_;
    
    std::thread workerThread_;
    
    void workerFunction();
    void rotateLog();
    std::string getTypeString(LogType type) const;
    void writeBatch(const std::vector<LogEntry>& batch);
    void createNewLogFile();
};

#endif // LOG_MANAGER_H