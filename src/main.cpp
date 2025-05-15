#include "ConfigManager/ConfigManager.h"
#include "DeviceManager/DeviceManager.h"
#include "UserManager/UserManager.h"
#include "LogManager/LogManager.h"
#include "MenuManager/MenuManager.h"
#include "ExceptionManager/ExceptionManager.h"
#include <iostream>
#include <ctime>
#include <iomanip>
#include <limits>

using namespace std;

// 获取当前时间字符串
string getCurrentTime() {
    time_t now = time(nullptr);
    tm* tm = localtime(&now);
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm);
    return string(buffer);
}

// 初始化系统配置
void initSystemConfig() {
    auto& config = ConfigManager::GetInstance();
    try {
        config.load("configs/system.cfg");
        cout << "[" << getCurrentTime() << "] 系统配置加载成功\n";
    } catch (const exception& e) {
        cerr << "[" << getCurrentTime() << "] 配置加载失败: " << e.what() << endl;
        exit(1);
    }
}

// 加载设备数据
void loadDevices() {
    auto& dm = DeviceManager::GetInstance();
    try {
        dm.Initialize("devices.db");
        cout << "[" << getCurrentTime() << "] 设备数据初始化完成\n";
    } catch (const exception& e) {
        cerr << "[" << getCurrentTime() << "] 设备初始化失败: " << e.what() << endl;
        exit(1);
    }
}

// 用户登录流程
void userLoginFlow() {  
    auto& um = UserManager::GetInstance();
    auto& logger = LogManager::GetInstance();
    
    cout << "\n=== 用户登录 ===" << endl;
    
    while (true) {
        string username, password;
        cout << "用户名: ";
        cin >> username;
        cout << "密码: ";
        cin >> password;
        
        if (um.LoginUser(username, password)) {
            if (auto user = um.GetCurrentUser()) {  // 使用GetCurrentUser
                logger.logAction(LogManager::LogType::UserAction, 
                               user->getId(), 
                               -1, 
                               "用户登录成功");
                return;
            }
        }
        cout << "登录失败，请重试\n";
    }
}

// 构建菜单系统
void setupMenuSystem() {
    auto& menu = MenuManager::GetInstance();
    auto& dm = DeviceManager::GetInstance();
    auto& um = UserManager::GetInstance();
    auto& logger = LogManager::GetInstance();
    
    // 主菜单
    menu.AddMenuItem(1, "设备控制", "管理智能设备", [] {
        DeviceManager::GetInstance().ListAllDevices();
    });
    
    menu.AddMenuItem(2, "系统日志", "查看操作记录", [] {
        try {
            LogManager::GetInstance().saveLogsToFile("system.log");
            system("cat system.log");
        } catch (const exception& e) {
            ExceptionHandler::handleException(e);
        }
    });

    
    // 设备控制子菜单
    auto deviceMenu = make_shared<MenuManager>();
    deviceMenu->AddMenuItem(1, "开关设备", "切换设备状态", [] {
        auto& dm = DeviceManager::GetInstance();
        auto& um = UserManager::GetInstance();
        auto& logger = LogManager::GetInstance();

        // 获取当前用户
        if (auto user = um.GetCurrentUser()) {  // 安全访问
            int deviceId;
            cout << "输入设备ID: ";
            if (!(cin >> deviceId)) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "无效输入\n";
                return;
            }

            // 获取设备状态
            string currentStatus = dm.GetDeviceStatus(deviceId);
            if (currentStatus == "设备不存在") {
                cout << "设备不存在\n";
                return;
            }

            // 切换状态
            string newStatus = (currentStatus == "开启") ? "关闭" : "开启";
            if (dm.UpdateDeviceStatus(deviceId, newStatus)) {
                // 记录日志（使用当前用户ID）
                logger.logAction(
                    LogManager::LogType::DeviceAction,
                    user->getId(),  // 直接使用GetCurrentUser的ID
                    deviceId,
                    "状态变更为: " + newStatus
                );
                cout << "操作成功\n";
            } else {
                cout << "操作失败\n";
            }
        } else {
            cout << "错误：请先登录\n";  // 未登录提示
        }
    });

    // 用户管理菜单
    auto userMenu = make_shared<MenuManager>();
    userMenu->AddMenuItem(1, "注销登录", "退出当前账号", [] {
        if (auto user = UserManager::GetInstance().GetCurrentUser()) {
            UserManager::GetInstance().LogoutUser(user->getId());
            cout << "已注销\n";
        }
    });

    // 挂载子菜单
    menu.AddSubMenu(1, deviceMenu);
    menu.AddMenuItem(3, "用户管理", "账号相关操作", [] {});
    menu.AddSubMenu(3, userMenu);
}

int main() {
    // 系统初始化
    cout << "=== 智能家居控制系统启动 ===" << endl;
    initSystemConfig();
    loadDevices();
    
    // 用户认证
    userLoginFlow();
    
    // 日志系统初始化
    auto& logger = LogManager::GetInstance();
    if (auto user = UserManager::GetInstance().GetCurrentUser()) {  // 安全访问
        logger.logAction(LogManager::LogType::System, 
                       user->getId(), 
                       -1, 
                       "系统启动完成");
    }

    // 菜单系统
    setupMenuSystem();
    auto& menu = MenuManager::GetInstance();
    
    // 主交互循环
    int choice = 0;
    do {
        menu.ShowCurrentMenu();
        cout << "请输入选项 (0返回/99退出): ";
        
        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "无效输入，请重新输入\n";
            continue;
        }
        
        if (choice == 99) break;
        
        if (!menu.ProcessCommand(choice)) {
            cout << "无效命令，请重试\n";
        }
        
        // 每次操作后检查登录状态
        if (UserManager::GetInstance().GetCurrentUser() == nullptr) {
            cout << "检测到登录已失效，请重新登录\n";
            userLoginFlow();  // 重新登录
        }
    } while (true);
    
    // 系统关闭
    if (auto user = UserManager::GetInstance().GetCurrentUser()) {
        logger.logAction(LogManager::LogType::System, 
                       user->getId(), 
                       -1, 
                       "系统正常关闭");
    }
    DeviceManager::GetInstance().Close();
    cout << "\n感谢使用，再见！" << endl;
    
    return 0;
}