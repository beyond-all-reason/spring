/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* exports the #defines from CobDefines.h to Lua */

#include "LuaConstCOB.h"
#include "LuaInclude.h"
#include "LuaUtils.h"
#include "Sim/Units/Scripts/CobDefines.h"
#include "Sim/Projectiles/PieceProjectile.h"


bool LuaConstCOB::PushEntries(lua_State* L)
{
#define PUSH_COB(cmd) LuaPushNamedNumber(L, #cmd, cmd)

	/***
	 * COB constants
	 * @enum COB
	 */
	/*** @field COB.ACTIVATION integer */
	PUSH_COB(ACTIVATION);
	/*** @field COB.STANDINGMOVEORDERS integer */
	PUSH_COB(STANDINGMOVEORDERS);
	/*** @field COB.STANDINGFIREORDERS integer */
	PUSH_COB(STANDINGFIREORDERS);
	/*** @field COB.HEALTH integer */
	PUSH_COB(HEALTH);
	/*** @field COB.INBUILDSTANCE integer */
	PUSH_COB(INBUILDSTANCE);
	/*** @field COB.BUSY integer */
	PUSH_COB(BUSY);
	/*** @field COB.PIECE_XZ integer */
	PUSH_COB(PIECE_XZ);
	/*** @field COB.PIECE_Y integer */
	PUSH_COB(PIECE_Y);
	/*** @field COB.UNIT_XZ integer */
	PUSH_COB(UNIT_XZ);
	/*** @field COB.UNIT_Y integer */
	PUSH_COB(UNIT_Y);
	/*** @field COB.UNIT_HEIGHT integer */
	PUSH_COB(UNIT_HEIGHT);
	/*** @field COB.XZ_ATAN integer */
	PUSH_COB(XZ_ATAN);
	/*** @field COB.XZ_HYPOT integer */
	PUSH_COB(XZ_HYPOT);
	/*** @field COB.ATAN integer */
	PUSH_COB(ATAN);
	/*** @field COB.HYPOT integer */
	PUSH_COB(HYPOT);
	/*** @field COB.GROUND_HEIGHT integer */
	PUSH_COB(GROUND_HEIGHT);
	/*** @field COB.BUILD_PERCENT_LEFT integer */
	PUSH_COB(BUILD_PERCENT_LEFT);
	/*** @field COB.YARD_OPEN integer */
	PUSH_COB(YARD_OPEN);
	/*** @field COB.BUGGER_OFF integer */
	PUSH_COB(BUGGER_OFF);
	/*** @field COB.ARMORED integer */
	PUSH_COB(ARMORED);

/*	PUSH_COB(WEAPON_AIM_ABORTED);
	PUSH_COB(WEAPON_READY);
	PUSH_COB(WEAPON_LAUNCH_NOW);
	PUSH_COB(FINISHED_DYING);
	PUSH_COB(ORIENTATION);*/
	/*** @field COB.IN_WATER integer */
	PUSH_COB(IN_WATER);
	/*** @field COB.CURRENT_SPEED integer */
	PUSH_COB(CURRENT_SPEED);
//	PUSH_COB(MAGIC_DEATH);
	/*** @field COB.VETERAN_LEVEL integer */
	PUSH_COB(VETERAN_LEVEL);
	/*** @field COB.ON_ROAD integer */
	PUSH_COB(ON_ROAD);

	/*** @field COB.MAX_ID integer */
	PUSH_COB(MAX_ID);
	/*** @field COB.MY_ID integer */
	PUSH_COB(MY_ID);
	/*** @field COB.UNIT_TEAM integer */
	PUSH_COB(UNIT_TEAM);
	/*** @field COB.UNIT_BUILD_PERCENT_LEFT integer */
	PUSH_COB(UNIT_BUILD_PERCENT_LEFT);
	/*** @field COB.UNIT_ALLIED integer */
	PUSH_COB(UNIT_ALLIED);
	/*** @field COB.MAX_SPEED integer */
	PUSH_COB(MAX_SPEED);
	/*** @field COB.CLOAKED integer */
	PUSH_COB(CLOAKED);
	/*** @field COB.WANT_CLOAK integer */
	PUSH_COB(WANT_CLOAK);
	/*** @field COB.GROUND_WATER_HEIGHT integer */
	PUSH_COB(GROUND_WATER_HEIGHT);
	/*** @field COB.UPRIGHT integer */
	PUSH_COB(UPRIGHT);
	/*** @field COB.POW integer */
	PUSH_COB(POW);
	/*** @field COB.PRINT integer */
	PUSH_COB(PRINT);
	/*** @field COB.HEADING integer */
	PUSH_COB(HEADING);
	/*** @field COB.TARGET_ID integer */
	PUSH_COB(TARGET_ID);
	/*** @field COB.LAST_ATTACKER_ID integer */
	PUSH_COB(LAST_ATTACKER_ID);
	/*** @field COB.LOS_RADIUS integer */
	PUSH_COB(LOS_RADIUS);
	/*** @field COB.AIR_LOS_RADIUS integer */
	PUSH_COB(AIR_LOS_RADIUS);
	/*** @field COB.RADAR_RADIUS integer */
	PUSH_COB(RADAR_RADIUS);
	/*** @field COB.JAMMER_RADIUS integer */
	PUSH_COB(JAMMER_RADIUS);
	/*** @field COB.SONAR_RADIUS integer */
	PUSH_COB(SONAR_RADIUS);
	/*** @field COB.SONAR_JAM_RADIUS integer */
	PUSH_COB(SONAR_JAM_RADIUS);
	/*** @field COB.SEISMIC_RADIUS integer */
	PUSH_COB(SEISMIC_RADIUS);
	/*** @field COB.DO_SEISMIC_PING integer */
	PUSH_COB(DO_SEISMIC_PING);
	/*** @field COB.CURRENT_FUEL integer */
	PUSH_COB(CURRENT_FUEL);
	/*** @field COB.TRANSPORT_ID integer */
	PUSH_COB(TRANSPORT_ID);
	/*** @field COB.SHIELD_POWER integer */
	PUSH_COB(SHIELD_POWER);
	/*** @field COB.STEALTH integer */
	PUSH_COB(STEALTH);
	/*** @field COB.CRASHING integer */
	PUSH_COB(CRASHING);
	/*** @field COB.CHANGE_TARGET integer */
	PUSH_COB(CHANGE_TARGET);
	/*** @field COB.CEG_DAMAGE integer */
	PUSH_COB(CEG_DAMAGE);
	/*** @field COB.COB_ID integer */
	PUSH_COB(COB_ID);
	/*** @field COB.PLAY_SOUND integer */
	PUSH_COB(PLAY_SOUND);
	/*** @field COB.KILL_UNIT integer */
	PUSH_COB(KILL_UNIT);
	/*** @field COB.SET_WEAPON_UNIT_TARGET integer */
	PUSH_COB(SET_WEAPON_UNIT_TARGET);
	/*** @field COB.SET_WEAPON_GROUND_TARGET integer */
	PUSH_COB(SET_WEAPON_GROUND_TARGET);
	/*** @field COB.SONAR_STEALTH integer */
	PUSH_COB(SONAR_STEALTH);
	/*** @field COB.REVERSING integer */
	PUSH_COB(REVERSING);

	// NOTE: [LUA0 - LUA9] are defined in CobThread.cpp as [110 - 119]

	/*** @field COB.FLANK_B_MODE integer */
	PUSH_COB(FLANK_B_MODE);
	/*** @field COB.FLANK_B_DIR integer */
	PUSH_COB(FLANK_B_DIR);
	/*** @field COB.FLANK_B_MOBILITY_ADD integer */
	PUSH_COB(FLANK_B_MOBILITY_ADD);
	/*** @field COB.FLANK_B_MAX_DAMAGE integer */
	PUSH_COB(FLANK_B_MAX_DAMAGE);
	/*** @field COB.FLANK_B_MIN_DAMAGE integer */
	PUSH_COB(FLANK_B_MIN_DAMAGE);
	/*** @field COB.WEAPON_RELOADSTATE integer */
	PUSH_COB(WEAPON_RELOADSTATE);
	/*** @field COB.WEAPON_RELOADTIME integer */
	PUSH_COB(WEAPON_RELOADTIME);
	/*** @field COB.WEAPON_ACCURACY integer */
	PUSH_COB(WEAPON_ACCURACY);
	/*** @field COB.WEAPON_SPRAY integer */
	PUSH_COB(WEAPON_SPRAY);
	/*** @field COB.WEAPON_RANGE integer */
	PUSH_COB(WEAPON_RANGE);
	/*** @field COB.WEAPON_PROJECTILE_SPEED integer */
	PUSH_COB(WEAPON_PROJECTILE_SPEED);
	/*** @field COB.MIN integer */
	LuaPushNamedNumber(L, "MIN", COB_MIN);
	/*** @field COB.MAX integer */
	LuaPushNamedNumber(L, "MAX", COB_MAX);
	/*** @field COB.ABS integer */
	PUSH_COB(ABS);
	/*** @field COB.GAME_FRAME  integer */
	PUSH_COB(GAME_FRAME);

	// NOTE: shared variables use codes [1024 - 5119]

	return true;
}

