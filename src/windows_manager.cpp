#include "windows_manager.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "tray_manager.h"
#include "i18n_manager.h"
#include "config.h"
#include <iostream>
#include <filesystem>

// 只在此处展开 stbi 的实现
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace fs = std::filesystem;

WindowManager &WindowManager::getInstance() {
  static WindowManager instance;
  return instance;
}

bool WindowManager::init() {
  if (!glfwInit()) return false;

#if defined(__APPLE__)
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  current_locale_ = Config::DEFAULT_LOCALE;
  I18nManager::getInstance().loadLanguage(current_locale_);
  settingsWin_.init(current_locale_);

  // 1. 初始化设置主窗口
  if (!initSettingsWindow()) return false;

  // 2. 初始化全屏透明截图遮罩层窗口 (双原生窗口架构)
  if (!initOverlayWindow()) return false;

  // 3. 初始化系统托盘 (彻底剥离窗口句柄)
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

bool WindowManager::initSettingsWindow() {
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  settings_window_ = glfwCreateWindow(Config::WINDOW_WIDTH, Config::WINDOW_HEIGHT, Config::WINDOW_TITLE, NULL, NULL);
  if (!settings_window_) return false;

  // C++作用域魔法：绑定当前实例至窗口句柄
  glfwSetWindowUserPointer(settings_window_, this);
  glfwSetWindowCloseCallback(settings_window_, settingsWindowCloseCallback);
  glfwSetWindowFocusCallback(settings_window_, settingsWindowFocusCallback);
  glfwSetWindowIconifyCallback(settings_window_, settingsWindowIconifyCallback);

#ifndef __APPLE__
  GLFWimage images[1];
  images[0].pixels = stbi_load("assets/icons/tray.png", &images[0].width, &images[0].height, 0, 4);
  if (images[0].pixels) {
    glfwSetWindowIcon(settings_window_, 1, images);
    stbi_image_free(images[0].pixels);
  }
#endif

  glfwMakeContextCurrent(settings_window_);
  glfwSwapInterval(1);

  IMGUI_CHECKVERSION();
  ctx_settings_ = ImGui::CreateContext();
  ImGui::SetCurrentContext(ctx_settings_);
  ImGui::StyleColorsDark();
  ImGui::GetStyle().WindowRounding = 0.0f;
  ImGui_ImplGlfw_InitForOpenGL(settings_window_, true);
  ImGui_ImplOpenGL3_Init("#version 330");

  return true;
}

bool WindowManager::initOverlayWindow() {
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
  glfwWindowHint(GLFW_FLOATING, GLFW_TRUE); // 永远置顶
  glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE); // 透明像素支持

  GLFWmonitor *primary = glfwGetPrimaryMonitor();
  const GLFWvidmode *mode = glfwGetVideoMode(primary);

  // 创建一个与主屏幕等宽高的透明窗口 (可选：支持多屏跨屏计算)
  overlay_window_ = glfwCreateWindow(mode->width, mode->height, "SnapTrans_Overlay", NULL, NULL);
  if (!overlay_window_) return false;

  glfwSetWindowUserPointer(overlay_window_, this);
  // 此处可拦截 ESC 等专属按键

  glfwMakeContextCurrent(overlay_window_);
  glfwSwapInterval(1);

  ctx_overlay_ = ImGui::CreateContext();
  ImGui::SetCurrentContext(ctx_overlay_);
  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(overlay_window_, true);
  ImGui_ImplOpenGL3_Init("#version 330");

  // 切回主窗口准备就绪
  glfwMakeContextCurrent(settings_window_);
  ImGui::SetCurrentContext(ctx_settings_);

  return true;
}

