/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaSyncedMoveCtrl.h"

#include "LuaInclude.h"

#include "LuaHandle.h"
#include "LuaHashString.h"
#include "LuaUtils.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/MoveTypes/ScriptMoveType.h"
#include "Sim/MoveTypes/GroundMoveType.h"
#include "Sim/MoveTypes/AAirMoveType.h"
#include "Sim/MoveTypes/StrafeAirMoveType.h"
#include "Sim/MoveTypes/HoverAirMoveType.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "System/SpringMath.h"
#include "System/Log/ILog.h"
#include "System/SpringHash.h"

#include <cctype>


/******************************************************************************
 * MoveCtrl
 *
 * @see rts/Lua/LuaSyncedMoveCtrl.cpp
******************************************************************************/


bool LuaSyncedMoveCtrl::PushMoveCtrl(lua_State* L)
{
	/***
	 * Accessed via `Spring.MoveCtrl`.
	 * 
	 * @see Spring.MoveCtrl
	 * @class MoveCtrl
	 */
	lua_pushliteral(L, "MoveCtrl");
	lua_createtable(L, 0, 32);

	REGISTER_LUA_CFUNC(IsEnabled);

	REGISTER_LUA_CFUNC(Enable);
	REGISTER_LUA_CFUNC(Disable);

	REGISTER_LUA_CFUNC(SetTag);
	REGISTER_LUA_CFUNC(GetTag);

	REGISTER_LUA_CFUNC(SetProgressState);

	REGISTER_LUA_CFUNC(SetExtrapolate);

	REGISTER_LUA_CFUNC(SetPhysics);
	REGISTER_LUA_CFUNC(SetPosition);
	REGISTER_LUA_CFUNC(SetVelocity);
	REGISTER_LUA_CFUNC(SetRelativeVelocity);
	REGISTER_LUA_CFUNC(SetRotation);
	REGISTER_LUA_CFUNC(SetRotationVelocity);
	REGISTER_LUA_CFUNC(SetRotationOffset);
	REGISTER_LUA_CFUNC(SetHeading);

	REGISTER_LUA_CFUNC(SetTrackSlope);
	REGISTER_LUA_CFUNC(SetTrackGround);
	REGISTER_LUA_CFUNC(SetTrackLimits);
	REGISTER_LUA_CFUNC(SetGroundOffset);
	REGISTER_LUA_CFUNC(SetGravity);
	REGISTER_LUA_CFUNC(SetDrag);

	REGISTER_LUA_CFUNC(SetWindFactor);

	REGISTER_LUA_CFUNC(SetLimits);

	REGISTER_LUA_CFUNC(SetNoBlocking);

	REGISTER_LUA_CFUNC(SetShotStop);
	REGISTER_LUA_CFUNC(SetSlopeStop);
	REGISTER_LUA_CFUNC(SetCollideStop);
	REGISTER_LUA_CFUNC(SetLimitsStop);

	REGISTER_LUA_CFUNC(SetAirMoveTypeData);
	REGISTER_LUA_CFUNC(SetGroundMoveTypeData);
	REGISTER_LUA_CFUNC(SetGunshipMoveTypeData);

	REGISTER_LUA_CFUNC(SetMoveDef);

	lua_rawset(L, -3);

	return true;
}




static inline CUnit* ParseUnit(lua_State* L, const char* caller, int index)
{
	CUnit* unit = unitHandler.GetUnit(luaL_checkint(L, index));

	if (unit == nullptr)
		return nullptr;
	if (!CanControlUnit(L, unit))
		return nullptr;

	return unit;
}


static inline CScriptMoveType* ParseScriptMoveType(lua_State* L, const char* caller, int index)
{
	CUnit* unit = ParseUnit(L, caller, index);

	if (unit == nullptr)
		return nullptr;
	if (!unit->UsingScriptMoveType())
		return nullptr;

	return (static_cast<CScriptMoveType*>(unit->moveType));
}

template<typename DerivedMoveType>
static inline DerivedMoveType* ParseDerivedMoveType(lua_State* L, const char* caller, int index)
{
	CUnit* unit = ParseUnit(L, caller, index);

	if (unit == nullptr)
		return nullptr;

	assert(unit->moveType != nullptr);

	return (dynamic_cast<DerivedMoveType*>(unit->moveType));
}



