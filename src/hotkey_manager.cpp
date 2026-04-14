#include "hotkey_manager.h"
#include <iostream>

// =====================================================================
// 全局公共调用接口 (跨平台统一入口)
// =====================================================================
void HotkeyManager::broadcastHotkey(int id) {
  // 把纯数字 id 包装成统一的事件结构体，向外广播
  HotkeyEvent event = { id };
  OnHotkeyPressed(event);
}

// =====================================================================
// 🪟 Windows 平台实现
// =====================================================================
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <future> // 引入 std::promise

#define WM_USER_REGISTER_HOTKEY   (WM_USER + 1)
#define WM_USER_UNREGISTER_HOTKEY (WM_USER + 2)

// 跨线程注册请求信封
struct RegisterRequest {
  int id;
  std::promise<bool> result;
};

void HotkeyManager::start() {
  if (m_running) return;
  m_running = true;
  m_listenerThread = std::thread(&HotkeyManager::nativeEventLoop, this);
}

void HotkeyManager::stop() {
  if (!m_running) return;
  m_running = false;
  // 发送 WM_QUIT 优雅唤醒并关闭 Windows 线程消息循环
  if (m_winThreadId.load() != 0) {
    PostThreadMessage(m_winThreadId.load(), WM_QUIT, 0, 0);
  }
  if (m_listenerThread.joinable()) {
    m_listenerThread.join();
  }
}

bool HotkeyManager::registerHotkey(int id, KeyModifier mod, KeyCode key) {
  {
    std::lock_guard<std::mutex> lock(m_hotkeyMutex);
    m_registeredHotkeys[id] = { mod, key };
  }

  // 🌟 如果后台线程正在运行，发送消息并【阻塞等待】它的真实执行结果
  if (m_running && m_winThreadId.load() != 0) {
    // 1. 创建堆上的信封，防止提前析构
    RegisterRequest *req = new RegisterRequest{ id };
    auto future = req->result.get_future();

    // 2. 发送给后台线程 (把信封指针藏在 LPARAM 里)
    PostThreadMessage(m_winThreadId.load(), WM_USER_REGISTER_HOTKEY, 0, reinterpret_cast<LPARAM>(req));

    // 3. 死等后台线程填入结果
    bool success = future.get();

    // 4. 如果底层注册失败(可能被占用)，回滚 C++ 内存里的记录
    if (!success) {
      std::lock_guard<std::mutex> lock(m_hotkeyMutex);
      m_registeredHotkeys.erase(id);
    }
    return success;
  }

  // 如果是在 start() 之前调用的，视为延迟注册，默认先返回 true
  return true;
}

void HotkeyManager::unregisterHotkey(int id) {
  {
    std::lock_guard<std::mutex> lock(m_hotkeyMutex);
    m_registeredHotkeys.erase(id);
  }
  // 异步通知后台注销即可，不需要等结果
  if (m_running && m_winThreadId.load() != 0) {
    PostThreadMessage(m_winThreadId.load(), WM_USER_UNREGISTER_HOTKEY, static_cast<WPARAM>(id), 0);
  }
}

