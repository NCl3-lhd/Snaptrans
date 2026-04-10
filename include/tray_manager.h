#ifndef TRAY_MANAGER_H
#define TRAY_MANAGER_H

#include <functional>
#include <string>
#include <array>
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

// 阅后即焚：防止宏污染外部文件
#if defined(TRAY_APPKIT)
#undef objc_msgSend 
#endif

class TrayManager {
  public:
  using MenuCallback = std::function<void()>;

  static TrayManager &getInstance();
  bool initialize(GLFWwindow *window, const std::string &iconPath);
  void update();
  void shutdown();
  void setWindowVisible(bool visible);
  void toggleWindowVisibility();
  bool isRunning() const { return running_; }
  void setExitCallback(MenuCallback callback) { exitCallback_ = callback; }
  void rebuildMenu();

  private:
  TrayManager() = default;
  ~TrayManager() = default;
  TrayManager(const TrayManager &) = delete;
  TrayManager &operator=(const TrayManager &) = delete;

  static void onShowHideClicked(struct tray_menu *item);
  static void onSettingsClicked(struct tray_menu *item);
  static void onAboutClicked(struct tray_menu *item);
  static void onQuitClicked(struct tray_menu *item);

  void setupMenu();

  private:
  GLFWwindow *window_ = nullptr;
  bool running_ = false;
  bool windowVisible_ = false; // 默认启动时隐藏
  struct tray tray_;
  MenuCallback exitCallback_;
  std::string storedIconPath_;

  enum MenuIndex : size_t {
    ShowHide = 0,
    Settings = 1,
    Separator1 = 2,
    About = 3,
    Separator2 = 4,
    Quit = 5,
    Terminator = 6,
    MAX_MENU_ITEMS = 15
  };

  std::array<std::string, MAX_MENU_ITEMS> menuTexts_;
  std::array<struct tray_menu, MAX_MENU_ITEMS> menuItems_;
};

#endif // TRAY_MANAGER_H