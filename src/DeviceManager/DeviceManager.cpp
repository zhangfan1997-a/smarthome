#include "DeviceManager.h"
#include <iostream>
#include <iomanip>
#include <sstream>

using namespace std;

DeviceManager::~DeviceManager() {
    Close();
}

DeviceManager& DeviceManager::GetInstance() {
    static DeviceManager instance;
    return instance;
}

const DeviceManager::DeviceInfo* DeviceManager::GetDevice(int id) const {
    std::lock_guard<std::mutex> dataLock(dataMutex_);
    auto it = devices_.find(id);
    return (it != devices_.end()) ? &it->second : nullptr;
}

bool DeviceManager::Initialize(const std::string& dbPath) {
    lock_guard<mutex> lock(dbMutex_);
    
    if (sqlite3_open(dbPath.c_str(), &db_) != SQLITE_OK) {
        cerr << "数据库连接失败: " << sqlite3_errmsg(db_) << endl;
        return false;
    }

    // 创建设备表
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS devices (
            id INTEGER PRIMARY KEY,
            name TEXT NOT NULL,
            type TEXT NOT NULL,
            status TEXT DEFAULT 'offline'
        );
    )";

    if (!ExecuteSQL(sql)) {
        cerr << "初始化数据库失败" << endl;
        return false;
    }

    LoadFromDatabase();
    return true;
}

void DeviceManager::Close() {
    lock_guard<mutex> lock(dbMutex_);
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool DeviceManager::ExecuteSQL(const std::string& sql) {
    lock_guard<mutex> lock(dbMutex_);
    char* errMsg = nullptr;
    if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        cerr << "SQL错误: " << errMsg << endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool DeviceManager::PrepareStatement(const std::string& sql, sqlite3_stmt** stmt) {
    lock_guard<mutex> lock(dbMutex_);
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, stmt, nullptr) != SQLITE_OK) {
        cerr << "准备语句失败: " << sqlite3_errmsg(db_) << endl;
        return false;
    }
    return true;
}

void DeviceManager::LoadFromDatabase() {
    lock_guard<mutex> dataLock(dataMutex_);
    devices_.clear();

    sqlite3_stmt* stmt;
    if (!PrepareStatement("SELECT id, name, type, status FROM devices;", &stmt)) return;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        DeviceInfo info;
        info.id = sqlite3_column_int(stmt, 0);
        info.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        info.type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        info.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        devices_[info.id] = info;
    }
    sqlite3_finalize(stmt);
}

bool DeviceManager::AddDevice(int id, const std::string& name, const std::string& type) {
    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO devices (id, name, type) VALUES (?, ?, ?);";
    if (!PrepareStatement(sql, &stmt)) return false;
    
    sqlite3_bind_int(stmt, 1, id);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, type.c_str(), -1, SQLITE_TRANSIENT);
    
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

bool DeviceManager::RemoveDevice(int id) {
    // 内存操作
    {
        lock_guard<mutex> dataLock(dataMutex_);
        if (!devices_.count(id)) return false;
        devices_.erase(id);
    }

    // 数据库操作
    stringstream sql;
    sql << "DELETE FROM devices WHERE id = " << id << ";";
    return ExecuteSQL(sql.str());
}

bool DeviceManager::UpdateDeviceStatus(int id, const std::string& newStatus) {
    // 内存操作
    {
        lock_guard<mutex> dataLock(dataMutex_);
        if (!devices_.count(id)) return false;
        devices_[id].status = newStatus;
    }

    // 数据库操作
    stringstream sql;
    sql << "UPDATE devices SET status = '" << newStatus 
        << "' WHERE id = " << id << ";";
    return ExecuteSQL(sql.str());
}

std::string DeviceManager::GetDeviceStatus(int id) const {
    lock_guard<mutex> dataLock(dataMutex_);
    auto it = devices_.find(id);
    return it != devices_.end() ? it->second.status : "设备不存在";
}

void DeviceManager::ListAllDevices() const {
    lock_guard<mutex> dataLock(dataMutex_);
    
    cout << "\n===== 设备列表 (" << devices_.size() << ") =====\n";
    cout << left << setw(6) << "ID" << setw(20) << "名称" 
         << setw(15) << "类型" << setw(10) << "状态\n";

    for (const auto& [id, info] : devices_) {
        cout << setw(6) << id << setw(20) << info.name 
             << setw(15) << info.type << setw(10) << info.status << endl;
    }
    cout << "==============================\n";
}