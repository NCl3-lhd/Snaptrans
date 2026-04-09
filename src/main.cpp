#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "tray_manager.h"
#include <iostream>

// 全局标志：是否应该退出应用
static bool shouldQuit = false;

void windowCloseCallback(GLFWwindow* window) {
    glfwSetWindowShouldClose(window, GLFW_FALSE);
    glfwHideWindow(window);
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
  GLFWwindow *window = glfwCreateWindow(1024, 768, "SnapTrans - Initializing...", NULL, NULL);
  if (!window) {
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // 开启垂直同步 (VSync)
  
  // 设置窗口关闭回调（最小化到托盘而不是退出）
  glfwSetWindowCloseCallback(window, windowCloseCallback);

  // 3. 初始化 ImGui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO(); (void)io;
  ImGui::StyleColorsDark(); // 使用暗黑主题

  // 初始化 ImGui 的 GLFW 和 OpenGL 后端
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");

  // 初始化系统托盘
  auto& trayManager = SnapTrans::TrayManager::getInstance();
  
  // macOS: App Bundle 使用 Template 图标名（imageNamed: 从 Resources 加载）
  // 文件名包含 "Template" 后缀，系统自动识别为 template image
  // Windows: 使用 .ico 格式（ExtractIconEx 只支持 .ico）
  // Linux: 使用 .png 格式
  #if defined(__APPLE__)
    std::string iconPath = "trayTemplate";
  #elif defined(_WIN32) || defined(_WIN64)
    std::string iconPath = "icons/tray.ico";
  #else
    std::string iconPath = "icons/tray.png";
  #endif
  
  if (!trayManager.initialize(window, iconPath)) {
    std::cerr << "Warning: Failed to initialize system tray" << std::endl;
  }
  
  // 设置托盘退出回调
  trayManager.setExitCallback([&window]() {
    shouldQuit = true;
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  });

  // 4. 主循环 (The Main Loop)
  while (!glfwWindowShouldClose(window) && !shouldQuit) {
    // 处理系统事件
    glfwPollEvents();
    
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