/******************************************************************************/
/******************************************************************************/

/***
 * @function MoveCtrl.IsEnabled
 * @param unitID integer
 * @return boolean? isEnabled
 */
int LuaSyncedMoveCtrl::IsEnabled(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	lua_pushboolean(L, unit->UsingScriptMoveType());
	return 1;
}


/***
 * @function MoveCtrl.Enable
 * @param unitID integer
 */
int LuaSyncedMoveCtrl::Enable(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	unit->EnableScriptMoveType();
	return 0;
}


/***
 * @function MoveCtrl.Disable
 * @param unitID integer
 */
int LuaSyncedMoveCtrl::Disable(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	unit->DisableScriptMoveType();
	return 0;
}


/******************************************************************************/

/***
 * @function MoveCtrl.SetTag
 * @param unitID integer
 * @param tag integer
 */
int LuaSyncedMoveCtrl::SetTag(lua_State* L)
{
	CScriptMoveType* moveType = ParseScriptMoveType(L, __func__, 1);

	if (moveType == nullptr)
		return 0;

	moveType->tag = luaL_checkint(L, 2);
	return 0;
}


/***
 * @function MoveCtrl.GetTag
 * @param tag integer?
 */
int LuaSyncedMoveCtrl::GetTag(lua_State* L)
{
	CScriptMoveType* moveType = ParseScriptMoveType(L, __func__, 1);

	if (moveType == nullptr)
		return 0;

	lua_pushnumber(L, moveType->tag);
	return 1;
}


/******************************************************************************/
/******************************************************************************/


/***
 * @function MoveCtrl.SetProgressState
 * @param unitID integer
 * @param state
 * | 0 # Done
 * | 1 # Active
 * | 2 # Failed
 * | "done"
 * | "active"
 * | "failed"
 */
int LuaSyncedMoveCtrl::SetProgressState(lua_State* L)
{
	CScriptMoveType* moveType = ParseScriptMoveType(L, __func__, 1);

	if (moveType == nullptr)
		return 0;

	const int args = lua_gettop(L); // number of arguments
	if (args < 2)
		luaL_error(L, "Incorrect arguments to SetProgressState()");

	if (lua_isnumber(L, 2)) {
		const int state = lua_toint(L, 2);
		if ((state < AMoveType::Done) || (state > AMoveType::Failed)) {
			luaL_error(L, "SetProgressState(): bad state value (%d)", state);
		}
		moveType->progressState = (AMoveType::ProgressState) state;
	}
	else if (lua_isstring(L, 2)) {
		const string state = lua_tostring(L, 2);
		if (state == "done") {
			moveType->progressState = AMoveType::Done;
		} else if (state == "active") {
			moveType->progressState = AMoveType::Active;
		} else if (state == "failed") {
			moveType->progressState = AMoveType::Failed;
		} else {
			luaL_error(L, "SetProgressState(): bad state value (%s)", state.c_str());
		}
	}
	else {
		luaL_error(L, "Incorrect arguments to SetProgressState()");
	}
	return 0;
}


/******************************************************************************/

/***
 * @function MoveCtrl.SetExtrapolate
 * @param unitID integer
 * @param extrapolate boolean
 */
int LuaSyncedMoveCtrl::SetExtrapolate(lua_State* L)
{
	CScriptMoveType* moveType = ParseScriptMoveType(L, __func__, 1);

	if (moveType == nullptr)
		return 0;

	moveType->extrapolate = luaL_checkboolean(L, 2);
	return 0;
}


/******************************************************************************/

/***
 * @function MoveCtrl.SetPhysics
 * @param unitID integer
 * @param posX number Position X component.
 * @param posY number Position Y component.
 * @param posZ number Position Z component.
 * @param velX number Velocity X component.
 * @param velY number Velocity Y component.
 * @param velZ number Velocity Z component.
 * @param rotX number Rotation X component.
 * @param rotY number Rotation Y component.
 * @param rotZ number Rotation Z component.
 */
