/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SimObjectIDPool.h"
#include "GlobalConstants.h"
#include "GlobalSynced.h"
#include "Sim/Objects/SolidObject.h"
#include "System/Cpp11Compat.hpp"
#include "System/creg/STL_Map.h"

#include <tracy/Tracy.hpp>


CR_BIND(SimObjectIDPool, )
CR_REG_METADATA(SimObjectIDPool, (
	CR_MEMBER(poolIDs),
	CR_MEMBER(freeIDs),
	CR_MEMBER(tempIDs)
))


void SimObjectIDPool::Expand(uint32_t baseID, uint32_t numIDs) {
	//ZoneScoped;
	std::vector<uint32_t> newIDs(numIDs);

	// allocate new batch of (randomly shuffled) id's
	std::iota(newIDs.begin(), newIDs.end(), baseID);

	// randomize so that Lua widgets can not easily determine counts
	spring::random_shuffle(newIDs.begin(), newIDs.end(), gsRNG);
	spring::random_shuffle(newIDs.begin(), newIDs.end(), gsRNG);

	// NOTE:
	//   any randomization would be undone by a sorted std::container
	//   instead create a bi-directional mapping from indices to ID's
	//   (where the ID's are a random permutation of the index range)
	//   such that ID's can be assigned and returned to the pool with
	//   their original index, e.g.
	//
	//     freeIDs<idx, uid> = {<0, 13>, < 1, 27>, < 2, 54>, < 3, 1>, ...}
	//     poolIDs<uid, idx> = {<1,  3>, <13,  0>, <27,  1>, <54, 2>, ...}
	//
	//   (the ID --> index map is never changed at runtime!)
	for (uint32_t offsetID = 0; offsetID < numIDs; offsetID++) {
		freeIDs.emplace(baseID + offsetID, newIDs[offsetID]);
		poolIDs.emplace(newIDs[offsetID], baseID + offsetID);
	}
}



void SimObjectIDPool::AssignID(CSolidObject* object) {
	//ZoneScoped;
	if (object->id < 0) {
		object->id = ExtractID();
	} else {
		ReserveID(object->id);
	}
}

uint32_t SimObjectIDPool::ExtractID() {
	//ZoneScoped;
	// extract a random ID from the pool
	//
	// should be unreachable, UnitHandler
	// and FeatureHandler have safeguards
	assert(!IsEmpty());

	const auto it = freeIDs.begin();
	const uint32_t uid = it->second;

	freeIDs.erase(it);

	if (IsEmpty())
		RecycleIDs();

	return uid;
}

void SimObjectIDPool::ReserveID(uint32_t uid) {
	//ZoneScoped;
	// reserve a chosen ID from the pool
	assert(HasID(uid));
	assert(!IsEmpty());

	const auto it = poolIDs.find(uid);
	const uint32_t idx = it->second;

	freeIDs.erase(idx);

	if (!IsEmpty())
		return;

	RecycleIDs();
}

void SimObjectIDPool::FreeID(uint32_t uid, bool delayed) {
	//ZoneScoped;
	// put an ID back into the pool either immediately
	// or after all remaining free ID's run out (which
	// is better iff the object count never gets close
	// to the maximum)
	assert(!HasID(uid));

	if (delayed) {
		tempIDs.emplace(poolIDs[uid], uid);
	} else {
		freeIDs.emplace(poolIDs[uid], uid);
	}

	//handle the corner case of maximum allocation
	if (IsEmpty())
		RecycleIDs();
}

bool SimObjectIDPool::RecycleID(uint32_t uid) {
	//ZoneScoped;
	assert(poolIDs.find(uid) != poolIDs.end());

	const uint32_t idx = poolIDs[uid];
	const auto it = tempIDs.find(idx);

	if (it == tempIDs.end())
		return false;

	tempIDs.erase(idx);
	freeIDs.emplace(idx, uid);
	return true;
}

void SimObjectIDPool::RecycleIDs() {
	//ZoneScoped;
	// throw each ID recycled up until now back into the pool
	freeIDs.insert(tempIDs.begin(), tempIDs.end());
	tempIDs.clear();
}


bool SimObjectIDPool::HasID(uint32_t uid) const {
	//ZoneScoped;
	assert(poolIDs.find(uid) != poolIDs.end());

	// check if given ID is available (to be assigned) in this pool
	const auto it = poolIDs.find(uid);
	const uint32_t idx = it->second;

	return (freeIDs.find(idx) != freeIDs.end());
}

