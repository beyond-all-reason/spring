/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "FactoryCAI.h"
#include "ExternalAI/EngineOutHandler.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Game/GameHelper.h"
#include "Game/GlobalUnsynced.h"
#include "Game/SelectedUnitsHandler.h"
#include "Game/WaitCommandsAI.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/BuildInfo.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitTypes/Factory.h"
#include "System/Log/ILog.h"
#include "System/creg/STL_Map.h"
#include "System/StringUtil.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"

#include "System/Misc/TracyDefs.h"

CR_BIND_DERIVED(CFactoryCAI ,CCommandAI , )

CR_REG_METADATA(CFactoryCAI , (
	CR_MEMBER(newUnitCommands),
	CR_MEMBER(buildOptions),
	CR_PREALLOC(GetPreallocContainer)
))

static std::string GetUnitDefBuildOptionToolTip(const UnitDef* ud, bool disabled) {
	std::string tooltip;

	if (disabled) {
		tooltip = "\xff\xff\x22\x22" "DISABLED: " "\xff\xff\xff\xff";
	} else {
		tooltip = "Build: ";
	}

	tooltip += (ud->humanName + " - " + ud->tooltip);
	tooltip += ("\nHealth "      + FloatToString(ud->health,      "%.0f"));
	tooltip += ("\nMetal cost "  + FloatToString(ud->cost.metal,  "%.0f"));
	tooltip += ("\nEnergy cost " + FloatToString(ud->cost.energy, "%.0f"));
	tooltip += ("\nBuild time "  + FloatToString(ud->buildTime,   "%.0f"));

	return tooltip;
}



CFactoryCAI::CFactoryCAI(): CCommandAI()
{}


CFactoryCAI::CFactoryCAI(CUnit* owner): CCommandAI(owner)
{
	commandQue.SetQueueType(CCommandQueue::BuildQueueType);
	newUnitCommands.SetQueueType(CCommandQueue::NewUnitQueueType);

	if (owner->unitDef->canmove) {
		SCommandDescription c;

		c.id        = CMD_MOVE;
		c.type      = CMDTYPE_ICON_MAP;

		c.action    = "move";
		c.name      = "Move";
		c.tooltip   = c.name + ": Order ready built units to move to a position";
		c.mouseicon = c.name;
		possibleCommands.push_back(commandDescriptionCache.GetPtr(std::move(c)));
	}

	if (owner->unitDef->canPatrol) {
		SCommandDescription c;

		c.id        = CMD_PATROL;
		c.type      = CMDTYPE_ICON_MAP;

		c.action    = "patrol";
		c.name      = "Patrol";
		c.tooltip   = c.name + ": Order ready built units to patrol to one or more waypoints";
		c.mouseicon = c.name;
		possibleCommands.push_back(commandDescriptionCache.GetPtr(std::move(c)));
	}

	if (owner->unitDef->canFight) {
		SCommandDescription c;

		c.id        = CMD_FIGHT;
		c.type      = CMDTYPE_ICON_MAP;

		c.action    = "fight";
		c.name      = "Fight";
		c.tooltip   = c.name + ": Order ready built units to take action while moving to a position";
		c.mouseicon = c.name;
		possibleCommands.push_back(commandDescriptionCache.GetPtr(std::move(c)));
	}

	if (owner->unitDef->canGuard) {
		SCommandDescription c;

		c.id        = CMD_GUARD;
		c.type      = CMDTYPE_ICON_UNIT;

		c.action    = "guard";
		c.name      = "Guard";
		c.tooltip   = c.name + ": Order ready built units to guard another unit and attack units attacking it";
		c.mouseicon = c.name;
		possibleCommands.push_back(commandDescriptionCache.GetPtr(std::move(c)));
	}

	CFactory* fac = static_cast<CFactory*>(owner);

	for (const auto& bi: fac->unitDef->buildOptions) {
		const std::string& name = bi.second;
		const UnitDef* ud = unitDefHandler->GetUnitDefByName(name);

		if (ud == nullptr) {
			string errmsg = "MOD ERROR: loading ";
			errmsg += name.c_str();
			errmsg += " for ";
			errmsg += owner->unitDef->name;
			throw content_error(errmsg);
		}

		{
			SCommandDescription c;

			c.id   = -ud->id; // build-options are always negative
			c.type = CMDTYPE_ICON;

			c.action    = "buildunit_" + StringToLower(ud->name);
			c.name      = name;
			c.mouseicon = c.name;
			c.tooltip   = GetUnitDefBuildOptionToolTip(ud, c.disabled = (ud->maxThisUnit <= 0));

			buildOptions[c.id] = 0;
			possibleCommands.push_back(commandDescriptionCache.GetPtr(std::move(c)));
		}
	}
}


static constexpr int GetCountMultiplierFromOptions(int opts)
{
	// The choice of keys and their associated multipliers are from OTA.
	int ret = 1;
	if (opts &   SHIFT_KEY) ret *=  5;
	if (opts & CONTROL_KEY) ret *= 20;
	return ret;
}

