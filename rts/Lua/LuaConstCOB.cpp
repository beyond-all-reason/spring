/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* exports the #defines from CobDefines.h to Lua */

#include "LuaConstCOB.h"
#include "LuaInclude.h"
#include "LuaUtils.h"
#include "Sim/Units/Scripts/CobDefines.h"
#include "Sim/Projectiles/PieceProjectile.h"


/******************************************************************************
 * COB constants
 * @module COB
 * @see rts/Lua/LuaConstCOB.cpp
******************************************************************************/


bool LuaConstCOB::PushEntries(lua_State* L)
{
#define PUSH_COB(cmd) LuaPushNamedNumber(L, #cmd, cmd)

	/*** @table COB
	 *
	 * @number ACTIVATION
	 * @number STANDINGMOVEORDERS
	 * @number STANDINGFIREORDERS
	 * @number HEALTH
	 * @number INBUILDSTANCE
	 * @number BUSY
	 * @number PIECE_XZ
	 * @number PIECE_Y
	 * @number UNIT_XZ
	 * @number UNIT_Y
	 * @number UNIT_HEIGHT
	 * @number XZ_ATAN
	 * @number XZ_HYPOT
	 * @number ATAN
	 * @number HYPOT
	 * @number GROUND_HEIGHT
	 * @number BUILD_PERCENT_LEFT
	 * @number YARD_OPEN
	 * @number BUGGER_OFF
	 * @number ARMORED
	 * @number IN_WATER
	 * @number CURRENT_SPEED
	 * @number VETERAN_LEVEL
	 * @number ON_ROAD
	 * @number MAX_ID
	 * @number MY_ID
	 * @number UNIT_TEAM
	 * @number UNIT_BUILD_PERCENT_LEFT
	 * @number UNIT_ALLIED
	 * @number MAX_SPEED
	 * @number CLOAKED
	 * @number WANT_CLOAK
	 * @number GROUND_WATER_HEIGHT
	 * @number UPRIGHT
	 * @number POW
	 * @number PRINT
	 * @number HEADING
	 * @number TARGET_ID
	 * @number LAST_ATTACKER_ID
	 * @number LOS_RADIUS
	 * @number AIR_LOS_RADIUS
	 * @number RADAR_RADIUS
	 * @number JAMMER_RADIUS
	 * @number SONAR_RADIUS
	 * @number SONAR_JAM_RADIUS
	 * @number SEISMIC_RADIUS
	 * @number DO_SEISMIC_PING
	 * @number CURRENT_FUEL
	 * @number TRANSPORT_ID
	 * @number SHIELD_POWER
	 * @number STEALTH
	 * @number CRASHING
	 * @number CHANGE_TARGET
	 * @number CEG_DAMAGE
	 * @number COB_ID
	 * @number PLAY_SOUND
	 * @number KILL_UNIT
	 * @number ALPHA_THRESHOLD
	 * @number SET_WEAPON_UNIT_TARGET
	 * @number SET_WEAPON_GROUND_TARGET
	 * @number SONAR_STEALTH
	 * @number REVERSING
	 * @number FLANK_B_MODE
	 * @number FLANK_B_DIR
	 * @number FLANK_B_MOBILITY_ADD
	 * @number FLANK_B_MAX_DAMAGE
	 * @number FLANK_B_MIN_DAMAGE
	 * @number WEAPON_RELOADSTATE
	 * @number WEAPON_RELOADTIME
	 * @number WEAPON_ACCURACY
	 * @number WEAPON_SPRAY
	 * @number WEAPON_RANGE
	 * @number WEAPON_PROJECTILE_SPEED
	 * @number MIN
	 * @number MAX
	 * @number ABS
	 * @number GAME_FRAME
	 * @number ENERGY_MAKE
	 * @number METAL_MAKE
	 * @number WORLD_SPACE_NONE
	 * @number WORLD_SPACE_POSITION
	 * @number WORLD_SPACE_ROTATION
	 * @number WORLD_SPACE_POS_ROT
	 * @number WORLD_SPACE_SCALE
	 * @number WORLD_SPACE_POS_SCALE
	 * @number WORLD_SPACE_ROT_SCALE
	 * @number WORLD_SPACE_ALL
	 * @number WORLD_SPACE_FLAGS
	 * @number WORLD_SPACE_FLAG_POSITION
	 * @number WORLD_SPACE_FLAG_ROTATION
	 * @number WORLD_SPACE_FLAG_SCALE
	 */

	PUSH_COB(ACTIVATION);
	PUSH_COB(STANDINGMOVEORDERS);
	PUSH_COB(STANDINGFIREORDERS);
	PUSH_COB(HEALTH);
	PUSH_COB(INBUILDSTANCE);
	PUSH_COB(BUSY);
	PUSH_COB(PIECE_XZ);
	PUSH_COB(PIECE_Y);
	PUSH_COB(UNIT_XZ);
	PUSH_COB(UNIT_Y);
	PUSH_COB(UNIT_HEIGHT);
	PUSH_COB(XZ_ATAN);
	PUSH_COB(XZ_HYPOT);
	PUSH_COB(ATAN);
	PUSH_COB(HYPOT);
	PUSH_COB(GROUND_HEIGHT);
	PUSH_COB(BUILD_PERCENT_LEFT);
	PUSH_COB(YARD_OPEN);
	PUSH_COB(BUGGER_OFF);
	PUSH_COB(ARMORED);

/*	PUSH_COB(WEAPON_AIM_ABORTED);
	PUSH_COB(WEAPON_READY);
	PUSH_COB(WEAPON_LAUNCH_NOW);
	PUSH_COB(FINISHED_DYING);
	PUSH_COB(ORIENTATION);*/
	PUSH_COB(IN_WATER);
	PUSH_COB(CURRENT_SPEED);
//	PUSH_COB(MAGIC_DEATH);
	PUSH_COB(VETERAN_LEVEL);
	PUSH_COB(ON_ROAD);

	PUSH_COB(MAX_ID);
	PUSH_COB(MY_ID);
	PUSH_COB(UNIT_TEAM);
	PUSH_COB(UNIT_BUILD_PERCENT_LEFT);
	PUSH_COB(UNIT_ALLIED);
	PUSH_COB(MAX_SPEED);
	PUSH_COB(CLOAKED);
	PUSH_COB(WANT_CLOAK);
	PUSH_COB(GROUND_WATER_HEIGHT);
	PUSH_COB(UPRIGHT);
	PUSH_COB(POW);
	PUSH_COB(PRINT);
	PUSH_COB(HEADING);
	PUSH_COB(TARGET_ID);
	PUSH_COB(LAST_ATTACKER_ID);
	PUSH_COB(LOS_RADIUS);
	PUSH_COB(AIR_LOS_RADIUS);
	PUSH_COB(RADAR_RADIUS);
	PUSH_COB(JAMMER_RADIUS);
	PUSH_COB(SONAR_RADIUS);
	PUSH_COB(SONAR_JAM_RADIUS);
	PUSH_COB(SEISMIC_RADIUS);
	PUSH_COB(DO_SEISMIC_PING);
	PUSH_COB(CURRENT_FUEL);
	PUSH_COB(TRANSPORT_ID);
	PUSH_COB(SHIELD_POWER);
	PUSH_COB(STEALTH);
	PUSH_COB(CRASHING);
	PUSH_COB(CHANGE_TARGET);
	PUSH_COB(CEG_DAMAGE);
	PUSH_COB(COB_ID);
	PUSH_COB(PLAY_SOUND);
	PUSH_COB(KILL_UNIT);
	PUSH_COB(SET_WEAPON_UNIT_TARGET);
	PUSH_COB(SET_WEAPON_GROUND_TARGET);
	PUSH_COB(SONAR_STEALTH);
	PUSH_COB(REVERSING);

	// NOTE: [LUA0 - LUA9] are defined in CobThread.cpp as [110 - 119]

	PUSH_COB(FLANK_B_MODE);
	PUSH_COB(FLANK_B_DIR);
	PUSH_COB(FLANK_B_MOBILITY_ADD);
	PUSH_COB(FLANK_B_MAX_DAMAGE);
	PUSH_COB(FLANK_B_MIN_DAMAGE);
	PUSH_COB(WEAPON_RELOADSTATE);
	PUSH_COB(WEAPON_RELOADTIME);
	PUSH_COB(WEAPON_ACCURACY);
	PUSH_COB(WEAPON_SPRAY);
	PUSH_COB(WEAPON_RANGE);
	PUSH_COB(WEAPON_PROJECTILE_SPEED);
	LuaPushNamedNumber(L, "MIN", COB_MIN);
	LuaPushNamedNumber(L, "MAX", COB_MAX);
	PUSH_COB(ABS);
	PUSH_COB(GAME_FRAME);
	PUSH_COB(ENERGY_MAKE);
	PUSH_COB(METAL_MAKE);

	PUSH_COB(WORLD_SPACE_NONE);
	PUSH_COB(WORLD_SPACE_POSITION);
	PUSH_COB(WORLD_SPACE_ROTATION);
	PUSH_COB(WORLD_SPACE_POS_ROT);
//	PUSH_COB(WORLD_SPACE_SCALE);
//	PUSH_COB(WORLD_SPACE_POS_SCALE);
//	PUSH_COB(WORLD_SPACE_ROT_SCALE);
//	PUSH_COB(WORLD_SPACE_ALL);
	PUSH_COB(WORLD_SPACE_FLAGS);
	PUSH_COB(WORLD_SPACE_FLAG_POSITION);
	PUSH_COB(WORLD_SPACE_FLAG_ROTATION);
//	PUSH_COB(WORLD_SPACE_FLAG_SCALE);

	// NOTE: shared variables use codes [1024 - 5119]

	return true;
}


