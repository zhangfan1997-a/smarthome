#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

#include "DatabaseManager/DatabaseManager.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>

// 设备接口
class Device {
public:
    virtual ~Device() = default;
    virtual std::string getType() const = 0;
    virtual std::string getStatus() const = 0;
    virtual void control(const std::string& command) = 0;
    virtual void updateDatabase(DatabaseManager& db) = 0;
    virtual int getId() const = 0;
};

// 设备工厂接口
class DeviceFactory {
public:
    virtual std::unique_ptr<Device> createDevice(int id, const std::string& config) = 0;
    virtual ~DeviceFactory() = default;
};

// 设备管理器
class DeviceManager {
public:
    DeviceManager(DatabaseManager& db, const std::string& configPath);
    
    void loadDevices();
    bool addDevice(const std::string& type, const std::string& config);
    bool removeDevice(int deviceId);
    Device* getDevice(int deviceId);
    std::vector<Device*> getAllDevices();
    
    // 设备控制接口
    bool setDeviceStatus(int deviceId, const std::string& command);
    std::string getDeviceStatus(int deviceId);

private:
    DatabaseManager& db_;
    std::unordered_map<int, std::unique_ptr<Device>> devices_;
    std::unordered_map<std::string, std::unique_ptr<DeviceFactory>> factories_;
    std::mutex devicesMutex_;
    std::atomic<int> nextDeviceId_{1};
    
    void registerFactory(const std::string& type, std::unique_ptr<DeviceFactory> factory);
    void loadConfigurations(const std::string& path);
};

// 具体设备工厂注册
template<typename T>
class DeviceFactoryImpl : public DeviceFactory {
public:
    std::unique_ptr<Device> createDevice(int id, const std::string& config) override {
        return std::make_unique<T>(id, config);
    }
};

#endif // DEVICE_MANAGER_H