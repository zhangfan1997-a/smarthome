#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

#include <sqlite3.h>
#include <string>
#include <map>
#include <mutex>
#include <memory>

class DeviceManager {
public:
    //设备信息
    struct DeviceInfo {
        int id;
        std::string name;
        std::string type;
        std::string status;
    };
    
    // 单例访问接口
    static DeviceManager& GetInstance();

    //获取设备信息
    const DeviceInfo* GetDevice(int id) const;//只读访问
    
    // 数据库管理
    bool Initialize(const std::string& dbPath = "devices.db");
    void Close();

    // 设备操作接口
    bool AddDevice(int id, const std::string& name, const std::string& type);
    bool RemoveDevice(int id);
    bool UpdateDeviceStatus(int id, const std::string& newStatus);
    std::string GetDeviceStatus(int id) const;
    void ListAllDevices() const;

    // 禁止拷贝和赋值
    DeviceManager(const DeviceManager&) = delete;
    void operator=(const DeviceManager&) = delete;

private:
    DeviceManager() = default;
    ~DeviceManager();

    // 数据库操作
    bool ExecuteSQL(const std::string& sql);
    bool PrepareStatement(const std::string& sql, sqlite3_stmt** stmt);
    void LoadFromDatabase();

    sqlite3* db_ = nullptr;
    std::map<int, DeviceInfo> devices_;
    mutable std::mutex dbMutex_;
    mutable std::mutex dataMutex_;
};

#endif // DEVICE_MANAGER_H