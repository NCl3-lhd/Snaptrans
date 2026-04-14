#ifndef WINDOWS_MANAGER_H
#define WINDOWS_MANAGER_H

#include "settings_window.h"
// #include "overlay_window.h" // 预留截图窗口头文件

class WindowManager {
  public:
  static WindowManager &getInstance() {
    static WindowManager instance;
    return instance;
  }

  // 批量操作
  bool initAll();
  void renderAll();
  void rebuildAllFonts();
  void shutdownAll();
  bool hasVisibleWindows() const;

  // 提供路径：让其他类找到具体的子窗口
  SettingsWindow *getSettingsWindow() { return &settings_window_; }
  // OverlayWindow* getOverlayWindow() { return &overlay_window_; } // 预留接口

  private:
  WindowManager() = default;
  ~WindowManager() = default;
  WindowManager(const WindowManager &) = delete;
  WindowManager &operator=(const WindowManager &) = delete;

  SettingsWindow settings_window_;
  // OverlayWindow overlay_window_; // 预留
};

#endif // WINDOWS_MANAGER_H