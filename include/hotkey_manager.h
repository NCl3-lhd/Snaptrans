#ifndef HOTKEY_MANAGER_H
#define HOTKEY_MANAGER_H
#include <functional>
#include <thread>
#include <atomic>
#include <unordered_map>
#include "sigslot/signal.hpp"
// 跨平台按键修饰符
enum class KeyModifier {
  None = 0,
  Alt = 1 << 0,
  Ctrl = 1 << 1,
  Shift = 1 << 2,
  Super = 1 << 3 // 专为 macOS 准备
};
inline KeyModifier operator|(KeyModifier a, KeyModifier b) {
  return static_cast<KeyModifier>(static_cast<int>(a) | static_cast<int>(b));
}

enum class KeyCode {
  // 字母
  A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
  // 数字
  Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
  // 功能键
  F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
  // 控制键
  Space, Enter, Esc, Tab, Backspace,
  // 符号键
  Minus, Equal, LeftBracket, RightBracket, Backslash, Semicolon, Quote, Comma, Period, Slash, Grave
};

// ========================================================
// 🌟 全局事件总线 (C++17 inline 特性，无需在 cpp 中定义)
// ========================================================
struct HotkeyEvent {
  int id;
};

// ========================================================
// 热键管理器 (单例) 通过库实现
// ========================================================
class HotkeyManager {
  public:
  static HotkeyManager &getInstance() {
    static HotkeyManager instance;
    return instance;
  }

  // 启动后台监听线程
  void start();
  // 停止并清理
  void stop();

  // 注册热键
  bool registerHotkey(int id, KeyModifier mod, KeyCode key);
  // 注销热键
  void unregisterHotkey(int id);

  template <typename Callable>
  sigslot::connection addCallback(Callable &&callback) {
    // 底层偷偷调用 sigslot 的 connect，并把传入的 callback 完美转发过去
    return OnHotkeyPressed.connect(std::forward<Callable>(callback));
  }
  void broadcastHotkey(int id);
  private:
  HotkeyManager() = default;
  ~HotkeyManager() { stop(); }
  HotkeyManager(const HotkeyManager &) = delete;
  HotkeyManager &operator=(const HotkeyManager &) = delete;

  sigslot::signal<const HotkeyEvent &> OnHotkeyPressed;
  std::thread m_listenerThread;
  std::atomic<bool> m_running{ false };

  // 平台特定的底层注册实现
  bool nativeRegister(int id, KeyModifier mod, KeyCode key);
  void nativeUnregister(int id);
  void nativeEventLoop(); // 后台线程的死循环
};
#endif