#include "settings_window.h"
#include "imgui.h"
#include "i18n_manager.h"
#include "tray_manager.h"
#include <filesystem>

namespace fs = std::filesystem;

// 扫描目录助手 (从 main.cpp 搬过来的)
static std::vector<std::string> scanDirectory(const std::string &path, const std::string &ext) {
  std::vector<std::string> files;
  if (fs::exists(path) && fs::is_directory(path)) {
    for (const auto &entry : fs::directory_iterator(path)) {
      if (entry.path().extension() == ext) {
        files.push_back(entry.path().filename().string());
      }
    }
  }
  return files;
}

void SettingsWindow::init(const std::string &initial_locale) {
  locales_ = scanDirectory("assets/locales", ".json");
  for (int i = 0; i < locales_.size(); ++i) {
    if (locales_[i].find(initial_locale) != std::string::npos) {
      current_locale_idx_ = i;
      break;
    }
  }
}

void SettingsWindow::render(bool &out_need_font_rebuild, std::string &out_current_locale) {
  ImGuiIO &io = ImGui::GetIO();

  // 设置窗口撑满整个 GLFW 窗口
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(io.DisplaySize);
  ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;

  ImGui::Begin("MainSettingsWindow", nullptr, windowFlags);

  if (ImGui::BeginTabBar("SettingsTabBar", ImGuiTabBarFlags_None)) {

    std::string prefix = "main_window.";
    // --- 选项卡 1：通用 ---
    if (ImGui::BeginTabItem(tr(prefix + "preferences.title"))) {
      ImGui::Spacing(); ImGui::Spacing();

      ImGui::Text("%s", tr(prefix + "preferences.language"));
      ImGui::SameLine(150);
      if (ImGui::BeginCombo("##LangCombo", locales_.empty() ? "None" : locales_[current_locale_idx_].c_str())) {
        for (int n = 0; n < locales_.size(); n++) {
          bool is_selected = (current_locale_idx_ == n);
          if (ImGui::Selectable(locales_[n].c_str(), is_selected)) {
            current_locale_idx_ = n;
            std::string code = locales_[n].substr(0, locales_[n].find_last_of('.'));

            // 通知主循环：语言变了，需要换字典、换托盘、重建字体！
            if (out_current_locale != code) {
              out_current_locale = code;
              I18nManager::getInstance().loadLanguage(out_current_locale);
              TrayManager::getInstance().rebuildMenu();
              out_need_font_rebuild = true;
            }
          }
          if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }

      ImGui::Spacing();
      ImGui::Text("%s", tr(prefix + "preferences.default_save_path"));
      ImGui::SameLine(150);
      ImGui::InputText("##SavePath", defaultSavePath_, IM_ARRAYSIZE(defaultSavePath_));

      ImGui::Spacing();
      ImGui::Checkbox(tr(prefix + "preferences.auto_start"), &autoStart_);

      ImGui::Spacing();
      if (ImGui::Button(tr(prefix + "preferences.save"), ImVec2(80, 0))) {
        // TODO: 保存配置
      }

      ImGui::EndTabItem();
    }

    // --- 选项卡 2：快捷键 ---
    if (ImGui::BeginTabItem(tr(prefix + "control.title"))) {
      ImGui::Spacing(); ImGui::Spacing();
      ImGui::Text("%s:", tr(prefix + "control.screenshot_hotkey")); ImGui::SameLine(150); ImGui::Button("Ctrl + Alt + A");
      ImGui::Spacing();
      ImGui::Text("%s:", tr(prefix + "control.translate_hotkey")); ImGui::SameLine(150); ImGui::Button("Ctrl + C + C");
      ImGui::EndTabItem();
    }

    // --- 选项卡 3：关于 ---
    if (ImGui::BeginTabItem(tr(prefix + "about.title"))) {
      ImGui::Spacing(); ImGui::Spacing();
      ImGui::Text("%s: %s", tr(prefix + "about.app_name"), tr(prefix + "about.description"));
      ImGui::Spacing();
      ImGui::Text("%s: %s", tr(prefix + "about.version"), tr(prefix + "about.app_version"));
      ImGui::Text("%s", tr(prefix + "about.author"));
      ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
  }

  ImGui::End();
}