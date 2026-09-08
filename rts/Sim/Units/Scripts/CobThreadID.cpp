/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CobThreadID.h"

#include <cassert>

#include "System/BranchPrediction.h"
#include "System/Log/ILog.h"

CR_BIND(CobThreadID, )
CR_REG_METADATA(CobThreadID, (
	CR_MEMBER(raw)
))

CobThreadID CobThreadID::Pack(const uint32_t generation, const size_t slotIndex) {
	assert(slotIndex <= SLOT_MAX);
	assert(generation <= GEN_MAX);
	if unlikely (slotIndex > SLOT_MAX || generation > GEN_MAX) {
		LOG_L(L_ERROR, "[CobThreadID::%s] cannot pack id (generation=%u, slotIndex=%zu) — exceeds GEN_MAX=%u or SLOT_MAX=%u",
			__func__, generation, slotIndex, GEN_MAX, SLOT_MAX);
		return CobThreadID::Invalid();
	}
	return CobThreadID(static_cast<int>((generation << SLOT_BITS) | slotIndex));
}
