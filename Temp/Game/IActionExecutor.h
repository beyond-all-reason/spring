/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef I_ACTION_EXECUTOR_H
#define I_ACTION_EXECUTOR_H

#include "Action.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/Log/ILog.h"

#include <string>

class IAction
{
protected:
	IAction(const Action& action)
		: action(action)
	{}
	virtual ~IAction() {}

public:
	/**
	 * Returns the action arguments.
	 */
	const std::string& GetCmd() const { return action.command; }
	const std::string& GetArgs() const { return action.extra; }

	const Action& GetInnerAction() const { return action; }

private:
	const Action& action;
};


template<class action_t, bool synced_v>
class IActionExecutor
{
protected:
	IActionExecutor(const std::string& command,
			const std::string& description, bool cheatRequired = false,
			std::vector<std::pair<std::string, std::string>> arguments = {})
		: command(command)
		, description(description)
		, cheatRequired(cheatRequired)
		, arguments(arguments)
	{}
	virtual ~IActionExecutor() {}

public:
	/**
	 * Returns the command string that is unique for this executor.
	 */
	const std::string& GetCommand() const { return command; }

	/**
	 * Returns a human readable description of the command handled by this
	 * executor.
	 * This text will eventually be shown to end-users of the engine (gamers).
	 */
	const std::string& GetDescription() const { return description; }

	/**
	 * Returns a list of string pairs describing each argument for the action
	 * in the form of <arg, description>.
	 */
	const std::vector<std::pair<std::string, std::string>>& GetArguments() const { return arguments; }

	/**
	 * Returns whether this executor handles synced or unsynced commands.
	 */
	bool IsSynced() const { return synced_v; }

	/**
	 * Returns the command string that is unique for this executor.
	 */
	bool IsCheatRequired() const { return cheatRequired; }

	/**
	 * Executes one instance of an action of this type.
	 * Does a few checks internally, and then calls Execute(args).
	 */
	bool ExecuteAction(const action_t& action) const;

protected:
	void SetDescription(const std::string& description) {
		this->description = description;
	}

private:
	/**
	 * Executes one instance of an action of this type.
	 */
	virtual bool Execute(const action_t& action) const = 0;

	std::string command;
	std::string description;
	bool cheatRequired;
	std::vector<std::pair<std::string, std::string>> arguments;
};



/*
 * Because this is a template enabled class,
 * the implementations have to be in the same file.
 */
template<class action_t, bool synced_v>
bool IActionExecutor<action_t, synced_v>::ExecuteAction(const action_t& action) const
{
	//assert(action.GetAction().command == GetCommand());

	if (IsCheatRequired() && !gs->cheatEnabled) {
		LOG_L(L_WARNING, "Chat command /%s (%s) cannot be executed (cheats required)!",
				GetCommand().c_str(),
				(IsSynced() ? "synced" : "unsynced"));
		return false;
	} else {
		return Execute(action);
	}
}


/// Logs the enabled/disabled status of a sub-system of the engine.
static inline void LogSystemStatus(const std::string& system, const bool status)
{
	LOG("%s is %s!", system.c_str(), (status ? "enabled" : "disabled"));
}

#endif // I_ACTION_EXECUTOR_H