int LuaSyncedMoveCtrl::SetPhysics(lua_State* L)
{
	CScriptMoveType* moveType = ParseScriptMoveType(L, __func__, 1);

	if (moveType == nullptr)
		return 0;

	const float3 pos(luaL_checkfloat(L, 2), luaL_checkfloat(L, 3), luaL_checkfloat(L,  4));
	const float3 vel(luaL_checkfloat(L, 5), luaL_checkfloat(L, 6), luaL_checkfloat(L,  7));
	const float3 rot(luaL_checkfloat(L, 8), luaL_checkfloat(L, 9), luaL_checkfloat(L, 10));
	ASSERT_SYNCED(pos);
	ASSERT_SYNCED(vel);
	ASSERT_SYNCED(rot);
	moveType->SetPhysics(pos, vel, rot);
	return 0;
}


/***
 * @function MoveCtrl.SetPosition
 * @param unitID integer
 * @param posX number Position X component.
 * @param posY number Position Y component.
 * @param posZ number Position Z component.
 */
int LuaSyncedMoveCtrl::SetPosition(lua_State* L)
{
	CScriptMoveType* moveType = ParseScriptMoveType(L, __func__, 1);

	if (moveType == nullptr)
		return 0;

	const float3 pos(luaL_checkfloat(L, 2),
	                 luaL_checkfloat(L, 3),
	                 luaL_checkfloat(L, 4));
	ASSERT_SYNCED(pos);
	moveType->SetPosition(pos);
	return 0;
}


/***
 * @function MoveCtrl.SetVelocity
 * @param unitID integer
 * @param velX number Velocity X component.
 * @param velY number Velocity Y component.
 * @param velZ number Velocity Z component.
 */
int LuaSyncedMoveCtrl::SetVelocity(lua_State* L)
{
	CScriptMoveType* moveType = ParseScriptMoveType(L, __func__, 1);

	if (moveType == nullptr)
		return 0;

	const float3 vel(luaL_checkfloat(L, 2),
	                 luaL_checkfloat(L, 3),
	                 luaL_checkfloat(L, 4));
	ASSERT_SYNCED(vel);
	moveType->SetVelocity(vel);
	return 0;
}


/***
 * @function MoveCtrl.SetRelativeVelocity
 * @param unitID integer
 * @param relVelX number Relative velocity X component.
 * @param relVelY number Relative velocity Y component.
 * @param relVelZ number Relative velocity Z component.
 */
int LuaSyncedMoveCtrl::SetRelativeVelocity(lua_State* L)
{
	CScriptMoveType* moveType = ParseScriptMoveType(L, __func__, 1);

	if (moveType == nullptr)
		return 0;

	const float3 relVel(luaL_checkfloat(L, 2),
	                    luaL_checkfloat(L, 3),
	                    luaL_checkfloat(L, 4));
	ASSERT_SYNCED(relVel);
	moveType->SetRelativeVelocity(relVel);
	return 0;
}


/***
 * @function MoveCtrl.SetRotation
 * @param unitID integer
 * @param rotX number Rotation X component.
 * @param rotY number Rotation Y component.
 * @param rotZ number Rotation Z component.
 */
int LuaSyncedMoveCtrl::SetRotation(lua_State* L)
{
	CScriptMoveType* moveType = ParseScriptMoveType(L, __func__, 1);

	if (moveType == nullptr)
		return 0;

	const float3 rot(luaL_checkfloat(L, 2),
	                 luaL_checkfloat(L, 3),
	                 luaL_checkfloat(L, 4));
	ASSERT_SYNCED(rot);
	moveType->SetRotation(rot);
	return 0;
}


/***
 * @function MoveCtrl.SetRotationOffset
 * @deprecated
 */
int LuaSyncedMoveCtrl::SetRotationOffset(lua_State* L)
{
	// DEPRECATED
	return 0;
}


/***
 * @function MoveCtrl.SetRotationVelocity
 * @param unitID integer
 * @param rotVelX number Rotation velocity X component.
 * @param rotVelY number Rotation velocity Y component.
 * @param rotVelZ number Rotation velocity Z component.
 */
int LuaSyncedMoveCtrl::SetRotationVelocity(lua_State* L)
{
	CScriptMoveType* moveType = ParseScriptMoveType(L, __func__, 1);

	if (moveType == nullptr)
		return 0;

	const float3 rotVel(luaL_checkfloat(L, 2),
	                    luaL_checkfloat(L, 3),
	                    luaL_checkfloat(L, 4));
	ASSERT_SYNCED(rotVel);
	moveType->SetRotationVelocity(rotVel);
	return 0;
}

