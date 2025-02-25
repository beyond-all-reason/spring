/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* exports the #defines from CobDefines.h to Lua */

#include "LuaConstCOB.h"
#include "LuaInclude.h"
#include "LuaUtils.h"
#include "Sim/Units/Scripts/CobDefines.h"
#include "Sim/Projectiles/PieceProjectile.h"


/******************************************************************************
 * COB constants
 * @see rts/Lua/LuaConstCOB.cpp
******************************************************************************/


bool LuaConstCOB::PushEntries(lua_State* L)
{
#define PUSH_COB(cmd) LuaPushNamedNumber(L, #cmd, cmd)

	/***
	 * @enum COB
	 * @field ACTIVATION number 
	 * @field STANDINGMOVEORDERS number 
	 * @field STANDINGFIREORDERS number 
	 * @field HEALTH number 
	 * @field INBUILDSTANCE number 
	 * @field BUSY number 
	 * @field PIECE_XZ number 
	 * @field PIECE_Y number 
	 * @field UNIT_XZ number 
	 * @field UNIT_Y number 
	 * @field UNIT_HEIGHT number 
	 * @field XZ_ATAN number 
	 * @field XZ_HYPOT number 
	 * @field ATAN number 
	 * @field HYPOT number 
	 * @field GROUND_HEIGHT number 
	 * @field BUILD_PERCENT_LEFT number 
	 * @field YARD_OPEN number 
	 * @field BUGGER_OFF number 
	 * @field ARMORED number 
	 * @field IN_WATER number 
	 * @field CURRENT_SPEED number 
	 * @field VETERAN_LEVEL number 
	 * @field ON_ROAD number 
	 * @field MAX_ID number 
	 * @field MY_ID number 
	 * @field UNIT_TEAM number 
	 * @field UNIT_BUILD_PERCENT_LEFT number 
	 * @field UNIT_ALLIED number 
	 * @field MAX_SPEED number 
	 * @field CLOAKED number 
	 * @field WANT_CLOAK number 
	 * @field GROUND_WATER_HEIGHT number 
	 * @field UPRIGHT number 
	 * @field POW number 
	 * @field PRINT number 
	 * @field HEADING number 
	 * @field TARGET_ID number 
	 * @field LAST_ATTACKER_ID number 
	 * @field LOS_RADIUS number 
	 * @field AIR_LOS_RADIUS number 
	 * @field RADAR_RADIUS number 
	 * @field JAMMER_RADIUS number 
	 * @field SONAR_RADIUS number 
	 * @field SONAR_JAM_RADIUS number 
	 * @field SEISMIC_RADIUS number 
	 * @field DO_SEISMIC_PING number 
	 * @field CURRENT_FUEL number 
	 * @field TRANSPORT_ID number 
	 * @field SHIELD_POWER number 
	 * @field STEALTH number 
	 * @field CRASHING number 
	 * @field CHANGE_TARGET number 
	 * @field CEG_DAMAGE number 
	 * @field COB_ID number 
	 * @field PLAY_SOUND number 
	 * @field KILL_UNIT number 
	 * @field ALPHA_THRESHOLD number 
	 * @field SET_WEAPON_UNIT_TARGET number 
	 * @field SET_WEAPON_GROUND_TARGET number 
	 * @field SONAR_STEALTH number 
	 * @field REVERSING number 
	 * @field FLANK_B_MODE number 
	 * @field FLANK_B_DIR number 
	 * @field FLANK_B_MOBILITY_ADD number 
	 * @field FLANK_B_MAX_DAMAGE number 
	 * @field FLANK_B_MIN_DAMAGE number 
	 * @field WEAPON_RELOADSTATE number 
	 * @field WEAPON_RELOADTIME number 
	 * @field WEAPON_ACCURACY number 
	 * @field WEAPON_SPRAY number 
	 * @field WEAPON_RANGE number 
	 * @field WEAPON_PROJECTILE_SPEED number 
	 * @field MIN number 
	 * @field MAX number 
	 * @field ABS number 
	 * @field GAME_FRAME  number 
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

	// NOTE: shared variables use codes [1024 - 5119]

	return true;
}


/******************************************************************************/
/******************************************************************************/


bool LuaConstSFX::PushEntries(lua_State* L)
{
	/*** 
	 * @enum SFX
	 * 
	 * @field SHATTER number
	 * Piece Flag for `Spring.UnitScript.Explode`
	 * 
	 * @field EXPLODE number
	 * Piece Flag for `Spring.UnitScript.Explode`
	 * 
	 * @field EXPLODE_ON_HIT number
	 * Piece Flag for `Spring.UnitScript.Explode`
	 * 
	 * @field FALL number
	 * Piece Flag for `Spring.UnitScript.Explode`
	 * 
	 * @field SMOKE number
	 * Piece Flag for `Spring.UnitScript.Explode`
	 * 
	 * @field FIRE number
	 * Piece Flag for `Spring.UnitScript.Explode`
	 * 
	 * @field NONE number
	 * Piece Flag for `Spring.UnitScript.Explode`
	 * 
	 * @field NO_CEG_TRAIL number
	 * Piece Flag for `Spring.UnitScript.Explode`
	 * 
	 * @field NO_HEATCLOUD number
	 * Piece Flag for `Spring.UnitScript.Explode`
	 * 
	 * @field RECURSIVE number
	 * Piece Flag for `Spring.UnitScript.Explode`
	 * 
	 * @field VTOL number 
	 * For `Spring.UnitScript.EmitSfx`.
	 * 
	 * @field WAKE number 
	 * For `Spring.UnitScript.EmitSfx`.
	 * 
	 * @field REVERSE_WAKE number 
	 * For `Spring.UnitScript.EmitSfx`.
	 * 
	 * @field WHITE_SMOKE number 
	 * For `Spring.UnitScript.EmitSfx`.
	 * 
	 * @field BLACK_SMOKE number 
	 * For `Spring.UnitScript.EmitSfx`.
	 * 
	 * @field BUBBLE number 
	 * For `Spring.UnitScript.EmitSfx`.
	 * 
	 * @field CEG number 
	 * For `Spring.UnitScript.EmitSfx`.
	 * 
	 * @field FIRE_WEAPON number 
	 * For `Spring.UnitScript.EmitSfx`.
	 * 
	 * @field DETONATE_WEAPON number 
	 * For `Spring.UnitScript.EmitSfx`.
	 * 
	 * @field GLOBAL number 
	 * For `Spring.UnitScript.EmitSfx`.
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