void WindowManager::runLoop() {
  while (!should_quit_) {
    // 1. 跨线程任务消费 (状态机切换)
    if (signal_screenshot_.exchange(false)) {
      is_screenshotting_ = true;
      // TODO: 处理翻译悬浮窗逻辑
      // glfwShowWindow(overlay_window_);
      // glfwFocusWindow(overlay_window_);

    }
    if (signal_translation_.exchange(false)) {
      show_translation_ = true;
      // TODO: 处理翻译悬浮窗逻辑
    }

    // 2. 字体动态合并机制 (针对独立上下文处理)
    if (need_font_rebuild_) {
      rebuildFonts();
      need_font_rebuild_ = false;
    }

    // 3. 事件分发与休眠调度 (按需渲染)
    if (show_settings_ || is_screenshotting_ || signal_screenshot_ || signal_translation_) {
      glfwPollEvents();
    }
    else {
      glfwWaitEventsTimeout(Config::IDLE_TIMEOUT);
    }

    // // 防御性拦截（点击关闭是隐藏而非销毁）
    // if (glfwWindowShouldClose(settings_window_)) {
    //   glfwSetWindowShouldClose(settings_window_, GLFW_FALSE);
    //   setSettingsVisible(false);
    // }

    TrayManager::getInstance().update();

    // ==========================================
    // 4. 双原生窗口独立渲染管道 (Dual Contexts)
    // ==========================================

    // 渲染管线 A: 设置主面板
    if (show_settings_) {
      renderSettingsFrame();
    }

    // // 渲染管线 B: 沉浸式透明截图层
    // if (is_screenshotting_) {
    //   glfwMakeContextCurrent(overlay_window_);
    //   ImGui::SetCurrentContext(ctx_overlay_);

    //   ImGui_ImplOpenGL3_NewFrame();
    //   ImGui_ImplGlfw_NewFrame();
    //   ImGui::NewFrame();

    //   ImGui::SetNextWindowPos(ImVec2(0, 0));
    //   ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    //   ImGui::Begin("ScreenshotLayer", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus);

    //   ImGui::TextColored(ImVec4(1, 0, 0, 1), "[Screenshot Mode Active] Press ESC to cancel.");
    //   if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
    //     is_screenshotting_ = false;
    //     glfwHideWindow(overlay_window_);
    //   }
    //   // TODO: 这里挂载截图选取框的逻辑

    //   ImGui::End();

    //   ImGui::Render();
    //   int display_w, display_h;
    //   glfwGetFramebufferSize(overlay_window_, &display_w, &display_h);
    //   glViewport(0, 0, display_w, display_h);
    //   // 铺设 20% 透明度的黑底遮罩
    //   glClearColor(0.0f, 0.0f, 0.0f, 0.2f);
    //   glClear(GL_COLOR_BUFFER_BIT);
    //   ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    //   glfwSwapBuffers(overlay_window_);
    // }

  }
}

void WindowManager::rebuildFonts() {
  auto setupContextFont = [](ImGuiContext *ctx, const std::string &locale) {
    ImGui::SetCurrentContext(ctx);
    ImGui_ImplOpenGL3_DestroyDeviceObjects();
    ImGuiIO &io = ImGui::GetIO();
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

    if (locale == "zh_CN" && fs::exists(Config::CJK_FONT_PATH)) {
      font_config.MergeMode = true;
      font_config.PixelSnapH = false;
      io.Fonts->AddFontFromFileTTF(Config::CJK_FONT_PATH, Config::FONT_SIZE, &font_config, io.Fonts->GetGlyphRangesChineseFull());
    }
    ImGui_ImplOpenGL3_CreateDeviceObjects();
  };

  setupContextFont(ctx_settings_, current_locale_);
  setupContextFont(ctx_overlay_, current_locale_);
}

void WindowManager::shutdown() {
  TrayManager::getInstance().shutdown();

  // 释放 Overlay
  glfwMakeContextCurrent(overlay_window_);
  ImGui::SetCurrentContext(ctx_overlay_);
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext(ctx_overlay_);
  glfwDestroyWindow(overlay_window_);

  // 释放主窗口
  glfwMakeContextCurrent(settings_window_);
  ImGui::SetCurrentContext(ctx_settings_);
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext(ctx_settings_);
  glfwDestroyWindow(settings_window_);

  glfwTerminate();
}

