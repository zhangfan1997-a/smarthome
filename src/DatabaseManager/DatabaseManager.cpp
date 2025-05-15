#include "DatabaseManager.h"
#include <iostream>

DatabaseManager::DatabaseManager(const std::string& db_name) {
    int rc = sqlite3_open(db_name.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::string err_msg = "Database error: ";
        err_msg += sqlite3_errmsg(db_);
        sqlite3_close(db_);
        throw std::runtime_error(err_msg);
    }
    createTables();
}

DatabaseManager::~DatabaseManager() {
    sqlite3_close(db_);
}

void DatabaseManager::executeSQL(const std::string& sql) {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::string error = "SQL error: ";
        error += errMsg;
        sqlite3_free(errMsg);
        throw std::runtime_error(error);
    }
}

void DatabaseManager::createTables() {
    executeSQL(
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "username TEXT NOT NULL UNIQUE,"
        "password_hash TEXT NOT NULL,"
        "role TEXT CHECK(role IN ('admin', 'user')) NOT NULL);"
    );

    executeSQL(
        "CREATE TABLE IF NOT EXISTS devices ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "device_type TEXT NOT NULL,"
        "status TEXT,"
        "last_modified TIMESTAMP DEFAULT CURRENT_TIMESTAMP);"
    );

    executeSQL(
        "CREATE TABLE IF NOT EXISTS logs ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "log_type TEXT NOT NULL,"
        "timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
        "user_id INTEGER,"
        "device_id INTEGER,"
        "FOREIGN KEY(user_id) REFERENCES users(id),"
        "FOREIGN KEY(device_id) REFERENCES devices(id));"
    );
}
