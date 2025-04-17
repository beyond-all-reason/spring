/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaConstCMD.h"

#include "LuaInclude.h"

#include "LuaUtils.h"
#include "Sim/Units/CommandAI/Command.h"


/***
 * Command constants.
 *
 * Table defining Command related constants.
 *
 * - Contains a mix of special constants like command options or move states, and the
 *   list of engine command IDs.
 * - Also supports integer keys, and those perform reverse mapping of command IDs.
 *
 * @see Spring.GiveOrderToUnit
 * @see Spring.GiveOrderArrayToUnitArray
 * @see Spring.GetUnitCurrentCommand
 * @see Callins:AllowCommand
 * @enum CMD
 */

bool LuaConstCMD::PushEntries(lua_State* L)
{
	/*** @field CMD.OPT_ALT 128 */
	LuaPushNamedNumber(L, "OPT_ALT",      ALT_KEY);
	/*** @field CMD.OPT_CTRL 64 */
	LuaPushNamedNumber(L, "OPT_CTRL",     CONTROL_KEY);
	/*** @field CMD.OPT_SHIFT 32 */
	LuaPushNamedNumber(L, "OPT_SHIFT",    SHIFT_KEY);
	/*** @field CMD.OPT_RIGHT 16 */
	LuaPushNamedNumber(L, "OPT_RIGHT",    RIGHT_MOUSE_KEY);
	/*** @field CMD.OPT_INTERNAL 8 */
	LuaPushNamedNumber(L, "OPT_INTERNAL", INTERNAL_ORDER);
	/*** @field CMD.OPT_META 4 */
	LuaPushNamedNumber(L, "OPT_META",     META_KEY);

	/*** @field CMD.MOVESTATE_NONE -1 */
	LuaPushNamedNumber(L, "MOVESTATE_NONE"    , MOVESTATE_NONE    );
	/*** @field CMD.MOVESTATE_HOLDPOS 0 */
	LuaPushNamedNumber(L, "MOVESTATE_HOLDPOS" , MOVESTATE_HOLDPOS );
	/*** @field CMD.MOVESTATE_MANEUVER 1 */
	LuaPushNamedNumber(L, "MOVESTATE_MANEUVER", MOVESTATE_MANEUVER);
	/*** @field CMD.MOVESTATE_ROAM 2 */
	LuaPushNamedNumber(L, "MOVESTATE_ROAM"    , MOVESTATE_ROAM    );

	/*** @field CMD.FIRESTATE_NONE -1 */
	LuaPushNamedNumber(L, "FIRESTATE_NONE"         , FIRESTATE_NONE         );
	/*** @field CMD.FIRESTATE_HOLDFIRE 0 */
	LuaPushNamedNumber(L, "FIRESTATE_HOLDFIRE"     , FIRESTATE_HOLDFIRE     );
	/*** @field CMD.FIRESTATE_RETURNFIRE 1 */
	LuaPushNamedNumber(L, "FIRESTATE_RETURNFIRE"   , FIRESTATE_RETURNFIRE   );
	/*** @field CMD.FIRESTATE_FIREATWILL 2 */
	LuaPushNamedNumber(L, "FIRESTATE_FIREATWILL"   , FIRESTATE_FIREATWILL   );
	/*** @field CMD.FIRESTATE_FIREATNEUTRAL 3 */
	LuaPushNamedNumber(L, "FIRESTATE_FIREATNEUTRAL", FIRESTATE_FIREATNEUTRAL);

	/*** @field CMD.WAITCODE_TIME 1 */
	LuaPushNamedNumber(L, "WAITCODE_TIME",   CMD_WAITCODE_TIMEWAIT);
	/*** @field CMD.WAITCODE_DEATH 2 */
	LuaPushNamedNumber(L, "WAITCODE_DEATH",  CMD_WAITCODE_DEATHWAIT);
	/*** @field CMD.WAITCODE_SQUAD 3 */
	LuaPushNamedNumber(L, "WAITCODE_SQUAD",  CMD_WAITCODE_SQUADWAIT);
	/*** @field CMD.WAITCODE_GATHER 4 */
	LuaPushNamedNumber(L, "WAITCODE_GATHER", CMD_WAITCODE_GATHERWAIT);

#define PUSH_CMD(cmd) LuaInsertDualMapPair(L, #cmd, CMD_ ## cmd);

	/***
	 * @field CMD.STOP 0
	 *
	 * Stop the current action and clear the unit's command queue.
	 *
	 * For factories, this will cancel the new unit orders queue.
	 * For units, this will cancel the current command and queue.
	 *
	 * Accepts no parameters.
	 *
	 * It won't do anything if used with `CMD.INSERT`, or the `shift` option.
	 */
	PUSH_CMD(STOP);
	/*** @field CMD.INSERT 1 */
	PUSH_CMD(INSERT);

	/***
	 * @field CMD.REMOVE 2
	 *
	 * Remove all commands from a unit's queue matching specific cmdIDs or tags.
	 *
	 * ## Modes of operation
	 *
	 * ### Filter by tag
	 *
	 * Removes any command with a tag matching those included in params.
	 *
	 * - `params` {tag1, tag2 ...} an array of tags to look for.
	 *
	 * This is the default mode of operation.
	 *
	 * ### Filter by id
	 *
	 * Removes any command with a `command id` matching those included in params.
	 *
	 * - `params` {id1, id2 ...} or {tag1, tag2, ...} an array of ids tags to look for.
	 *
	 * To use this mode you need to pass the `alt` option.
	 *
	 * ## Command Options
	 *
	 * - `alt` Tag/Id switch
	 * - `ctrl` Alternative queue selection.
	 *   - For factories alternative queue is the factory command queue, default queue is the rally queue.
	 *   - For other units no effect.
	 *
	 * ## Examples
	 *
	 * Delete all attack orders from unit, or factory rally queue if factory:
	 * ```lua
	 * Spring.GiveOrderToUnit(unitID, CMD.REMOVE, CMD.ATTACK)
	 * ```
	 *
	 * Delete all attack and fight orders from unit, or factory rally queue if factory:
	 * ```lua
	 * Spring.GiveOrderToUnit(unitID, CMD.REMOVE, {CMD.ATTACK, CMD.FIGHT}, CMD.OPT_ALT)
	 * ```
	 *
	 * Delete commands with specific tags:
	 * ```lua
	 * Spring.GiveOrderToUnit(unitID, CMD.REMOVE, {tag1, tag2, tag3})
	 * ```
	 *
	 * Delete all commands to build units with UnitDef ids unitDefId1 and unitDefId2 from factory queue:
	 * ```lua
	 * Spring.GiveOrderToUnit(unitID, CMD.REMOVE, {-unitDefId1, -unitDefId2}, CMD.OPT_ALT + CMD.OPT_CTRL)
	 * ```
	 *
	 * @see Spring.GiveOrderToUnit
	 */
	PUSH_CMD(REMOVE);
	/*** @field CMD.WAIT 5 */
	PUSH_CMD(WAIT);
	/*** @field CMD.TIMEWAIT 6 */
	PUSH_CMD(TIMEWAIT);
	/*** @field CMD.DEATHWAIT 7 */
	PUSH_CMD(DEATHWAIT);
	/*** @field CMD.SQUADWAIT 8 */
	PUSH_CMD(SQUADWAIT);
	/*** @field CMD.GATHERWAIT 9 */
	PUSH_CMD(GATHERWAIT);
	/*** @field CMD.MOVE 10 */
	PUSH_CMD(MOVE);
	/*** @field CMD.PATROL 15 */
	PUSH_CMD(PATROL);
	/*** @field CMD.FIGHT 16 */
	PUSH_CMD(FIGHT);

	/***
	 * @field CMD.ATTACK 20
	 *
	 * Attack command. Gives an order to attack some target(s).
	 *
	 * The command has different modes of operation, depending on the number
	 * of parameters and options used.
	 *
	 * ## Modes of operation
	 *
	 * ### Attack single target
	 *
	 * - `params` {unitID}: Attack a unit
	 *
	 * The command will end once the target is dead or not valid any more.
	 *
	 * ### Area attack
	 *
	 * Will create a number of `single target` actions by finding targets in a circle.
	 *
	 * **Note:** this is different than CMD.AREA_ATTACK, since this initially finds the targets
	 * but then doesn't consider the area any more.
	 *
	 * - `params` {x,y,z,r} when radius is greater than 0.
	 *   - r: radius
	 *   - x,y,z: map position
	 *
	 * ### Ground attack
	 *
	 * - `params` {x,y,z,0} or {x,y,z}
	 *   - x,y,z: map position
	 *
	 * ## Command Options
	 *
	 * - `alt` Also target stunned targets. Without this stunned targets will be skipped.
	 * - `meta` Override `manualFire`, and `noAutoTarget` weapon behaviours.
	 *
	 * ## Other modifiers
	 *
	 * - `modInfo.targetableTransportedUnits`: Controls whether transported units are targetable.
	 *
	 * ## Callins
	 *
	 *  - UnitCmdDone: Run when the command is finished.
	 *
	 * ## Examples
	 *
	 * Attack unit with id `targetID`.
	 * ```lua
	 * Spring.GiveOrderToUnit(unitID, CMD.ATTACK, targetID)
	 * ```
	 *
	 * Area attack:
	 * ```lua
	 * Spring.GiveOrderToUnit(unitID, CMD.ATTACK, {100, 1000,100,1000})
	 * ```
	 *
	 * Ground attack:
	 * ```lua
	 * Spring.GiveOrderToUnit(unitID, CMD.ATTACK, {1000,100,1000})
	 * ```
	 * @see Spring.GiveOrderToUnit
	 * @see Callins:UnitCmdDone
	 */
	PUSH_CMD(ATTACK);
	/*** @field CMD.AREA_ATTACK 21 */
	PUSH_CMD(AREA_ATTACK);
	/*** @field CMD.GUARD 25 */
	PUSH_CMD(GUARD);
	/*** @field CMD.GROUPSELECT 35 */
	PUSH_CMD(GROUPSELECT);
	/*** @field CMD.GROUPADD 36 */
	PUSH_CMD(GROUPADD);
	/*** @field CMD.GROUPCLEAR 37 */
	PUSH_CMD(GROUPCLEAR);
	/*** @field CMD.REPAIR 40 */
	PUSH_CMD(REPAIR);
	/*** @field CMD.FIRE_STATE 45 */
	PUSH_CMD(FIRE_STATE);
	/*** @field CMD.MOVE_STATE 50 */
	PUSH_CMD(MOVE_STATE);
	/*** @field CMD.SETBASE 55 */
	PUSH_CMD(SETBASE);
	/*** @field CMD.INTERNAL 60 */
	PUSH_CMD(INTERNAL);
	/*** @field CMD.SELFD 65 */
	PUSH_CMD(SELFD);
	/*** @field CMD.LOAD_UNITS 75 */
	PUSH_CMD(LOAD_UNITS);
	/*** @field CMD.LOAD_ONTO 76 */
	PUSH_CMD(LOAD_ONTO);
	/*** @field CMD.UNLOAD_UNITS 80 */
	PUSH_CMD(UNLOAD_UNITS);
	/*** @field CMD.UNLOAD_UNIT 81 */
	PUSH_CMD(UNLOAD_UNIT);
	/*** @field CMD.ONOFF 85 */
	PUSH_CMD(ONOFF);
	/*** @field CMD.RECLAIM 90 */
	PUSH_CMD(RECLAIM);
	/*** @field CMD.CLOAK 95 */
	PUSH_CMD(CLOAK);
	/*** @field CMD.STOCKPILE 100 */
	PUSH_CMD(STOCKPILE);
	/*** @field CMD.MANUALFIRE 105 */
	PUSH_CMD(MANUALFIRE);
	/*** @field CMD.DGUN 105 */
	LuaInsertDualMapPair(L, "DGUN", CMD_MANUALFIRE); // backward compatibility (TODO: find a way to print a warning when used!)
	/*** @field CMD.RESTORE 110 */
	PUSH_CMD(RESTORE);
	/*** @field CMD.REPEAT 115 */
	PUSH_CMD(REPEAT);
	/*** @field CMD.TRAJECTORY 120 */
	PUSH_CMD(TRAJECTORY);
	/*** @field CMD.RESURRECT 125 */
	PUSH_CMD(RESURRECT);
	/*** @field CMD.CAPTURE 130 */
	PUSH_CMD(CAPTURE);
	/*** @field CMD.AUTOREPAIRLEVEL 135 */
	PUSH_CMD(AUTOREPAIRLEVEL);
	/*** @field CMD.LOOPBACKATTACK 20 */
	LuaInsertDualMapPair(L, "LOOPBACKATTACK", CMD_ATTACK); // backward compatibility (TODO: find a way to print a warning when used!)
	/*** @field CMD.IDLEMODE 145  */
	PUSH_CMD(IDLEMODE);

	return true;
}
