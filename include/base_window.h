#ifndef BASE_WINDOW_H
#define BASE_WINDOW_H

#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <string>
#include "config.h"
#include "i18n_manager.h"
class BaseWindow {
  public:
  BaseWindow() = default;
  virtual ~BaseWindow() = default;

  // 必须由子类实现的接口
  virtual bool init() = 0;
  virtual void render() = 0;
  virtual void rebuildFonts() {

    if (!ctx_) return; // 保护机制
    glfwMakeContextCurrent(window_);
    ImGui::SetCurrentContext(ctx_);
    ImGui_ImplOpenGL3_DestroyDeviceObjects();
    ImGuiIO &io = ImGui::GetIO();
    io.Fonts->Clear();

    ImFontConfig font_config;
    font_config.OversampleH = 2;
    font_config.OversampleV = 2;
    // 删除了 MergeMode，现在每次只加载一种主字体

    bool font_loaded = false;

    // 1. 根据 locale 加载对应的语言字体
    std::string fontFilePath = I18nManager::getInstance().getFontPath();
    if (std::filesystem::exists(fontFilePath)) {
      // 中文环境：直接加载中文字体，GetGlyphRangesChineseFull() 内部已包含基础英文字符
      io.Fonts->AddFontFromFileTTF(fontFilePath.c_str(), Config::FONT_SIZE, &font_config, io.Fonts->GetGlyphRangesChineseFull());
      font_loaded = true;
    }

    // 2. 极致兜底方案：如果指定的字体文件被用户误删了，至少保证程序不崩溃、有字看
    if (!font_loaded) {
      io.Fonts->AddFontDefault();
    }

    ImGui_ImplOpenGL3_CreateDeviceObjects();
  }

  // 窗口生命周期自治
  virtual void shutdown() {
    if (ctx_) {
      ImGui::SetCurrentContext(ctx_);
      ImGui_ImplOpenGL3_Shutdown();
      ImGui_ImplGlfw_Shutdown();
      ImGui::DestroyContext(ctx_);
      ctx_ = nullptr;
    }
    if (window_) {
      glfwDestroyWindow(window_);
      window_ = nullptr;
    }
  }

  // 基础状态控制
  virtual void show() {
    is_visible_ = true;
    if (window_) {
      if (glfwGetWindowAttrib(window_, GLFW_ICONIFIED)) {
        glfwGetWindowAttrib(window_, GLFW_MAXIMIZED) ? glfwMaximizeWindow(window_) : glfwRestoreWindow(window_);
      }
      glfwShowWindow(window_);
      glfwFocusWindow(window_);
    }
  }

  virtual void hide() {
    is_visible_ = false;
    if (window_) glfwHideWindow(window_);
  }

  virtual void toggle() { is_visible_ ? hide() : show(); }
  bool isVisible() const { return is_visible_; }
  GLFWwindow *getGLFWWindow() const { return window_; }

  protected:
  GLFWwindow *window_ = nullptr;
  ImGuiContext *ctx_ = nullptr;
  bool is_visible_ = false;
};

#endif // BASE_WINDOW_H