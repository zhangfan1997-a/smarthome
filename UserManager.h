#ifndef USER_MANAGER_H
#define USER_MANAGER_H

#include <string>
#include <unordered_map>
#include <mutex>
#include "DatabaseManager/DatabaseManager.h"

struct UserSession {
    std::string username;
    std::string role;
    std::string ipAddress;
    time_t loginTime;
    time_t lastActivity;
};

class UserManager {
public:
    UserManager(DatabaseManager& dbManager);
    
    bool registerUser(const std::string& username, const std::string& password, const std::string& role);
    bool login(const std::string& username, const std::string& password, const std::string& ip);
    void logout(const std::string& sessionId);
    bool validateSession(const std::string& sessionId);
    std::string getCurrentUserRole(const std::string& sessionId);
    
private:
    DatabaseManager& db_;
    std::unordered_map<std::string, UserSession> activeSessions_;
    std::mutex sessionMutex_;
    
    std::string generateSessionId();
    std::string hashPassword(const std::string& password);
    void updateSessionActivity(const std::string& sessionId);
    void clearExpiredSessions();
};

#endif // USER_MANAGER_H