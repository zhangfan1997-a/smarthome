#include "LogManager/LogManager.h"
#include <iomanip>
#include <sstream>
#include <algorithm>

using namespace std::chrono;

constexpr auto FLUSH_INTERVAL = 5s;
constexpr size_t BATCH_SIZE = 100;

LogManager::~LogManager() {
    shutdown();
}

LogManager& LogManager::getInstance() {
    static LogManager instance;
    return instance;
}

void LogManager::init(const std::string& logDir, size_t maxFileSize, int maxBackups) {
    if (running_) return;

    logDir_ = fs::path(logDir);
    maxFileSize_ = maxFileSize;
    maxBackups_ = maxBackups;
    
    fs::create_directories(logDir_);
    createNewLogFile();
    
    running_ = true;
    workerThread_ = std::thread(&LogManager::workerFunction, this);
}

void LogManager::log(LogType type, const std::string& message, int userId, int deviceId) {
    if (!running_) return;

    LogEntry entry{
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()),
        type,
        message,
        userId,
        deviceId
    };

    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        logQueue_.push(entry);
    }
    cv_.notify_one();
}

void LogManager::workerFunction() {
    std::vector<LogEntry> batch;
    batch.reserve(BATCH_SIZE);
    
    auto lastFlush = steady_clock::now();

    while (running_) {
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            cv_.wait_for(lock, FLUSH_INTERVAL, [this] {
                return !logQueue_.empty() || !running_;
            });

            while (!logQueue_.empty() && batch.size() < BATCH_SIZE) {
                batch.push_back(logQueue_.front());
                logQueue_.pop();
            }
        }

        if (!batch.empty()) {
            writeBatch(batch);
            batch.clear();
        }

        // 定时刷新和文件大小检查
        if (steady_clock::now() - lastFlush >= FLUSH_INTERVAL || currentFileSize_ >= maxFileSize_) {
            flush();
            lastFlush = steady_clock::now();
        }
    }

    // 退出前写入剩余日志
    if (!batch.empty()) {
        writeBatch(batch);
    }
}

void LogManager::writeBatch(const std::vector<LogEntry>& batch) {
    std::stringstream ss;
    
    for (const auto& entry : batch) {
        ss.str("");
        ss.clear();
        
        // 格式化时间戳
        ss << std::put_time(std::localtime(&entry.timestamp), "%Y-%m-%d %H:%M:%S");
        ss << " [" << getTypeString(entry.type) << "] ";
        
        // 添加用户和设备信息
        if (entry.userId != -1) {
            ss << "[UID:" << entry.userId << "] ";
        }
        if (entry.deviceId != -1) {
            ss << "[DID:" << entry.deviceId << "] ";
        }
        
        ss << entry.message << "\n";
        
        const std::string logLine = ss.str();
        const size_t lineSize = logLine.size();
        
        if (currentFileSize_ + lineSize > maxFileSize_) {
            rotateLog();
        }
        
        logFile_ << logLine;
        currentFileSize_ += lineSize;
    }
    
    logFile_.flush();
}

void LogManager::rotateLog() {
    logFile_.close();
    
    // 滚动现有备份文件
    for (int i = maxBackups_ - 1; i >= 0; --i) {
        fs::path oldFile = logDir_ / ("log_" + std::to_string(i) + ".txt");
        if (fs::exists(oldFile)) {
            if (i == maxBackups_ - 1) {
                fs::remove(oldFile);
            } else {
                fs::rename(oldFile, logDir_ / ("log_" + std::to_string(i + 1) + ".txt"));
            }
        }
    }
    
    // 重命名当前文件
    fs::rename(logDir_ / "log.txt", logDir_ / "log_0.txt");
    
    createNewLogFile();
}

void LogManager::createNewLogFile() {
    logFile_.open(logDir_ / "log.txt", std::ios::app);
    currentFileSize_ = logFile_.tellp();
}

std::string LogManager::getTypeString(LogType type) const {
    switch (type) {
        case LogType::USER_ACTION:   return "USER";
        case LogType::DEVICE_OPERATION: return "DEVICE";
        case LogType::SYSTEM_EVENT: return "SYSTEM";
        case LogType::ERROR_LOG:    return "ERROR";
        default:                    return "UNKNOWN";
    }
}

void LogManager::flush() {
    if (logFile_.is_open()) {
        logFile_.flush();
    }
}

void LogManager::shutdown() {
    if (running_) {
        running_ = false;
        cv_.notify_all();
        if (workerThread_.joinable()) {
            workerThread_.join();
        }
        flush();
    }
}