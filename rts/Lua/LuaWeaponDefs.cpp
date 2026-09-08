/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <string>
#include <vector>
#include <cctype>

#include "LuaWeaponDefs.h"

#include "LuaInclude.h"

#include "LuaDefs.h"
#include "LuaHandle.h"
#include "LuaUtils.h"
#include "Game/TraceRay.h"
#include "Rendering/Models/IModelParser.h"
#include "Sim/Misc/CategoryHandler.h"
#include "Sim/Misc/DamageArrayHandler.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "System/FileSystem/SimpleParser.h"
#include "System/Log/ILog.h"
#include "System/StringUtil.h"
#include "Sim/Misc/GlobalSynced.h"


static ParamMap paramMap;

static bool InitParamMap();

// iteration routines
static int Next(lua_State* L);
static int Pairs(lua_State* L);

// meta-table calls
static int WeaponDefIndex(lua_State* L);
static int WeaponDefNewIndex(lua_State* L);
static int WeaponDefMetatable(lua_State* L);

// special access functions
static int NoFeatureCollide(lua_State* L, const void* data);
static int NoFriendlyCollide(lua_State* L, const void* data);
static int NoNeutralCollide(lua_State* L, const void* data);
static int NoFireBaseCollide(lua_State* L, const void* data);
static int NoNonTargetCollide(lua_State* L, const void* data);

static int VisualsTable(lua_State* L, const void* data);
static int DamagesArray(lua_State* L, const void* data);
static int CustomParamsTable(lua_State* L, const void* data);
static int GuiSoundSetTable(lua_State* L, const void* data);
//static int CategorySetFromBits(lua_State* L, const void* data);


/******************************************************************************/
/******************************************************************************/

bool LuaWeaponDefs::PushEntries(lua_State* L)
{
	InitParamMap();

	typedef int (*IndxFuncType)(lua_State*);
	typedef int (*IterFuncType)(lua_State*);

	const auto& defsVec = weaponDefHandler->GetWeaponDefsVec();

	const std::array<const LuaHashString, 3> indxOpers = {{
		LuaHashString("__index"),
		LuaHashString("__newindex"),
		LuaHashString("__metatable")
	}};
	const std::array<const LuaHashString, 2> iterOpers = {{
		LuaHashString("pairs"),
		LuaHashString("next")
	}};

	const std::array<const IndxFuncType, 3> indxFuncs = {{WeaponDefIndex, WeaponDefNewIndex, WeaponDefMetatable}};
	const std::array<const IterFuncType, 2> iterFuncs = {{Pairs, Next}};

	for (const auto& weaponDef: defsVec) {
		// The first weaponDef is invalid (dummy), so we skip it
		const auto def = weaponDefHandler->GetWeaponDefByID(weaponDef.id);

		if (def == nullptr)
			continue;

		PushObjectDefProxyTable(L, indxOpers, iterOpers, indxFuncs, iterFuncs, def);
	}

	return true;
}


/******************************************************************************/

static int WeaponDefIndex(lua_State* L)
{
	// not a default value
	if (!lua_isstring(L, 2)) {
		lua_rawget(L, 1);
		return 1;
	}

	const char* name = lua_tostring(L, 2);
	ParamMap::const_iterator it = paramMap.find(name);

	// not a default value
	if (it == paramMap.end()) {
		lua_rawget(L, 1);
		return 1;
	}

	const void* userData = lua_touserdata(L, lua_upvalueindex(1));
	const WeaponDef* wd = static_cast<const WeaponDef*>(userData);
	const DataElement& elem = it->second;
	const void* p = ((const char*)wd) + elem.offset;
	switch (elem.type) {
		case READONLY_TYPE: {
			lua_rawget(L, 1);
			return 1;
		}
		case INT_TYPE: {
			lua_pushnumber(L, *reinterpret_cast<const int*>(p));
			return 1;
		}
		case BOOL_TYPE: {
			lua_pushboolean(L, *reinterpret_cast<const bool*>(p));
			return 1;
		}
		case FLOAT_TYPE: {
			lua_pushnumber(L, *reinterpret_cast<const float*>(p));
			return 1;
		}
		case STRING_TYPE: {
			lua_pushsstring(L, *reinterpret_cast<const std::string*>(p));
			return 1;
		}
		case FUNCTION_TYPE: {
			return elem.func(L, p);
		}
		case ERROR_TYPE: {
			LOG_L(L_ERROR, "[%s] ERROR_TYPE for key \"%s\" in WeaponDefs __index", __func__, name);
			lua_pushnil(L);
			return 1;
		}
	}

	return 0;
}


