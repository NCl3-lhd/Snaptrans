#include "tray_manager.h"
#include <iostream>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
// Windows 中文防乱码转码
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

bool TrayManager::initialize(GLFWwindow *window, const std::string &iconPath) {
	window_ = window;
	storedIconPath_ = iconPath;
	tray_.icon = const_cast<char *>(storedIconPath_.c_str());

	setupMenu();

	if (tray_init(&tray_) < 0) {
		std::cerr << "Failed to initialize system tray" << std::endl;
		return false;
	}

	running_ = true;
	std::cout << "System tray initialized successfully" << std::endl;
	return true;
}

void TrayManager::update() {
	if (!running_) return;
	if (tray_loop(0) != 0) {
		running_ = false;
	}
}

void TrayManager::shutdown() {
	if (running_) {
		tray_exit();
		running_ = false;
	}
}

void TrayManager::setWindowVisible(bool visible) {
	if (!window_) return;

	windowVisible_ = visible;
	if (visible) {
		glfwShowWindow(window_);
		glfwFocusWindow(window_);
	}
	else {
		glfwHideWindow(window_);
	}

	// 🌟 使用常量索引，代码阅读起来像自然语言一样顺畅
	menuItems_[MenuIndex::ShowHide].checked = windowVisible_ ? 1 : 0;
	tray_update(&tray_);
}

void TrayManager::toggleWindowVisibility() {
	setWindowVisible(!windowVisible_);
}

void TrayManager::setupMenu() {
	// 🌟 核心优化：定义一个局部 Lambda 匿名函数，专门用来构建普通菜单项
	// 这样能消灭大量重复的 memset 和 #ifdef 代码，极度清爽
	auto buildItem = [&](MenuIndex index, const std::string &text, void (*cb)(struct tray_menu *), bool checked = false) {
#ifdef _WIN32
		menuTexts_[index] = utf8ToLocalCodepage(text.c_str());
#else
		menuTexts_[index] = text;
#endif
		memset(&menuItems_[index], 0, sizeof(struct tray_menu)); // 清零洗净内存
		menuItems_[index].text = const_cast<char *>(menuTexts_[index].c_str());
		menuItems_[index].checked = checked ? 1 : 0;
		menuItems_[index].cb = cb;
		menuItems_[index].context = this;
	};

	// 局部的 Lambda：专门用来构建分隔线
	auto buildSeparator = [&](MenuIndex index) {
		memset(&menuItems_[index], 0, sizeof(struct tray_menu));
		menuItems_[index].text = const_cast<char *>("-");
	};

	// ⬇️ 接下来组装菜单，逻辑清晰得就像在写配置文件 ⬇️

	buildItem(MenuIndex::ShowHide, "显示/隐藏", onShowHideClicked, windowVisible_);
	buildItem(MenuIndex::Settings, "设置", onSettingsClicked);
	buildSeparator(MenuIndex::Separator1);
	buildItem(MenuIndex::About, "关于", onAboutClicked);
	buildSeparator(MenuIndex::Separator2);
	buildItem(MenuIndex::Quit, "退出", onQuitClicked);

	// 结束符 (必须全空)
	memset(&menuItems_[MenuIndex::Terminator], 0, sizeof(struct tray_menu));

	// 把数组底层内存交给 C 库
	tray_.menu = menuItems_.data();
	}

void TrayManager::onShowHideClicked(struct tray_menu *item) {
	TrayManager *manager = static_cast<TrayManager *>(item->context);
	if (manager) manager->toggleWindowVisibility();
}

void TrayManager::onSettingsClicked(struct tray_menu *item) {
	std::cout << "Settings clicked - TODO: implement settings dialog" << std::endl;
}

void TrayManager::onAboutClicked(struct tray_menu *item) {
	std::cout << "SnapTrans - Screen Translation Tool" << std::endl;
	std::cout << "Version: 0.1.0" << std::endl;
}

void TrayManager::onQuitClicked(struct tray_menu *item) {
	TrayManager *manager = static_cast<TrayManager *>(item->context);
	if (manager && manager->exitCallback_) {
		// 仅仅发信号给 main.cpp 退出循环，严禁在此处调用 shutdown() 避免进程暴毙
		manager->exitCallback_();
	}
}