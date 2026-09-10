/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <cstdint>
#include <vector>

#include "System/UnorderedMap.hpp"

class CBuilder;
class CFactory;
class CUnit;
struct float3;

namespace NanoParticles {

/*
 * Decides how much spray a builder produces, and keeps the bookkeeping that
 * needs for itself.
 *
 * Legacy nano spray emits exactly one particle per work tick, so a builder's
 * visible output tracks its nano piece count rather than the work it is
 * actually doing: a four-armed shipyard with modest buildpower outsprays a
 * high-power constructor doing the same job. The effect instead emits
 * proportionally to (buildSpeed * buildPower), which means a variable number of
 * particles per tick and a fractional accumulator to carry the remainder.
 *
 * That accumulator, and the reclaim contributor tracking, live here rather than
 * on CBuilder/CUnit: they are unsynced presentation state, they must not be
 * serialised, and none of the sim needs to know they exist.
 *
 * Nothing here perturbs the simulation. The synced side of a work tick - the
 * QueryNanoPiece script poll and its synced RNG draw - happens exactly once per
 * tick either way, and every particle the effect adds is spawned from the
 * unsynced RNG, as legacy nano spray already was.
 */
class Emitter {
public:
	void Init();
	void Kill();

	/// Builder emission for one work tick. Mirrors CBuilder::CreateNanoParticle's contract.
	void EmitBuilderSpray(CBuilder* builder, const float3& goal, float radius, bool inverse, bool highPriority, const CUnit* targetUnit, bool fadeWhenTargetComplete);

	/// Factory emission for one work tick. Mirrors CFactory::CreateNanoParticle's contract.
	void EmitFactorySpray(CFactory* factory, bool highPriority);

	/// Notes that `builder` poured reclaim into `reclaimee` this frame.
	void RecordReclaimContributor(const CUnit* reclaimee, const CUnit* builder);

	/// Fires the completion burst from every builder that contributed to the reclaim.
	void EmitReclaimBurst(const CUnit* reclaimee, CUnit* finishingBuilder, float reclaimedMetal);

private:
	struct EmitterState {
		float accumulator = 0.0f;
		int lastEmitFrame = 0;
		int lastSeenFrame = 0;
	};

	struct Contributor {
		int unitID = -1;
		std::int64_t syncID = -1;
		int lastFrame = -1;
	};

	struct ReclaimTracker {
		std::vector<Contributor> contributors;
		int lastFrame = -1;
	};

	int TakeEmitCount(int unitID, float builderBuildSpeed, float buildPower);
	int GetBurstParticleCount(float reclaimedMetal, int contributorCount) const;
	void EmitBuilderBurst(CBuilder* builder, const CUnit* reclaimee, int burstCount);
	void PruneEmitterStates(int frame);

	spring::unordered_map<int, EmitterState> emitterStates;
	spring::unordered_map<int, ReclaimTracker> reclaimTrackers;

	/// Emitter-state sweeps are amortised rather than run every frame.
	int nextPruneFrame = 0;
};

extern Emitter emitter;

} // namespace NanoParticles