static int WeaponDefNewIndex(lua_State* L)
{
	// not a default value, set it
	if (!lua_isstring(L, 2)) {
		lua_rawset(L, 1);
		return 0;
	}

	const char* name = lua_tostring(L, 2);
	ParamMap::const_iterator it = paramMap.find(name);

	// not a default value, set it
	if (it == paramMap.end()) {
		lua_rawset(L, 1);
		return 0;
	}

	const void* userData = lua_touserdata(L, lua_upvalueindex(1));
	const WeaponDef* wd = static_cast<const WeaponDef*>(userData);

	// write-protected
	if (!gs->editDefsEnabled) {
	 	luaL_error(L, "Attempt to write WeaponDefs[%d].%s", wd->id, name);
	 	return 0;
	}

	// Definition editing
	const DataElement& elem = it->second;
	const void* p = ((const char*)wd) + elem.offset;

	switch (elem.type) {
		case FUNCTION_TYPE:
		case READONLY_TYPE: {
			luaL_error(L, "Can not write to %s", name);
			return 0;
		}
		case INT_TYPE: {
			*((int*)p) = lua_toint(L, -1);
			return 0;
		}
		case BOOL_TYPE: {
			*((bool*)p) = lua_toboolean(L, -1);
			return 0;
		}
		case FLOAT_TYPE: {
			*((float*)p) = lua_tofloat(L, -1);
			return 0;
		}
		case STRING_TYPE: {
			*((string*)p) = lua_tostring(L, -1);
			return 0;
		}
		case ERROR_TYPE: {
			LOG_L(L_ERROR, "[%s] ERROR_TYPE for key \"%s\" in WeaponDefs __newindex", __func__, name);
			lua_pushnil(L);
			return 1;
		}
	}

	return 0;
}


static int WeaponDefMetatable(lua_State* L)
{
	//const void* userData = lua_touserdata(L, lua_upvalueindex(1));
	//const WeaponDef* wd = (const WeaponDef*)userData;
	return 0;
}


/******************************************************************************/

static int Next(lua_State* L)
{
	return LuaUtils::Next(paramMap, L);
}


static int Pairs(lua_State* L)
{
	luaL_checktype(L, 1, LUA_TTABLE);
	lua_pushcfunction(L, Next);	// iterator
	lua_pushvalue(L, 1);        // state (table)
	lua_pushnil(L);             // initial value
	return 3;
}


/******************************************************************************/
/******************************************************************************/

static int DamagesArray(lua_State* L, const void* data)
{
	const DamageArray& d = *static_cast<const DamageArray*>(data);

	lua_createtable(L, damageArrayHandler.GetNumTypes(), 5);
	LuaPushNamedNumber(L, "impulseFactor",      d.impulseFactor);
	LuaPushNamedNumber(L, "impulseBoost",       d.impulseBoost);
	LuaPushNamedNumber(L, "craterMult",         d.craterMult);
	LuaPushNamedNumber(L, "craterBoost",        d.craterBoost);
	LuaPushNamedNumber(L, "paralyzeDamageTime", d.paralyzeDamageTime);

	// damage values
	for (int i = 0, n = damageArrayHandler.GetNumTypes(); i < n; i++) {
		lua_pushnumber(L, i);
		lua_pushnumber(L, d.Get(i));
		lua_rawset(L, -3);
	}

	return 1;
}


