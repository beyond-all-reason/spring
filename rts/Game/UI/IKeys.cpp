/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IKeys.h"
#include "System/StringUtil.h"

#include "System/Misc/TracyDefs.h"


int IKeys::GetCode(const std::string& name) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	const auto iter = std::lower_bound(nameToCode.begin(), nameToCode.end(), NameCodePair{name, 0}, namePred);

	if (iter == nameToCode.end() || iter->first != name)
		return 0;

	return iter->second;
}


bool IKeys::AddKeySymbol(const std::string& name, int code)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if ((code <= 0) || !IsValidLabel(name))
		return false;

	const std::string keysym = StringToLower(name);

	// do not allow existing keysyms to be renamed
	const auto nameIter = std::lower_bound(nameToCode.begin(), nameToCode.end(), NameCodePair{keysym, 0}, namePred);

	if (nameIter != nameToCode.end() && nameIter->first == name)
		return false;

	nameToCode.emplace_back(keysym, code);
	// assumes that the user would rather see their own names
	codeToName.emplace_back(code, keysym);

	// swap into position
	for (size_t i = nameToCode.size() - 1; i > 0; i--) {
		if (nameToCode[i - 1].first < nameToCode[i].first)
			break;

		std::swap(nameToCode[i - 1], nameToCode[i]);
	}
	for (size_t i = codeToName.size() - 1; i > 0; i--) {
		if (codeToName[i - 1].first < codeToName[i].first)
			break;

		std::swap(codeToName[i - 1], codeToName[i]);
	}

	return true;
}


bool IKeys::IsPrintable(int code) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	const auto iter = std::lower_bound(printableCodes.begin(), printableCodes.end(), code);

	return (iter != printableCodes.end() && *iter == code);
}

void IKeys::SaveUserKeySymbols(FILE* file) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	bool output = false;

	for (const auto& p: nameToCode) {
		const auto defSymIter = std::lower_bound(defaultNameToCode.begin(), defaultNameToCode.end(), NameCodePair{p.first, 0}, namePred);

		if (defSymIter != defaultNameToCode.end() && defSymIter->first == p.first)
			continue;

		// this keysym is not standard
		const std::string& extSymName = p.first;
		const std::string& defSymName = GetDefaultName(p.second);
		fprintf(file, "keysym  %-10s  %s\n", extSymName.c_str(), defSymName.c_str());
		output = true;
	}

	if (!output)
		return;

	fprintf(file, "\n");
}

bool IKeys::IsValidLabel(const std::string& label)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (label.empty())
		return false;

	if (!isalpha(label[0]))
		return false;

	// if any character is not alpha-numeric *and* not space, reject label as invalid
	return (std::find_if(label.begin(), label.end(), [](char c) { return (!isalnum(c) && (c != '_')); }) == label.end());
}


void IKeys::AddPair(const std::string& name, const int code, const bool printable)
{
	RECOIL_DETAILED_TRACY_ZONE;
	nameToCode.emplace_back(name, code);
	codeToName.emplace_back(code, name);

	if (!printable)
		return;

	printableCodes.push_back(code);
}
