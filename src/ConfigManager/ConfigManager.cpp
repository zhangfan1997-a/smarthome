#include "ConfigManager.h"
#include <fstream>
#include <sstream>
#include <algorithm>

ConfigManager& ConfigManager::GetInstance() {
    static ConfigManager instance;
    return instance;
}

std::string ConfigManager::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t");
    return str.substr(first, (last - first + 1));
}

void ConfigManager::load(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);
    configFile_ = filename;
    configData_.clear();
    
    std::ifstream file(filename);
    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = trim(line.substr(0, pos));
            std::string value = trim(line.substr(pos + 1));
            configData_[key] = value;
        }
    }
}

void ConfigManager::saveConfig() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ofstream file(configFile_);
    for (const auto& pair : configData_) {
        file << pair.first << "=" << pair.second << "\n";
    }
}

void ConfigManager::updateConfig(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    configData_[key] = value;
}



// 字符串类型特化
template<>
std::string ConfigManager::get<std::string>(const std::string& key, std::string defaultValue) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = configData_.find(key);
    return (it != configData_.end()) ? it->second : defaultValue;
}

// 布尔类型特化
template<>
bool ConfigManager::get<bool>(const std::string& key, bool defaultValue) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = configData_.find(key);
    if (it != configData_.end()) {
        std::string value = it->second;
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);
        if (value == "true" || value == "1") return true;
        if (value == "false" || value == "0") return false;
    }
    return defaultValue;
}