static int VisualsTable(lua_State* L, const void* data)
{
	const struct WeaponDef::Visuals& v = *static_cast<const struct WeaponDef::Visuals*>(data);
	lua_createtable(L, 0, 28);
	LuaPushNamedString(L, "modelName",            modelLoader.FindModelPath(v.modelName));
	LuaPushNamedNumber(L, "colorR",               v.color.x);
	LuaPushNamedNumber(L, "colorG",               v.color.y);
	LuaPushNamedNumber(L, "colorB",               v.color.z);
	LuaPushNamedNumber(L, "color2R",              v.color2.x);
	LuaPushNamedNumber(L, "color2G",              v.color2.y);
	LuaPushNamedNumber(L, "color2B",              v.color2.z);
	LuaPushNamedBool  (L, "smokeTrail",           v.smokeTrail);
	LuaPushNamedBool  (L, "smokeTrailCastShadow", v.smokeTrailCastShadow);
	LuaPushNamedNumber(L, "smokePeriod",          v.smokePeriod);
	LuaPushNamedNumber(L, "smokeTime",            v.smokeTime);
	LuaPushNamedNumber(L, "smokeSize",            v.smokeSize);
	LuaPushNamedNumber(L, "smokeColor",           v.smokeColor);
	LuaPushNamedNumber(L, "tileLength",           v.tilelength);
	LuaPushNamedNumber(L, "scrollSpeed",          v.scrollspeed);
	LuaPushNamedNumber(L, "pulseSpeed",           v.pulseSpeed);
	LuaPushNamedNumber(L, "laserFlareSize",       v.laserflaresize);
	LuaPushNamedNumber(L, "thickness",            v.thickness);
	LuaPushNamedNumber(L, "coreThickness",        v.corethickness);
	LuaPushNamedNumber(L, "beamDecay",            v.beamdecay);
	LuaPushNamedNumber(L, "stages",               v.stages);
	LuaPushNamedNumber(L, "sizeDecay",            v.sizeDecay);
	LuaPushNamedNumber(L, "alphaDecay",           v.alphaDecay);
	LuaPushNamedNumber(L, "separation",           v.separation);
	LuaPushNamedBool  (L, "castShadow",           v.castShadow);
	LuaPushNamedBool  (L, "noGap",                v.noGap);
	LuaPushNamedBool  (L, "alwaysVisible",        v.alwaysVisible);
	LuaPushNamedBool  (L, "beamWeapon",           false); // DEPRECATED

	return 1;
}



static int NoEnemyCollide(lua_State* L, const void* data)
{
	lua_pushboolean(L, (*reinterpret_cast<const int*>(data)) & Collision::NOENEMIES);
	return 1;
}

static int NoFriendlyCollide(lua_State* L, const void* data)
{
	lua_pushboolean(L, (*reinterpret_cast<const int*>(data)) & Collision::NOFRIENDLIES);
	return 1;
}

static int NoFeatureCollide(lua_State* L, const void* data)
{
	lua_pushboolean(L, (*reinterpret_cast<const int*>(data)) & Collision::NOFEATURES);
	return 1;
}

static int NoNeutralCollide(lua_State* L, const void* data)
{
	lua_pushboolean(L, (*reinterpret_cast<const int*>(data)) & Collision::NONEUTRALS);
	return 1;
}

static int NoFireBaseCollide(lua_State* L, const void* data)
{
	lua_pushboolean(L, (*reinterpret_cast<const int*>(data)) & Collision::NOFIREBASES);
	return 1;
}

static int NoNonTargetCollide(lua_State* L, const void* data)
{
	lua_pushboolean(L, (*reinterpret_cast<const int*>(data)) & Collision::NONONTARGETS);
	return 1;
}

static int NoGroundCollide(lua_State* L, const void* data)
{
	lua_pushboolean(L, (*reinterpret_cast<const int*>(data)) & Collision::NOGROUND);
	return 1;
}


static int FramesToSeconds(lua_State* L, const void* data)
{
	const auto frames = *reinterpret_cast <const int *> (data);
	lua_pushnumber(L, frames * INV_GAME_SPEED);
	return 1;
}

