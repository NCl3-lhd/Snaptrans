#ifndef SETTINGS_WINDOW_H
#define SETTINGS_WINDOW_H
#include <string>
#include <vector>

class SettingsWindow {
  public:
  SettingsWindow() = default;
  ~SettingsWindow() = default;

  // 初始化：负责扫描本地语言包，设置初始选中的语言
  void init(const std::string &initial_locale);

  // 核心渲染函数：每帧调用。
  // 传入引用，当用户切换语言时，通知主循环去重建字体
  void render(bool &out_need_font_rebuild, std::string &out_current_locale);

  private:
  std::vector<std::string> locales_;
  int current_locale_idx_ = 0;

  // UI 状态数据
  bool autoStart_ = false;
  char defaultSavePath_[256] = "C:\\SnapTrans\\Images";
};
#endif // 
