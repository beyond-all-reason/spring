/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#pragma once

#include <vector>
#include <array>

#include "Sim/Units/BuildInfo.h"
#include "Sim/Units/UnitDef.h"
#include "System/float3.h"

struct LuaBuildSquareOptions {
	bool unbuildable = false;
};

struct LuaBuildSquare {
	float3 pos;
	uint8_t state; // 0:buildable, 1:feature, 2:illegal, 3:unbuildable
};

struct LuaBuildSquareTask {
	BuildInfo buildInfo;
	int cacheUntil; // -1 if squares are not built yet, 0 if displayed
	std::vector<LuaBuildSquare> squares;
	LuaBuildSquareOptions opts;
	inline LuaBuildSquareTask(const BuildInfo& buildInfo, const LuaBuildSquareOptions& opts): buildInfo(buildInfo), opts(opts), cacheUntil(-1) {}
};

struct LuaBuildSquareTaskKey {
	int unitDefId;
	float x, z; // y is skipped on purpose, not relevent for the key
	int facing;

  inline LuaBuildSquareTaskKey(const BuildInfo& buildInfo): 
    unitDefId(buildInfo.def->id), 
    x(buildInfo.pos.x), 
    z(buildInfo.pos.z), 
    facing(buildInfo.buildFacing) {}
};

struct LuaBuildSquareTaskHash {
	std::size_t operator()(const LuaBuildSquareTaskKey& buildKey) const;
	bool operator()(const LuaBuildSquareTaskKey& buildKey1, const LuaBuildSquareTaskKey& buildKey2) const;
};

using LuaBuildSquareTasksMap = std::unordered_map<LuaBuildSquareTaskKey, LuaBuildSquareTask, LuaBuildSquareTaskHash, LuaBuildSquareTaskHash>;
