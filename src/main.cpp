#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "tray_manager.h"
#include "i18n_manager.h" 
#include "settings_window.h"

// 🌟 引入热键管理器与原子操作
#include "hotkey_manager.h"
#include "config.h"

#include <atomic> 

#include <iostream>
#include <filesystem>

// 只能在某一个 .cpp 文件里定义 IMPLEMENTATION，通常就在 main.cpp
#define STB_IMAGE_IMPLEMENTATION 
#include "stb_image.h"
namespace fs = std::filesystem;


static bool should_quit = false;
static bool g_need_font_rebuild = true;
static std::string g_current_locale = Config::DEFAULT_LOCALE;

// 哨兵回调
void windowCloseCallback(GLFWwindow *window) {
  glfwSetWindowShouldClose(window, GLFW_FALSE);
  TrayManager::getInstance().setWindowVisible(false);
}
void windowIconifyCallback(GLFWwindow *window, int iconified) {
  if (iconified) TrayManager::getInstance().setWindowVisible(false);
  else TrayManager::getInstance().setWindowVisible(true);
}
void windowFocusCallback(GLFWwindow *window, int focused) {
  if (focused) TrayManager::getInstance().setWindowVisible(true);
}

int main() {
  if (!glfwInit()) return -1;

#if defined(__APPLE__)
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  // 初始化语言与 UI
  g_current_locale = Config::DEFAULT_LOCALE;
  I18nManager::getInstance().loadLanguage(g_current_locale);
  SettingsWindow settingsWin;
  settingsWin.init(g_current_locale);

  // 创建窗口
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  GLFWwindow *window = glfwCreateWindow(Config::WINDOW_WIDTH, Config::WINDOW_HEIGHT, Config::WINDOW_TITLE, NULL, NULL);
  if (!window) { glfwTerminate(); return -1; }

  //加载窗口图片
#ifndef __APPLE__
  GLFWimage images[1];
  images[0].pixels = stbi_load("assets/icons/tray.png", &images[0].width, &images[0].height, 0, 4);
  if (images[0].pixels) {
    glfwSetWindowIcon(window, 1, images);
    stbi_image_free(images[0].pixels);
  }
  else {
    std::cerr << "Warning: Failed to load window icon! Reason: " << stbi_failure_reason() << std::endl;
  }
#endif

  glfwMakeContextCurrent(window); // 告诉 OpenGL：接下来的所有画画操作，都画在这个 window 上！
  glfwSwapInterval(1);

  glfwSetWindowCloseCallback(window, windowCloseCallback);
  glfwSetWindowFocusCallback(window, windowFocusCallback);
  glfwSetWindowIconifyCallback(window, windowIconifyCallback);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO(); (void)io;
  ImGui::StyleColorsDark();
  ImGui::GetStyle().WindowRounding = 0.0f;
  ImGui_ImplGlfw_InitForOpenGL(window, true); // 告诉 ImGui：去盯着这个 window 抓鼠标和键盘！
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

  // ==========================================================
  // 🌟 初始化系统级全局热键模块
  // ==========================================================
  auto &hotkeyMgr = HotkeyManager::getInstance();
  hotkeyMgr.start(); // 启动后台守护线程

  // 假设 1 代表截图，注册 Ctrl + Alt + A
  // hotkeyMgr.registerHotkey(Config::SCREENSHOT_EVENT_ID, KeyModifier::Alt, KeyCode::S); 交给control_window注册快捷键
  // 🌟 跨线程事件标记：当后台监听到capture热键时，置为 true
  std::atomic<bool> trigger_screenshot{ false };
  hotkeyMgr.addCallback([&trigger_screenshot](const HotkeyEvent &event) {
    if (event.id == Config::SCREENSHOT_EVENT_ID) {
      // ⚠️ 此时处于后台线程触发回调函数，绝对不能直接操作 UI 或 OpenGL
      // 将原子标记置为 true，让主线程在下一帧去消费它
      trigger_screenshot = true;
      std::cerr << "screenshot" << std::endl;
    }
  });
  // 假设 1 代表截图，注册 Ctrl + Alt + A
  // hotkeyMgr.registerHotkey(Config::TRANSLATION_EVENT_ID, KeyModifier::Alt, KeyCode::T);
  std::atomic<bool> trigger_translation{ false };
  hotkeyMgr.addCallback([&trigger_translation](const HotkeyEvent &event) {
    if (event.id == Config::TRANSLATION_EVENT_ID) {
      trigger_translation = true;
      std::cerr << "translation" << std::endl;
    }
  });
  // ==========================================================

  // =================== 主循环 ===================
  while (!glfwWindowShouldClose(window) && !should_quit) {

    // 🌟 1. 跨线程任务消费：在这里执行业务逻辑，绝对安全！
    if (trigger_screenshot.exchange(false)) { // 读取并重置为 false
      std::cerr << "[SnapTrans] 触发全局截图唤醒！" << std::endl;

      // 唤醒主窗口 (如果之前被隐藏了)
      trayManager.setWindowVisible(true);

      // TODO: 在这里触发你的跨平台截屏库逻辑
    }

    // 2. 字体动态合并机制
    if (g_need_font_rebuild) {
      ImGui_ImplOpenGL3_DestroyDeviceObjects();
      io.Fonts->Clear();

      ImFontConfig font_config;
      font_config.OversampleH = 2;
      font_config.OversampleV = 2;

      if (fs::exists(Config::BASE_FONT_PATH)) {
        io.Fonts->AddFontFromFileTTF(Config::BASE_FONT_PATH, Config::FONT_SIZE, &font_config, io.Fonts->GetGlyphRangesDefault());
      }
      else {
        io.Fonts->AddFontDefault();
      }

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

    // 3. 事件分发与休眠
    // 巧妙修改：如果收到热键信号，立即放弃休眠，全速渲染下一帧
    if (glfwGetWindowAttrib(window, GLFW_VISIBLE) || trigger_screenshot) {
      glfwPollEvents();
    }
    else {
      glfwWaitEventsTimeout(Config::IDLE_TIMEOUT);
    }

    trayManager.update();

    // 4. 渲染骨架
    if (glfwGetWindowAttrib(window, GLFW_VISIBLE)) {
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      glfwSetWindowTitle(window, tr("main_window.title"));

      // 核心：把控制权交给 UI 类
      settingsWin.render(g_need_font_rebuild, g_current_locale);

      ImGui::Render();

      int display_w, display_h;
      glfwGetFramebufferSize(window, &display_w, &display_h);
      glViewport(0, 0, display_w, display_h);

      glClearColor(Config::CLEAR_COLOR[0], Config::CLEAR_COLOR[1], Config::CLEAR_COLOR[2], Config::CLEAR_COLOR[3]);
      glClear(GL_COLOR_BUFFER_BIT);

      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
      glfwSwapBuffers(window);
    }
  }

  // =================== 清理与退出 ===================
  // 🌟 必须在程序退出前关闭后台守护线程，释放系统句柄
  hotkeyMgr.stop();

  trayManager.shutdown();
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}