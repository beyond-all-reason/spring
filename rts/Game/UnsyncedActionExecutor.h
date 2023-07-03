/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNSYNCED_ACTION_EXECUTOR_H
#define UNSYNCED_ACTION_EXECUTOR_H

#include "IActionExecutor.h"

#include <string>

class Action;


class UnsyncedAction : public IAction
{
public:
	UnsyncedAction(const Action& action, bool repeat)
		: IAction(action)
		, repeat(repeat)
	{}

	/**
	 * Returns whether the action is to be executed repeatedly.
	 */
	bool IsRepeat() const { return repeat; }

private:
	bool repeat;
};


class IUnsyncedActionExecutor : public IActionExecutor<UnsyncedAction, false>
{
protected:
	IUnsyncedActionExecutor(const std::string& command, const std::string& description, bool cheatRequired = false, std::vector<std::pair<std::string, std::string>> arguments = {})
		: IActionExecutor<UnsyncedAction, false>(command, description, cheatRequired, arguments)
	{

	}

public:
	virtual ~IUnsyncedActionExecutor() {}
};

#endif // UNSYNCED_ACTION_EXECUTOR_H
