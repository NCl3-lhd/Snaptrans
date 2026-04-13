#ifndef MAC_KEY_MAP_H
#define MAC_KEY_MAP_H

#ifdef __APPLE__
#include <Carbon/Carbon.h>
#include <unordered_map>
#include "hotkey_manager.h"
// =====================================================================
// 🍎 macOS 专属按键映射表 (利用 C++17 inline 特性，安全的 Header-Only)
// =====================================================================

inline const std::unordered_map<KeyCode, UInt32> g_macKeyMap = {
  // === 26个英文字母 ===
  {KeyCode::A, kVK_ANSI_A}, {KeyCode::B, kVK_ANSI_B}, {KeyCode::C, kVK_ANSI_C},
  {KeyCode::D, kVK_ANSI_D}, {KeyCode::E, kVK_ANSI_E}, {KeyCode::F, kVK_ANSI_F},
  {KeyCode::G, kVK_ANSI_G}, {KeyCode::H, kVK_ANSI_H}, {KeyCode::I, kVK_ANSI_I},
  {KeyCode::J, kVK_ANSI_J}, {KeyCode::K, kVK_ANSI_K}, {KeyCode::L, kVK_ANSI_L},
  {KeyCode::M, kVK_ANSI_M}, {KeyCode::N, kVK_ANSI_N}, {KeyCode::O, kVK_ANSI_O},
  {KeyCode::P, kVK_ANSI_P}, {KeyCode::Q, kVK_ANSI_Q}, {KeyCode::R, kVK_ANSI_R},
  {KeyCode::S, kVK_ANSI_S}, {KeyCode::T, kVK_ANSI_T}, {KeyCode::U, kVK_ANSI_U},
  {KeyCode::V, kVK_ANSI_V}, {KeyCode::W, kVK_ANSI_W}, {KeyCode::X, kVK_ANSI_X},
  {KeyCode::Y, kVK_ANSI_Y}, {KeyCode::Z, kVK_ANSI_Z},

  // === 主键盘区数字 (0-9) ===
  {KeyCode::Num0, kVK_ANSI_0}, {KeyCode::Num1, kVK_ANSI_1},
  {KeyCode::Num2, kVK_ANSI_2}, {KeyCode::Num3, kVK_ANSI_3},
  {KeyCode::Num4, kVK_ANSI_4}, {KeyCode::Num5, kVK_ANSI_5},
  {KeyCode::Num6, kVK_ANSI_6}, {KeyCode::Num7, kVK_ANSI_7},
  {KeyCode::Num8, kVK_ANSI_8}, {KeyCode::Num9, kVK_ANSI_9},

  // === 常用标点符号 ===
  {KeyCode::Minus, kVK_ANSI_Minus},               // -
  {KeyCode::Equal, kVK_ANSI_Equal},               // =
  {KeyCode::LeftBracket, kVK_ANSI_LeftBracket},   // [
  {KeyCode::RightBracket, kVK_ANSI_RightBracket}, // ]
  {KeyCode::Backslash, kVK_ANSI_Backslash},       // '\\' 反斜杠 
  {KeyCode::Semicolon, kVK_ANSI_Semicolon},       // ;
  {KeyCode::Quote, kVK_ANSI_Quote},               // '
  {KeyCode::Comma, kVK_ANSI_Comma},               // ,
  {KeyCode::Period, kVK_ANSI_Period},             // .
  {KeyCode::Slash, kVK_ANSI_Slash},               // /
  {KeyCode::Grave, kVK_ANSI_Grave},               // ` (波浪号那个键)

  // === 常用功能键 (F1-F12) ===
  {KeyCode::F1, kVK_F1}, {KeyCode::F2, kVK_F2}, {KeyCode::F3, kVK_F3},
  {KeyCode::F4, kVK_F4}, {KeyCode::F5, kVK_F5}, {KeyCode::F6, kVK_F6},
  {KeyCode::F7, kVK_F7}, {KeyCode::F8, kVK_F8}, {KeyCode::F9, kVK_F9},
  {KeyCode::F10, kVK_F10}, {KeyCode::F11, kVK_F11}, {KeyCode::F12, kVK_F12},

  // === 常用控制键 ===
  {KeyCode::Space, kVK_Space},
  {KeyCode::Enter, kVK_Return},
  {KeyCode::Esc, kVK_Escape},
  {KeyCode::Tab, kVK_Tab},
  {KeyCode::Backspace, kVK_Delete} // 注意：Mac键盘上的 Delete 实际上等于 Windows 的 Backspace
};

// =====================================================================
// 映射转换内联函数
// =====================================================================

// 将跨平台 KeyCode 转换为 macOS 虚拟键码
inline UInt32 mapMacKey(KeyCode key) {
  auto it = g_macKeyMap.find(key);
  return (it != g_macKeyMap.end()) ? it->second : 0;
}

// 将跨平台 KeyModifier 转换为 macOS 修饰键掩码
inline UInt32 mapMacModifier(KeyModifier mod) {
  UInt32 macMod = 0;
  if ((int)mod & (int)KeyModifier::Super)   macMod |= cmdKey;
  if ((int)mod & (int)KeyModifier::Alt)   macMod |= optionKey;
  if ((int)mod & (int)KeyModifier::Ctrl)  macMod |= controlKey;
  if ((int)mod & (int)KeyModifier::Shift) macMod |= shiftKey;
  return macMod;
}

#endif // __APPLE__
#endif