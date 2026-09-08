/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef COB_THREAD_ID_H
#define COB_THREAD_ID_H

#include <compare>
#include <cstdint>
#include <ostream>

#include "System/creg/creg_cond.h"

#include <cstddef>

// Opaque handle for a thread inside CCobEngine's slot pool. The payload
// packs a generation and a slot index.
class CobThreadID
{
	CR_DECLARE_STRUCT(CobThreadID)

public:
	// We're using 13 generation bits / 18 slot bits so that:
	// * at 32k MAX_UNITS, every unit could have 8 cob scripts
	// * there's room for 8,192 generations
	// both of which are WELL beyond the supported use case of the engine.
	static constexpr uint32_t GEN_BITS  = 13;
	static constexpr uint32_t SLOT_BITS = 18;
	static constexpr uint32_t GEN_MAX   = (1u << GEN_BITS)  - 1;
	static constexpr uint32_t SLOT_MAX  = (1u << SLOT_BITS) - 1;
	static_assert(GEN_BITS + SLOT_BITS <= 31, "thread id must fit in non-negative int");

	constexpr CobThreadID() = default;
	constexpr explicit CobThreadID(int rawValue) : raw(rawValue) {}

	static constexpr CobThreadID Invalid() { return CobThreadID(-1); }
	static CobThreadID Pack(uint32_t generation, size_t slotIndex);

	constexpr bool IsValid() const { return raw >= 0; }
	constexpr int  Raw()     const { return raw; }

	// inlining because unpack gets called at least once per cob thread per sim frame
	void Unpack(uint32_t& generation, size_t& slotIndex) const {
		const uint32_t bits = static_cast<uint32_t>(raw);
		generation = (bits >> SLOT_BITS) & GEN_MAX;
		slotIndex  = bits & SLOT_MAX;
	}

	friend constexpr bool operator==(CobThreadID, CobThreadID) = default;
	friend constexpr auto operator<=>(CobThreadID, CobThreadID) = default;

	friend std::ostream& operator<<(std::ostream& os, CobThreadID id) {
		return os << id.raw;
	}

private:
	int32_t raw = -1;
};

#endif // COB_THREAD_ID_H
