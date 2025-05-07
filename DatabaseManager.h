#ifndef DATABASE_MANAGER_HPP
#define DATABASE_MANAGER_HPP

#include <sqlite3.h>
#include <string>
#include <stdexcept>

class DatabaseManager {
public:
    explicit DatabaseManager(const std::string& db_name);
    ~DatabaseManager();

private:
    sqlite3* db_;
    
    void executeSQL(const std::string& sql);
    void createTables();
};

#endif
