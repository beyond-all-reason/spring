/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "UnitDrawerCache.h"

#include "System/HashSpec.h"
#include "System/SpringHash.h"

std::size_t LuaBuildSquareTaskHash::operator()(const LuaBuildSquareTaskKey& buildKey) const {
	std::size_t hashKey = spring::LiteHash(buildKey.unitDefId);
	hashKey = spring::hash_combine(spring::LiteHash(buildKey.x), hashKey);
	hashKey = spring::hash_combine(spring::LiteHash(buildKey.z), hashKey);
	hashKey = spring::hash_combine(spring::LiteHash(buildKey.facing), hashKey);
	return hashKey;
}

bool LuaBuildSquareTaskHash::operator()(const LuaBuildSquareTaskKey& buildKey1, const LuaBuildSquareTaskKey& buildKey2) const {
	return buildKey1.unitDefId == buildKey2.unitDefId &&
		buildKey1.x == buildKey2.x &&
		buildKey1.z == buildKey2.z &&
		buildKey1.facing == buildKey2.facing;
}
