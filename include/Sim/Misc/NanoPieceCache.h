/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _NANO_PIECE_CACHE_H
#define _NANO_PIECE_CACHE_H

#include <bit>
#include <limits>
#include <vector>

#include "Sim/Misc/GlobalConstants.h"
#include "System/creg/creg_cond.h"

class CUnitScript;

struct NanoPieceCache
{
	CR_DECLARE_STRUCT(NanoPieceCache)

public:
	NanoPieceCache(): lastNanoPieceCnt(0), curBuildPowerMask(0) {
	}

	void Update() { curBuildPowerMask >>= 1; }

	float GetBuildPower() const { return std::popcount(curBuildPowerMask) / static_cast <float> (MASK_BITS); }

	/// returns modelPiece (NOT scriptModelPiece)
	int GetNanoPiece(CUnitScript* ownerScript);

	const std::vector<int>& GetNanoPieces() const { return nanoPieces; }
	      std::vector<int>& GetNanoPieces()       { return nanoPieces; }

	void StopPolling() { lastNanoPieceCnt = MAX_QUERYNANOPIECE_CALLS * 2; }

private:
	// model-piece indices
	std::vector<int> nanoPieces;

	static const int MAX_QUERYNANOPIECE_CALLS = 30;

	int lastNanoPieceCnt;
	uint32_t curBuildPowerMask;

	/* Mask length controls for how long each frame of pouring buildpower "saturates" the counter. In this case it takes
	 * half a second of building to reach 100% buildpower after starting building, and to reach 0% after stopping.
	 * The min() call is just for technical reasons. */
	static constexpr int MASK_BITS = std::min(GAME_SPEED / 2, std::numeric_limits<decltype(curBuildPowerMask)>::digits);
};

#endif

