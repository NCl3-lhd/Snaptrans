#include "i18n_manager.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// 递归压扁嵌套 JSON
static void parseNestedJson(const json &j, const std::string &prefix, std::unordered_map<std::string, std::string> &dict) {
    for (auto &el : j.items()) {
        std::string currentKey = prefix.empty() ? el.key() : prefix + "." + el.key();

        if (el.value().is_object()) {
            parseNestedJson(el.value(), currentKey, dict);
        }
        else if (el.value().is_string()) {
            dict[currentKey] = el.value().get<std::string>();
        }
        else {
            dict[currentKey] = el.value().dump();
        }
    }
}

I18nManager &I18nManager::getInstance() {
    static I18nManager instance;
    return instance;
}

bool I18nManager::loadLanguage(const std::string &localeCode) {
    std::string filePath = "assets/locales/" + localeCode + ".json";
    std::ifstream file(filePath);

    if (!file.is_open()) {
        std::cerr << "Failed to open locale file: " << filePath << std::endl;
        return false;
    }

    try {
        json j;
        file >> j;
        dictionary_.clear();
        parseNestedJson(j, "", dictionary_);
        currentLocale_ = localeCode;
        return true;
    }
    catch (const json::exception &e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
        return false;
    }
}

std::string I18nManager::get(const std::string &key) {
    auto it = dictionary_.find(key);
    if (it != dictionary_.end()) return it->second;
    return "[" + key + "]";
}