/******************************************************************************/
/******************************************************************************/


bool LuaConstSFX::PushEntries(lua_State* L)
{
	/*** Piece Flags for Spring.UnitScript.Explode
	 *
	 * @table SFX
	 *
	 * @number SHATTER
	 * @number EXPLODE
	 * @number EXPLODE_ON_HIT
	 * @number FALL
	 * @number SMOKE
	 * @number FIRE
	 * @number NONE
	 * @number NO_CEG_TRAIL
	 * @number NO_HEATCLOUD
	 * @number RECURSIVE
	 */
	LuaPushNamedNumber(L, "SHATTER", PF_Shatter);
	LuaPushNamedNumber(L, "EXPLODE", PF_Explode);
	LuaPushNamedNumber(L, "EXPLODE_ON_HIT", PF_Explode);
	LuaPushNamedNumber(L, "FALL",  0);
	LuaPushNamedNumber(L, "SMOKE", PF_Smoke);
	LuaPushNamedNumber(L, "FIRE",  PF_Fire);
	LuaPushNamedNumber(L, "NONE",  PF_NONE); // BITMAP_ONLY
	LuaPushNamedNumber(L, "NO_CEG_TRAIL", PF_NoCEGTrail);
	LuaPushNamedNumber(L, "NO_HEATCLOUD", PF_NoHeatCloud);
	LuaPushNamedNumber(L, "RECURSIVE", PF_Recursive);

	/*** For Spring.UnitScript.EmitSfx
	 *
	 * @table SFX
	 *
	 * @number VTOL
	 * @number WAKE
	 * @number REVERSE_WAKE
	 * @number WHITE_SMOKE
	 * @number BLACK_SMOKE
	 * @number BUBBLE
	 * @number CEG
	 * @number FIRE_WEAPON
	 * @number DETONATE_WEAPON
	 * @number GLOBAL
	 */
	LuaPushNamedNumber(L, "VTOL",            SFX_VTOL);
	LuaPushNamedNumber(L, "WAKE",            SFX_WAKE);
	LuaPushNamedNumber(L, "REVERSE_WAKE",    SFX_REVERSE_WAKE);
	// no need for WAKE_2 and REVERSE_WAKE_2 as they are same as WAKE and REVERSE_WAKE
	//LuaPushNamedNumber(L, "WAKE_2",          SFX_WAKE_2);
	//LuaPushNamedNumber(L, "REVERSE_WAKE_2",  SFX_REVERSE_WAKE_2);
	LuaPushNamedNumber(L, "WHITE_SMOKE",     SFX_WHITE_SMOKE);
	LuaPushNamedNumber(L, "BLACK_SMOKE",     SFX_BLACK_SMOKE);
	LuaPushNamedNumber(L, "BUBBLE",          SFX_BUBBLE);
	LuaPushNamedNumber(L, "CEG",             SFX_CEG);
	LuaPushNamedNumber(L, "FIRE_WEAPON",     SFX_FIRE_WEAPON);
	LuaPushNamedNumber(L, "DETONATE_WEAPON", SFX_DETONATE_WEAPON);
	LuaPushNamedNumber(L, "GLOBAL",          SFX_GLOBAL);

	return true;
}


/******************************************************************************/
/******************************************************************************/