static inline int BuildCategorySet(lua_State* L, const vector<string>& cats)
{
	lua_createtable(L, 0, cats.size());

	for (size_t i = 0, n = cats.size(); i < n; i++) {
		lua_pushsstring(L, cats[i]);
		lua_pushboolean(L, true);
		lua_rawset(L, -3);
	}

	return 1;
}


/*
static int CategorySetFromBits(lua_State* L, const void* data)
{
	const int bits = *((const int*)data);
	const vector<string> &cats =
		CCategoryHandler::Instance()->GetCategoryNames(bits);
	return BuildCategorySet(L, cats);
}
*/

/*static int CategorySetFromString(lua_State* L, const void* data)
{
	const string& str = *((const string*)data);
	const string lower = StringToLower(str);
	const vector<string> &cats = CSimpleParser::Tokenize(lower, 0);
	return BuildCategorySet(L, cats);
}*/


static int CustomParamsTable(lua_State* L, const void* data)
{
	const spring::unordered_map<std::string, std::string>& params = *((const spring::unordered_map<std::string, std::string>*)data);
	lua_createtable(L, 0, params.size());

	for (const auto& param: params) {
		lua_pushsstring(L, param.first);
		lua_pushsstring(L, param.second);
		lua_rawset(L, -3);
	}
	return 1;
}


static int GuiSoundSetTable(lua_State* L, const void* data)
{
	const GuiSoundSet& soundSet = *static_cast<const GuiSoundSet*>(data);

	lua_createtable(L, soundSet.NumSounds(), 0);

	for (size_t i = 0, n = soundSet.NumSounds(); i < n; i++) {
		lua_pushnumber(L, i + 1);
		lua_createtable(L, 0, 3);

		const GuiSoundSetData& sound = soundSet.GetSoundData(i);

		LuaPushNamedString(L, "name",   sound.name);
		LuaPushNamedNumber(L, "volume", sound.volume);

		if (!CLuaHandle::GetHandleSynced(L)) {
			LuaPushNamedNumber(L, "id", sound.id);
		}

		lua_rawset(L, -3);
	}

	return 1;
}



/******************************************************************************/
/******************************************************************************/

