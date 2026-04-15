#include "tray_manager.h"
#include "windows_manager.h" // 🌟 引入窗口管家进行状态委派
#include "i18n_manager.h"
#include <iostream>
#include <cstring>
#include "application.h"
// Windows 平台的中文乱码处理保留
#ifdef _WIN32
#include <windows.h>
static std::string utf8ToLocalCodepage(const char *utf8) {
	int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
	if (wlen <= 0) return utf8;
	std::wstring wstr(wlen, 0);
	MultiByteToWideChar(CP_UTF8, 0, utf8, -1, &wstr[0], wlen);
	int clen = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
	if (clen <= 0) return utf8;
	std::string result(clen, 0);
	WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, &result[0], clen, nullptr, nullptr);
	result.pop_back();
	return result;
}
#endif

TrayManager &TrayManager::getInstance() {
	static TrayManager instance;
	return instance;
}

bool TrayManager::initialize(const std::string &iconPath) {
	storedIconPath_ = iconPath;
	tray_.icon = const_cast<char *>(storedIconPath_.c_str());

	setupMenu();

	if (tray_init(&tray_) < 0) {
		std::cerr << "Failed to initialize system tray" << std::endl;
		return false;
	}

	running_ = true;
	return true;
}

void TrayManager::update() {
	if (!running_) return;
	if (tray_loop(0) != 0) running_ = false;
}

void TrayManager::shutdown() {
	if (running_) {
		tray_exit();
		running_ = false;
	}
}

// 🌟 新增：供 WindowManager 调用的同步接口
void TrayManager::updateMenuCheckState(bool isSettingsVisible) {
	if (!running_) return;
	// 更新“显示/隐藏”菜单项的勾选状态
	menuItems_[MenuIndex::ShowHide].checked = isSettingsVisible ? 1 : 0;
	// 通知底层托盘组件重绘
	tray_update(&tray_);
}

void TrayManager::setupMenu() {
	auto buildItem = [&](MenuIndex index, const std::string &text, void (*cb)(struct tray_menu *), bool checked = false) {
#ifdef _WIN32
		menuTexts_[index] = utf8ToLocalCodepage(text.c_str());
#else
		menuTexts_[index] = text;
#endif
		memset(&menuItems_[index], 0, sizeof(struct tray_menu));
		menuItems_[index].text = const_cast<char *>(menuTexts_[index].c_str());
		menuItems_[index].checked = checked ? 1 : 0;
		menuItems_[index].cb = cb;
		menuItems_[index].context = this;
	};

	auto buildSeparator = [&](MenuIndex index) {
		memset(&menuItems_[index], 0, sizeof(struct tray_menu));
		menuItems_[index].text = const_cast<char *>("-");
	};

	// 默认初始状态为 false，真正的勾选状态将由 WindowManager 初始化后同步过来
	buildItem(MenuIndex::ShowHide, tr("menu.show_hide"), onShowHideClicked, false);
	buildItem(MenuIndex::Settings, tr("menu.settings"), onSettingsClicked);
	buildSeparator(MenuIndex::Separator1);
	buildItem(MenuIndex::About, tr("menu.about"), onAboutClicked);
	buildSeparator(MenuIndex::Separator2);
	buildItem(MenuIndex::Quit, tr("menu.quit"), onQuitClicked);

	memset(&menuItems_[MenuIndex::Terminator], 0, sizeof(struct tray_menu));
	tray_.menu = menuItems_.data();
	}

void TrayManager::rebuildMenu() {
	if (!running_) return;
	// 重新执行一次菜单构建逻辑以读取新语言
	setupMenu();
	// 拿着组装好的新菜单，通知操作系统底层进行刷新
	tray_update(&tray_);
}

// ==========================================================
// 🌟 托盘事件回调：将执行权全部上交至 WindowManager
// ==========================================================

void TrayManager::onShowHideClicked(struct tray_menu *item) {
	WindowManager::getInstance().getSettingsWindow()->toggle();
}

void TrayManager::onSettingsClicked(struct tray_menu *item) {
	WindowManager::getInstance().getSettingsWindow()->show();
}

void TrayManager::onAboutClicked(struct tray_menu *item) {
	WindowManager::getInstance().getSettingsWindow()->show();
}

void TrayManager::onQuitClicked(struct tray_menu *item) {
	Application::getInstance().shouldQuit();
}