/***
 * @function MoveCtrl.SetHeading
 * @param unitID integer
 * @param heading Heading
 */
int LuaSyncedMoveCtrl::SetHeading(lua_State* L)
{
	CScriptMoveType* moveType = ParseScriptMoveType(L, __func__, 1);

	if (moveType == nullptr)
		return 0;

	const short heading = (short)luaL_checknumber(L, 2);
	ASSERT_SYNCED((short)heading);
	moveType->SetHeading(heading);
	return 0;
}


/******************************************************************************/

/***
 * @function MoveCtrl.SetTrackSlope
 * @param unitID integer
 * @param trackSlope boolean
 */
int LuaSyncedMoveCtrl::SetTrackSlope(lua_State* L)
{
	CScriptMoveType* moveType = ParseScriptMoveType(L, __func__, 1);

	if (moveType == nullptr)
		return 0;

	moveType->trackSlope = luaL_checkboolean(L, 2);
	return 0;
}


/***
 * @function MoveCtrl.SetTrackGround
 * @param unitID integer
 * @param trackGround boolean
 */
int LuaSyncedMoveCtrl::SetTrackGround(lua_State* L)
{
	CScriptMoveType* moveType = ParseScriptMoveType(L, __func__, 1);

	if (moveType == nullptr)
		return 0;

	moveType->trackGround = luaL_checkboolean(L, 2);
	return 0;
}


/***
 * @function MoveCtrl.SetTrackLimits
 * @param unitID integer
 * @param trackLimits boolean
 */
int LuaSyncedMoveCtrl::SetTrackLimits(lua_State* L)
{
	CScriptMoveType* moveType = ParseScriptMoveType(L, __func__, 1);

	if (moveType == nullptr)
		return 0;

	moveType->trackLimits = luaL_checkboolean(L, 2);
	return 0;
}


/***
 * @function MoveCtrl.SetGroundOffset
 * @param unitID integer
 * @param groundOffset number
 */
int LuaSyncedMoveCtrl::SetGroundOffset(lua_State* L)
{
	CScriptMoveType* moveType = ParseScriptMoveType(L, __func__, 1);

	if (moveType == nullptr)
		return 0;

	moveType->groundOffset = luaL_checkfloat(L, 2);
	return 0;
}


/***
 * @function MoveCtrl.SetGravity
 * @param unitID integer
 * @param gravityFactor number
 */
int LuaSyncedMoveCtrl::SetGravity(lua_State* L)
{
	CScriptMoveType* moveType = ParseScriptMoveType(L, __func__, 1);

	if (moveType == nullptr)
		return 0;

	moveType->gravityFactor = luaL_checkfloat(L, 2);
	return 0;
}


/***
 * @function MoveCtrl.SetDrag
 * @param unitID integer
 * @param drag number
 */
int LuaSyncedMoveCtrl::SetDrag(lua_State* L)
{
	CScriptMoveType* moveType = ParseScriptMoveType(L, __func__, 1);

	if (moveType == nullptr)
		return 0;

	moveType->drag = luaL_checkfloat(L, 2);
	return 0;
}


/***
 * @function MoveCtrl.SetWindFactor
 * @param unitID integer
 * @param windFactor number
 */
int LuaSyncedMoveCtrl::SetWindFactor(lua_State* L)
{
	CScriptMoveType* moveType = ParseScriptMoveType(L, __func__, 1);

	if (moveType == nullptr)
		return 0;

	moveType->windFactor = luaL_checkfloat(L, 2);
	return 0;
}


/***
 * @function MoveCtrl.SetLimits
 * @param unitID integer
 * @param minX number Minimum position X component.
 * @param minY number Minimum position Y component.
 * @param minZ number Minimum position Z component.
 * @param maxX number Maximum position X component.
 * @param maxY number Maximum position Y component.
 * @param maxZ number Maximum position Z component.
 */
int LuaSyncedMoveCtrl::SetLimits(lua_State* L)
{
	CScriptMoveType* moveType = ParseScriptMoveType(L, __func__, 1);

	if (moveType == nullptr)
		return 0;

	moveType->mins = {luaL_checkfloat(L, 2), luaL_checkfloat(L, 3), luaL_checkfloat(L, 4)};
	moveType->maxs = {luaL_checkfloat(L, 5), luaL_checkfloat(L, 6), luaL_checkfloat(L, 7)};
	return 0;
}