static bool InitParamMap()
{
	spring::clear_unordered_map(paramMap);

	paramMap["next"]  = DataElement(READONLY_TYPE);
	paramMap["pairs"] = DataElement(READONLY_TYPE);

	// dummy WeaponDef for offset generation
	const WeaponDef wd;
	const char* start = ADDRESS(wd);

	ADD_FUNCTION("damages",      wd.damages,   DamagesArray);
	ADD_FUNCTION("visuals",      wd.visuals,   VisualsTable);

	ADD_FUNCTION("hitSound",     wd.hitSound,  GuiSoundSetTable);
	ADD_FUNCTION("fireSound",    wd.fireSound, GuiSoundSetTable);

	ADD_FUNCTION("customParams",         wd.customParams,   CustomParamsTable);
	ADD_FUNCTION("noEnemyCollide",       wd.collisionFlags, NoEnemyCollide);
	ADD_FUNCTION("noFriendlyCollide",    wd.collisionFlags, NoFriendlyCollide);
	ADD_FUNCTION("noFeatureCollide",     wd.collisionFlags, NoFeatureCollide);
	ADD_FUNCTION("noNeutralCollide",     wd.collisionFlags, NoNeutralCollide);
	ADD_FUNCTION("noFireBaseCollide",    wd.collisionFlags, NoFireBaseCollide);
	ADD_FUNCTION("noNonTargetCollide",   wd.collisionFlags, NoNonTargetCollide);
	ADD_FUNCTION("noGroundCollide",      wd.collisionFlags, NoGroundCollide);

	ADD_DEPRECATED_LUADEF_KEY("areaOfEffect");
	ADD_DEPRECATED_LUADEF_KEY("maxVelocity");
	ADD_DEPRECATED_LUADEF_KEY("onlyTargetCategories");
	ADD_DEPRECATED_LUADEF_KEY("restTime");

	ADD_INT("id", wd.id);

	ADD_STRING("name",        wd.name);
	ADD_STRING("description", wd.description);

	// FIXME: why is this expgen-tag exposed but not the other two?
	ADD_STRING("cegTag", wd.visuals.ptrailExpGenTag);

	ADD_STRING("type", wd.type);

	ADD_FLOAT("range", wd.range);
	ADD_FLOAT("heightMod", wd.heightmod);
	ADD_FLOAT("accuracy", wd.accuracy);
	ADD_FLOAT("sprayAngle", wd.sprayAngle);
	ADD_FLOAT("movingAccuracy", wd.movingAccuracy);
	ADD_FLOAT("targetMoveError", wd.targetMoveError);
	ADD_FLOAT("leadLimit", wd.leadLimit);
	ADD_FLOAT("leadBonus", wd.leadBonus);
	ADD_FLOAT("predictBoost", wd.predictBoost);
	ADD_INT("highTrajectory", wd.highTrajectory);

	ADD_FLOAT("dynDamageExp", wd.damages.dynDamageExp);
	ADD_FLOAT("dynDamageMin", wd.damages.dynDamageMin);
	ADD_FLOAT("dynDamageRange", wd.damages.dynDamageRange);
	ADD_BOOL("dynDamageInverted", wd.damages.dynDamageInverted);

	ADD_BOOL("noSelfDamage",  wd.noSelfDamage);
	ADD_BOOL("impactOnly",    wd.impactOnly);

	ADD_FLOAT("craterAreaOfEffect", wd.damages.craterAreaOfEffect);
	ADD_FLOAT("damageAreaOfEffect", wd.damages.damageAreaOfEffect);
	ADD_FLOAT("edgeEffectiveness",  wd.damages.edgeEffectiveness);
	ADD_FLOAT("fireStarter",        wd.fireStarter);
	ADD_FLOAT("size",               wd.size);
	ADD_FLOAT("sizeGrowth",         wd.sizeGrowth);
	ADD_FLOAT("collisionSize",      wd.collisionSize);

	ADD_INT("salvoSize",    wd.salvosize);
	ADD_INT("projectiles",  wd.projectilespershot);
	ADD_FLOAT("salvoDelay", wd.salvodelay);
	ADD_FUNCTION("windup", wd.salvoWindup, FramesToSeconds);
	ADD_FLOAT("reload",     wd.reload);
	ADD_FLOAT("beamtime",   wd.beamtime);
	ADD_BOOL("beamburst",   wd.beamburst);

	ADD_BOOL("waterbounce",    wd.waterBounce);
	ADD_BOOL("groundbounce",   wd.groundBounce);
	ADD_FLOAT("groundslip",    wd.bounceSlip);
	ADD_FLOAT("bouncerebound", wd.bounceRebound);
	ADD_INT("numbounce",       wd.numBounce);

	ADD_FLOAT("maxAngle", wd.maxAngle);

	ADD_FLOAT("uptime", wd.uptime);

	ADD_FLOAT( "metalCost", wd.cost.metal);
	ADD_FLOAT("energyCost", wd.cost.energy);

	ADD_BOOL("turret", wd.turret);
	ADD_BOOL("onlyForward", wd.onlyForward);
	ADD_BOOL("waterWeapon", wd.waterweapon);
	ADD_BOOL("tracks", wd.tracks);
	ADD_BOOL("paralyzer", wd.paralyzer);

	ADD_BOOL("noAutoTarget",   wd.noAutoTarget);
	ADD_BOOL("manualFire",     wd.manualfire);
	ADD_INT("targetable",      wd.targetable);
	ADD_BOOL("stockpile",      wd.stockpile);
	ADD_INT("interceptor",     wd.interceptor);
	ADD_BOOL("interceptSolo",  wd.interceptSolo);
	ADD_FLOAT("coverageRange", wd.coverageRange);

	ADD_FLOAT("stockpileTime", wd.stockpileTime);

	ADD_FLOAT("intensity", wd.intensity);
	ADD_FLOAT("duration", wd.duration);
	ADD_INT("beamTTL", wd.beamLaserTTL);

	ADD_BOOL("soundTrigger", wd.soundTrigger);

	ADD_BOOL("selfExplode", wd.selfExplode);
	ADD_BOOL("gravityAffected", wd.gravityAffected);
	ADD_FLOAT("myGravity", wd.myGravity);
	ADD_BOOL("noExplode", wd.noExplode);
	ADD_FLOAT("startvelocity", wd.startvelocity);
	ADD_FLOAT("weaponAcceleration", wd.weaponacceleration);
	ADD_FLOAT("turnRate", wd.turnrate);

	ADD_FLOAT("projectilespeed", wd.projectilespeed);
	ADD_FLOAT("explosionSpeed", wd.damages.explosionSpeed);

	ADD_FLOAT("wobble", wd.wobble);
	ADD_FLOAT("dance",  wd.dance);

	ADD_FLOAT("trajectoryHeight", wd.trajectoryHeight);
	ADD_INT("flightTime", wd.flighttime);

	ADD_BOOL("largeBeamLaser", wd.largeBeamLaser);
	ADD_BOOL("laserHardStop", wd.laserHardStop);

	ADD_BOOL("isShield",                wd.isShield);
	ADD_BOOL("shieldRepulser",          wd.shieldRepulser);
	ADD_BOOL("smartShield",             wd.smartShield);
	ADD_BOOL("exteriorShield",          wd.exteriorShield);
	ADD_BOOL("visibleShield",           wd.visibleShield);
	ADD_BOOL("visibleShieldRepulse",    wd.visibleShieldRepulse);
	ADD_INT( "visibleShieldHitFrames",  wd.visibleShieldHitFrames);
	ADD_FLOAT("shieldEnergyUse",        wd.shieldResourceUse.energy);
	ADD_FLOAT("shieldRadius",           wd.shieldRadius);
	ADD_FLOAT("shieldForce",            wd.shieldForce);
	ADD_FLOAT("shieldMaxSpeed",         wd.shieldMaxSpeed);
	ADD_FLOAT("shieldPower",            wd.shieldPower);
	ADD_FLOAT("shieldPowerRegen",       wd.shieldPowerRegen);
	ADD_FLOAT("shieldPowerRegenEnergy", wd.shieldPowerRegenCost.energy);
	ADD_INT(  "shieldRechargeDelay",    wd.shieldRechargeDelay);
	ADD_FLOAT("shieldGoodColorR",       wd.shieldGoodColor.x);
	ADD_FLOAT("shieldGoodColorG",       wd.shieldGoodColor.y);
	ADD_FLOAT("shieldGoodColorB",       wd.shieldGoodColor.z);
	ADD_FLOAT("shieldGoodColorA",       wd.shieldGoodColor.w);
	ADD_FLOAT("shieldBadColorR",        wd.shieldBadColor.x);
	ADD_FLOAT("shieldBadColorG",        wd.shieldBadColor.y);
	ADD_FLOAT("shieldBadColorB",        wd.shieldBadColor.z);
	ADD_FLOAT("shieldBadColorA",        wd.shieldBadColor.w);
	ADD_FLOAT("shieldAlpha",            wd.shieldAlpha);

	ADD_INT("shieldInterceptType",      wd.shieldInterceptType);
	ADD_INT("interceptedByShieldType",  wd.interceptedByShieldType);

	ADD_BOOL("avoidFriendly", wd.avoidFriendly);
	ADD_BOOL("avoidFeature",  wd.avoidFeature);
	ADD_BOOL("avoidNeutral",  wd.avoidNeutral);

	ADD_FLOAT("targetBorder",       wd.targetBorder);
	ADD_FLOAT("cylinderTargeting", wd.cylinderTargeting);
	ADD_FLOAT("cylinderTargetting", wd.cylinderTargeting); // FIXME deprecated misspelling
	ADD_FLOAT("minIntensity",       wd.minIntensity);
	ADD_FLOAT("heightBoostFactor",  wd.heightBoostFactor);
	ADD_FLOAT("proximityPriority",  wd.proximityPriority);

	ADD_BOOL("sweepFire", wd.sweepFire);

	ADD_BOOL("canAttackGround", wd.canAttackGround);

	return true;
}


/******************************************************************************/
/******************************************************************************/
