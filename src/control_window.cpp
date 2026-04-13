#include "control_window.h"
#include "imgui.h"
#include "i18n_manager.h"
#include "config.h"

#include <cstdlib> // 提供给 Mac 使用 std::system 

// 🌟 跨平台宏：如果当前是 Windows 系统，则包含 Windows.h
#ifdef _WIN32
#include <windows.h>
#endif
// 假设全局的翻译函数
// extern std::string tr(const std::string &key);

// 🌟 新增：严格的物理主键白名单拦截器
static bool isValidMainKey(int imgui_key) {
  // 允许字母 A-Z
  if (imgui_key >= ImGuiKey_A && imgui_key <= ImGuiKey_Z) return true;
  // 允许数字 0-9 (包括小键盘)
  if (imgui_key >= ImGuiKey_0 && imgui_key <= ImGuiKey_9) return true;
  if (imgui_key >= ImGuiKey_Keypad0 && imgui_key <= ImGuiKey_Keypad9) return true;
  // 允许功能键 F1-F12
  if (imgui_key >= ImGuiKey_F1 && imgui_key <= ImGuiKey_F12) return true;

  // 允许常用控制键和符号键
  switch (imgui_key) {
    case ImGuiKey_Space:
    case ImGuiKey_Enter:
    case ImGuiKey_KeypadEnter:
    case ImGuiKey_Escape:
    case ImGuiKey_Tab:
    case ImGuiKey_Backspace:
    case ImGuiKey_Minus:
    case ImGuiKey_Equal:
    case ImGuiKey_LeftBracket:
    case ImGuiKey_RightBracket:
    case ImGuiKey_Backslash:
    case ImGuiKey_Semicolon:
    case ImGuiKey_Apostrophe:
    case ImGuiKey_Comma:
    case ImGuiKey_Period:
    case ImGuiKey_Slash:
    case ImGuiKey_GraveAccent:
      return true;
    default:
      return false; // 鼠标点击、手柄、修饰键全部会被挡在这里！
  }
}

// 🌟 新增：提取出来的统一取消逻辑
void ControlWindow::cancelRecording() {
  if (recording_hotkey_id_ == screenshot_hotkey_.id) {
    HotkeyManager::getInstance().registerHotkey(screenshot_hotkey_.id, screenshot_hotkey_.mod, screenshot_hotkey_.key);
  }
  else if (recording_hotkey_id_ == translate_hotkey_.id) {
    HotkeyManager::getInstance().registerHotkey(translate_hotkey_.id, translate_hotkey_.mod, translate_hotkey_.key);
  }
  recording_hotkey_id_ = -1;
}

void ControlWindow::init() {
  // 初始化快捷键默认值
  screenshot_hotkey_ = { Config::SCREENSHOT_EVENT_ID, Config::SCREENSHOT_KEYMODIFIER, Config::SCREENSHOT_KEYCODE, formatHotkeyName(Config::SCREENSHOT_KEYMODIFIER, Config::SCREENSHOT_KEYCODE) };
  translate_hotkey_ = { Config::TRANSLATION_EVENT_ID, Config::TRANSLATION_KEYMODIFIER, Config::TRANSLATION_KEYCODE, formatHotkeyName(Config::TRANSLATION_KEYMODIFIER, Config::TRANSLATION_KEYCODE) };
  // translate_hotkey_ = { 2, KeyModifier::Alt, KeyCode::E, "Alt + E" };

  // // 向系统底层注册
  HotkeyManager::getInstance().registerHotkey(screenshot_hotkey_.id, screenshot_hotkey_.mod, screenshot_hotkey_.key);
  HotkeyManager::getInstance().registerHotkey(translate_hotkey_.id, translate_hotkey_.mod, translate_hotkey_.key);
}