void CFactoryCAI::GiveCommandReal(const Command& c, bool fromSynced)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const int cmdID = c.GetID();

	// move is always allowed for factories (passed to units it produces)
	if ((cmdID != CMD_MOVE) && !AllowedCommand(c, fromSynced))
		return;

	auto boi = buildOptions.find(cmdID);

	// not a build order (or a build order we do not support, eg. if multiple
	// factories of different types were selected) so queue it to built units
	if (boi == buildOptions.end()) {
		if (cmdID < 0)
			return;

		if (nonQueingCommands.find(cmdID) != nonQueingCommands.end()) {
			CCommandAI::GiveAllowedCommand(c);
			return;
		}

		if (cmdID == CMD_INSERT || cmdID == CMD_REMOVE) {
			CCommandAI::GiveAllowedCommand(c);
			return;
		}

		if (!(c.GetOpts() & SHIFT_KEY) && (cmdID == CMD_WAIT || cmdID == CMD_SELFD)) {
			CCommandAI::GiveAllowedCommand(c);
			return;
		}

		if (!(c.GetOpts() & SHIFT_KEY)) {
 			waitCommandsAI.ClearUnitQueue(owner, newUnitCommands);
			CCommandAI::ClearCommandDependencies();
			newUnitCommands.clear();
		}

		CCommandAI::AddCommandDependency(c);

		if (cmdID != CMD_STOP) {
			if ((cmdID == CMD_WAIT) || (cmdID == CMD_SELFD)) {
				if (!newUnitCommands.empty() && (newUnitCommands.back().GetID() == cmdID)) {
					if (cmdID == CMD_WAIT) {
						waitCommandsAI.RemoveWaitCommand(owner, c);
					}
					newUnitCommands.pop_back();
				} else {
					newUnitCommands.push_back(c);
				}
			} else {
				bool dummy;
				if (CancelCommands(c, newUnitCommands, dummy) > 0) {
					return;
				} else {
					if (GetOverlapQueued(c, newUnitCommands).empty()) {
						newUnitCommands.push_back(c);
					} else {
						return;
					}
				}
			}
		}

		// the first new-unit build order can not be WAIT or SELFD
		while (!newUnitCommands.empty()) {
			const Command& newUnitCommand = newUnitCommands.front();
			const int id = newUnitCommand.GetID();

			if ((id == CMD_WAIT) || (id == CMD_SELFD)) {
				if (cmdID == CMD_WAIT) {
					waitCommandsAI.RemoveWaitCommand(owner, c);
				}
				newUnitCommands.pop_front();
			} else {
				break;
			}
		}

		return;
	}

	int& numQueued = boi->second;
	int numItems = GetCountMultiplierFromOptions(c.GetOpts());

	if (c.GetOpts() & RIGHT_MOUSE_KEY) {
		numQueued -= numItems;
		numQueued  = std::max(numQueued, 0);

		int numToErase = numItems;
		if (c.GetOpts() & ALT_KEY) {
			for (unsigned int cmdNum = 0; cmdNum < commandQue.size() && numToErase; ++cmdNum) {
				if (commandQue[cmdNum].GetID() == cmdID) {
					commandQue[cmdNum] = Command(CMD_STOP);
					numToErase--;
				}
			}
		} else {
			for (int cmdNum = commandQue.size() - 1; cmdNum != -1 && numToErase; --cmdNum) {
				if (commandQue[cmdNum].GetID() == cmdID) {
					commandQue[cmdNum] = Command(CMD_STOP);
					numToErase--;
				}
			}
		}
	} else {
		if (c.GetOpts() & ALT_KEY) {
			Command nc(c);
			nc.SetOpts(nc.GetOpts() | INTERNAL_ORDER);
			for (int a = 0; a < numItems; ++a) {
				if (repeatOrders) {
					if (commandQue.empty()) {
						commandQue.push_front(nc);
					} else {
						commandQue.insert(commandQue.begin() + 1, nc);
					}
				} else {
					commandQue.push_front(c);
				}
			}

			if (!repeatOrders)
				static_cast<CFactory*>(owner)->StopBuild();

		} else {
			for (int a = 0; a < numItems; ++a) {
				commandQue.push_back(c);
			}
		}
		numQueued += numItems;
	}

	UpdateIconName(cmdID, numQueued);
	SlowUpdate();
}


void CFactoryCAI::InsertBuildCommand(CCommandQueue::iterator& it,
                                     const Command& newCmd)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const auto boi = buildOptions.find(newCmd.GetID());
	auto buildCount = GetCountMultiplierFromOptions(newCmd.GetOpts());
	if (boi != buildOptions.end()) {
		boi->second += buildCount;
		UpdateIconName(newCmd.GetID(), boi->second);
	}
	if (!commandQue.empty() && (it == commandQue.begin())) {
		// ExecuteStop(), without the pop_front()
		CFactory* fac = static_cast<CFactory*>(owner);
		fac->StopBuild();
	}
	while (buildCount--)
		it = commandQue.insert(it, newCmd);
}