void HotkeyManager::nativeEventLoop() {
  m_winThreadId.store(GetCurrentThreadId());

  // 强制操作系统为当前线程创建消息队列
  MSG msg;
  PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

  // 初始化：注册在 start() 之前积压的热键
  {
    std::lock_guard<std::mutex> lock(m_hotkeyMutex);
    for (const auto &[id, data] : m_registeredHotkeys) {
      RegisterHotKey(NULL, id, mapModifier(data.mod), mapKey(data.key));
    }
  }

  // 🌟 绝对阻塞的消息循环：没有事件时 CPU 占用稳定 0%
  while (m_running && GetMessage(&msg, NULL, 0, 0) > 0) {
    if (msg.message == WM_HOTKEY) {
      broadcastHotkey(static_cast<int>(msg.wParam));
    }
    else if (msg.message == WM_USER_REGISTER_HOTKEY) {
      // 1. 拆开主线程递过来的信封
      RegisterRequest *req = reinterpret_cast<RegisterRequest *>(msg.lParam);
      bool success = false;

      // 2. 亲自执行底层注册
      {
        std::lock_guard<std::mutex> lock(m_hotkeyMutex);
        if (m_registeredHotkeys.count(req->id)) {
          auto &data = m_registeredHotkeys[req->id];
          success = RegisterHotKey(NULL, req->id, mapModifier(data.mod), mapKey(data.key));
        }
      }

      // 3. 将结果填入信封，唤醒等待的主线程
      req->result.set_value(success);
      delete req; // 销毁堆内存
    }
    else if (msg.message == WM_USER_UNREGISTER_HOTKEY) {
      UnregisterHotKey(NULL, static_cast<int>(msg.wParam));
    }

    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  // 退出前清理所有的系统热键
  for (const auto &[id, data] : m_registeredHotkeys) {
    UnregisterHotKey(NULL, id);
  }
  m_winThreadId.store(0);
}

// =====================================================================
// 🍎 macOS 平台实现
// =====================================================================
#elif defined(__APPLE__)
#include <Carbon/Carbon.h>

static std::unordered_map<int, EventHotKeyRef> g_macHotkeys;

OSStatus macHotkeyCallback(EventHandlerCallRef nextHandler, EventRef theEvent, void *userData) {
  EventHotKeyID hkCom;
  GetEventParameter(theEvent, kEventParamDirectObject, typeEventHotKeyID, NULL,
                    sizeof(hkCom), NULL, &hkCom);
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

  // 🌟 精准唤醒：关停后台的 RunLoop
  if (m_macRunLoop.load() != nullptr) {
    CFRunLoopStop(static_cast<CFRunLoopRef>(m_macRunLoop.load()));
  }
  if (m_listenerThread.joinable()) {
    m_listenerThread.join();
  }
}

bool HotkeyManager::registerHotkey(int id, KeyModifier mod, KeyCode key) {
  unregisterHotkey(id); // 防止同一 ID 重复注册

  EventHotKeyID hkId;
  hkId.signature = 'SNAP';
  hkId.id = id;
  EventHotKeyRef hkRef;

  // macOS 允许直接在主线程操作 Carbon API
  OSStatus status = RegisterEventHotKey(
      mapKey(key),
      mapModifier(mod),
      hkId,
      GetApplicationEventTarget(),
      0,
      &hkRef
  );

  if (status == noErr) {
    std::lock_guard<std::mutex> lock(m_hotkeyMutex);
    g_macHotkeys[id] = hkRef;
    m_registeredHotkeys[id] = { mod, key };
    return true;
  }
  return false;
}

void HotkeyManager::unregisterHotkey(int id) {
  std::lock_guard<std::mutex> lock(m_hotkeyMutex);
  auto it = g_macHotkeys.find(id);
  if (it != g_macHotkeys.end()) {
    UnregisterEventHotKey(it->second);
    g_macHotkeys.erase(it);
  }
  m_registeredHotkeys.erase(id);
}

void HotkeyManager::nativeEventLoop() {
  m_macRunLoop.store(CFRunLoopGetCurrent());

  // 🌟 核心镇定剂：给 RunLoop 塞一个 Dummy Source
  CFRunLoopSourceContext ctx = { 0 };
  CFRunLoopSourceRef dummySource = CFRunLoopSourceCreate(kCFAllocatorDefault, 0, &ctx);
  CFRunLoopAddSource(CFRunLoopGetCurrent(), dummySource, kCFRunLoopDefaultMode);

  // 安装键盘监听事件
  EventTypeSpec eventType;
  eventType.eventClass = kEventClassKeyboard;
  eventType.eventKind = kEventHotKeyPressed;
  InstallApplicationEventHandler(&macHotkeyCallback, 1, &eventType, NULL, NULL);

  // 🌟 内核级挂起：CPU 0%
  CFRunLoopRun();

  // 收到下班指令，清理现场
  CFRunLoopRemoveSource(CFRunLoopGetCurrent(), dummySource, kCFRunLoopDefaultMode);
  CFRelease(dummySource);
  m_macRunLoop.store(nullptr);
}

// =====================================================================
// 🐧 Linux 平台兜底实现
// =====================================================================
#else 
void HotkeyManager::start() {}
void HotkeyManager::stop() {}
bool HotkeyManager::registerHotkey(int id, KeyModifier mod, KeyCode key) { return false; }
void HotkeyManager::unregisterHotkey(int id) {}
void HotkeyManager::nativeEventLoop() {}
#endif