void ControlWindow::render() {
  std::string prefix = "main_window.";

  ImGui::Spacing(); ImGui::Spacing();

  // --- 截图快捷键 UI ---
  ImGui::Text("%s:", tr(prefix + "control.screenshot_hotkey"));
  ImGui::SameLine(150);

  if (recording_hotkey_id_ == screenshot_hotkey_.id) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
    ImGui::Button("请按下新快捷键 (ESC取消)...", ImVec2(220, 0));
    ImGui::PopStyleColor();
  }
  else {
    if (ImGui::Button(screenshot_hotkey_.display_name.c_str(), ImVec2(220, 0))) {
      recording_hotkey_id_ = screenshot_hotkey_.id;
      HotkeyManager::getInstance().unregisterHotkey(screenshot_hotkey_.id);
    }
  }

  ImGui::Spacing();

  // --- 翻译快捷键 UI ---
  ImGui::Text("%s:", tr(prefix + "control.translate_hotkey"));
  ImGui::SameLine(150);

  if (recording_hotkey_id_ == translate_hotkey_.id) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
    ImGui::Button("请按下新快捷键 (ESC取消)...", ImVec2(220, 0));
    ImGui::PopStyleColor();
  }
  else {
    if (ImGui::Button(translate_hotkey_.display_name.c_str(), ImVec2(220, 0))) {
      recording_hotkey_id_ = translate_hotkey_.id;
      HotkeyManager::getInstance().unregisterHotkey(translate_hotkey_.id);
    }
  }

  // 🌟 增强版状态机轮询
  // if (recording_hotkey_id_ != -1) {
  //   // 焦点检测：如果窗口失去了焦点（用户切到了别的软件、最小化了等）
  //   // ImGuiFocusedFlags_RootAndChildWindows 确保焦点在主窗口或其子窗口内
  //   if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
  //     cancelRecording(); // 自动取消录制，恢复原样
  //   }
  //   else {
  //     processHotkeyRecording(); // 正常拦截按键
  //   }
  // }
  // if (recording_hotkey_id_ != -1) {
  //   // ImGuiFocusedFlags_AnyWindow 意味着：只要操作系统把焦点切给了微信、Chrome等别的软件，立刻打断
  //   if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow)) {
  //     cancelRecording();
  //   }
  //   else {
  //     processHotkeyRecording();
  //   }
  // }
  if (recording_hotkey_id_ != -1) {
    processHotkeyRecording(); // 直接调用拦截器，所有的取消逻辑都在拦截器内部判断
  }
}

