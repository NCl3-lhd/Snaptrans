#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "tray_manager.h"
#include "i18n_manager.h" 
#include <iostream>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;
static bool should_quit = false;

// 🌟 全局标记：用于动态重建字体图集
static bool g_need_font_rebuild = true;
// 记录当前的语言状态，用于判断是否需要合并中文字体
static std::string g_current_locale = "en_US";

// 扫描目录助手
std::vector<std::string> scanDirectory(const std::string &path, const std::string &ext) {
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

// 哨兵回调
void windowCloseCallback(GLFWwindow *window) {
  glfwSetWindowShouldClose(window, GLFW_FALSE);
  TrayManager::getInstance().setWindowVisible(false);
}
void windowFocusCallback(GLFWwindow *window, int focused) {
  if (focused) TrayManager::getInstance().setWindowVisible(true);
}
void windowIconifyCallback(GLFWwindow *window, int iconified) {
  if (iconified) TrayManager::getInstance().setWindowVisible(false);
  else TrayManager::getInstance().setWindowVisible(true);
}

int main() {
  if (!glfwInit()) return -1;

#if defined(__APPLE__)
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  // =====================================================================
  // 1. 初始化语言：默认硬编码为英文 en_US
  // =====================================================================
  g_current_locale = "en_US";
  I18nManager::getInstance().loadLanguage(g_current_locale);

  std::vector<std::string> locales = scanDirectory("assets/locales", ".json");
  int current_locale_idx = 0;
  for (int i = 0; i < locales.size(); ++i) {
    if (locales[i].find(g_current_locale) != std::string::npos) {
      current_locale_idx = i;
      break;
    }
  }

  // 默认启动隐藏窗口
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  GLFWwindow *window = glfwCreateWindow(700, 450, "SnapTrans", NULL, NULL);
  if (!window) { glfwTerminate(); return -1; }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  glfwSetWindowCloseCallback(window, windowCloseCallback);
  glfwSetWindowFocusCallback(window, windowFocusCallback);
  glfwSetWindowIconifyCallback(window, windowIconifyCallback);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO(); (void)io;

  ImGui::StyleColorsDark();
  ImGuiStyle &style = ImGui::GetStyle();
  style.WindowRounding = 0.0f;

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");

  auto &trayManager = TrayManager::getInstance();

#if defined(__APPLE__)
  std::string iconPath = "trayTemplate";
#elif defined(_WIN32) || defined(_WIN64)
  std::string iconPath = "assets/icons/tray.ico";
#else
  std::string iconPath = "assets/icons/tray.png";
#endif

  trayManager.initialize(window, iconPath);
  trayManager.setWindowVisible(false);

  trayManager.setExitCallback([&window]() {
    should_quit = true;
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  });

  static bool autoStart = false;
  static char defaultSavePath[256] = "C:\\SnapTrans\\Images";

  // =================== 主循环 ===================
  while (!glfwWindowShouldClose(window) && !should_quit) {

    // =====================================================================
    // 🌟 核心逻辑：根据语言动态重载和合并字体
    // =====================================================================
    if (g_need_font_rebuild) {
      ImGui_ImplOpenGL3_DestroyDeviceObjects();
      io.Fonts->Clear();

      ImFontConfig font_config;
      font_config.OversampleH = 2;
      font_config.OversampleV = 2;

      // 1. 无条件加载基础英文字体 (默认排在第一位)
      std::string engFontPath = "assets/fonts/NotoSans_Medium.ttf";
      if (fs::exists(engFontPath)) {
        io.Fonts->AddFontFromFileTTF(engFontPath.c_str(), 18.0f, &font_config, io.Fonts->GetGlyphRangesDefault());
      }
      else {
        std::cerr << "Warning: Base English font missing!" << std::endl;
        io.Fonts->AddFontDefault(); // 兜底：如果连英文字体都找不到，用 ImGui 自带的像素字体
      }

      // 2. 如果当前语言包含中文 (zh_CN)，则动态合并中文字体
      if (g_current_locale == "zh_CN") {
        std::string chnFontPath = "assets/fonts/NotoSansSC_Medium.ttf"; // 确保你也有这个中文字体文件
        if (fs::exists(chnFontPath)) {
          font_config.MergeMode = true; // 开启合并模式！
          font_config.PixelSnapH = false; // 建议关闭像素对齐，防止中英文混合时挤在一起
          io.Fonts->AddFontFromFileTTF(chnFontPath.c_str(), 18.0f, &font_config, io.Fonts->GetGlyphRangesChineseFull());
        }
        else {
          std::cerr << "Warning: Chinese font missing!" << std::endl;
        }
      }

      ImGui_ImplOpenGL3_CreateDeviceObjects();
      g_need_font_rebuild = false;
    }
    // =====================================================================

    if (glfwGetWindowAttrib(window, GLFW_VISIBLE)) glfwPollEvents();
    else glfwWaitEventsTimeout(0.016);

    trayManager.update();

    if (glfwGetWindowAttrib(window, GLFW_VISIBLE)) {
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      glfwSetWindowTitle(window, tr("settings.title"));

      ImGui::SetNextWindowPos(ImVec2(0, 0));
      ImGui::SetNextWindowSize(io.DisplaySize);
      ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;

      ImGui::Begin("MainSettingsWindow", nullptr, windowFlags);

      if (ImGui::BeginTabBar("SettingsTabBar", ImGuiTabBarFlags_None)) {

        // --- 选项卡 1：通用 ---
        if (ImGui::BeginTabItem(tr("settings.title"))) {
          ImGui::Spacing(); ImGui::Spacing();

          ImGui::Text("%s", tr("settings.language"));
          ImGui::SameLine(150);
          if (ImGui::BeginCombo("##LangCombo", locales.empty() ? "None" : locales[current_locale_idx].c_str())) {
            for (int n = 0; n < locales.size(); n++) {
              bool is_selected = (current_locale_idx == n);
              if (ImGui::Selectable(locales[n].c_str(), is_selected)) {
                current_locale_idx = n;
                std::string code = locales[n].substr(0, locales[n].find_last_of('.'));

                // 如果语言发生了实质性改变
                if (g_current_locale != code) {
                  g_current_locale = code;
                  I18nManager::getInstance().loadLanguage(g_current_locale);
                  TrayManager::getInstance().rebuildMenu();

                  // 🌟 触发字体图集重建
                  g_need_font_rebuild = true;
                }
              }
              if (is_selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
          }

          ImGui::Spacing();
          ImGui::Text("%s", tr("settings.default_save_path"));
          ImGui::SameLine(150);
          ImGui::InputText("##SavePath", defaultSavePath, IM_ARRAYSIZE(defaultSavePath));

          ImGui::Spacing();
          ImGui::Checkbox(tr("settings.auto_start"), &autoStart);

          ImGui::Spacing();
          if (ImGui::Button(tr("common.save"), ImVec2(80, 0))) {
            // TODO: 保存配置到 config.json
          }

          ImGui::EndTabItem();
        }

        // --- 选项卡 2：快捷键 ---
        if (ImGui::BeginTabItem(tr("settings.hotkey"))) {
          ImGui::Spacing(); ImGui::Spacing();
          ImGui::Text("%s:", tr("settings.screenshot_hotkey")); ImGui::SameLine(150); ImGui::Button("Ctrl + Alt + A");
          ImGui::Spacing();
          ImGui::Text("%s:", tr("settings.translate_hotkey")); ImGui::SameLine(150); ImGui::Button("Ctrl + C + C");
          ImGui::EndTabItem();
        }

        // --- 选项卡 3：关于 ---
        if (ImGui::BeginTabItem(tr("menu.about"))) {
          ImGui::Spacing(); ImGui::Spacing();
          ImGui::Text("%s", tr("about.title"));
          ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", tr("about.description"));
          ImGui::Spacing();
          ImGui::Text("%s: %s", tr("about.version"), tr("app_version"));
          ImGui::Text("%s", tr("about.author"));
          ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
      }

      ImGui::End();
      ImGui::Render();

      int display_w, display_h;
      glfwGetFramebufferSize(window, &display_w, &display_h);
      glViewport(0, 0, display_w, display_h);
      glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT);
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
      glfwSwapBuffers(window);
    }
  }

  trayManager.shutdown();
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}