bool CFactoryCAI::RemoveBuildCommand(CCommandQueue::iterator& it)
{
	RECOIL_DETAILED_TRACY_ZONE;
	Command& cmd = *it;
	const auto boi = buildOptions.find(cmd.GetID());
	if (boi != buildOptions.end()) {
		boi->second--;
		UpdateIconName(cmd.GetID(), boi->second);
	}
	if (!commandQue.empty() && (it == commandQue.begin())) {
		ExecuteStop(cmd);
		return true;
	}

	if (cmd.GetID() < 0) {
		// build command, convert into a stop command
		cmd = Command(CMD_STOP);
	}

	return false;
}


void CFactoryCAI::DecreaseQueueCount(const Command& buildCommand, int& numQueued)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// copy in case we get pop'ed
	// NOTE: the queue should not be empty at this point!
	const Command frontCommand = commandQue.empty()? Command(CMD_STOP): commandQue.front();

	if (!repeatOrders || buildCommand.IsInternalOrder())
		numQueued--;

	UpdateIconName(buildCommand.GetID(), numQueued);

	// if true, factory was set to wait and its buildee
	// could only have been finished by assisting units
	// --> make sure not to cancel the wait-order
	if (frontCommand.GetID() == CMD_WAIT)
		commandQue.pop_front();

	// can only finish the real build-command command if
	// we still have it in our queue (FinishCommand also
	// asserts this)
	if (!commandQue.empty())
		FinishCommand();

	if (frontCommand.GetID() == CMD_WAIT)
		commandQue.push_front(frontCommand);
}



// NOTE:
//   only called if Factory::QueueBuild returned FACTORY_NEXT_BUILD_ORDER
//   (meaning the order was not rejected and the callback was installed)
void CFactoryCAI::FactoryFinishBuild(const Command& command) {
	DecreaseQueueCount(command, buildOptions[command.GetID()]);
}

void CFactoryCAI::SlowUpdate()
{
	RECOIL_DETAILED_TRACY_ZONE;
	// Commands issued may invoke SlowUpdate when paused
	if (gs->paused)
		return;
	if (commandQue.empty() || owner->beingBuilt)
		return;

	CFactory* fac = static_cast<CFactory*>(owner);

	while (!commandQue.empty()) {
		Command& c = commandQue.front();

		const size_t oldQueueSize = commandQue.size();

		if (buildOptions.find(c.GetID()) != buildOptions.end()) {
			// build-order
			switch (fac->QueueBuild(unitDefHandler->GetUnitDefByID(-c.GetID()), c)) {
				case CFactory::FACTORY_SKIP_BUILD_ORDER: {
					// order rejected and we want to skip it permanently
					DecreaseQueueCount(c, buildOptions[c.GetID()]);
				} break;
			}
		} else {
			// regular order (move/wait/etc)
			switch (c.GetID()) {
				case CMD_STOP: {
					/* Targeted hack to optimize bulk STOP orders.
					 * Build orders get replaced by STOP instead of being removed,
					 * this is due to the buildqueue's internal implementation as `std::deque`
					 * whose interface doesn't support removal from the middle that well.
					 * Units often get added and removed in large quantities via CTRL/SHIFT,
					 * such multiple STOPs commands in a row would then produce a freeze
					 * when the engine tries to process them all in one frame.
					 * Just execute the last in each series to ensure last build is cancelled
					 * otherwise last unit stays being built. */
					if (oldQueueSize == 1 || commandQue[1].GetID() != CMD_STOP) {
						ExecuteStop(c);
					} else {
						commandQue.pop_front();
					}

				} break;
				default: {
					CCommandAI::SlowUpdate();
				} break;
			}
		}

		// exit if no command was consumed
		if (oldQueueSize == commandQue.size())
			break;
	}
}


void CFactoryCAI::ExecuteStop(Command& c)
{
	RECOIL_DETAILED_TRACY_ZONE;
	CFactory* fac = static_cast<CFactory*>(owner);
	fac->StopBuild();

	commandQue.pop_front();
}


int CFactoryCAI::GetDefaultCmd(const CUnit* pointed, const CFeature* feature)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (pointed == nullptr)
		return CMD_MOVE;

	if (!teamHandler.Ally(gu->myAllyTeam, pointed->allyteam))
		return CMD_MOVE;

	if (!owner->unitDef->canGuard)
		return CMD_MOVE;

	return CMD_GUARD;
}


void CFactoryCAI::UpdateIconName(int cmdID, const int& numQueued)
{
	RECOIL_DETAILED_TRACY_ZONE;
	for (const SCommandDescription*& cd: possibleCommands) {
		if (cd->id != cmdID)
			continue;

		char t[32];
		SNPRINTF(t, 10, "%d", numQueued);

		SCommandDescription ucd = *cd;
		ucd.params.clear();

		if (numQueued > 0)
			ucd.params.push_back(t);

		commandDescriptionCache.DecRef(*cd);
		cd = commandDescriptionCache.GetPtr(std::move(ucd));
		break;
	}

	selectedUnitsHandler.PossibleCommandChange(owner);
}
