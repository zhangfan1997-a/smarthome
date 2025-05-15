#include "UserManager.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>

using namespace std;

UserManager::~UserManager() {
    Close();
}

UserManager& UserManager::GetInstance() {
    static UserManager instance;
    return instance;
}

bool UserManager::Initialize(const std::string& dbPath) {
    lock_guard<mutex> lock(dbMutex_);
    
    if (sqlite3_open(dbPath.c_str(), &db_) != SQLITE_OK) {
        cerr << "数据库连接失败: " << sqlite3_errmsg(db_) << endl;
        return false;
    }

    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            password_hash TEXT NOT NULL,
            role INTEGER DEFAULT 1
        );
        
        CREATE TABLE IF NOT EXISTS sessions (
            user_id INTEGER PRIMARY KEY,
            token TEXT NOT NULL,
            expiry INTEGER NOT NULL,
            FOREIGN KEY(user_id) REFERENCES users(id)
        );
    )";

    if (!ExecuteSQL(sql)) {
        cerr << "初始化数据库失败" << endl;
        return false;
    }

    LoadUsersFromDB();
    return true;
}

void UserManager::Close() {
    lock_guard<mutex> lock(dbMutex_);
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool UserManager::ExecuteSQL(const std::string& sql) {
    lock_guard<mutex> lock(dbMutex_);
    char* errMsg = nullptr;
    if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        cerr << "SQL错误: " << errMsg << endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool UserManager::PrepareStatement(const std::string& sql, sqlite3_stmt** stmt) {
    lock_guard<mutex> lock(dbMutex_);
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, stmt, nullptr) != SQLITE_OK) {
        cerr << "准备语句失败: " << sqlite3_errmsg(db_) << endl;
        return false;
    }
    return true;
}

void UserManager::LoadUsersFromDB() {
    lock_guard<mutex> dataLock(dataMutex_);
    users_.clear();

    sqlite3_stmt* stmt;
    if (!PrepareStatement("SELECT id, username, password_hash, role FROM users;", &stmt)) return;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        string username(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        string password_hash(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        User::Role role = static_cast<User::Role>(sqlite3_column_int(stmt, 3));
        
        users_.emplace(id, User(id, username, password_hash, role));
    }
    sqlite3_finalize(stmt);
}

string UserManager::HashPassword(const string& password) const {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(password.c_str()), 
          password.length(), hash);

    stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << hex << setw(2) << setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}

bool UserManager::RegisterUser(const string& username, const string& password) {
    string hashedPassword = HashPassword(password);
    
    // 数据库操作
    stringstream sql;
    sql << "INSERT INTO users (username, password_hash) VALUES ('"
        << username << "', '" << hashedPassword << "');";
    
    if (!ExecuteSQL(sql.str())) return false;

    // 重新加载用户数据
    LoadUsersFromDB();
    return true;
}

bool UserManager::LoginUser(const std::string& username, const std::string& password) {
    lock_guard<mutex> dataLock(dataMutex_);
    
    for (auto& [id, user] : users_) {
        if (user.getUsername() == username) {
            if (user.getPasswordHash() == HashPassword(password)) {
                // 生成会话令牌
                Session session;
                session.token = HashPassword(username + to_string(time(nullptr)));
                session.expiry = time(nullptr) + 3600;

                // 更新会话和当前用户
                sessions_[id] = session;
                currentUserId_ = id;  // 设置当前用户ID

                // 保存到数据库
                stringstream sql;
                sql << "INSERT OR REPLACE INTO sessions VALUES ("
                    << id << ", '" << session.token << "', " << session.expiry << ");";
                return ExecuteSQL(sql.str());
            }
            break;
        }
    }
    return false;
}

void UserManager::LogoutUser(int userId) {
    lock_guard<mutex> dataLock(dataMutex_);
    sessions_.erase(userId);
    
    // 如果登出的是当前用户，则重置
    if (currentUserId_ == userId) {
        currentUserId_ = -1;
    }

    stringstream sql;
    sql << "DELETE FROM sessions WHERE user_id = " << userId << ";";
    ExecuteSQL(sql.str());
}

User* UserManager::GetUser(int userId) {
    lock_guard<mutex> dataLock(dataMutex_);
    auto it = users_.find(userId);
    return it != users_.end() ? &it->second : nullptr;
}

User* UserManager::GetCurrentUser() {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    // 检查是否已登录
    if (currentUserId_ == -1) {
        return nullptr;
    }

    // 检查会话是否存在且未过期
    auto sessionIt = sessions_.find(currentUserId_);
    if (sessionIt == sessions_.end() || sessionIt->second.expiry < time(nullptr)) {
        currentUserId_ = -1;  // 会话无效则重置
        return nullptr;
    }

    // 返回用户对象
    auto userIt = users_.find(currentUserId_);
    return (userIt != users_.end()) ? &userIt->second : nullptr;
}


