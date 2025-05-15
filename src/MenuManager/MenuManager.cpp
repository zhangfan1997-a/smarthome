#include "MenuManager.h"
#include <iostream>
#include <iomanip>

MenuManager& MenuManager::GetInstance() {
    static MenuManager instance;
    return instance;
}

void MenuManager::AddMenuItem(int id, const std::string& title, 
                             const std::string& desc, MenuItem::Handler handler) {
    items_.push_back({id, title, desc, handler, nullptr});
    BuildCommandMap();
}

void MenuManager::AddSubMenu(int parentId, std::shared_ptr<MenuManager> submenu) {
    for (auto& item : items_) {
        if (item.id == parentId) {
            item.submenu = submenu;
            break;
        }
    }
}

void MenuManager::ShowCurrentMenu() const {
    if (!menuStack_.empty()) {
        menuStack_.back()->PrintMenu(menuStack_.back()->items_);
    } else {
        PrintMenu(items_);
    }
}

bool MenuManager::ProcessCommand(int commandId) {
    auto* currentMenu = menuStack_.empty() ? this : menuStack_.back().get();
    
    if (commandId == 0 && !menuStack_.empty()) {
        NavigateBack();
        return true;
    }

    auto it = currentMenu->commandMap_.find(commandId);
    if (it != currentMenu->commandMap_.end()) {
        if (it->second.submenu) {
            menuStack_.push_back(it->second.submenu);
            ShowCurrentMenu();
        } else if (it->second.handler) {
            it->second.handler();
        }
        return true;
    }
    return false;
}

void MenuManager::NavigateBack() {
    if (!menuStack_.empty()) {
        menuStack_.pop_back();
        ShowCurrentMenu();
    }
}

void MenuManager::ResetToRoot() {
    menuStack_.clear();
}

void MenuManager::PrintMenu(const std::vector<MenuItem>& menu) const {
    std::cout << "\n=== Smart Home Control Panel ===\n";
    for (const auto& item : menu) {
        std::cout << std::setw(2) << item.id << ") " << item.title << "\n";
        std::cout << "    " << item.description << "\n";
    }
    if (!menuStack_.empty()) {
        std::cout << " 0) Return to previous menu\n";
    }
    std::cout << "--------------------------------\n";
}

void MenuManager::BuildCommandMap() {
    commandMap_.clear();
    for (const auto& item : items_) {
        commandMap_[item.id] = item;
    }
}