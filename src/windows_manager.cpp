#include "windows_manager.h"

bool WindowManager::initAll() {

  if (!settings_window_.init()) return false;

  // if (!overlay_window_.init(initial_locale)) return false; // 预留

  return true;
}

void WindowManager::renderAll() {
  // 只渲染当前处于可见状态的窗口
  if (settings_window_.isVisible()) {
    settings_window_.render();
  }
  // if (overlay_window_.isVisible()) { overlay_window_.render(); }
}

void WindowManager::rebuildAllFonts() {
  settings_window_.rebuildFonts();
  // overlay_window_.rebuildFonts();
}

void WindowManager::shutdownAll() {
  settings_window_.shutdown();
  // overlay_window_.shutdown();
}

bool WindowManager::hasVisibleWindows() const {
  return settings_window_.isVisible(); // || overlay_window_.isVisible();
}