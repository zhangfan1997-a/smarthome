#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <string>
#include <map>
#include <mutex>

class ConfigManager {
public:
    // 单例访问接口
    static ConfigManager& GetInstance();
    
    // 配置文件操作
    void load(const std::string& filename);
    void saveConfig();
    void updateConfig(const std::string& key, const std::string& value);
    
    // 配置获取接口
    template<typename T>
    T get(const std::string& key, T defaultValue) const;

    // 禁止拷贝和赋值
    ConfigManager(const ConfigManager&) = delete;
    void operator=(const ConfigManager&) = delete;

private:
    ConfigManager() = default;  // 私有构造函数
    
    // 辅助方法
    static std::string trim(const std::string& str);
    
    std::map<std::string, std::string> configData_;
    std::string configFile_;
    mutable std::mutex mutex_;
};

// 模板特化声明
template<typename T>
T ConfigManager::get(const std::string& key, T defaultValue) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = configData_.find(key);
    if (it != configData_.end()) {
        std::stringstream ss(it->second);
        T value;
        if (ss >> value) return value;
    }
    return defaultValue;
}
template<>
bool ConfigManager::get<bool>(const std::string& key, bool defaultValue) const;

#endif // CONFIG_MANAGER_H