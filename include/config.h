#ifndef CONFIG_H
#define CONFIG_H
#include "hotkey_manager.h"
// =====================================================================
// 全局常量配置区
// =====================================================================
namespace Config {
  constexpr int   WINDOW_WIDTH = 450;
  constexpr int   WINDOW_HEIGHT = 250;
  constexpr const char *WINDOW_TITLE = "Snaptrans";

  constexpr float CLEAR_COLOR[4] = { 0.12f, 0.12f, 0.12f, 1.0f }; // 背景色
  constexpr double IDLE_TIMEOUT = 0.016;

  constexpr float FONT_SIZE = 18.0f;
  constexpr const char *LANGUAGE_INFO_PATH = "assets/language/language.json";
  // constexpr const char *BASE_FONT_PATH = "assets/fonts/NotoSans_Medium.ttf";
  // constexpr const char *CJK_FONT_PATH = "assets/fonts/NotoSansSC_Medium.ttf";
  constexpr const char *DEFAULT_LANGUAGE = "English";

  constexpr const int SCREENSHOT_EVENT_ID = 1;
  constexpr const KeyModifier SCREENSHOT_KEYMODIFIER = KeyModifier::Alt;
  constexpr const KeyCode SCREENSHOT_KEYCODE = KeyCode::S;

  constexpr const int TRANSLATION_EVENT_ID = 2;
  constexpr const KeyModifier TRANSLATION_KEYMODIFIER = KeyModifier::Alt;
  constexpr const KeyCode TRANSLATION_KEYCODE = KeyCode::T;
}
#endif