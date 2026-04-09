#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "tray_manager.h"
// #include "i18n_manager.h" // 等你写好了再解开注释
#include <iostream>
#include <filesystem>

// 全局标志：是否应该退出应用
static bool should_quit = false;

// =================== 窗口事件哨兵 (Callbacks) ===================

// 回调 1：拦截点击 "X" 的关闭事件，改为“隐藏到托盘”
void windowCloseCallback(GLFWwindow *window) {
  // 阻止窗口物理销毁
  glfwSetWindowShouldClose(window, GLFW_FALSE);
  // 通知 TrayManager 更新内部账本，并执行隐藏逻辑
  TrayManager::getInstance().setWindowVisible(false);
}

// 回调 2 (哨兵 1)：监听窗口焦点变化 (解决 Mac 拓展坞、Cmd+Tab 强行唤醒的问题)
void windowFocusCallback(GLFWwindow* window, int focused) {
  if (focused) {
    // 窗口重新获得焦点，必然是可见的，强制同步托盘打勾状态
    TrayManager::getInstance().setWindowVisible(true);
  }
}

// 回调 3 (哨兵 2)：监听窗口最小化状态 (解决 Windows 任务栏最小化的同步错位)
void windowIconifyCallback(GLFWwindow* window, int iconified) {
  if (iconified) {
    // 被系统最小化时，通知 TrayManager 取消菜单打勾，并彻底隐藏
    TrayManager::getInstance().setWindowVisible(false);
  } else {
    // 从任务栏点出来恢复时，通知 TrayManager 打上勾
    TrayManager::getInstance().setWindowVisible(true);
  }
}

// ================================================================

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
  GLFWwindow *window = glfwCreateWindow(1024, 768, "Snaptrans - Running", NULL, NULL);
  if (!window) {
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // 开启垂直同步 (VSync)

  // 🌟 注册所有事件拦截器和哨兵
  glfwSetWindowCloseCallback(window, windowCloseCallback);
  glfwSetWindowFocusCallback(window, windowFocusCallback);
  glfwSetWindowIconifyCallback(window, windowIconifyCallback);

  // 3. 初始化 ImGui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO(); (void)io;
  ImGui::StyleColorsDark();

  // 初始化 ImGui 的 GLFW 和 OpenGL 后端
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");

  // 初始化系统托盘
  auto &trayManager = TrayManager::getInstance();

  // 修复后的相对路径
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

  // 设置托盘的“退出”按钮回调
  trayManager.setExitCallback([&window]() {
    should_quit = true;
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  });

  // 4. 主循环 (The Main Loop)
  while (!glfwWindowShouldClose(window) && !should_quit) {
    
    // 🌟 核心性能优化：动态休眠机制
    if (glfwGetWindowAttrib(window, GLFW_VISIBLE)) {
      // 窗口可见时：需要最高刷新率处理鼠标拖拽和渲染，使用非阻塞的 Poll
      glfwPollEvents();
    } else {
      // 窗口隐藏时：程序在后台，主动挂起线程让出 CPU，每 16 毫秒唤醒一次
      glfwWaitEventsTimeout(0.016); 
    }

    // 更新托盘事件（极速看一眼，非阻塞）
    trayManager.update();

    // 只在窗口可见时执行高昂的渲染逻辑
    if (glfwGetWindowAttrib(window, GLFW_VISIBLE)) {
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      // === 在这里写你的 UI 代码 ===
      ImGui::ShowDemoWindow(); // ImGui 官方全家桶演示

      ImGui::Begin("SnapTrans Debug Console");
      ImGui::Text("Hello, SnapTrans is perfectly running!");
      if (ImGui::Button("Click Me")) {
        std::cout << "Button clicked!" << std::endl;
      }
      ImGui::End();
      // ============================

      ImGui::Render();

      int display_w, display_h;
      glfwGetFramebufferSize(window, &display_w, &display_h);
      glViewport(0, 0, display_w, display_h);
      glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT);

      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
      glfwSwapBuffers(window);
    }
  }

  // 5. 退出前清理资源
  trayManager.shutdown(); // 销毁托盘图标，防止出现幽灵残影
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
