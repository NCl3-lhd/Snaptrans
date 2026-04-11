#include "hotkey_manager.h"
#include <iostream>

#if defined(_WIN32) || defined(_WIN64)
// =====================================================================
// 🪟 Windows 平台实现
// =====================================================================
#include <windows.h>

UINT mapModifier(KeyModifier mod) {
  UINT winMod = MOD_NOREPEAT;
  if ((int)mod & (int)KeyModifier::Alt) winMod |= MOD_ALT;
  if ((int)mod & (int)KeyModifier::Ctrl) winMod |= MOD_CONTROL;
  if ((int)mod & (int)KeyModifier::Shift) winMod |= MOD_SHIFT;
  return winMod;
}

UINT mapKey(KeyCode key) {
  switch (key) {
    case KeyCode::A: return 'A';
    case KeyCode::B: return 'B';
    case KeyCode::C: return 'C';
    case KeyCode::F1: return VK_F1;
    case KeyCode::Space: return VK_SPACE;
    default: return 0;
  }
}

void HotkeyManager::start() {
  if (m_running) return;
  m_running = true;
  m_listenerThread = std::thread(&HotkeyManager::nativeEventLoop, this);
}

void HotkeyManager::stop() {
  if (!m_running) return;
  m_running = false;
  PostThreadMessage(GetThreadId(m_listenerThread.native_handle()), WM_QUIT, 0, 0);
  if (m_listenerThread.joinable()) {
    m_listenerThread.join();
  }
}

bool HotkeyManager::nativeRegister(int id, KeyModifier mod, KeyCode key) {
  return RegisterHotKey(NULL, id, mapModifier(mod), mapKey(key));
}

void HotkeyManager::nativeUnregister(int id) {
  UnregisterHotKey(NULL, id);
}

void HotkeyManager::nativeEventLoop() {
  MSG msg;
  while (m_running && GetMessage(&msg, NULL, 0, 0) > 0) {
    if (msg.message == WM_HOTKEY) {
      int hotkeyId = static_cast<int>(msg.wParam);
      HotkeyEvent event{ hotkeyId };
      OnHotkeyPressed(event); // 触发全局广播
    }
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
}

#elif defined(__APPLE__)
// =====================================================================
// 🍎 macOS 平台实现
// =====================================================================
#include <Carbon/Carbon.h>
#include <unordered_map>

// 🌟 引入刚刚写好的独立头文件
#include "mac_key_map.h"

static std::unordered_map<int, EventHotKeyRef> g_macHotkeys;

OSStatus macHotkeyCallback(EventHandlerCallRef nextHandler, EventRef theEvent, void *userData) {
  EventHotKeyID hkCom;
  GetEventParameter(theEvent, kEventParamDirectObject, typeEventHotKeyID, NULL,
                    sizeof(hkCom), NULL, &hkCom);

  // HotkeyEvent event{ (int)hkCom.id };
  // OnHotkeyPressed(event);
  HotkeyManager::getInstance().broadcastHotkey((int)hkCom.id);
  return noErr;
}

void HotkeyManager::start() {
  if (m_running) return;
  m_running = true;
  m_listenerThread = std::thread(&HotkeyManager::nativeEventLoop, this);
}

void HotkeyManager::stop() {
  if (!m_running) return;
  m_running = false;
  CFRunLoopStop(CFRunLoopGetCurrent());
  if (m_listenerThread.joinable()) {
    m_listenerThread.join();
  }
}

bool HotkeyManager::nativeRegister(int id, KeyModifier mod, KeyCode key) {
  nativeUnregister(id);

  EventHotKeyID hkId;
  hkId.signature = 'SNAP';
  hkId.id = id;

  EventHotKeyRef hkRef;

  // 🌟 直接调用 mac_key_map.h 中的内联函数
  OSStatus status = RegisterEventHotKey(
      mapMacKey(key),
      mapMacModifier(mod),
      hkId,
      GetApplicationEventTarget(),
      0,
      &hkRef
  );

  if (status == noErr) {
    g_macHotkeys[id] = hkRef;
    return true;
  }
  return false;
}

void HotkeyManager::nativeUnregister(int id) {
  auto it = g_macHotkeys.find(id);
  if (it != g_macHotkeys.end()) {
    UnregisterEventHotKey(it->second);
    g_macHotkeys.erase(it);
  }
}

void HotkeyManager::nativeEventLoop() {
  EventTypeSpec eventType;
  eventType.eventClass = kEventClassKeyboard;
  eventType.eventKind = kEventHotKeyPressed;

  InstallApplicationEventHandler(&macHotkeyCallback, 1, &eventType, NULL, NULL);

  while (m_running) {
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.1, false);
  }
}

#else 
// =====================================================================
// 🐧 Linux 平台兜底 (防止万一在 Linux 环境编译报错)
// =====================================================================
void HotkeyManager::start() {}
void HotkeyManager::stop() {}
bool HotkeyManager::nativeRegister(int id, KeyModifier mod, KeyCode key) { return false; }
void HotkeyManager::nativeUnregister(int id) {}
void HotkeyManager::nativeEventLoop() {}
#endif

// =====================================================================
// 全局公共调用接口
// =====================================================================
bool HotkeyManager::registerHotkey(int id, KeyModifier mod, KeyCode key) {
  return nativeRegister(id, mod, key);
}

void HotkeyManager::unregisterHotkey(int id) {
  nativeUnregister(id);
}

void HotkeyManager::broadcastHotkey(int id) {
  // 1. 把操作系统传来的纯数字 id，包装成咱们 C++ 统一的事件结构体
  HotkeyEvent event = { id };
  // 2. 触发私有信号 (调用 sigslot 的 operator())
  OnHotkeyPressed(event);
}