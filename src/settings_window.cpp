#include "settings_window.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "i18n_manager.h"
#include "tray_manager.h"
#include "config_manager.h"
#include <filesystem>
#include "application.h"

#include <iostream>
namespace fs = std::filesystem;

bool SettingsWindow::init() {
  languages_ = I18nManager::getInstance().getSupportLanguages();

  auto &cfg = ConfigManager::getInstance();
  std::string savedLang = cfg.getLanguage();
  for (int i = 0; i < languages_.size(); ++i) {
    if (languages_[i] == savedLang) {
      current_language_idx_ = i;
      break;
    }
  }
  if (current_language_idx_ == -1) {
    for (int i = 0; i < languages_.size(); ++i) {
      if (languages_[i] == Config::DEFAULT_LANGUAGE) {
        current_language_idx_ = i;
        break;
      }
    }
  }
  if (current_language_idx_ == -1) return false;

  autoStart_ = cfg.getAutoStart();
  std::string savedPath = cfg.getDefaultSavePath();
  if (!savedPath.empty()) {
    strncpy(defaultSavePath_, savedPath.c_str(), IM_ARRAYSIZE(defaultSavePath_) - 1);
    defaultSavePath_[IM_ARRAYSIZE(defaultSavePath_) - 1] = '\0';
  }

  // ==========================================
  // 🌟 补回丢失的肉体：创建底层窗口与上下文
  // ==========================================
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // 初始隐藏
  window_ = glfwCreateWindow(Config::WINDOW_WIDTH, Config::WINDOW_HEIGHT, tr("main_window.title"), NULL, NULL); // 使用你 Config 里的长宽
  if (!window_) return false;

  glfwSetWindowUserPointer(window_, this);
  glfwSetWindowCloseCallback(window_, windowCloseCallback);
  glfwSetWindowFocusCallback(window_, windowFocusCallback);
  glfwSetWindowIconifyCallback(window_, windowIconifyCallback);

  glfwMakeContextCurrent(window_);
  glfwSwapInterval(1);

  IMGUI_CHECKVERSION();
  ctx_ = ImGui::CreateContext();
  ImGui::SetCurrentContext(ctx_);
  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(window_, true);
  ImGui_ImplOpenGL3_Init("#version 330");

  // 2. 初始化子组件和字体
  control_window_.init();
  rebuildFonts(); // 🌟 必须在这里初始化一次字体！

  return true;
}

// ==========================================
// 🌟 核心渲染主控函数 (路由分发)
// ==========================================
void SettingsWindow::render() {
  if (!window_ || !ctx_) return;

  // 🌟 开头：告诉显卡，准备开始在这张画布上画画了
  glfwMakeContextCurrent(window_);
  ImGui::SetCurrentContext(ctx_);
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  ImGuiIO &io = ImGui::GetIO();

  // 设置窗口撑满整个 GLFW 窗口
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(io.DisplaySize);
  ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;

  ImGui::Begin("MainSettingsWindow", nullptr, windowFlags);

  if (ImGui::BeginTabBar("SettingsTabBar", ImGuiTabBarFlags_None)) {
    // 按顺序调用各个独立抽离的 Tab 渲染函数
    renderPreferencesTab();
    renderControlTab();
    renderAboutTab();

    ImGui::EndTabBar();
  }

  ImGui::End();

  // 🌟 结尾：封箱打包，交给显卡渲染上屏
  ImGui::Render();
  int display_w, display_h;
  glfwGetFramebufferSize(window_, &display_w, &display_h);
  glViewport(0, 0, display_w, display_h);
  glClearColor(Config::CLEAR_COLOR[0], Config::CLEAR_COLOR[1], Config::CLEAR_COLOR[2], Config::CLEAR_COLOR[3]); // 你的背景色
  glClear(GL_COLOR_BUFFER_BIT);
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  glfwSwapBuffers(window_);
}

