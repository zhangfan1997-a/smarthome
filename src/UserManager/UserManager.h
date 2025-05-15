#ifndef USER_MANAGER_H
#define USER_MANAGER_H

#include <sqlite3.h>
#include <string>
#include <map>
#include <mutex>
#include <ctime>

class User {
public:
    enum class Role { Admin, User };
    
    User(int id, const std::string& username, 
         const std::string& password_hash, Role role)
        : id(id), username(username), 
          password_hash(password_hash), role(role) {}
    
    int getId() const { return id; }
    const std::string& getUsername() const { return username; }
    const std::string& getPasswordHash() const { return password_hash; }
    Role getRole() const { return role; }

private:
    int id;
    std::string username;
    std::string password_hash;
    Role role;
};

class UserManager {
public:
    static UserManager& GetInstance();
    
    bool Initialize(const std::string& dbPath = "users.db");
    void Close();
    
    bool RegisterUser(const std::string& username, const std::string& password);
    bool LoginUser(const std::string& username, const std::string& password);
    void LogoutUser(int userId);
    User* GetUser(int userId);
    User* GetCurrentUser();//获取当前登陆用户
    
    // 禁止拷贝和赋值
    UserManager(const UserManager&) = delete;
    void operator=(const UserManager&) = delete;

private:
    UserManager() = default;
    ~UserManager();
    
    struct Session {
        std::string token;
        time_t expiry;
    };
    int currentUserId_=-1;//当前用户ID，-1表示未登录
    // 数据库操作
    bool ExecuteSQL(const std::string& sql);
    bool PrepareStatement(const std::string& sql, sqlite3_stmt** stmt);
    void LoadUsersFromDB();
    std::string HashPassword(const std::string& password) const;
    
    sqlite3* db_ = nullptr;
    std::map<int, User> users_;
    std::map<int, Session> sessions_;
    mutable std::mutex dbMutex_;
    mutable std::mutex dataMutex_;
};

#endif // USER_MANAGER_H