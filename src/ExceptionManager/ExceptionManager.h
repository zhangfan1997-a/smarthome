#ifndef EXCEPTION_MANGER_H
#define EXCEPTION_MANGER_H

#include <exception>
#include <string>
#include <mutex>

class ExceptionHandler {
public:
    static void handleException(const std::exception& e);
    static void logException(const std::exception& e);

private:
    static std::mutex logMutex;
    static const std::string logFilePath;
    
    static void writeToConsole(const std::string& message);
    static void writeToFile(const std::string& message);
};

#endif // EXCEPTION_MANGER_H