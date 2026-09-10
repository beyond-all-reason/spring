/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "NanoParticleEmitter.h"

#include <algorithm>
#include <cmath>

#include "NanoParticleConfig.h"
#include "NanoParticleDefs.h"

#include "Game/GlobalUnsynced.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitTypes/Builder.h"
#include "Sim/Units/UnitTypes/Factory.h"
#include "System/SpringMath.h"

#include "System/Misc/TracyDefs.h"

namespace NanoParticles {

Emitter emitter;

namespace {
	/// How often the idle-emitter sweep runs, in frames.
	constexpr int PRUNE_INTERVAL_FRAMES = 150;
	/// Burst particles closer to the builder than this are skipped; the direction is meaningless.
	constexpr float BURST_MIN_LENGTH = 1.0f;

	/*
	 * Walks `emitCount` of `unit`'s nano pieces, starting from the one the script
	 * just handed us, and calls `emit(modelNanoPiece, worldPos)` for each piece
	 * that still exists on the model.
	 *
	 * Spreading a tick's emissions over the pieces is what keeps a multi-armed
	 * builder from firing its whole allowance out of a single arm; starting from
	 * the script's pick keeps the leading arm as random as it was before.
	 */
	template<typename EmitFn>
	void ForEachEmission(const CUnit* unit, const std::vector<int>& nanoPieces, int firstNanoPiece, int emitCount, EmitFn&& emit)
	{
		const auto firstIt = std::find(nanoPieces.begin(), nanoPieces.end(), firstNanoPiece);
		const std::size_t firstIndex = (firstIt != nanoPieces.end()) ? std::distance(nanoPieces.begin(), firstIt) : 0;

		for (int i = 0; i < emitCount; ++i) {
			const int modelNanoPiece = nanoPieces.empty()
				? firstNanoPiece
				: nanoPieces[(firstIndex + i) % nanoPieces.size()];

			if (!unit->localModel.HasPiece(modelNanoPiece))
				continue;

			emit(modelNanoPiece, unit->GetObjectSpacePos(unit->localModel.GetRawPiecePos(modelNanoPiece)));
		}
	}
} // namespace


void Emitter::Init()
{
	emitterStates.clear();
	reclaimTrackers.clear();
	nextPruneFrame = 0;
}

void Emitter::Kill()
{
	emitterStates.clear();
	reclaimTrackers.clear();
	nextPruneFrame = 0;
}


int Emitter::TakeEmitCount(int unitID, float builderBuildSpeed, float buildPower)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const Config& cfg = GetConfig();
	const EmissionConfig& ec = cfg.emission;
	const int frame = gs->frameNum;

	EmitterState& state = emitterStates[unitID];
	state.lastSeenFrame = frame;

	if (projectileHandler.maxNanoParticles <= 0 || cfg.rate <= 0.0f) {
		state.accumulator = 0.0f;
		return 0;
	}

	const float rate = std::max(0.0f, builderBuildSpeed)
		* std::clamp(buildPower, 0.0f, 1.0f)
		* (cfg.rate / ec.referenceBuildSpeed);

	const float accumulated = state.accumulator + rate;
	int emitCount = static_cast<int>(std::floor(accumulated));
	state.accumulator = accumulated - emitCount;

	/* A builder working at a tiny fraction of its buildpower can go a long time
	 * without the accumulator tipping over. Force a particle out occasionally so
	 * the player can still tell the job is progressing; the forced emit is
	 * debited, so the long-run rate stays proportional. */
	const int feedbackGap = std::max(1, static_cast<int>(std::ceil(ec.feedbackEmitMinGap * ec.feedbackEmitReferenceRate / cfg.rate)));
	if (emitCount == 0 && buildPower > 0.0f && (frame - state.lastEmitFrame) >= feedbackGap) {
		emitCount = 1;
		state.accumulator = 0.0f;
	}

	if (emitCount > 0)
		state.lastEmitFrame = frame;

	return emitCount;
}

