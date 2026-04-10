#include "tray_manager.h"
#include "i18n_manager.h" // 引入 i18n
#include <iostream>
#include <cstring>

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

void TrayManager::setWindowVisible(bool visible) {
	if (!window_) return;

	windowVisible_ = visible;
	if (visible) {
		// 1. 先搭台：精准施药
		if (glfwGetWindowAttrib(window_, GLFW_ICONIFIED)) {
			if (glfwGetWindowAttrib(window_, GLFW_MAXIMIZED)) {
				glfwMaximizeWindow(window_);
			}
			else {
				glfwRestoreWindow(window_);
			}
		}
		// 2. 再拉幕
		glfwShowWindow(window_);
		// 3. 聚焦
		glfwFocusWindow(window_);
	}
	else {
		glfwHideWindow(window_);
	}

	menuItems_[MenuIndex::ShowHide].checked = windowVisible_ ? 1 : 0;
	tray_update(&tray_);
}

void TrayManager::toggleWindowVisibility() {
	setWindowVisible(!windowVisible_);
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

	// 使用 i18n 提取菜单文本
	buildItem(MenuIndex::ShowHide, tr("menu.show_hide"), onShowHideClicked, windowVisible_);
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

	// 1. 重新执行一遍菜单构建逻辑（这会触发 tr() 重新去读最新的 JSON）
	setupMenu();
	// 2. 拿着组装好的新菜单，通知操作系统底层进行强制刷新
	tray_update(&tray_);
}


void TrayManager::onShowHideClicked(struct tray_menu *item) {
	TrayManager *manager = static_cast<TrayManager *>(item->context);
	if (manager) manager->toggleWindowVisibility();
}

void TrayManager::onSettingsClicked(struct tray_menu *item) {
	TrayManager *manager = static_cast<TrayManager *>(item->context);
	if (manager) manager->setWindowVisible(true);
}

void TrayManager::onAboutClicked(struct tray_menu *item) {
	// 暂时也可以呼出主窗口
	TrayManager *manager = static_cast<TrayManager *>(item->context);
	if (manager) manager->setWindowVisible(true);
}

void TrayManager::onQuitClicked(struct tray_menu *item) {
	TrayManager *manager = static_cast<TrayManager *>(item->context);
	if (manager && manager->exitCallback_) manager->exitCallback_();
}