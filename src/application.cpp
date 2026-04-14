#include "application.h"
#include <GLFW/glfw3.h>
#include "windows_manager.h"
#include "tray_manager.h"
#include "config.h"
#include "i18n_manager.h"

bool Application::init() {
  if (!glfwInit()) return false;

#if defined(__APPLE__)
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
  // 确保所有窗口init之前，tr函数没问题
  I18nManager::getInstance().init();  // 读取language info
  I18nManager::getInstance().loadLanguage(Config::DEFAULT_LANGUAGE);

  // 初始化所有窗口
  if (!WindowManager::getInstance().initAll()) {
    return false;
  }

  // 初始化托盘
#if defined(__APPLE__)
  std::string iconPath = "trayTemplate";
#elif defined(_WIN32) || defined(_WIN64)
  std::string iconPath = "assets/icons/tray.ico";
#else
  std::string iconPath = "assets/icons/tray.png";
#endif
  TrayManager::getInstance().initialize(iconPath);

  return true;
}

void Application::changeLanguage(const std::string &new_language) {
  if (I18nManager::getInstance().getCurrentLanguage() != new_language) {
    // current_language_ = new_language;
    // 确保所有窗口rebuilFonts之前，tr函数没问题
    I18nManager::getInstance().loadLanguage(new_language);  // 会修改其中的current_language_
    TrayManager::getInstance().rebuildMenu();
    need_font_rebuild_ = true;
  }
}

void Application::runLoop() {
  auto &winMgr = WindowManager::getInstance();

  while (!should_quit_) {
    // 1. 处理热键跨线程信号
    if (signal_screenshot_.exchange(false)) {
      // TODO: winMgr.getOverlayWindow()->show();
    }
    if (signal_translation_.exchange(false)) {
      // TODO: 处理翻译动作
    }

    // 2. 动态字体更新
    if (need_font_rebuild_) {
      winMgr.rebuildAllFonts();
      need_font_rebuild_ = false;
    }

    // 3. 智能休眠调度
    if (winMgr.hasVisibleWindows() || signal_screenshot_ || signal_translation_) {
      glfwPollEvents();
    }
    else {
      glfwWaitEventsTimeout(Config::IDLE_TIMEOUT);
    }

    TrayManager::getInstance().update();

    // 4. 让管家通知所有激活的子窗口渲染自己
    winMgr.renderAll();
  }
}

void Application::shutdown() {
  TrayManager::getInstance().shutdown();
  WindowManager::getInstance().shutdownAll();
  glfwTerminate();
}