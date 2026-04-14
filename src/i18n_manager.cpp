#include "i18n_manager.h"
#include <fstream>
#include <iostream>
#include "config.h"

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

bool I18nManager::init() {

	std::string filePath = Config::LANGUAGE_INFO_PATH;
	std::ifstream file(filePath);

	if (!file.is_open()) {
		std::cerr << "Failed to open language json file: " << filePath << std::endl;
		return false;
	}
	try {
		// json j;
		file >> language_info;
		return true;
	}
	catch (const json::exception &e) {
		std::cerr << "JSON parse error: " << e.what() << std::endl;
		return false;
	}
}

bool I18nManager::loadLanguage(const std::string &language) {
	current_language_ = language;
	std::string filePath = getTranslationPath();
	std::ifstream file(filePath);

	if (!file.is_open()) {
		std::cerr << "Failed to open translation file: " << filePath << std::endl;
		return false;
	}

	try {
		json j;
		file >> j;
		dictionary_.clear();
		parseNestedJson(j, "", dictionary_);
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
std::vector<std::string>  I18nManager::getSupportLanguages() {
	std::vector<std::string> languages;
	for (auto &el : language_info.items()) {
		languages.push_back(el.key());
	}
	return languages;
}