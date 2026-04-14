#ifndef KEY_MAP_H
#define KEY_MAP_H

// 跨平台按键修饰符
enum class KeyModifier {
  None = 0,
  Alt = 1 << 0,
  Ctrl = 1 << 1,
  Shift = 1 << 2,
  Super = 1 << 3 // Win/Cmd
};
inline KeyModifier operator|(KeyModifier a, KeyModifier b) {
  return static_cast<KeyModifier>(static_cast<int>(a) | static_cast<int>(b));
}

// ⚠️ 注意：这个 Enum 的顺序极其重要！下面的映射数组严格依赖这里的顺序。
enum class KeyCode {
  None, // None
  // === 0-25: 字母 ===
  A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
  // === 26-35: 数字 ===
  Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
  // === 36-47: 功能键 ===
  F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
  // === 48-52: 控制键 ===
  Space, Enter, Esc, Tab, Backspace,
  // === 53-63: 符号键 ===
  Minus, Equal, LeftBracket, RightBracket, Backslash, Semicolon, Quote, Comma, Period, Slash, Grave
};

// =====================================================================
// 🪟 Windows 专属按键映射
// =====================================================================
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>

inline UINT mapModifier(KeyModifier mod) {
  UINT winMod = MOD_NOREPEAT; // 防止长按连发
  if ((int)mod & (int)KeyModifier::Alt) winMod |= MOD_ALT;
  if ((int)mod & (int)KeyModifier::Ctrl) winMod |= MOD_CONTROL;
  if ((int)mod & (int)KeyModifier::Shift) winMod |= MOD_SHIFT;
  if ((int)mod & (int)KeyModifier::Super) winMod |= MOD_WIN;
  return winMod;
}

inline UINT mapKey(KeyCode key) {
  // 🌟 Windows 虚拟键码数组 (顺序与 KeyCode 严丝合缝)
  static constexpr UINT kWinKeyMap[] = {
    0, // None
    // 字母 0-25
    'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
    // 数字 26-35
    '0','1','2','3','4','5','6','7','8','9',
    // 功能键 36-47
    VK_F1, VK_F2, VK_F3, VK_F4, VK_F5, VK_F6, VK_F7, VK_F8, VK_F9, VK_F10, VK_F11, VK_F12,
    // 控制键 48-52
    VK_SPACE, VK_RETURN, VK_ESCAPE, VK_TAB, VK_BACK,
    // 符号键 53-63
    VK_OEM_MINUS, VK_OEM_PLUS, VK_OEM_4, VK_OEM_6, VK_OEM_5, VK_OEM_1, VK_OEM_7, VK_OEM_COMMA, VK_OEM_PERIOD, VK_OEM_2, VK_OEM_3
  };

  size_t index = static_cast<size_t>(key);
  // 安全越界检查
  if (index < (sizeof(kWinKeyMap) / sizeof(UINT))) {
    return kWinKeyMap[index];
  }
  return 0; // 未知按键
}


// =====================================================================
// 🍎 macOS 专属按键映射
// =====================================================================
#elif defined(__APPLE__)
#include <Carbon/Carbon.h>

inline UInt32 mapModifier(KeyModifier mod) {
  UInt32 macMod = 0;
  if ((int)mod & (int)KeyModifier::Super)   macMod |= cmdKey;
  if ((int)mod & (int)KeyModifier::Alt)     macMod |= optionKey;
  if ((int)mod & (int)KeyModifier::Ctrl)    macMod |= controlKey;
  if ((int)mod & (int)KeyModifier::Shift)   macMod |= shiftKey;
  return macMod;
}

inline UInt32 mapKey(KeyCode key) {
  // 🌟 macOS Carbon 虚拟键码数组 (顺序与 KeyCode 严丝合缝)
  static constexpr UInt32 kMacKeyMap[] = {
    0, // None
    // 字母 0-25
    kVK_ANSI_A, kVK_ANSI_B, kVK_ANSI_C, kVK_ANSI_D, kVK_ANSI_E, kVK_ANSI_F, kVK_ANSI_G, kVK_ANSI_H, kVK_ANSI_I, kVK_ANSI_J, kVK_ANSI_K, kVK_ANSI_L, kVK_ANSI_M, kVK_ANSI_N, kVK_ANSI_O, kVK_ANSI_P, kVK_ANSI_Q, kVK_ANSI_R, kVK_ANSI_S, kVK_ANSI_T, kVK_ANSI_U, kVK_ANSI_V, kVK_ANSI_W, kVK_ANSI_X, kVK_ANSI_Y, kVK_ANSI_Z,
    // 数字 26-35
    kVK_ANSI_0, kVK_ANSI_1, kVK_ANSI_2, kVK_ANSI_3, kVK_ANSI_4, kVK_ANSI_5, kVK_ANSI_6, kVK_ANSI_7, kVK_ANSI_8, kVK_ANSI_9,
    // 功能键 36-47
    kVK_F1, kVK_F2, kVK_F3, kVK_F4, kVK_F5, kVK_F6, kVK_F7, kVK_F8, kVK_F9, kVK_F10, kVK_F11, kVK_F12,
    // 控制键 48-52 (注意：Mac 的 kVK_Delete 对应 PC 的 Backspace)
    kVK_Space, kVK_Return, kVK_Escape, kVK_Tab, kVK_Delete,
    // 符号键 53-63
    kVK_ANSI_Minus, kVK_ANSI_Equal, kVK_ANSI_LeftBracket, kVK_ANSI_RightBracket, kVK_ANSI_Backslash, kVK_ANSI_Semicolon, kVK_ANSI_Quote, kVK_ANSI_Comma, kVK_ANSI_Period, kVK_ANSI_Slash, kVK_ANSI_Grave
  };

  size_t index = static_cast<size_t>(key);
  // 安全越界检查
  if (index < (sizeof(kMacKeyMap) / sizeof(UInt32))) {
    return kMacKeyMap[index];
  }
  return 0; // 未知按键
}

#endif // 平台宏结束

#endif // KEY_MAP_H