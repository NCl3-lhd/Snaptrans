#include "application.h"
#include "hotkey_manager.h"
#include "config.h"
#include <iostream> //

int main() {
  auto &hotkeyMgr = HotkeyManager::getInstance();
  auto &app = Application::getInstance();

  hotkeyMgr.start();
  // 热键事件触发后，告诉 Application
  hotkeyMgr.addCallback([&app](const HotkeyEvent &event) {
    if (event.id == Config::SCREENSHOT_EVENT_ID) {
      // app.triggerScreenshot();
      std::cerr << "triggerScreenshot" << "\n";
    }
    else if (event.id == Config::TRANSLATION_EVENT_ID) {
      // app.triggerTranslation();
      std::cerr << "triggerTranslation" << "\n";

    }
  });
  
  // 把核心大管家启动
  if (!app.init()) return -1;


  // 移交主线程控制权给 Application
  app.runLoop();

  // 退出清理
  hotkeyMgr.stop();
  app.shutdown();

  return 0;
}