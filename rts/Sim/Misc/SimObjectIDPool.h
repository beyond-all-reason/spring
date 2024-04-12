/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "System/creg/creg_cond.h"
#include "System/UnorderedMap.hpp"
#include "Sim/Objects/WorldObject.h"

class CSolidObject;
class SimObjectIDPool {
	CR_DECLARE_STRUCT(SimObjectIDPool)

public:
	SimObjectIDPool() {} // FIXME: creg, needs PostLoad
	SimObjectIDPool(uint32_t maxObjects) {
		// pools are reused as part of object handlers, internal table sizes must be
		// constant at runtime to prevent desyncs between fresh and reloaded clients
		// (both must execute Expand since it touches the RNG)
		poolIDs.reserve(maxObjects);
		freeIDs.reserve(maxObjects);
		tempIDs.reserve(maxObjects);
	}

	void Expand(uint32_t baseID, uint32_t numIDs);
	void Clear() {
		freeIDs.clear();
		poolIDs.clear();
		tempIDs.clear();
	}

	void AssignID(CWorldObject* object);
	void FreeID(uint32_t uid, bool delayed);

	bool RecycleID(uint32_t uid);
	bool HasID(uint32_t uid) const;
	bool IsEmpty() const { return (freeIDs.empty()); }

	uint32_t GetSize() const { return (freeIDs.size()); } // number of ID's still unused
	uint32_t MaxSize() const { return (poolIDs.size()); } // number of ID's this pool owns

private:
	uint32_t ExtractID();

	void ReserveID(uint32_t uid);
	void RecycleIDs();

private:
	spring::unordered_map<uint32_t, uint32_t> poolIDs; // uid to idx
	spring::unordered_map<uint32_t, uint32_t> freeIDs; // idx to uid
	spring::unordered_map<uint32_t, uint32_t> tempIDs; // idx to uid
};