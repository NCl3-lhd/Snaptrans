#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "tray_manager.h"
#include "i18n_manager.h" 
#include "settings_window.h"
#include <iostream>
#include <filesystem>
// 只能在某一个 .cpp 文件里定义 IMPLEMENTATION，通常就在 main.cpp
#define STB_IMAGE_IMPLEMENTATION 
#include "stb_image.h"
namespace fs = std::filesystem;

// =====================================================================
// 全局常量配置区
// =====================================================================
namespace Config {
  // 窗口配置
  constexpr int   WINDOW_WIDTH = 450;
  constexpr int   WINDOW_HEIGHT = 250;
  constexpr const char *WINDOW_TITLE = "Snaptrans";

  // 渲染与休眠控制
  constexpr float CLEAR_COLOR[4] = { 0.12f, 0.12f, 0.12f, 1.0f }; // 深灰色背景
  constexpr double IDLE_TIMEOUT = 0.016; // 后台休眠唤醒间隔 (约 60FPS)

  // 字体配置
  constexpr float FONT_SIZE = 18.0f;
  constexpr const char *BASE_FONT_PATH = "assets/fonts/NotoSans_Medium.ttf";
  constexpr const char *CJK_FONT_PATH = "assets/fonts/NotoSansSC_Medium.ttf";

  // 默认状态
  constexpr const char *DEFAULT_LOCALE = "en_US";
}
// =====================================================================

static bool should_quit = false;
static bool g_need_font_rebuild = true;
static std::string g_current_locale = Config::DEFAULT_LOCALE;

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

  // 初始化语言
  g_current_locale = Config::DEFAULT_LOCALE;
  I18nManager::getInstance().loadLanguage(g_current_locale);

  // 实例化 UI 模块并初始化
  SettingsWindow settingsWin;
  settingsWin.init(g_current_locale);

  // 创建窗口 (引用 Config)
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  GLFWwindow *window = glfwCreateWindow(Config::WINDOW_WIDTH, Config::WINDOW_HEIGHT, Config::WINDOW_TITLE, NULL, NULL);
  if (!window) { glfwTerminate(); return -1; }

  // ==========================================================
  // 🌟 代码运行时：动态注入窗口图标 (仅限 Windows / Linux)
  // macOS 窗口没有标题栏图标，且 Dock 图标由 .app 里的 .icns 接管，故跳过
  // ==========================================================
#ifndef __APPLE__
  GLFWimage images[1];
  images[0].pixels = stbi_load("assets/icons/tray.png", &images[0].width, &images[0].height, 0, 4);

  if (images[0].pixels) {
    glfwSetWindowIcon(window, 1, images);
    stbi_image_free(images[0].pixels);
  }
  else {
    // 如果在 Windows 下依然报错，可以打印真正的错误原因：
    std::cerr << "Warning: Failed to load window icon! Reason: " << stbi_failure_reason() << std::endl;
  }
#endif
  
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  glfwSetWindowCloseCallback(window, windowCloseCallback);
  glfwSetWindowFocusCallback(window, windowFocusCallback);
  glfwSetWindowIconifyCallback(window, windowIconifyCallback);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO(); (void)io;

  ImGui::StyleColorsDark();
  ImGui::GetStyle().WindowRounding = 0.0f;

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

  // =================== 主循环 ===================
  while (!glfwWindowShouldClose(window) && !should_quit) {

    // 1. 字体动态合并机制 (引用 Config)
    if (g_need_font_rebuild) {
      ImGui_ImplOpenGL3_DestroyDeviceObjects();
      io.Fonts->Clear();

      ImFontConfig font_config;
      font_config.OversampleH = 2;
      font_config.OversampleV = 2;

      // 无条件加载保底字体
      if (fs::exists(Config::BASE_FONT_PATH)) {
        io.Fonts->AddFontFromFileTTF(Config::BASE_FONT_PATH, Config::FONT_SIZE, &font_config, io.Fonts->GetGlyphRangesDefault());
      }
      else {
        io.Fonts->AddFontDefault();
      }

      // 根据语言动态追加 CJK 字体
      if (g_current_locale == "zh_CN") {
        if (fs::exists(Config::CJK_FONT_PATH)) {
          font_config.MergeMode = true;
          font_config.PixelSnapH = false;
          io.Fonts->AddFontFromFileTTF(Config::CJK_FONT_PATH, Config::FONT_SIZE, &font_config, io.Fonts->GetGlyphRangesChineseFull());
        }
      }

      ImGui_ImplOpenGL3_CreateDeviceObjects();
      g_need_font_rebuild = false;
    }

    // 2. 事件分发与休眠 (引用 Config)
    if (glfwGetWindowAttrib(window, GLFW_VISIBLE)) glfwPollEvents();
    else glfwWaitEventsTimeout(Config::IDLE_TIMEOUT);

    trayManager.update();

    // 3. 渲染骨架
    if (glfwGetWindowAttrib(window, GLFW_VISIBLE)) {
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      glfwSetWindowTitle(window, tr("main_window.title"));

      // 核心：把控制权交给 UI 类！
      settingsWin.render(g_need_font_rebuild, g_current_locale);

      ImGui::Render();

      int display_w, display_h;
      glfwGetFramebufferSize(window, &display_w, &display_h);
      glViewport(0, 0, display_w, display_h);

      // 引用 Config 中的背景颜色
      glClearColor(Config::CLEAR_COLOR[0], Config::CLEAR_COLOR[1], Config::CLEAR_COLOR[2], Config::CLEAR_COLOR[3]);
      glClear(GL_COLOR_BUFFER_BIT);

      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
      glfwSwapBuffers(window);
    }
  }

  // =================== 清理与退出 ===================
  trayManager.shutdown();
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}