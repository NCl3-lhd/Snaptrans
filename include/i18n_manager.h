#ifndef I18N_MANAGER_H
#define I18N_MANAGER_H

#include <string>
#include <unordered_map>

class I18nManager {
    public:
    static I18nManager &getInstance();
    bool loadLanguage(const std::string &localeCode);
    std::string get(const std::string &key);
    std::string currentLocale() const { return currentLocale_; }

    private:
    I18nManager() = default;
    std::unordered_map<std::string, std::string> dictionary_;
    std::string currentLocale_ = "zh-CN";
};

#define tr(key) I18nManager::getInstance().get(key).c_str()

#endif