/******************************************************************************/

/***
 * @function MoveCtrl.SetNoBlocking
 * @param unitID integer
 * @param noBlocking boolean
 */
int LuaSyncedMoveCtrl::SetNoBlocking(lua_State* L)
{
	CScriptMoveType* moveType = ParseScriptMoveType(L, __func__, 1);

	if (moveType == nullptr)
		return 0;

	// marks or unmarks the unit on the blocking-map, but
	// does not change its blocking (collidable) state
	moveType->SetNoBlocking(luaL_checkboolean(L, 2));
	return 0;
}


int LuaSyncedMoveCtrl::SetShotStop(lua_State* L) { return 0; }
int LuaSyncedMoveCtrl::SetSlopeStop(lua_State* L) { return 0; }


/***
 * @function MoveCtrl.SetCollideStop
 * @param unitID integer
 * @param collideStop boolean
 */
int LuaSyncedMoveCtrl::SetCollideStop(lua_State* L)
{
	CScriptMoveType* moveType = ParseScriptMoveType(L, __func__, 1);

	if (moveType == nullptr)
		return 0;

	moveType->groundStop = lua_toboolean(L, 2); // FIXME
	return 0;
}


/***
 * @function MoveCtrl.SetLimitsStop
 * @param unitID integer
 * @param limitsStop boolean
 */
int LuaSyncedMoveCtrl::SetLimitsStop(lua_State* L)
{
	CScriptMoveType* moveType = ParseScriptMoveType(L, __func__, 1);

	if (moveType == nullptr)
		return 0;

	moveType->limitsStop = lua_toboolean(L, 2);
	return 0;
}



/******************************************************************************/
/* MoveType member-value handling */

template<typename ValueType>
static bool SetMoveTypeValue(AMoveType* mt, const char* key, ValueType val)
{
	// NOTE: only supports floats and bools, callee MUST reinterpret &val as float* or bool*
	return (mt->SetMemberValue(spring::LiteHash(key, strlen(key), 0), &val));
}

static inline bool SetMoveTypeValue(lua_State* L, AMoveType* moveType, int keyIdx, int valIdx)
{
	if (lua_isnumber(L, valIdx))
		return (SetMoveTypeValue<float>(moveType, lua_tostring(L, keyIdx), lua_tofloat(L, valIdx)));

	if (lua_isboolean(L, valIdx))
		return (SetMoveTypeValue<bool>(moveType, lua_tostring(L, keyIdx), lua_toboolean(L, valIdx)));

	return false;
}

/***
 * @alias GenericMoveTypeNumberKey
 * | "maxSpeed"
 * | "maxWantedSpeed"
 * | "maneuverLeash"
 * | "waterline"
 */

/***
 * @alias GenericMoveTypeBooleanKey
 * | "useWantedSpeed[0]" # Use wanted speed for individual orders.
 * | "useWantedSpeed[1]" # Use wanted speed for formation orders.
 */

/***
 * @class GenericMoveType
 * @field maxSpeed number?
 * @field maxWantedSpeed number?
 * @field maneuverLeash number?
 * @field waterline number?
 * @field ["useWantedSpeed[0]"] boolean? # Use wanted speed for individual orders.
 * @field ["useWantedSpeed[1]"] boolean? # Use wanted speed for formation orders.
 */

/** - Not exported.
 * 
 * Parses a MoveType object.
 * 
 * Parses params, starting at param 2:
 * 
 * Overload 1:
 * @param <MoveTypeTable> boolean
 * @return number numAssignedValues
 *
 * Overload 2:
 * @param <NumberKey> number
 * @return number numAssignedValues
 *
 * Overload 3:
 * @param <BooleanKey> boolean
 * @return number numAssignedValues
 */
