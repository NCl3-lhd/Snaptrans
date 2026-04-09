#ifndef TRAY_MANAGER_H
#define TRAY_MANAGER_H

#include <functional>
#include <string>
#include <GLFW/glfw3.h>

// 平台检测并定义对应的 tray 宏
#if defined(__APPLE__) || defined(__MACH__)
  #ifndef TRAY_APPKIT
    #define TRAY_APPKIT 1
  #endif
#elif defined(_WIN32) || defined(_WIN64)
  #ifndef TRAY_WINAPI
    #define TRAY_WINAPI 1
  #endif
#elif defined(__linux__) || defined(linux) || defined(__linux)
  #ifndef TRAY_APPINDICATOR
    #define TRAY_APPINDICATOR 1
  #endif
#endif

// macOS: 修复 objc_msgSend 调用问题
#if defined(TRAY_APPKIT)
  #include <objc/message.h>
  
  // 定义 objc_msgSend 的正确函数指针类型
  template<typename ReturnType, typename... Args>
  static inline ReturnType safe_objc_msgSend(id self, SEL op, Args... args) {
    using MsgSendFunc = ReturnType(*)(id, SEL, Args...);
    return ((MsgSendFunc)objc_msgSend)(self, op, args...);
  }
  
  #define objc_msgSend safe_objc_msgSend<id>
#endif

// 包含 tray 库（C 语言库）
extern "C" {
#include "tray.h"
}

namespace SnapTrans {

/**
 * @brief 系统托盘管理器
 * 
 * 封装 tray 库，提供简洁的 C++ 接口来管理系统托盘图标和菜单。
 * 与 GLFW 窗口系统集成，支持最小化到托盘等功能。
 */
class TrayManager {
public:
    using MenuCallback = std::function<void()>;
    
    /**
     * @brief 获取单例实例
     */
    static TrayManager& getInstance();
    
    /**
     * @brief 初始化系统托盘
     * @param window 关联的 GLFW 窗口指针
     * @param iconPath 托盘图标路径
     * @return 成功返回 true，失败返回 false
     */
    bool initialize(GLFWwindow* window, const std::string& iconPath = "icons/tray.png");
    
    /**
     * @brief 更新托盘状态（非阻塞）
     * 
     * 应在 GLFW 主循环中调用，处理托盘事件
     */
    void update();
    
    /**
     * @brief 清理并退出托盘
     */
    void shutdown();
    
    /**
     * @brief 设置窗口可见性
     */
    void setWindowVisible(bool visible);
    
    /**
     * @brief 切换窗口显示/隐藏
     */
    void toggleWindowVisibility();
    
    /**
     * @brief 检查是否正在运行
     */
    bool isRunning() const { return running_; }
    
    /**
     * @brief 设置退出回调
     */
    void setExitCallback(MenuCallback callback) { exitCallback_ = callback; }
    
private:
    TrayManager() = default;
    ~TrayManager() = default;
    TrayManager(const TrayManager&) = delete;
    TrayManager& operator=(const TrayManager&) = delete;
    
    // 托盘菜单回调（静态函数，用于 C 接口）
    static void onShowHideClicked(struct tray_menu* item);
    static void onSettingsClicked(struct tray_menu* item);
    static void onAboutClicked(struct tray_menu* item);
    static void onQuitClicked(struct tray_menu* item);
    
    // 初始化托盘菜单
    void setupMenu();
    
private:
    GLFWwindow* window_ = nullptr;
    bool running_ = false;
    bool windowVisible_ = true;
    struct tray tray_;
    MenuCallback exitCallback_;
    std::string storedIconPath_;
    
    std::string menuTexts_[10];
    struct tray_menu menuItems_[10];
};

} // namespace SnapTrans

#endif // TRAY_MANAGER_H
