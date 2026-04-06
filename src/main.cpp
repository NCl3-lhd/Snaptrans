#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <iostream>

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

  // 3. 初始化 ImGui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO(); (void)io;
  ImGui::StyleColorsDark(); // 使用暗黑主题

  // 初始化 ImGui 的 GLFW 和 OpenGL 后端
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");

  // 4. 主循环 (The Main Loop)
  while (!glfwWindowShouldClose(window)) {
    // 处理系统事件
    glfwPollEvents();

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
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f); // 窗口背景色 (深灰色)
    glClear(GL_COLOR_BUFFER_BIT);

    // 把 ImGui 的数据交给 OpenGL 画出来
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // 交换缓冲，显示画面
    glfwSwapBuffers(window);
  }

  // 5. 退出前清理资源
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}