static int SetMoveTypeData(lua_State* L, AMoveType* moveType, const char* caller)
{
	int numAssignedValues = 0;

	if (moveType == nullptr) {
		luaL_error(L, "[%s] unit %d has incompatible movetype for %s", __func__, lua_toint(L, 1), caller);
		return numAssignedValues;
	}

	switch (lua_gettop(L)) {
		case 2: {
			// two args (unitID, {"key1" = (number | bool) value1, ...})
			constexpr int tableIdx = 2;

			if (lua_istable(L, tableIdx)) {
				for (lua_pushnil(L); lua_next(L, tableIdx) != 0; lua_pop(L, 1)) {
					if (lua_israwstring(L, -2) && SetMoveTypeValue(L, moveType, -2, -1)) {
						numAssignedValues++;
					} else {
						LOG_L(L_WARNING, "[%s] incompatible movetype key for %s", __func__, caller);
					}
				}
			}
		} break;

		case 3: {
			// three args (unitID, "key", (number | bool) value)
			if (lua_isstring(L, 2) && SetMoveTypeValue(L, moveType, 2, 3)) {
				numAssignedValues++;
			} else {
				LOG_L(L_WARNING, "[%s] incompatible movetype key for %s", __func__, caller);
			}
		} break;
	}

	lua_pushnumber(L, numAssignedValues);
	return 1;
}

/***
 * @class HoverAirMoveType : GenericMoveType
 * @field collide boolean?
 * @field dontLand boolean?
 * @field airStrafe boolean?
 * @field useSmoothMesh boolean?
 * @field bankingAllowed boolean?
 * @field wantedHeight number?
 * @field accRate number?
 * @field decRate number?
 * @field turnRate number?
 * @field altitudeRate number?
 * @field currentBank number?
 * @field currentPitch number?
 * @field maxDrift number?
 */

/***
 * @function MoveCtrl.SetGunshipMoveTypeData
 * @param unitID integer
 * @param moveType HoverAirMoveType
 * @return number numAssignedValues
 */
/***
 * @function MoveCtrl.SetGunshipMoveTypeData
 * @param unitID integer
 * @param key
 * | GenericMoveTypeBooleanKey
 * | "collide"
 * | "dontLand"
 * | "airStrafe"
 * | "useSmoothMesh"
 * | "bankingAllowed"
 * @param value boolean
 * @return number numAssignedValues
 */
/***
 * @function MoveCtrl.SetGunshipMoveTypeData
 * @param unitID integer
 * @param key
 * | GenericMoveTypeNumberKey
 * | "wantedHeight"
 * | "accRate"
 * | "decRate"
 * | "turnRate"
 * | "altitudeRate"
 * | "currentBank"
 * | "currentPitch"
 * | "maxDrift"
 * @param value number
 * @return number numAssignedValues
 */
int LuaSyncedMoveCtrl::SetGunshipMoveTypeData(lua_State* L)
{
	return (SetMoveTypeData(L, ParseDerivedMoveType<CHoverAirMoveType>(L, __func__, 1), __func__));
}

/***
 * @class StrafeAirMoveType : GenericMoveType
 * @field collide boolean?
 * @field useSmoothMesh boolean?
 * @field loopbackAttack boolean?
 * @field maneuverBlockTime integer?
 * @field wantedHeight number?
 * @field turnRadius number?
 * @field accRate number?
 * @field decRate number?
 * @field maxAcc number? Synonym for `accRate`.
 * @field maxDec number? Synonym for `decRate`.
 * @field maxBank number?
 * @field maxPitch number?
 * @field maxAileron number?
 * @field maxElevator number?
 * @field maxRudder number?
 * @field attackSafetyDistance number?
 * @field myGravity number?
 */

/***
 * @function MoveCtrl.SetAirMoveTypeData
 * @param unitID integer
 * @param moveType StrafeAirMoveType
 * @return number numAssignedValues
 */
/***
 * @function MoveCtrl.SetAirMoveTypeData
 * @param unitID integer
 * @param key
 * | GenericMoveTypeBooleanKey
 * | "collide"
 * | "useSmoothMesh"
 * | "loopbackAttack"
  * @param value boolean
 * @return number numAssignedValues
 */
/***
 * @function MoveCtrl.SetAirMoveTypeData
 * @param unitID integer
 * @param key
 * | GenericMoveTypeNumberKey
 * | "wantedHeight" 
 * | "turnRadius" 
 * | "accRate" 
 * | "decRate" 
 * | "maxAcc" # Synonym for `accRate`.
 * | "maxDec" # Synonym for `decRate`.
 * | "maxBank" 
 * | "maxPitch" 
 * | "maxAileron" 
 * | "maxElevator" 
 * | "maxRudder" 
 * | "attackSafetyDistance" 
 * | "myGravity" 
 * @param value number
 * @return number numAssignedValues
 */
