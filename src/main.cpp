#include "windows_manager.h"
#include "hotkey_manager.h"
#include "config.h"
#include <iostream> // 记得删除
int main() {
  // 1. 初始化窗口大管家
  auto &windosMgr = WindowManager::getInstance();
  if (!windosMgr.init()) return -1;

  // 2. 启动系统级全局热键守护线程
  auto &hotkeyMgr = HotkeyManager::getInstance();
  hotkeyMgr.start();

  // 3. 跨线程事件订阅，直接向管家发射信号
  hotkeyMgr.addCallback([&windosMgr](const HotkeyEvent &event) {
    if (event.id == Config::SCREENSHOT_EVENT_ID) {
      // windosMgr.triggerScreenshot();
      std::cerr << "Screenshot" << "\n";
    }
    else if (event.id == Config::TRANSLATION_EVENT_ID) {
      // windosMgr.triggerTranslation();
      std::cerr << "Translation" << "\n";
    }
  });

  // 4. 移交主线程控制权，进入帧渲染驱动引擎
  windosMgr.runLoop();

  // 5. 退出清理
  hotkeyMgr.stop();
  windosMgr.shutdown();

  return 0;
}