#ifndef CONTROL_WINDOW_H
#define CONTROL_WINDOW_H

#include <string>
#include "hotkey_manager.h" // 引入底层管理器以便注册/注销热键

// UI 用的快捷键显示结构体
struct UIHotkeyConfig {
  int id;
  KeyModifier mod;
  KeyCode key;
  std::string display_name;
};

class ControlWindow {
  public:
  ControlWindow() = default;
  ~ControlWindow() = default;

  // 初始化：设定默认快捷键并注册到底层
  void init();

  // 渲染 UI：被主窗口在对应的 Tab 里调用
  void render();
  bool isRecording();
  void cancelRecording(); // 把 ESC 取消和失去焦点取消的逻辑统一
  private:
  // 核心逻辑：处理键盘拦截
  void processHotkeyRecording(bool is_hovered);
  // 辅助转换函数
  std::string formatHotkeyName(KeyModifier mod, KeyCode key);
  KeyCode mapImGuiKeyToMyKeyCode(int imgui_key);

  // --- 快捷键状态数据 ---
  UIHotkeyConfig screenshot_hotkey_;
  UIHotkeyConfig translate_hotkey_;

  // 录制状态机：-1 表示空闲，其它数字代表正在录制的快捷键 ID
  int recording_hotkey_id_ = -1;
};

#endif // CONTROL_WINDOW_H