#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "tray_manager.h"
#include "i18n_manager.h"
#include <iostream>
#include <filesystem>

// 全局标志：是否应该退出应用
static bool should_quit = false;

void windowCloseCallback(GLFWwindow *window) {
  glfwSetWindowShouldClose(window, GLFW_FALSE);
  TrayManager::getInstance().setWindowVisible(false); // glfwHideWindow(window);
}

void windowIconifyCallback(GLFWwindow *window, int iconified) {
  if (iconified) {
    TrayManager::getInstance().setWindowVisible(false);
  }
}

int main() {
  // 1. 初始化 GLFW
  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW!" << std::endl;
    return -1;
  }

  // 设置 OpenGL 版本 (Mac 需要明确指定核心模式，Windows 随意)
#if defined(__APPLE__)
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  // 2. 创建窗口
  GLFWwindow *window = glfwCreateWindow(1024, 768, "Snaptrans - Initializing...", NULL, NULL);
  if (!window) {
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // 开启垂直同步 (VSync)

  // 设置窗口关闭和最小化回调（交给托盘处理）
  glfwSetWindowCloseCallback(window, windowCloseCallback);
  glfwSetWindowIconifyCallback(window, windowIconifyCallback);
  // 3. 初始化 ImGui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO(); (void)io;
  ImGui::StyleColorsDark();

  // // 加载中文字体
  // std::string fontPath = "assets/fonts/NotoSansSC-Regular.ttf";
  // bool fontLoaded = false;

  // if (std::filesystem::exists(fontPath)) {
  //   ImFontConfig fontConfig;
  //   fontConfig.OversampleH = 2;
  //   fontConfig.OversampleV = 1;
  //   fontConfig.PixelSnapH = true;

  //   static const ImWchar chineseRanges[] = {
  //     0x0020, 0x00FF,  // Basic Latin + Latin Supplement
  //     0x4E00, 0x9FFF,  // CJK Unified Ideographs
  //     0
  //   };

  //   ImFont *font = io.Fonts->AddFontFromFileTTF(
  //     fontPath.c_str(),
  //     18.0f,
  //     &fontConfig,
  //     chineseRanges
  //   );

  //   if (font) {
  //     io.FontDefault = font;
  //     fontLoaded = true;
  //     std::cout << "Loaded Chinese font: " << fontPath << std::endl;
  //   }
  // }

  // if (!fontLoaded) {
  //   std::cout << "Warning: Chinese font not found, using default font" << std::endl;
  //   std::cout << "Download NotoSansSC-Regular.ttf to assets/fonts/ for Chinese support" << std::endl;
  // }

  // // 初始化国际化
  // auto &i18n = I18nManager::getInstance();
  // if (!i18n.initialize("assets/locales")) {
  //   std::cerr << "Warning: Failed to initialize i18n manager" << std::endl;
  // }

  // 初始化 ImGui 的 GLFW 和 OpenGL 后端
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");

  // 初始化系统托盘
  auto &trayManager = TrayManager::getInstance();

  // macOS: App Bundle 使用 Template 图标名（imageNamed: 从 Resources 加载）
  // 文件名包含 "Template" 后缀，系统自动识别为 template image
  // Windows: 使用 .ico 格式（ExtractIconEx 只支持 .ico）
  // Linux: 使用 .png 格式
#if defined(__APPLE__)
  std::string iconPath = "trayTemplate";
#elif defined(_WIN32) || defined(_WIN64)
  std::string iconPath = "assets/icons/tray.ico";
#else
  std::string iconPath = "assets/icons/tray.png";
#endif

  if (!trayManager.initialize(window, iconPath)) {
    std::cerr << "Warning: Failed to initialize system tray" << std::endl;
  }

  // 设置托盘退出回调
  trayManager.setExitCallback([&window]() {
    should_quit = true;
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  });

  // 4. 主循环 (The Main Loop)
  while (!glfwWindowShouldClose(window) && !should_quit) {
    // 处理系统事件
    if (glfwGetWindowAttrib(window, GLFW_VISIBLE)) {
      // 窗口可见时：需要最高刷新率处理鼠标拖拽和渲染，使用非阻塞的 Poll
      glfwPollEvents();
    }
    else {
      // 窗口隐藏时：程序在后台，主动挂起线程让出 CPU，每 16 毫秒唤醒一次更新托盘
      glfwWaitEventsTimeout(0.015);
    }

    // 更新托盘事件（非阻塞）
    trayManager.update();

    // 只在窗口可见时渲染
    if (glfwGetWindowAttrib(window, GLFW_VISIBLE)) {
      // 开启 ImGui 新的一帧
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      // === 在这里写你的 UI 代码 ===
      // 召唤 ImGui 官方的强大功能演示面板 (看完效果后可以删掉这行)
      ImGui::ShowDemoWindow();

      // 渲染你的第一个悬浮窗
      ImGui::Begin("SnapTrans Debug Console");
      ImGui::Text("Hello, SnapTrans is running!");
      if (ImGui::Button("Click Me")) {
        std::cout << "Button clicked!" << std::endl;
      }
      ImGui::End();
      // ============================

      // 结束逻辑计算，生成渲染数据
      ImGui::Render();

      // OpenGL 清屏并绘制
      int display_w, display_h;
      glfwGetFramebufferSize(window, &display_w, &display_h);
      glViewport(0, 0, display_w, display_h);
      glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT);

      // 把 ImGui 的数据交给 OpenGL 画出来
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

      // 交换缓冲，显示画面
      glfwSwapBuffers(window);
    }
  }

  // 5. 退出前清理资源
  trayManager.shutdown();
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}