void Emitter::PruneEmitterStates(int frame)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (frame < nextPruneFrame)
		return;

	nextPruneFrame = frame + PRUNE_INTERVAL_FRAMES;

	const int maxIdleFrames = GetConfig().emission.emitterStateMaxIdleFrames;
	const int contributorMaxAge = GetConfig().reclaimBurstParams.contributorMaxAge;

	for (auto it = emitterStates.begin(); it != emitterStates.end();) {
		if (frame - it->second.lastSeenFrame > maxIdleFrames)
			it = emitterStates.erase(it);
		else
			++it;
	}

	for (auto it = reclaimTrackers.begin(); it != reclaimTrackers.end();) {
		if (frame - it->second.lastFrame > contributorMaxAge)
			it = reclaimTrackers.erase(it);
		else
			++it;
	}
}


void Emitter::EmitBuilderSpray(CBuilder* builder, const float3& goal, float radius, bool inverse, bool highPriority, const CUnit* targetUnit, bool fadeWhenTargetComplete)
{
	RECOIL_DETAILED_TRACY_ZONE;
	NanoPieceCache& nanoPieceCache = builder->GetNanoPieceCache();

	/* Poll the script exactly once, as the legacy path does: this is the only
	 * part of a work tick the simulation can observe. */
	const int firstNanoPiece = nanoPieceCache.GetNanoPiece(builder->script);

	if (!builder->localModel.Initialized() || !builder->localModel.HasPiece(firstNanoPiece))
		return;

	PruneEmitterStates(gs->frameNum);

	const int emitCount = TakeEmitCount(builder->id, builder->unitDef->buildSpeed, nanoPieceCache.GetBuildPower());
	if (emitCount <= 0)
		return;

	ForEachEmission(builder, nanoPieceCache.GetNanoPieces(), firstNanoPiece, emitCount,
		[&](int modelNanoPiece, const float3& nanoPos) {
			/* Outbound particles are bound to the target unit; inbound (reclaim)
			 * ones to the builder's own nano piece, since that is what they are
			 * converging on. Losing either fades the spray out. */
			const SpawnParams spawnParams = {
				inverse ? static_cast<const CUnit*>(builder) : targetUnit,
				inverse ? modelNanoPiece : -1,
				inverse,
				!inverse && fadeWhenTargetComplete,
			};

			projectileHandler.AddNanoParticle(nanoPos, goal, builder->unitDef, builder->team, radius, inverse, highPriority, spawnParams);
		}
	);
}

void Emitter::EmitFactorySpray(CFactory* factory, bool highPriority)
{
	RECOIL_DETAILED_TRACY_ZONE;
	NanoPieceCache& nanoPieceCache = factory->GetNanoPieceCache();
	const int firstNanoPiece = nanoPieceCache.GetNanoPiece(factory->script);

	if (factory->curBuild == nullptr || !factory->localModel.Initialized() || !factory->localModel.HasPiece(firstNanoPiece))
		return;

	PruneEmitterStates(gs->frameNum);

	const int emitCount = TakeEmitCount(factory->id, factory->unitDef->buildSpeed, nanoPieceCache.GetBuildPower());
	if (emitCount <= 0)
		return;

	ForEachEmission(factory, nanoPieceCache.GetNanoPieces(), firstNanoPiece, emitCount,
		[&](int /*modelNanoPiece*/, const float3& nanoPos) {
			projectileHandler.AddNanoParticle(nanoPos, factory->curBuild->midPos, factory->unitDef, factory->team, highPriority);
		}
	);
}


void Emitter::RecordReclaimContributor(const CUnit* reclaimee, const CUnit* builder)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const int frame = gs->frameNum;
	const int maxAge = GetConfig().reclaimBurstParams.contributorMaxAge;

	ReclaimTracker& tracker = reclaimTrackers[reclaimee->id];
	tracker.lastFrame = frame;

	const int oldestFrame = frame - maxAge;
	for (std::size_t i = 0; i < tracker.contributors.size();) {
		Contributor& contributor = tracker.contributors[i];

		if (contributor.lastFrame < oldestFrame) {
			contributor = tracker.contributors.back();
			tracker.contributors.pop_back();
			continue;
		}

		if (contributor.unitID == builder->id && contributor.syncID == builder->GetSyncID()) {
			contributor.lastFrame = frame;
			return;
		}

		++i;
	}

	tracker.contributors.push_back({builder->id, builder->GetSyncID(), frame});
}

