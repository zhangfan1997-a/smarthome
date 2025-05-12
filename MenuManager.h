#ifndef MENU_MANAGER_H
#define MENU_MANAGER_H

#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <unordered_map>

class MenuManager {
public:
    struct MenuItem {
        using Handler = std::function<void()>;
        
        int id;
        std::string title;
        std::string description;
        Handler handler;
        std::shared_ptr<MenuManager> submenu;
    };

    static MenuManager& GetInstance();
    
    void AddMenuItem(int id, const std::string& title, 
                    const std::string& desc, MenuItem::Handler handler);
    void AddSubMenu(int parentId, std::shared_ptr<MenuManager> submenu);
    void ShowCurrentMenu() const;
    bool ProcessCommand(int commandId);
    void NavigateBack();
    void ResetToRoot();

private:
    MenuManager() = default;
    
    std::vector<MenuItem> items_;
    std::unordered_map<int, MenuItem> commandMap_;
    std::vector<std::shared_ptr<MenuManager>> menuStack_;
    
    void PrintMenu(const std::vector<MenuItem>& menu) const;
    void BuildCommandMap();
};

#endif // MENU_MANAGER_H