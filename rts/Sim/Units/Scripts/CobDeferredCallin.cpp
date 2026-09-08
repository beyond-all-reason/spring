/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */


#include "CobDeferredCallin.h"

#include "CobEngine.h"

#include "Lua/LuaHashString.h"
#include "Lua/LuaUI.h"

#include "System/Misc/TracyDefs.h"


CCobDeferredCallin::CCobDeferredCallin(const CUnit* unit, const LuaHashString& hs, const std::vector<int>& dataStack, const int stackStart)
	: unit(unit), argCount(std::min(stackStart, MAX_LUA_COB_ARGS)), funcName(hs.GetString()), funcHash(hs.GetHash())
{
	const int size = static_cast<int>(dataStack.size());

	const int start = std::max(0, size - stackStart);
	const int end = std::min(size, start + argCount);

	for (int a = 0, i = start; i < end; i++) {
		luaArgs[a++] = dataStack[i];
	}
}

CCobDeferredCallin& CCobDeferredCallin::operator = (CCobDeferredCallin&& t) {
	unit = t.unit;
	argCount = t.argCount;
	funcName = t.funcName;
	funcHash = t.funcHash;
	std::memcpy(luaArgs, t.luaArgs, sizeof(int)*argCount);
	return *this;
}


CCobDeferredCallin& CCobDeferredCallin::operator = (const CCobDeferredCallin& t) {
	unit = t.unit;
	argCount = t.argCount;
	funcName = t.funcName;
	funcHash = t.funcHash;
	std::memcpy(luaArgs, t.luaArgs, sizeof(int)*argCount);
	return *this;
}


void CCobDeferredCallin::Call()
{
	// unused since batching it atm
	luaRules->unsyncedLuaHandle.Cob2Lua(LuaHashString(funcName.c_str()), unit, argCount, luaArgs);
	if (luaUI)
		luaUI->Cob2Lua(LuaHashString(funcName.c_str()), unit, argCount, luaArgs);
}
