#include "DatabaseManager/DatabaseManager.h"
#include "UserManager/UserManager.h"
#include "DeviceManager/DeviceManager.h"
#include <iostream>
#include <string>
#include <stdexcept>

using namespace std;

int main() {
    try {
        DatabaseManager db("manage.db");
        UserManager userManager(db);
        
        // 注册示例
        userManager.registerUser("admin", "secure123", "admin");
        userManager.registerUser("user1", "password1", "user");
        
        // 登录示例
        if(userManager.login("admin", "secure123", "192.168.1.100")) {
            cout << "管理员登录成功" << endl;
        }
        
         DeviceManager deviceManager(db, "config/devices.json");
         // 添加新设备
         deviceManager.addDevice("light", R"({"brightness": 75})");
        
         // 控制设备
         if(auto* light = deviceManager.getDevice(1)) {
            light->control(R"({"power": true, "brightness": 80})");
            std::cout << "当前状态: " << light->getStatus() << std::endl;
        }

    } catch(const exception& e) {
        cerr << "系统错误: " << e.what() << endl;
        return 1;
    }
    return 0;
}