/***
 * @function MoveCtrl.SetAirMoveTypeData
 * @param unitID integer
 * @param key
 * | "maneuverBlockTime"
 * @param value integer
 * @return number numAssignedValues
 */
int LuaSyncedMoveCtrl::SetAirMoveTypeData(lua_State* L)
{
	return (SetMoveTypeData(L, ParseDerivedMoveType<CStrafeAirMoveType>(L, __func__, 1), __func__));
}

/***
 * @class GroundMoveType : GenericMoveType
 * @field atGoal boolean?
 * @field atEndOfPath boolean?
 * @field pushResistant boolean?
 * @field minScriptChangeHeading integer?
 * @field turnRate number?
 * @field turnAccel number?
 * @field accRate number?
 * @field decRate number?
 * @field myGravity number?
 * @field maxReverseDist number?
 * @field minReverseAngle number?
 * @field maxReverseSpeed number?
 * @field sqSkidSpeedMult number?
 */

/***
 * @function MoveCtrl.SetGroundMoveTypeData
 * @param unitID integer
 * @param moveType GroundMoveType
 * @return number numAssignedValues
 */
/***
 * @function MoveCtrl.SetGroundMoveTypeData
 * @param unitID integer
 * @param key
 * | GenericMoveTypeBooleanKey
 * | "atGoal"
 * | "atEndOfPath"
 * | "pushResistant"
 * @param value boolean
 * @return number numAssignedValues
 */
/***
 * @function MoveCtrl.SetGroundMoveTypeData
 * @param unitID integer
 * @param key
 * | GenericMoveTypeNumberKey
 * | "turnRate"
 * | "turnAccel"
 * | "accRate"
 * | "decRate"
 * | "myGravity"
 * | "maxReverseDist"
 * | "minReverseAngle"
 * | "maxReverseSpeed"
 * | "sqSkidSpeedMult"
 * @param value number
 * @return number numAssignedValues
 */
/***
 * @function MoveCtrl.SetGroundMoveTypeData
 * @param unitID integer
 * @param key
 * | "minScriptChangeHeading"
 * @param value integer
 * @return number numAssignedValues
 */
int LuaSyncedMoveCtrl::SetGroundMoveTypeData(lua_State* L)
{
	return (SetMoveTypeData(L, ParseDerivedMoveType<CGroundMoveType>(L, __func__, 1), __func__));
}



/******************************************************************************/
/******************************************************************************/

/***
 * @function MoveCtrl.SetMoveDef
 * @param unitID integer
 * @param moveDef integer|string Name or path type of the MoveDef.
 * @return boolean success `true` if the `MoveDef` was set, `false` if `unitID` or `moveDef` were invalid, or if the unit does not support a `MoveDef`.
 */
int LuaSyncedMoveCtrl::SetMoveDef(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);
	MoveDef* moveDef = nullptr;

	if (unit == nullptr) {
		lua_pushboolean(L, false);
		return 1;
	}
	if (unit->moveDef == nullptr) {
		// aircraft or structure, not supported
		lua_pushboolean(L, false);
		return 1;
	}

	// MoveType instance must already have been assigned
	assert(unit->moveType != nullptr);

	// parse a MoveDef by number *or* by string (mutually exclusive)
	if (lua_israwnumber(L, 2))
		moveDef = moveDefHandler.GetMoveDefByPathType(std::clamp(luaL_checkint(L, 2), 0, int(moveDefHandler.GetNumMoveDefs()) - 1));
	if (lua_israwstring(L, 2))
		moveDef = moveDefHandler.GetMoveDefByName(lua_tostring(L, 2));

	if (moveDef == nullptr) {
		lua_pushboolean(L, false);
		return 1;
	}

	// PFS might have cached data by path-type which must be cleared
	if (unit->UsingScriptMoveType()) {
		unit->prevMoveType->StopMoving();
	} else {
		unit->moveType->StopMoving();
	}

	// the case where moveDef->pathType == unit->moveDef->pathType does no harm
	// note: if a unit (ID) is available, then its current MoveDef should always
	// be taken over the MoveDef corresponding to its UnitDef::pathType wherever
	// MoveDef properties are used in decision logic
	lua_pushboolean(L, (unit->moveDef = moveDef) != nullptr);
	return 1;
}
