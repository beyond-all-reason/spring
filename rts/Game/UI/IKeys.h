/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef IKEYS_H
#define IKEYS_H

#include <string>
#include <vector>


class IKeys {
public:
	int GetCode(const std::string& name) const;
	std::string GetName(int code) const;
	std::string GetDefaultName(int code) const;
	bool AddKeySymbol(const std::string& name, int code);
	bool IsPrintable(int code) const;

	void SaveUserKeySymbols(FILE* file) const;

	virtual void Reset() {};

public:

	virtual void PrintNameToCode() const {};
	virtual void PrintCodeToName() const {};
	virtual bool IsModifier(int code) { return false; };

	static bool IsValidLabel(const std::string& label);

protected:
	void AddPair(const std::string& name, const int code, const bool printable = false);

protected:
	typedef std::pair<std::string, int> NameCodePair;
	typedef std::pair<int, std::string> CodeNamePair;

	std::vector<NameCodePair> nameToCode;
	std::vector<CodeNamePair> codeToName;
	std::vector<NameCodePair> defaultNameToCode;
	std::vector<CodeNamePair> defaultCodeToName;
	std::vector<         int> printableCodes;
};

#endif /* IKEYS_H */
