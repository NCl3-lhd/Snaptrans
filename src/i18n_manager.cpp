#include "i18n_manager.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>

namespace Snaptrans {

    I18nManager &I18nManager::getInstance() {
        static I18nManager instance;
        return instance;
    }

    bool I18nManager::initialize(const std::string &localesPath) {
        localesPath_ = localesPath;

        // 检测系统默认语言
        std::string defaultLang = detectSystemLanguage();

        // 尝试加载系统语言，失败则使用英语
        if (!loadLanguage(defaultLang)) {
            if (!loadLanguage("en_US")) {
                std::cerr << "Failed to load any language file!" << std::endl;
                return false;
            }
        }

        initialized_ = true;
        std::cout << "I18nManager initialized with language: " << currentLanguage_ << std::endl;
        return true;
    }

    bool I18nManager::loadLanguage(const std::string &languageCode) {
        std::string filePath = localesPath_ + "/" + languageCode + ".json";

        // 检查文件是否存在
        if (!std::filesystem::exists(filePath)) {
            std::cerr << "Language file not found: " << filePath << std::endl;
            return false;
        }

        // 读取 JSON 文件
        std::ifstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "Failed to open language file: " << filePath << std::endl;
            return false;
        }

        try {
            file >> translations_;
            currentLanguage_ = languageCode;

            std::cout << "Loaded language: " << languageCode << std::endl;

            // 触发语言切换回调
            if (onLanguageChange_) {
                onLanguageChange_();
            }

            return true;
        }
        catch (const nlohmann::json::exception &e) {
            std::cerr << "JSON parse error: " << e.what() << std::endl;
            return false;
        }
    }

    std::vector<std::string> I18nManager::getAvailableLanguages() const {
        std::vector<std::string> languages;

        if (!std::filesystem::exists(localesPath_)) {
            return languages;
        }

        for (const auto &entry : std::filesystem::directory_iterator(localesPath_)) {
            if (entry.path().extension() == ".json") {
                languages.push_back(entry.path().stem().string());
            }
        }

        return languages;
    }

    const char *I18nManager::tr(const std::string &key) const {
        lastTranslation_ = trString(key);
        return lastTranslation_.c_str();
    }

    std::string I18nManager::trString(const std::string &key) const {
        const nlohmann::json *value = getNestedValue(key);

        if (value && value->is_string()) {
            return value->get<std::string>();
        }

        // 未找到翻译，返回键名本身
        return key;
    }

    const nlohmann::json *I18nManager::getNestedValue(const std::string &key) const {
        // 分割键名（如 "menu.show_hide" -> ["menu", "show_hide"]）
        std::vector<std::string> parts;
        std::string part;
        std::istringstream iss(key);

        while (std::getline(iss, part, '.')) {
            parts.push_back(part);
        }

        // 逐级访问 JSON
        const nlohmann::json *current = &translations_;

        for (const auto &p : parts) {
            if (!current->is_object() || !current->contains(p)) {
                return nullptr;
            }
            current = &(*current)[p];
        }

        return current;
    }

    void I18nManager::setLanguageChangeCallback(LanguageChangeCallback callback) {
        onLanguageChange_ = callback;
    }

    std::string I18nManager::detectSystemLanguage() const {
        // 检测系统语言环境
        // macOS/Linux: 检查 LANG 环境变量
        // Windows: 使用 GetUserDefaultUILanguage

        std::string lang = "en_US";  // 默认英语

#ifdef _WIN32
        // Windows: 使用 Windows API
        // 这里简化处理，实际应使用 GetUserDefaultUILanguage
        const char *envLang = std::getenv("LANG");
        if (envLang) {
            std::string langStr(envLang);
            if (langStr.find("zh") != std::string::npos || langStr.find("CN") != std::string::npos) {
                lang = "zh_CN";
            }
        }
#else
        // macOS/Linux: 检查 LANG 环境变量
        const char *envLang = std::getenv("LANG");
        if (envLang) {
            std::string langStr(envLang);

            // 提取语言代码（如 "zh_CN.UTF-8" -> "zh_CN"）
            size_t dotPos = langStr.find('.');
            if (dotPos != std::string::npos) {
                langStr = langStr.substr(0, dotPos);
            }

            // 检查是否支持
            std::string testPath = localesPath_ + "/" + langStr + ".json";
            if (std::filesystem::exists(testPath)) {
                lang = langStr;
            }
            else if (langStr.find("zh") != std::string::npos) {
                lang = "zh_CN";  // 中文回退
            }
        }
#endif

        return lang;
    }

} // namespace SnapTrans
