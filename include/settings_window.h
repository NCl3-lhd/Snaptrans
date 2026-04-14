#ifndef SETTINGS_WINDOW_H
#define SETTINGS_WINDOW_H
#include <string>
#include <vector>
#include "base_window.h"
#include "control_window.h" // 🌟 引入你的快捷键子组件
class SettingsWindow : public BaseWindow {
  public:
  SettingsWindow() = default;
  ~SettingsWindow() = default;

  // 初始化：负责扫描本地语言包，设置初始选中的语言
  bool init() override;
  void render() override;

  // 核心渲染函数：每帧调用。
  // 传入引用，当用户切换语言时，通知主循环去重建字体
  void stopRecordingHottkey();
  private:
  
  static void windowCloseCallback(GLFWwindow *window);
  static void windowIconifyCallback(GLFWwindow *window, int iconified);
  static void windowFocusCallback(GLFWwindow *window, int focused);

  // --- 🌟 重构新增：将子选项卡的渲染逻辑抽离为独立函数 ---
  void renderPreferencesTab();
  void renderControlTab();
  void renderAboutTab();

  // 🌟 核心：组合！把快捷键面板作为成员变量装进来
  ControlWindow control_window_;
  // --- UI 状态数据 ---
  std::vector<std::string> languages_;
  int current_language_idx_ = -1;

  bool autoStart_ = false;
  char defaultSavePath_[256] = "C:\\SnapTrans\\Images";
};
#endif // SETTINGS_WINDOW_H