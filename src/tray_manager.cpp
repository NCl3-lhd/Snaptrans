#include "tray_manager.h"
#include <iostream>
#include <cstring>

namespace SnapTrans {

TrayManager& TrayManager::getInstance() {
    static TrayManager instance;
    return instance;
}

bool TrayManager::initialize(GLFWwindow* window, const std::string& iconPath) {
    window_ = window;
    
    // Store icon path (must remain valid throughout tray lifetime)
    storedIconPath_ = iconPath;
    tray_.icon = const_cast<char*>(storedIconPath_.c_str());
    setupMenu();
    
    if (tray_init(&tray_) < 0) {
        std::cerr << "Failed to initialize system tray" << std::endl;
        return false;
    }
    
    running_ = true;
    std::cout << "System tray initialized successfully" << std::endl;
    return true;
}

void TrayManager::update() {
    if (!running_) return;
    
    int result = tray_loop(0);
    if (result != 0) {
        running_ = false;
    }
}

void TrayManager::shutdown() {
    if (running_) {
        tray_exit();
        running_ = false;
    }
}

void TrayManager::setWindowVisible(bool visible) {
    if (!window_) return;
    
    windowVisible_ = visible;
    if (visible) {
        glfwShowWindow(window_);
        glfwFocusWindow(window_);
    } else {
        glfwHideWindow(window_);
    }
    
    tray_update(&tray_);
}

void TrayManager::toggleWindowVisibility() {
    setWindowVisible(!windowVisible_);
}

void TrayManager::setupMenu() {
    int idx = 0;
    
    menuItems_[idx].text = const_cast<char*>("显示/隐藏");
    menuItems_[idx].disabled = 0;
    menuItems_[idx].checked = 0;
    menuItems_[idx].cb = onShowHideClicked;
    menuItems_[idx].context = this;
    menuItems_[idx].submenu = nullptr;
    idx++;
    
    menuItems_[idx].text = const_cast<char*>("设置");
    menuItems_[idx].disabled = 0;
    menuItems_[idx].checked = 0;
    menuItems_[idx].cb = onSettingsClicked;
    menuItems_[idx].context = this;
    menuItems_[idx].submenu = nullptr;
    idx++;
    
    menuItems_[idx].text = const_cast<char*>("-");
    menuItems_[idx].disabled = 0;
    menuItems_[idx].checked = 0;
    menuItems_[idx].cb = nullptr;
    menuItems_[idx].context = nullptr;
    menuItems_[idx].submenu = nullptr;
    idx++;
    
    menuItems_[idx].text = const_cast<char*>("关于");
    menuItems_[idx].disabled = 0;
    menuItems_[idx].checked = 0;
    menuItems_[idx].cb = onAboutClicked;
    menuItems_[idx].context = this;
    menuItems_[idx].submenu = nullptr;
    idx++;
    
    menuItems_[idx].text = const_cast<char*>("-");
    menuItems_[idx].disabled = 0;
    menuItems_[idx].checked = 0;
    menuItems_[idx].cb = nullptr;
    menuItems_[idx].context = nullptr;
    menuItems_[idx].submenu = nullptr;
    idx++;
    
    menuItems_[idx].text = const_cast<char*>("退出");
    menuItems_[idx].disabled = 0;
    menuItems_[idx].checked = 0;
    menuItems_[idx].cb = onQuitClicked;
    menuItems_[idx].context = this;
    menuItems_[idx].submenu = nullptr;
    idx++;
    
    menuItems_[idx].text = nullptr;
    menuItems_[idx].disabled = 0;
    menuItems_[idx].checked = 0;
    menuItems_[idx].cb = nullptr;
    menuItems_[idx].context = nullptr;
    menuItems_[idx].submenu = nullptr;
    
    tray_.menu = menuItems_;
}

void TrayManager::onShowHideClicked(struct tray_menu* item) {
    TrayManager* manager = static_cast<TrayManager*>(item->context);
    if (manager) {
        manager->toggleWindowVisibility();
    }
}

void TrayManager::onSettingsClicked(struct tray_menu* item) {
    std::cout << "Settings clicked - TODO: implement settings dialog" << std::endl;
}

void TrayManager::onAboutClicked(struct tray_menu* item) {
    std::cout << "SnapTrans - Screen Translation Tool" << std::endl;
    std::cout << "Version: 0.1.0" << std::endl;
}

void TrayManager::onQuitClicked(struct tray_menu* item) {
    TrayManager* manager = static_cast<TrayManager*>(item->context);
    if (manager) {
        if (manager->exitCallback_) {
            manager->exitCallback_();
        }
        manager->shutdown();
    }
}

}