void ControlWindow::processHotkeyRecording() {
  ImGuiIO &io = ImGui::GetIO();

  // 1. 记录当前按下的修饰键
  int current_mod = 0;
#if  defined(__APPLE__)
  if (io.KeyCtrl)  current_mod |= (int)KeyModifier::Super;
  if (io.KeySuper) current_mod |= (int)KeyModifier::Ctrl;
#else
  if (io.KeyCtrl)  current_mod |= (int)KeyModifier::Ctrl;
  if (io.KeySuper) current_mod |= (int)KeyModifier::Super;
#endif
  if (io.KeyShift) current_mod |= (int)KeyModifier::Shift;
  if (io.KeyAlt)   current_mod |= (int)KeyModifier::Alt;


  // 2. 终极防呆取消逻辑 (ESC + 鼠标点击 + 失去焦点)
  bool focus_lost = !ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows); // 窗口失去焦点（比如切到了别的软件）
  bool mouse_clicked = ImGui::IsMouseClicked(0) || ImGui::IsMouseClicked(1);        // 鼠标左键或右键被点击

  if (ImGui::IsKeyPressed(ImGuiKey_Escape) || focus_lost || mouse_clicked) {
    cancelRecording();
    return;
  }

  // 3. 拦截有效的主键
  for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; key++) {
    // 💡 绝杀：只有按下的键存在于我们的“白名单”中，才认为是合法的快捷键输入！
    if (ImGui::IsKeyPressed((ImGuiKey)key) && isValidMainKey(key)) {

      KeyCode new_key = mapImGuiKeyToMyKeyCode(key);
      KeyModifier new_mod = static_cast<KeyModifier>(current_mod);
      std::string new_name = formatHotkeyName(new_mod, new_key);

      bool success = false;

      // 更新对应功能的快捷键数据，并向底层重新注册
      if (recording_hotkey_id_ == screenshot_hotkey_.id) {
        success = HotkeyManager::getInstance().registerHotkey(screenshot_hotkey_.id, new_mod, new_key);
        if (success) {
          screenshot_hotkey_.mod = new_mod;
          screenshot_hotkey_.key = new_key;
          screenshot_hotkey_.display_name = new_name;
          // 架构提示：如果你最终使用了 ConfigManager，记得在这里同步一下数据
          // ConfigManager::getInstance().screenshotHotkey.mod = new_mod;
          // ConfigManager::getInstance().screenshotHotkey.key = new_key;
        }
      }
      else if (recording_hotkey_id_ == translate_hotkey_.id) {
        success = HotkeyManager::getInstance().registerHotkey(translate_hotkey_.id, new_mod, new_key);
        if (success) {
          translate_hotkey_.mod = new_mod;
          translate_hotkey_.key = new_key;
          translate_hotkey_.display_name = new_name;
          // 架构提示：同上，同步全局配置
          // ConfigManager::getInstance().translateHotkey.mod = new_mod;
          // ConfigManager::getInstance().translateHotkey.key = new_key;
        }
      }
      // std::cerr << success << "\n";
      // 🌟 错误处理分支
      if (!success) {
        // 1. 恢复原本的快捷键
        cancelRecording();
        // 2. 跨平台播放系统错误提示音
#if defined(_WIN32) || defined(_WIN64)
        // Windows 环境：播放系统标准的“错误/停止”音效 (无需额外音频文件)
        MessageBeep(MB_ICONERROR);
#elif defined(__APPLE__)
        // macOS 环境：后台播放系统自带的 Basso 提示音
        std::system("afplay /System/Library/Sounds/Basso.aiff &");
#else
        // Linux 兜底 (如果你以后要支持的话，通常用响铃符)
        // printf("\a"); 
#endif
      }
      else {
        // 如果成功，正常结束录制
        recording_hotkey_id_ = -1;
      }
      // recording_hotkey_id_ = -1; // 录制完美结束，重置状态
      break;
    }
  }
}

// ... formatHotkeyName 和 mapImGuiKeyToMyKeyCode 辅助函数的实现和之前一样 ...
// 辅助函数：将按键组合格式化为给人看的字符串
std::string ControlWindow::formatHotkeyName(KeyModifier mod, KeyCode key) {
  std::string name = "";
  int m = static_cast<int>(mod);

  // 1. 处理修饰键前缀
  if (m & static_cast<int>(KeyModifier::Ctrl))  name += "Ctrl + ";
  if (m & static_cast<int>(KeyModifier::Shift)) name += "Shift + ";
  if (m & static_cast<int>(KeyModifier::Alt))   name += "Alt + ";
#if defined(__APPLE__)
  if (m & static_cast<int>(KeyModifier::Super)) name += "Cmd + ";
#elif defined(_WIN32) || defined(_WIN64)
  if (m & static_cast<int>(KeyModifier::Super)) name += "Win + ";
#endif
  // 2. 处理主键 (利用 C++ 枚举的连续性进行巧妙转换)

  // 处理字母 A-Z
  if (key >= KeyCode::A && key <= KeyCode::Z) {
    char c = 'A' + (static_cast<int>(key) - static_cast<int>(KeyCode::A));
    name += c;
    return name;
  }

  // 处理数字 Num0-Num9
  if (key >= KeyCode::Num0 && key <= KeyCode::Num9) {
    char c = '0' + (static_cast<int>(key) - static_cast<int>(KeyCode::Num0));
    name += c;
    return name;
  }

  // 处理功能键 F1-F12
  if (key >= KeyCode::F1 && key <= KeyCode::F12) {
    int f_num = 1 + (static_cast<int>(key) - static_cast<int>(KeyCode::F1));
    name += "F" + std::to_string(f_num);
    return name;
  }

  // 3. 处理无法用数学推导的控制键和符号键
  switch (key) {
    case KeyCode::Space:        name += "Space"; break;
    case KeyCode::Enter:        name += "Enter"; break;
    case KeyCode::Esc:          name += "Esc"; break;
    case KeyCode::Tab:          name += "Tab"; break;
    case KeyCode::Backspace:    name += "Backspace"; break;

    case KeyCode::Minus:        name += "-"; break;
    case KeyCode::Equal:        name += "="; break;
    case KeyCode::LeftBracket:  name += "["; break;
    case KeyCode::RightBracket: name += "]"; break;
    case KeyCode::Backslash:    name += "\\"; break;
    case KeyCode::Semicolon:    name += ";"; break;
    case KeyCode::Quote:        name += "'"; break;
    case KeyCode::Comma:        name += ","; break;
    case KeyCode::Period:       name += "."; break;
    case KeyCode::Slash:        name += "/"; break;
    case KeyCode::Grave:        name += "`"; break;

    default:                    name += "UnmappedKey"; break;
  }

  return name;
}


