#include "DeviceManager/DeviceManager.h"
#include "DatabaseManager/DatabaseManager.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <sqlite3.h>
#include <algorithm>

using namespace std;
using json = nlohmann::json;

//--------------------- 具体设备实现 ---------------------
class Light : public Device {
public:
    Light(int id, const string& config) : id_(id) {
        try {
            json configJson = json::parse(config);
            brightness_ = configJson.value("brightness", 50);
            isOn_ = configJson.value("power", false);
        } catch(...) {
            brightness_ = 50;
            isOn_ = false;
        }
    }

    string getType() const override { return "light"; }
    
    string getStatus() const override {
        json status;
        status["power"] = isOn_;
        status["brightness"] = brightness_;
        return status.dump();
    }

    void control(const string& command) override {
        json cmd = json::parse(command);
        if(cmd.contains("power")) {
            isOn_ = cmd["power"].get<bool>();
        }
        if(cmd.contains("brightness")) {
            int value = cmd["brightness"].get<int>();
            brightness_ = clamp(value, 0, 100);
        }
    }

    void updateDatabase(DatabaseManager& db) override {
        stringstream ss;
        ss << "UPDATE devices SET status = '" << getStatus() 
           << "', last_modified = CURRENT_TIMESTAMP WHERE id = " << id_;
        db.executeSQL(ss.str());
    }

    int getId() const override { return id_; }

private:
    int id_;
    bool isOn_;
    int brightness_;
};

class Thermostat : public Device {
public:
    Thermostat(int id, const string& config) : id_(id) {
        try {
            json configJson = json::parse(config);
            currentTemp_ = configJson.value("currentTemp", 22.0);
            targetTemp_ = configJson.value("targetTemp", 22.0);
        } catch(...) {
            currentTemp_ = 22.0;
            targetTemp_ = 22.0;
        }
    }

    string getType() const override { return "thermostat"; }
    
    string getStatus() const override {
        json status;
        status["currentTemp"] = currentTemp_;
        status["targetTemp"] = targetTemp_;
        return status.dump();
    }

    void control(const string& command) override {
        json cmd = json::parse(command);
        if(cmd.contains("targetTemp")) {
            targetTemp_ = clamp(cmd["targetTemp"].get<double>(), 10.0, 30.0);
        }
        // 模拟温度变化
        currentTemp_ += (targetTemp_ - currentTemp_) * 0.1;
    }

    void updateDatabase(DatabaseManager& db) override {
        stringstream ss;
        ss << "UPDATE devices SET status = '" << getStatus() 
           << "', last_modified = CURRENT_TIMESTAMP WHERE id = " << id_;
        db.executeSQL(ss.str());
    }

    int getId() const override { return id_; }

private:
    int id_;
    double currentTemp_;
    double targetTemp_;
};

//--------------------- 设备管理器实现 ---------------------
DeviceManager::DeviceManager(DatabaseManager& db, const string& configPath) 
    : db_(db) 
{
    // 注册设备工厂
    registerFactory("light", make_unique<DeviceFactoryImpl<Light>>());
    registerFactory("thermostat", make_unique<DeviceFactoryImpl<Thermostat>>());
    // 可扩展其他设备类型...

    // 初始化设备
    try {
        loadConfigurations(configPath);
        loadDevices();
    } catch(const exception& e) {
        cerr << "设备初始化失败: " << e.what() << endl;
    }
}

void DeviceManager::registerFactory(const string& type, unique_ptr<DeviceFactory> factory) {
    lock_guard<mutex> lock(devicesMutex_);
    factories_.emplace(type, move(factory));
}

void DeviceManager::loadConfigurations(const string& path) {
    ifstream configFile(path);
    if(!configFile.is_open()) {
        throw runtime_error("配置文件打开失败: " + path);
    }

    json config;
    try {
        configFile >> config;
    } catch(const json::exception& e) {
        throw runtime_error("配置文件解析错误: " + string(e.what()));
    }

    for(const auto& deviceConfig : config["devices"]) {
        string type = deviceConfig["type"];
        string configStr = deviceConfig["config"].dump();
        addDevice(type, configStr);
    }
}

void DeviceManager::loadDevices() {
    lock_guard<mutex> lock(devicesMutex_);
    
    const char* sql = "SELECT id, device_type, status FROM devices;";
    sqlite3_stmt* stmt;

    if(sqlite3_prepare_v2(db_.getHandle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw runtime_error("数据库查询失败: " + string(sqlite3_errmsg(db_.getHandle())));
    }

    while(sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const char* type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* config = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));

        if(factories_.find(type) != factories_.end()) {
            auto device = factories_[type]->createDevice(id, config);
            if(device) {
                devices_[id] = move(device);
                nextDeviceId_ = max(nextDeviceId_, id + 1);
            }
        }
    }
    sqlite3_finalize(stmt);
}

bool DeviceManager::addDevice(const string& type, const string& config) {
    lock_guard<mutex> lock(devicesMutex_);
    
    if(factories_.find(type) == factories_.end()) {
        cerr << "未知设备类型: " << type << endl;
        return false;
    }

    int newId = nextDeviceId_++;
    auto device = factories_[type]->createDevice(newId, config);
    if(!device) return false;

    // 插入数据库
    stringstream sql;
    sql << "INSERT INTO devices (id, device_type, status) VALUES ("
        << newId << ", '" << type << "', '" << device->getStatus() << "');";
    
    try {
        db_.executeSQL(sql.str());
        devices_.emplace(newId, move(device));
        return true;
    } catch(const exception& e) {
        cerr << "设备添加失败: " << e.what() << endl;
        return false;
    }
}

bool DeviceManager::removeDevice(int deviceId) {
    lock_guard<mutex> lock(devicesMutex_);
    
    if(devices_.find(deviceId) == devices_.end()) {
        return false;
    }

    stringstream sql;
    sql << "DELETE FROM devices WHERE id = " << deviceId;
    
    try {
        db_.executeSQL(sql.str());
        devices_.erase(deviceId);
        return true;
    } catch(const exception& e) {
        cerr << "设备删除失败: " << e.what() << endl;
        return false;
    }
}

bool DeviceManager::setDeviceStatus(int deviceId, const string& command) {
    auto* device = getDevice(deviceId);
    if(!device) return false;

    try {
        device->control(command);
        device->updateDatabase(db_);
        return true;
    } catch(const exception& e) {
        cerr << "设备控制失败: " << e.what() << endl;
        return false;
    }
}

string DeviceManager::getDeviceStatus(int deviceId) {
    auto* device = getDevice(deviceId);
    return device ? device->getStatus() : "";
}

Device* DeviceManager::getDevice(int deviceId) {
    lock_guard<mutex> lock(devicesMutex_);
    auto it = devices_.find(deviceId);
    return (it != devices_.end()) ? it->second.get() : nullptr;
}

vector<Device*> DeviceManager::getAllDevices() {
    lock_guard<mutex> lock(devicesMutex_);
    vector<Device*> devices;
    for(auto& [id, device] : devices_) {
        devices.push_back(device.get());
    }
    return devices;
}