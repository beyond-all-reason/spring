/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef ACTION_H
#define ACTION_H

#include <string>
#include "Game/UI/KeySet.h"

class Action
{
public:
	Action() {}
	Action(const std::string& line);

	int         bindingIndex; ///< the order for the action trigger
	std::string command;      ///< first word, lowercase
	std::string extra;        ///< everything but the first word, stripped of comments (//)
	std::string line;         ///< the whole command line, sanitized
	std::string rawline;      ///< includes the command, case preserved
	std::string boundWith;    ///< the string that defined the binding keyset
	CKeyChain   keyChain;     ///< the bound keychain/keyset

	bool operator<(const Action& action) const
	{
		bool selfAnyMod = keyChain.back().AnyMod();
		bool actionAnyMod = action.keyChain.back().AnyMod();

		if (selfAnyMod == actionAnyMod)
			return bindingIndex < action.bindingIndex;
		else
			return actionAnyMod;
	}

	bool operator==(const Action& action) const
	{
		return line == action.line;
	}
};

#endif // ACTION_H