// 辅助函数：将 ImGui 按键映射为你自己底层的 KeyCode
KeyCode ControlWindow::mapImGuiKeyToMyKeyCode(int imgui_key) {
  // 1. 处理连续区间的映射 (大幅减少代码量)

  // 映射字母 A-Z
  if (imgui_key >= ImGuiKey_A && imgui_key <= ImGuiKey_Z) {
    return static_cast<KeyCode>(static_cast<int>(KeyCode::A) + (imgui_key - ImGuiKey_A));
  }

  // 映射键盘顶部数字 0-9
  if (imgui_key >= ImGuiKey_0 && imgui_key <= ImGuiKey_9) {
    return static_cast<KeyCode>(static_cast<int>(KeyCode::Num0) + (imgui_key - ImGuiKey_0));
  }
  // 映射小键盘数字 0-9 (如果用户用了小键盘输入)
  if (imgui_key >= ImGuiKey_Keypad0 && imgui_key <= ImGuiKey_Keypad9) {
    return static_cast<KeyCode>(static_cast<int>(KeyCode::Num0) + (imgui_key - ImGuiKey_Keypad0));
  }

  // 映射功能键 F1-F12
  if (imgui_key >= ImGuiKey_F1 && imgui_key <= ImGuiKey_F12) {
    return static_cast<KeyCode>(static_cast<int>(KeyCode::F1) + (imgui_key - ImGuiKey_F1));
  }

  // 2. 映射单独的控制键和符号键 (ImGui 的名字有时候比较特别)
  switch (imgui_key) {
    // 控制键
    case ImGuiKey_Space:        return KeyCode::Space;
    case ImGuiKey_Enter:
    case ImGuiKey_KeypadEnter:  return KeyCode::Enter; // 大小键盘的回车统一映射
    case ImGuiKey_Escape:       return KeyCode::Esc;
    case ImGuiKey_Tab:          return KeyCode::Tab;
    case ImGuiKey_Backspace:    return KeyCode::Backspace;

      // 符号键
    case ImGuiKey_Minus:        return KeyCode::Minus;
    case ImGuiKey_Equal:        return KeyCode::Equal;
    case ImGuiKey_LeftBracket:  return KeyCode::LeftBracket;
    case ImGuiKey_RightBracket: return KeyCode::RightBracket;
    case ImGuiKey_Backslash:    return KeyCode::Backslash;
    case ImGuiKey_Semicolon:    return KeyCode::Semicolon;
    case ImGuiKey_Apostrophe:   return KeyCode::Quote; // ImGui 把单引号叫 Apostrophe
    case ImGuiKey_Comma:        return KeyCode::Comma;
    case ImGuiKey_Period:       return KeyCode::Period;
    case ImGuiKey_Slash:        return KeyCode::Slash;
    case ImGuiKey_GraveAccent:  return KeyCode::Grave; // ImGui 把波浪号下面的反引号叫 GraveAccent

      // 兜底策略
    default:                    return KeyCode::A;
  }
}

bool ControlWindow::isRecording() {
  return recording_hotkey_id_ != -1;
}