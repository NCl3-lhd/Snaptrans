#ifndef I18N_MANAGER_H
#define I18N_MANAGER_H

#include <string>
#include <functional>
#include <nlohmann/json.hpp>

namespace SnapTrans {

/**
 * @brief 国际化管理器（单例模式）
 * 
 * 负责加载多语言 JSON 配置文件，提供键值映射，
 * 支持运行时热重载语言配置。
 */
class I18nManager {
public:
    using LanguageChangeCallback = std::function<void()>;
    
    /**
     * @brief 获取单例实例
     */
    static I18nManager& getInstance();
    
    /**
     * @brief 初始化国际化管理器
     * @param localesPath 语言文件目录路径（相对路径）
     * @return 成功返回 true
     */
    bool initialize(const std::string& localesPath = "assets/locales");
    
    /**
     * @brief 加载指定语言的配置文件
     * @param languageCode 语言代码（如 "zh_CN", "en_US"）
     * @return 成功返回 true
     */
    bool loadLanguage(const std::string& languageCode);
    
    /**
     * @brief 获取当前语言代码
     */
    const std::string& getCurrentLanguage() const { return currentLanguage_; }
    
    /**
     * @brief 获取支持的 语言列表
     */
    std::vector<std::string> getAvailableLanguages() const;
    
    /**
     * @brief 翻译键值
     * @param key 键名，支持多级路径（如 "menu.show_hide"）
     * @return 翻译后的文本，未找到则返回键名本身
     */
    const char* tr(const std::string& key) const;
    
    /**
     * @brief 翻译键值（std::string 版本）
     */
    std::string trString(const std::string& key) const;
    
    /**
     * @brief 设置语言切换回调（用于通知 UI 刷新）
     */
    void setLanguageChangeCallback(LanguageChangeCallback callback);
    
    /**
     * @brief 检查是否已初始化
     */
    bool isInitialized() const { return initialized_; }

private:
    I18nManager() = default;
    ~I18nManager() = default;
    I18nManager(const I18nManager&) = delete;
    I18nManager& operator=(const I18nManager&) = delete;
    
    /**
     * @brief 从 JSON 对象中获取嵌套键值
     */
    const nlohmann::json* getNestedValue(const std::string& key) const;
    
    /**
     * @brief 检测系统默认语言
     */
    std::string detectSystemLanguage() const;

private:
    bool initialized_ = false;
    std::string localesPath_;
    std::string currentLanguage_;
    nlohmann::json translations_;
    LanguageChangeCallback onLanguageChange_;
    
    // 缓存最后翻译结果（避免频繁构造 std::string）
    mutable std::string lastTranslation_;
};

} // namespace SnapTrans

/**
 * @brief 便捷翻译宏
 * 
 * 用法：ImGui::Button(TR("menu.settings"))
 */
#define TR(key) SnapTrans::I18nManager::getInstance().tr(key)

#endif // I18N_MANAGER_H
