#include "config_manager.h"
#include "config.h"
#include "windows_manager.h"
#include <fstream>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

std::string ConfigManager::getConfigDirPath() const {
#if defined(__APPLE__)
  return fs::path(getenv("HOME")) / "Library" / "Application Support" / "Snaptrans";
#elif defined(_WIN32) || defined(_WIN64)
  return fs::path(getenv("APPDATA")) / "Snaptrans";
#else
  std::string xdg = getenv("XDG_CONFIG_HOME") ? getenv("XDG_CONFIG_HOME") : "";
  if (xdg.empty()) xdg = std::string(getenv("HOME")) + "/.config";
  return fs::path(xdg) / "Snaptrans";
#endif
}

std::string ConfigManager::getConfigFilePath() const {
  return (fs::path(getConfigDirPath()) / "config.json").string();
}

bool ConfigManager::load() {
  std::string filePath = getConfigFilePath();
  std::ifstream file(filePath);

  if (!file.is_open()) {
    language_ = Config::DEFAULT_LANGUAGE;
    screenshot_mod_ = Config::SCREENSHOT_KEYMODIFIER;
    screenshot_key_ = Config::SCREENSHOT_KEYCODE;
    translate_mod_ = Config::TRANSLATION_KEYMODIFIER;
    translate_key_ = Config::TRANSLATION_KEYCODE;
    auto_start_ = false;
    default_save_path_ = "";
    return true;
  }

  try {
    json j;
    file >> j;

    language_ = j.value("language", Config::DEFAULT_LANGUAGE);
    auto_start_ = j.value("auto_start", false);
    default_save_path_ = j.value("default_save_path", "");

    if (j.contains("screenshot_hotkey")) {
      screenshot_mod_ = static_cast<KeyModifier>(j["screenshot_hotkey"].value("modifier", static_cast<int>(Config::SCREENSHOT_KEYMODIFIER)));
      screenshot_key_ = static_cast<KeyCode>(j["screenshot_hotkey"].value("keycode", static_cast<int>(Config::SCREENSHOT_KEYCODE)));
    } else {
      screenshot_mod_ = Config::SCREENSHOT_KEYMODIFIER;
      screenshot_key_ = Config::SCREENSHOT_KEYCODE;
    }

    if (j.contains("translate_hotkey")) {
      translate_mod_ = static_cast<KeyModifier>(j["translate_hotkey"].value("modifier", static_cast<int>(Config::TRANSLATION_KEYMODIFIER)));
      translate_key_ = static_cast<KeyCode>(j["translate_hotkey"].value("keycode", static_cast<int>(Config::TRANSLATION_KEYCODE)));
    } else {
      translate_mod_ = Config::TRANSLATION_KEYMODIFIER;
      translate_key_ = Config::TRANSLATION_KEYCODE;
    }

    return true;
  } catch (const json::exception &e) {
    std::cerr << "Config parse error: " << e.what() << std::endl;
    return false;
  }
}

bool ConfigManager::save() {
  std::string dirPath = getConfigDirPath();
  std::error_code ec;
  if (!fs::exists(dirPath)) {
    fs::create_directories(dirPath, ec);
    if (ec) {
      std::cerr << "Failed to create config dir: " << ec.message() << std::endl;
      return false;
    }
  }

  json j;
  j["language"] = language_;
  j["auto_start"] = auto_start_;
  j["default_save_path"] = default_save_path_;
  j["screenshot_hotkey"] = {
    {"modifier", static_cast<int>(screenshot_mod_)},
    {"keycode", static_cast<int>(screenshot_key_)}
  };
  j["translate_hotkey"] = {
    {"modifier", static_cast<int>(translate_mod_)},
    {"keycode", static_cast<int>(translate_key_)}
  };

  std::string filePath = getConfigFilePath();
  std::ofstream file(filePath);
  if (!file.is_open()) {
    std::cerr << "Failed to write config: " << filePath << std::endl;
    return false;
  }

  file << j.dump(2);
  return true;
}

void ConfigManager::update() {
  auto *sw = WindowManager::getInstance().getSettingsWindow();
  if (!sw) return;

  const auto &langs = sw->getLanguages();
  int idx = sw->getCurrentLanguageIdx();
  if (idx >= 0 && idx < (int)langs.size()) {
    language_ = langs[idx];
  }
  default_save_path_ = sw->getDefaultSavePath();
  auto_start_ = sw->getAutoStart();

  auto &cw = sw->getControlWindow();
  screenshot_mod_ = cw.getScreenshotHotkey().mod;
  screenshot_key_ = cw.getScreenshotHotkey().key;
  translate_mod_ = cw.getTranslateHotkey().mod;
  translate_key_ = cw.getTranslateHotkey().key;
}
