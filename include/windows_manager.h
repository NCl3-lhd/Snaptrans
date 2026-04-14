#ifndef WINDOWS_MANAGER_H
#define WINDOWS_MANAGER_H

#include <GLFW/glfw3.h>
#include "imgui.h"
#include <string>
#include <atomic>
#include "settings_window.h"

class WindowManager {
  public:
  static WindowManager &getInstance();

  // 初始化整个底层视窗与 UI 上下文
  bool init();

  // 主渲染调度循环
  void runLoop();

  // 销毁并释放资源
  void shutdown();

  // ======= 状态机调度控制 =======
  void setShouldQuit();
  void setSettingsVisible(bool visible);
  void toggleSettingsVisible();

  // 跨线程安全触发接口
  void triggerScreenshot();
  void triggerTranslation();

  private:
  WindowManager() = default;
  ~WindowManager() = default;

  // 严禁拷贝
  WindowManager(const WindowManager &) = delete;
  WindowManager &operator=(const WindowManager &) = delete;

  // 🌟 核心：将 GLFW C风格全局回调转为类的私有静态方法，利用 UserPointer 分发
  static void settingsWindowCloseCallback(GLFWwindow *window);
  static void settingsWindowIconifyCallback(GLFWwindow *window, int iconified);
  static void settingsWindowFocusCallback(GLFWwindow *window, int focused);

  // 内部初始化拆分
  bool initSettingsWindow();
  bool initOverlayWindow();
  void rebuildFonts();
  void renderSettingsFrame();

  private:
  // ======= 双原生窗口与独立上下文 =======
  GLFWwindow *settings_window_ = nullptr;
  GLFWwindow *overlay_window_ = nullptr; // 用于全屏截图层
  ImGuiContext *ctx_settings_ = nullptr;
  ImGuiContext *ctx_overlay_ = nullptr;

  // ======= 状态机调度机制 =======
  bool show_settings_ = false;
  bool is_screenshotting_ = false;
  bool show_translation_ = false;
  bool should_quit_ = false;

  // ======= 跨线程信号 =======
  std::atomic<bool> signal_screenshot_{ false };
  std::atomic<bool> signal_translation_{ false };

  // ======= UI 模块与数据 =======
  SettingsWindow settingsWin_;
  bool need_font_rebuild_ = true;
  std::string current_locale_;
};

#endif // WINDOW_MANAGER_H