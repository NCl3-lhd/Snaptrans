#ifndef I18N_MANAGER_H
#define I18N_MANAGER_H

#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include "config.h"

class I18nManager {
    using json = nlohmann::json;
    public:
    static I18nManager &getInstance();
    bool init();
    bool loadLanguage(const std::string &language);
    std::string get(const std::string &key);
    std::string getCurrentLanguage() const { return current_language_; };
    std::string getFontPath() const { return language_info.at(current_language_).at("font").get<std::string>(); };
    std::string getTranslationPath() const { return language_info.at(current_language_).at("translation").get<std::string>(); };
    std::vector<std::string> getSupportLanguages();
    private:
    I18nManager() = default;
    ~I18nManager() = default;
    I18nManager(const I18nManager &) = delete;
    I18nManager &operator=(const I18nManager &) = delete;
    std::unordered_map<std::string, std::string> dictionary_;
    std::string current_language_ = Config::DEFAULT_LANGUAGE;
    json language_info;

};

#define tr(key) I18nManager::getInstance().get(key).c_str()

#endif