#include "UserManager/UserManager.h"
#include <ctime>
#include <random>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>
#include <sqlite3.h>
#include <algorithm>

using namespace std;

// 登录失败锁定策略（5次失败锁定15分钟）
constexpr int MAX_LOGIN_ATTEMPTS = 5;
constexpr int LOCKOUT_DURATION = 900; 

UserManager::UserManager(DatabaseManager& dbManager) : db_(dbManager) {}

string UserManager::hashPassword(const string& password) {
    // 生成随机盐值
    random_device rd;
    array<uint8_t, 16> salt;
    generate(salt.begin(), salt.end(), ref(rd));

    // 使用PBKDF2-HMAC-SHA256进行密钥派生
    const int iterations = 10000;
    const int keyLength = 32;
    vector<uint8_t> derivedKey(keyLength);

    PKCS5_PBKDF2_HMAC(
        password.c_str(), password.length(),
        salt.data(), salt.size(),
        iterations,
        EVP_sha256(),
        keyLength, derivedKey.data()
    );

    // 存储格式：算法$迭代次数$盐$密钥
    stringstream ss;
    ss << "pbkdf2-sha256$" << iterations << "$";
    ss << hex << setfill('0');
    for(auto b : salt) ss << setw(2) << (int)b;
    ss << "$";
    for(auto b : derivedKey) ss << setw(2) << (int)b;
    
    return ss.str();
}

bool UserManager::registerUser(const string& username, const string& password, const string& role) {
    // 输入验证
    if(username.empty() || password.empty()) return false;
    if(role != "admin" && role != "user") return false;

    lock_guard<mutex> lock(sessionMutex_);
    
    // 检查用户名是否已存在
    sqlite3_stmt* stmt;
    const char* checkSql = "SELECT 1 FROM users WHERE username = ?;";
    if(sqlite3_prepare_v2(db_.getHandle(), checkSql, -1, &stmt, nullptr) != SQLITE_OK) {
        cerr << "数据库错误: " << sqlite3_errmsg(db_.getHandle()) << endl;
        return false;
    }
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    
    if(sqlite3_step(stmt) == SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return false; // 用户已存在
    }
    sqlite3_finalize(stmt);

    // 插入新用户
    const char* insertSql = "INSERT INTO users (username, password_hash, role) VALUES (?, ?, ?);";
    if(sqlite3_prepare_v2(db_.getHandle(), insertSql, -1, &stmt, nullptr) != SQLITE_OK) {
        cerr << "数据库错误: " << sqlite3_errmsg(db_.getHandle()) << endl;
        return false;
    }

    string hashedPassword = hashPassword(password);
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, hashedPassword.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, role.c_str(), -1, SQLITE_STATIC);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    
    return success;
}

bool UserManager::login(const string& username, const string& password, const string& ip) {
    lock_guard<mutex> lock(sessionMutex_);
    clearExpiredSessions();

    // 检查登录失败次数
    static map<string, pair<int, time_t>> loginAttempts;
    auto it = loginAttempts.find(username);
    if(it != loginAttempts.end()) {
        if(it->second.first >= MAX_LOGIN_ATTEMPTS && 
           time(nullptr) - it->second.second < LOCKOUT_DURATION) 
        {
            cerr << "账户已锁定，请稍后再试" << endl;
            return false;
        }
    }

    // 查询用户信息
    sqlite3_stmt* stmt;
    const char* sql = "SELECT password_hash, role FROM users WHERE username = ?;";
    if(sqlite3_prepare_v2(db_.getHandle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        cerr << "数据库错误: " << sqlite3_errmsg(db_.getHandle()) << endl;
        return false;
    }
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

    if(sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        loginAttempts[username].first++;
        loginAttempts[username].second = time(nullptr);
        return false; // 用户不存在
    }

    // 验证密码
    const char* storedHash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    string inputHash = hashPassword(password);
    
    if(inputHash != storedHash) {
        sqlite3_finalize(stmt);
        loginAttempts[username].first++;
        loginAttempts[username].second = time(nullptr);
        return false;
    }

    // 获取用户角色
    string role = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    sqlite3_finalize(stmt);

    // 创建会话
    string sessionId = generateSessionId();
    activeSessions_[sessionId] = {
        username,
        role,
        ip,
        time(nullptr), // 登录时间
        time(nullptr)  // 最后活动时间
    };

    // 重置登录失败计数
    loginAttempts.erase(username);
    return true;
}

void UserManager::logout(const string& sessionId) {
    lock_guard<mutex> lock(sessionMutex_);
    activeSessions_.erase(sessionId);
}

bool UserManager::validateSession(const string& sessionId) {
    lock_guard<mutex> lock(sessionMutex_);
    auto it = activeSessions_.find(sessionId);
    if(it == activeSessions_.end()) return false;

    // 检查会话超时
    if(time(nullptr) - it->second.lastActivity > SESSION_TIMEOUT) {
        activeSessions_.erase(it);
        return false;
    }

    // 更新最后活动时间
    it->second.lastActivity = time(nullptr);
    return true;
}

string UserManager::getCurrentUserRole(const string& sessionId) {
    lock_guard<mutex> lock(sessionMutex_);
    auto it = activeSessions_.find(sessionId);
    return (it != activeSessions_.end()) ? it->second.role : "";
}

string UserManager::generateSessionId() {
    // 使用CryptoPP生成密码学安全的随机数
    random_device rd;
    array<uint8_t, 32> randomData;
    generate(randomData.begin(), randomData.end(), ref(rd));

    stringstream ss;
    ss << hex << setfill('0');
    for(auto b : randomData) {
        ss << setw(2) << static_cast<int>(b);
    }
    return ss.str();
}

void UserManager::clearExpiredSessions() {
    time_t now = time(nullptr);
    vector<string> expiredSessions;

    for(const auto& [sessionId, session] : activeSessions_) {
        if(now - session.lastActivity > SESSION_TIMEOUT) {
            expiredSessions.push_back(sessionId);
        }
    }

    for(const auto& sessionId : expiredSessions) {
        activeSessions_.erase(sessionId);
    }
}