void WindowManager::setShouldQuit() { should_quit_ = true; }
void WindowManager::toggleSettingsVisible() { setSettingsVisible(!show_settings_); }

void WindowManager::setSettingsVisible(bool visible) {
  if (!settings_window_) return;
  show_settings_ = visible;
  if (visible) {
    if (glfwGetWindowAttrib(settings_window_, GLFW_ICONIFIED)) {
      glfwGetWindowAttrib(settings_window_, GLFW_MAXIMIZED) ? glfwMaximizeWindow(settings_window_) : glfwRestoreWindow(settings_window_);
    }
    glfwShowWindow(settings_window_);
    glfwFocusWindow(settings_window_);
  }
  else {
    settingsWin_.stopRecordingHottkey(); // 失焦时自动停止快捷键录制
    // 隐藏前也强制刷新一帧，防止操作系统截取残影
    if (glfwGetWindowAttrib(settings_window_, GLFW_VISIBLE)) {
      renderSettingsFrame();
    }
    glfwHideWindow(settings_window_); // 隐藏后opengl不让这帧不会进入缓存池
  }
  // 同步给系统托盘更新勾选状态
  TrayManager::getInstance().updateMenuCheckState(visible);
}

void WindowManager::triggerScreenshot() { signal_screenshot_ = true; }
void WindowManager::triggerTranslation() { signal_translation_ = true; }

// === 哨兵回调路由机制 ===
void WindowManager::settingsWindowCloseCallback(GLFWwindow *window) {
  auto wm = static_cast<WindowManager *>(glfwGetWindowUserPointer(window));
  if (wm) wm->setSettingsVisible(false);
}
void WindowManager::settingsWindowIconifyCallback(GLFWwindow *window, int iconified) {
  auto wm = static_cast<WindowManager *>(glfwGetWindowUserPointer(window));
  if (wm && iconified) wm->setSettingsVisible(false);
}
void WindowManager::settingsWindowFocusCallback(GLFWwindow *window, int focused) {
  auto wm = static_cast<WindowManager *>(glfwGetWindowUserPointer(window));
  if (wm) { 
    if (focused) {
      wm->setSettingsVisible(true);
    }
    else {
      // 🌟 核心机制：一旦失焦（发生于最小化动作之前）
      // 1. 立刻停止录制状态
      wm->settingsWin_.stopRecordingHottkey();
      // 2. 强制 OpenGL 渲染一帧干净的 UI，覆盖掉底层的显存缓存
      wm->renderSettingsFrame();
    }
  }
}

void WindowManager::renderSettingsFrame() {
  glfwMakeContextCurrent(settings_window_);
  ImGui::SetCurrentContext(ctx_settings_);  // 记录着 上一帧的 UI 状态、交互逻辑、资源资产、渲染指令

  ImGui_ImplOpenGL3_NewFrame(); 
  ImGui_ImplGlfw_NewFrame();  // 把 GLFW 截获的鼠标坐标、按键状态、滚轮数值以及两次采样之间的时间增量（Delta Time）塞进 ImGuiIO 结构体中
  ImGui::NewFrame();  // 清空内存中所有的顶点缓冲区 

  // 业务 UI 代码
  glfwSetWindowTitle(settings_window_, tr("main_window.title"));
  settingsWin_.render(need_font_rebuild_, current_locale_);

  ImGui::Render();  // 封箱，将上述描述转换为顶点数
  int display_w, display_h;
  glfwGetFramebufferSize(settings_window_, &display_w, &display_h);
  glViewport(0, 0, display_w, display_h);
  glClearColor(Config::CLEAR_COLOR[0], Config::CLEAR_COLOR[1], Config::CLEAR_COLOR[2], Config::CLEAR_COLOR[3]);
  glClear(GL_COLOR_BUFFER_BIT);
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData()); // 搬运。把封好的箱子交给 OpenGL 渲染器发往显卡
  glfwSwapBuffers(settings_window_);  // 翻牌。把显卡画好的结果展示给用户
}