// ==========================================
// 子选项卡 1：通用设置
// ==========================================
void SettingsWindow::renderPreferencesTab() {
  std::string prefix = "main_window.";
  if (ImGui::BeginTabItem(tr(prefix + "preferences.title"))) {
    ImGui::Spacing(); ImGui::Spacing();

    ImGui::Text("%s", tr(prefix + "preferences.language"));
    ImGui::SameLine(150);
    if (ImGui::BeginCombo("##LangCombo", languages_.empty() ? "None" : languages_[current_language_idx_].c_str())) {
      for (int n = 0; n < languages_.size(); n++) {
        bool is_selected = (current_language_idx_ == n);
        if (ImGui::Selectable(languages_[n].c_str(), is_selected)) {
          current_language_idx_ = n;
          std::string language = languages_[n];

          if (I18nManager::getInstance().getCurrentLanguage() != language) {
            I18nManager::getInstance().loadLanguage(language);
            ConfigManager::getInstance().setLanguage(language);
            TrayManager::getInstance().rebuildMenu();
            Application::getInstance().setNeedFontRebuild(true);
            dirty_ = true;
          }
        }
        if (is_selected) ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }

    ImGui::Spacing();
    ImGui::Text("%s", tr(prefix + "preferences.default_save_path"));
    ImGui::SameLine(150);
    if (ImGui::InputText("##SavePath", defaultSavePath_, IM_ARRAYSIZE(defaultSavePath_))) {
      dirty_ = true;
    }

    ImGui::Spacing();
    if (ImGui::Checkbox(tr(prefix + "preferences.auto_start"), &autoStart_)) {
      dirty_ = true;
    }

    ImGui::Spacing();
    if (ImGui::Button(tr(prefix + "preferences.save"), ImVec2(80, 0))) {
      ConfigManager::getInstance().update();
      ConfigManager::getInstance().save();
      dirty_ = false;
    }

    ImGui::EndTabItem();
  }
}

// ==========================================
// 子选项卡 2：快捷键设置 (🌟 我们接下来的战场)
// ==========================================
void SettingsWindow::renderControlTab() {
  std::string prefix = "main_window.";
  if (ImGui::BeginTabItem(tr(prefix + "control.title"))) {
    // 🌟 绝杀：主窗口什么都不管了，直接把画笔交给子组件！
    control_window_.render();

    ImGui::EndTabItem();
  }
}

// ==========================================
// 子选项卡 3：关于
// ==========================================
void SettingsWindow::renderAboutTab() {
  std::string prefix = "main_window.";
  if (ImGui::BeginTabItem(tr(prefix + "about.title"))) {
    ImGui::Spacing(); ImGui::Spacing();
    ImGui::Text("%s: %s", tr(prefix + "about.app_name"), tr(prefix + "about.description"));
    ImGui::Spacing();
    ImGui::Text("%s: %s", tr(prefix + "about.version"), tr(prefix + "about.app_version"));
    ImGui::Text("%s", tr(prefix + "about.author"));

    ImGui::EndTabItem();
  }
}

void SettingsWindow::stopRecordingHottkey() {
  if (control_window_.isRecording()) {
    control_window_.cancelRecording();
  }
}

void SettingsWindow::windowCloseCallback(GLFWwindow *window) {
  auto *win = static_cast<SettingsWindow *>(glfwGetWindowUserPointer(window));
  if (win) {
    if (win->dirty_) {
      ConfigManager::getInstance().update();
      ConfigManager::getInstance().save();
      win->dirty_ = false;
    }
    win->hide();
    glfwSetWindowShouldClose(window, GLFW_FALSE);
  }
}

void SettingsWindow::windowIconifyCallback(GLFWwindow *window, int iconified) {
  auto *win = static_cast<SettingsWindow *>(glfwGetWindowUserPointer(window));
  if (win && iconified) win->hide();
}

void SettingsWindow::windowFocusCallback(GLFWwindow *window, int focused) {
  auto *win = static_cast<SettingsWindow *>(glfwGetWindowUserPointer(window));
  if (win) {
    if (focused) {
      win->show();
    }
    else {
      win->stopRecordingHottkey();
      win->render(); // 失焦时刷一帧防止残影
    }
  }
}