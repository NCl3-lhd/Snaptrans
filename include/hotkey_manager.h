#ifndef HOTKEY_MANAGER_H
#define HOTKEY_MANAGER_H

#include <functional>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include "sigslot/signal.hpp"
#include "key_map.h"

struct HotkeyEvent {
  int id;
};

class HotkeyManager {
  public:
  static HotkeyManager &getInstance() {
    static HotkeyManager instance;
    return instance;
  }

  void start();
  void stop();

  bool registerHotkey(int id, KeyModifier mod, KeyCode key);
  void unregisterHotkey(int id);

  template <typename Callable>
  sigslot::connection addCallback(Callable &&callback) {
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

  // 🌟 新增：跨线程热键状态存储，保护动态注册
  struct HotkeyData { KeyModifier mod; KeyCode key; };
  std::unordered_map<int, HotkeyData> m_registeredHotkeys;
  std::mutex m_hotkeyMutex;

#if defined(_WIN32) || defined(_WIN64)
  // Windows 需要记录后台线程 ID 来发送跨线程消息
  std::atomic<unsigned long> m_winThreadId{ 0 };
#elif defined(__APPLE__)
  // macOS 需要记录 RunLoop 引用以便优雅关闭
  std::atomic<void *> m_macRunLoop{ nullptr };
#endif

  void nativeEventLoop();
};
#endif // HOTKEY_MANAGER_H