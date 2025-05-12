#include "ExpectionManager.h"
#include <iostream>
#include <fstream>
#include <ctime>
#include <iomanip>

std::mutex ExceptionHandler::logMutex;
const std::string ExceptionHandler::logFilePath = "exceptions.log";

void ExceptionHandler::handleException(const std::exception& e) {
    const std::string errorMsg = "Exception handled: " + std::string(e.what());
    
    // 控制台输出
    writeToConsole(errorMsg);
    
    // 文件记录
    writeToFile(errorMsg);
}

void ExceptionHandler::logException(const std::exception& e) {
    std::lock_guard<std::mutex> lock(logMutex);
    
    try {
        std::time_t now = std::time(nullptr);
        std::tm tm = *std::localtime(&now);
        
        std::ofstream logFile(logFilePath, std::ios::app);
        if (logFile.is_open()) {
            logFile << "["
                    << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
                    << "] Exception occurred: "
                    << e.what()
                    << "\n";
        }
    } catch (...) {
        // 确保日志失败不会抛出新异常
        writeToConsole("Failed to write exception to log file");
    }
}

void ExceptionHandler::writeToConsole(const std::string& message) {
    std::cerr << "\033[31m" << message << "\033[0m" << std::endl;
}

void ExceptionHandler::writeToFile(const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex);
    
    try {
        std::ofstream logFile(logFilePath, std::ios::app);
        if (logFile.is_open()) {
            logFile << message << "\n";
        }
    } catch (...) {
        std::cerr << "Critical error: Failed to write to exception log" << std::endl;
    }
}