bool LuaConstSFX::PushEntries(lua_State* L)
{
	/*** 
	 * @enum SFX
	 */

	/***
	 * Piece flag for `Spring.UnitScript.Explode`.
	 * 
	 * @field SFX.SHATTER integer
	 * @field SFX.EXPLODE integer
	 * @field SFX.EXPLODE_ON_HIT integer
	 * @field SFX.FALL integer
	 * @field SFX.SMOKE integer
	 * @field SFX.FIRE integer
	 * @field SFX.NONE integer
	 * @field SFX.NO_CEG_TRAIL integer
	 * @field SFX.NO_HEATCLOUD integer
	 * @field SFX.RECURSIVE integer
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

	/***
	 * Piece flag for `Spring.UnitScript.EmitSfx`.
	 * @field SFX.VTOL integer
	 * @field SFX.WAKE integer
	 * @field SFX.REVERSE_WAKE integer
	 */
	LuaPushNamedNumber(L, "VTOL",            SFX_VTOL);
	LuaPushNamedNumber(L, "WAKE",            SFX_WAKE);
	LuaPushNamedNumber(L, "REVERSE_WAKE",    SFX_REVERSE_WAKE);
	// no need for WAKE_2 and REVERSE_WAKE_2 as they are same as WAKE and REVERSE_WAKE
	//LuaPushNamedNumber(L, "WAKE_2",          SFX_WAKE_2);
	//LuaPushNamedNumber(L, "REVERSE_WAKE_2",  SFX_REVERSE_WAKE_2);

	/***
	 * Piece flag for `Spring.UnitScript.EmitSfx`.
	 * @field SFX.WHITE_SMOKE integer
	 * @field SFX.BLACK_SMOKE integer
	 * @field SFX.BUBBLE integer
	 * @field SFX.CEG integer
	 * @field SFX.FIRE_WEAPON integer
	 * @field SFX.DETONATE_WEAPON integer
	 * @field SFX.GLOBAL integer
	 */
	LuaPushNamedNumber(L, "WHITE_SMOKE",     SFX_WHITE_SMOKE);
	LuaPushNamedNumber(L, "BLACK_SMOKE",     SFX_BLACK_SMOKE);
	LuaPushNamedNumber(L, "BUBBLE",          SFX_BUBBLE);
	LuaPushNamedNumber(L, "CEG",             SFX_CEG);
	LuaPushNamedNumber(L, "FIRE_WEAPON",     SFX_FIRE_WEAPON);
	LuaPushNamedNumber(L, "DETONATE_WEAPON", SFX_DETONATE_WEAPON);
	LuaPushNamedNumber(L, "GLOBAL",          SFX_GLOBAL);

	return true;
}
