#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <string>
#include "hotkey_manager.h"
#include <nlohmann/json.hpp>

class ConfigManager {
  using json = nlohmann::json;

public:
  static ConfigManager &getInstance() {
    static ConfigManager instance;
    return instance;
  }

  bool load();
  bool save();
  void update();

  std::string getLanguage() const { return language_; }
  void setLanguage(const std::string &lang) { language_ = lang; }

  std::string getDefaultSavePath() const { return default_save_path_; }
  void setDefaultSavePath(const std::string &path) { default_save_path_ = path; }

  bool getAutoStart() const { return auto_start_; }
  void setAutoStart(bool val) { auto_start_ = val; }

  KeyModifier getScreenshotMod() const { return screenshot_mod_; }
  KeyCode getScreenshotKey() const { return screenshot_key_; }
  void setScreenshotHotkey(KeyModifier mod, KeyCode key) { screenshot_mod_ = mod; screenshot_key_ = key; }

  KeyModifier getTranslateMod() const { return translate_mod_; }
  KeyCode getTranslateKey() const { return translate_key_; }
  void setTranslateHotkey(KeyModifier mod, KeyCode key) { translate_mod_ = mod; translate_key_ = key; }

private:
  ConfigManager() = default;
  ~ConfigManager() = default;
  ConfigManager(const ConfigManager &) = delete;
  ConfigManager &operator=(const ConfigManager &) = delete;

  std::string getConfigFilePath() const;
  std::string getConfigDirPath() const;

  std::string language_;
  std::string default_save_path_;
  bool auto_start_ = false;

  KeyModifier screenshot_mod_ = KeyModifier::None;
  KeyCode screenshot_key_ = KeyCode::None;
  KeyModifier translate_mod_ = KeyModifier::None;
  KeyCode translate_key_ = KeyCode::None;
};

#endif
