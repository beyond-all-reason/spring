/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TRANSLATIONTABLE_H
#define TRANSLATIONTABLE_H

#include <string>
#include <unordered_map>

class TranslationTable
{
public:
	TranslationTable() = default;

	// no-copy
	TranslationTable(const TranslationTable &) = delete;

	bool addTranslation(const std::string& key, const std::string& translation)
	{
		bool existed = exists(key);
		translations[key] = translation;
		return existed;
	};

	bool exists(std::string key) {
		return translations.contains(key);
	}

	std::string getTranslationString(const std::string& key)
	{
		return translations[key];
	};

	void clear()
	{
		translations.clear();
	};

private:
	std::unordered_map<std::string, std::string> translations;
};

#endif // TRANSLATIONTABLE_H