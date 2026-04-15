#ifndef APPLICATION_H
#define APPLICATION_H

#include <string>
#include <atomic>

class Application {
  public:
  static Application &getInstance() {
    static Application instance;
    return instance;
  }

  bool init();
  void runLoop();
  void shutdown();
  void shouldQuit() { should_quit_ = true; }

  // 暴露给外部更改全局状态
  void changeLanguage(const std::string &new_Language);

  // 跨线程安全触发接口
  void triggerScreenshot() { signal_screenshot_ = true; }
  void triggerTranslation() { signal_translation_ = true; }
  void setNeedFontRebuild(bool val) {need_font_rebuild_ = val;}
  private:
  Application() = default;
  ~Application() = default;
  Application(const Application &) = delete;
  Application &operator=(const Application &) = delete;

  bool should_quit_ = false;
  bool need_font_rebuild_ = false;

  std::atomic<bool> signal_screenshot_{ false };
  std::atomic<bool> signal_translation_{ false };
};

#endif // APPLICATION_H