int Emitter::GetBurstParticleCount(float reclaimedMetal, int contributorCount) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	const ReclaimBurstConfig& rb = GetConfig().reclaimBurstParams;

	/* Logarithmic in cost so a cheap unit still puffs and an expensive one does
	 * not swamp the particle budget, and sub-linear in reclaimer count so a
	 * coordinated swarm reads as bigger without scaling straight up. */
	const float scaledMetal = std::max(0.0f, reclaimedMetal);
	const int perBuilder = rb.base + static_cast<int>(std::floor(rb.logK * std::log(1.0f + scaledMetal / rb.logNorm) + 0.5f));
	const float contributorScale = std::pow(static_cast<float>(std::max(1, contributorCount)), rb.builderExponent);

	return std::clamp(static_cast<int>(std::floor(perBuilder * contributorScale + 0.5f)), 1, rb.maxParticles);
}

void Emitter::EmitReclaimBurst(const CUnit* reclaimee, CUnit* finishingBuilder, float reclaimedMetal)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto* reclaimBuilder = dynamic_cast<CBuilder*>(finishingBuilder);
	if (reclaimee == nullptr || reclaimBuilder == nullptr)
		return;

	const int frame = gs->frameNum;
	const int maxAge = GetConfig().reclaimBurstParams.contributorMaxAge;

	std::vector<CBuilder*> contributors;

	if (const auto trackerIt = reclaimTrackers.find(reclaimee->id); trackerIt != reclaimTrackers.end()) {
		for (const Contributor& contributor : trackerIt->second.contributors) {
			if (contributor.lastFrame < frame - maxAge)
				continue;

			CUnit* unit = unitHandler.GetUnit(contributor.unitID);
			if (unit == nullptr || unit->GetSyncID() != contributor.syncID || unit->isDead || unit->team != reclaimBuilder->team)
				continue;

			if (auto* contributorBuilder = dynamic_cast<CBuilder*>(unit); contributorBuilder != nullptr)
				contributors.push_back(contributorBuilder);
		}

		reclaimTrackers.erase(trackerIt);
	}

	if (contributors.empty())
		contributors.push_back(reclaimBuilder);

	const int burstCount = GetBurstParticleCount(reclaimedMetal, static_cast<int>(contributors.size()));
	const int baseCount = burstCount / static_cast<int>(contributors.size());
	const int remainder = burstCount - baseCount * static_cast<int>(contributors.size());

	for (std::size_t i = 0; i < contributors.size(); ++i)
		EmitBuilderBurst(contributors[i], reclaimee, baseCount + (static_cast<int>(i) < remainder));
}

void Emitter::EmitBuilderBurst(CBuilder* builder, const CUnit* reclaimee, int burstCount)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (burstCount <= 0 || !builder->localModel.Initialized())
		return;

	const ReclaimBurstConfig& rb = GetConfig().reclaimBurstParams;
	const std::vector<int>& nanoPieces = builder->GetNanoPieceCache().GetNanoPieces();
	if (nanoPieces.empty())
		return;

	/* Spawn inside the reclaimee's collision volume rather than at its midpos,
	 * so the burst reads as the whole unit coming apart. */
	const float3& collisionScales = reclaimee->collisionVolume.GetScales();
	const float smallestCollisionScale = std::min(collisionScales.x, std::min(collisionScales.y, collisionScales.z));
	const float burstRadius = smallestCollisionScale * 0.5f * rb.volumeFraction;
	if (burstRadius <= 0.0f)
		return;

	const float3 burstCenter = reclaimee->midPos + reclaimee->collisionVolume.GetOffsets();
	const unsigned firstPiece = guRNG.NextInt(nanoPieces.size());

	for (int i = 0; i < burstCount; ++i) {
		const int modelNanoPiece = nanoPieces[(firstPiece + i) % nanoPieces.size()];
		if (!builder->localModel.HasPiece(modelNanoPiece))
			continue;

		const float3 nanoPos = builder->GetObjectSpacePos(builder->localModel.GetRawPiecePos(modelNanoPiece));
		const float3 burstPos = burstCenter + guRNG.NextVector() * burstRadius;
		const float burstLength = fastmath::apxsqrt2((burstPos - nanoPos).SqLength());

		if (burstLength < BURST_MIN_LENGTH)
			continue;

		const SpawnParams spawnParams = {builder, modelNanoPiece, true, false};

		projectileHandler.AddNanoParticle(
			nanoPos,
			burstPos,
			builder->unitDef,
			builder->team,
			burstLength * rb.directionJitter,
			true,
			true,
			spawnParams
		);
	}
}

} // namespace NanoParticles
