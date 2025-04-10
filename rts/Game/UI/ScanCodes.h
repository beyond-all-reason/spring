/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SCANCODES_H
#define SCANCODES_H

#include "IKeys.h"

class CScanCodes : public IKeys {
public:

	void Reset() override;

	bool IsModifier(int code) const override;
	void PrintNameToCode() const override;
	void PrintCodeToName() const override;
	std::string GetName(int code) const override;
	std::string GetDefaultName(int code) const override;

	static std::string GetCodeString(int code);
	static int GetNormalizedSymbol(int sym);

	static int GetMouseButtonSymbol(int button);
};

extern CScanCodes scanCodes;

#endif /* SCANCODES_H */
