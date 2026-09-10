/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "NanoParticleSystem.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

#include "NanoParticleConfig.h"
#include "NanoParticleEmitter.h"
#include "NanoParticleRenderer.h"

#include "Game/GlobalUnsynced.h"
#include "System/Config/ConfigHandler.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "System/EventHandler.h"

#include "System/Misc/TracyDefs.h"

namespace NanoParticles {

System system;

namespace {
	/*
	 * Homing particles chase a target that may be a fast-moving unit. Without a
	 * ceiling on the correction they would visibly slingshot; these multiply the
	 * spawn speed to give the cap.
	 */
	constexpr float HOMING_SPEED_LIMIT_MULT_AIR = 1.35f;
	constexpr float HOMING_SPEED_LIMIT_MULT_GROUND = 2.0f;
	/// A target has to move at least this far (elmos) before a re-aim is worth it.
	constexpr float HOMING_MIN_TARGET_MOVE_SQ = 1.0f;
	/// Ordinary spray may spend this much of the budget; bursts may spend all of it.
	constexpr float NORMAL_SPRAY_BUDGET_FRACTION = 0.95f;
	/// The clamped route's peak is kept away from the endpoints so both legs stay sane.
	constexpr float GROUND_CLAMP_PEAK_MIN = 0.15f;
	constexpr float GROUND_CLAMP_PEAK_MAX = 0.85f;

	// Hash multipliers for the spatial caches below. Arbitrary large primes.
	constexpr std::uint32_t HASH_X = 73856093u;
	constexpr std::uint32_t HASH_Y = 19349663u;
	constexpr std::uint32_t HASH_Z = 83492791u;
	constexpr std::uint32_t HASH_W = 2654435761u;

	int Quantize(float value, float cellSize)
	{
		return static_cast<int>(std::floor(value / cellSize + 0.5f));
	}

	/*
	 * Ground height, plus the clamp margin, cached for one frame. Route
	 * evaluation samples the same few columns repeatedly, so even a
	 * single-frame cache removes most of the heightmap reads.
	 */
	struct GroundHeightCache {
		std::vector<std::uint64_t> keys;
		std::vector<std::uint32_t> stamps;
		std::vector<float> heights;
		const CReadMap* map = nullptr;
		int lastFrame = -1;

		void Reset(std::uint32_t slots)
		{
			keys.assign(slots, 0);
			stamps.assign(slots, std::numeric_limits<std::uint32_t>::max());
			heights.assign(slots, 0.0f);
		}
	};

	GroundHeightCache groundHeightCache;

	float GetGroundYMargin(float x, float z, int frame)
	{
		const Config& cfg = GetConfig();
		const GroundClampConfig& gc = cfg.groundClampParams;

		if (groundHeightCache.keys.size() != gc.heightCacheSlots)
			groundHeightCache.Reset(gc.heightCacheSlots);

		if (groundHeightCache.map != readMap || frame < groundHeightCache.lastFrame) {
			std::fill(groundHeightCache.stamps.begin(), groundHeightCache.stamps.end(), std::numeric_limits<std::uint32_t>::max());
			groundHeightCache.map = readMap;
		}
		groundHeightCache.lastFrame = frame;

		const int quantizedX = Quantize(x, gc.heightCacheCellSize);
		const int quantizedZ = Quantize(z, gc.heightCacheCellSize);
		const auto key = (static_cast<std::uint64_t>(static_cast<std::uint32_t>(quantizedX)) << 32u) | static_cast<std::uint32_t>(quantizedZ);
		const std::size_t slot = (static_cast<std::uint32_t>(quantizedX) * HASH_X ^ static_cast<std::uint32_t>(quantizedZ) * HASH_Y) & (gc.heightCacheSlots - 1);
		const auto stamp = static_cast<std::uint32_t>(frame);

		if (groundHeightCache.stamps[slot] == stamp && groundHeightCache.keys[slot] == key)
			return groundHeightCache.heights[slot];

		const float height = CGround::GetHeightReal(x, z, false) + gc.margin;
		groundHeightCache.keys[slot] = key;
		groundHeightCache.stamps[slot] = stamp;
		groundHeightCache.heights[slot] = height;
		return height;
	}

	/*
	 * Whether a straight path from start to end would sink into terrain, and if
	 * so how high it has to be lifted and where the worst dip is. Keyed on the
	 * quantised endpoints, because a builder pouring into one spot asks the same
	 * question for every particle it emits.
	 */
	struct RouteCacheEntry {
		std::array<int, 6> endpoints = {};
		int expiresFrame = -1;
		float guideY = 0.0f;
		float peakT = 0.5f;
		bool requiresClamp = false;
	};

	struct RouteCache {
		std::vector<RouteCacheEntry> entries;
		const CReadMap* map = nullptr;
		int lastFrame = -1;
	};

	RouteCache routeCache;

	bool EvaluateGroundClampRoute(const float3& startPos, const float3& finalPos, float& guideY, float& peakT)
	{
		RECOIL_DETAILED_TRACY_ZONE;
		const Config& cfg = GetConfig();
		const GroundClampConfig& gc = cfg.groundClampParams;
		const int frame = gs->frameNum;
		const float3 path = finalPos - startPos;

		const std::array<int, 6> endpoints = {
			Quantize(startPos.x, gc.routeCacheCellSize),
			Quantize(startPos.y, gc.routeCacheCellSize),
			Quantize(startPos.z, gc.routeCacheCellSize),
			Quantize(finalPos.x, gc.routeCacheCellSize),
			Quantize(finalPos.y, gc.routeCacheCellSize),
			Quantize(finalPos.z, gc.routeCacheCellSize),
		};
		const std::size_t slot = (
			static_cast<std::uint32_t>(endpoints[0]) * HASH_X ^
			static_cast<std::uint32_t>(endpoints[1]) * HASH_Y ^
			static_cast<std::uint32_t>(endpoints[2]) * HASH_Z ^
			static_cast<std::uint32_t>(endpoints[3]) * HASH_W ^
			static_cast<std::uint32_t>(endpoints[4]) * 97531u ^
			static_cast<std::uint32_t>(endpoints[5]) * 1099511627u
		) & (gc.routeCacheSlots - 1);

		if (routeCache.entries.size() != gc.routeCacheSlots)
			routeCache.entries.assign(gc.routeCacheSlots, RouteCacheEntry{});

		if (routeCache.map != readMap || frame < routeCache.lastFrame) {
			for (RouteCacheEntry& entry : routeCache.entries)
				entry.expiresFrame = -1;
			routeCache.map = readMap;
		}
		routeCache.lastFrame = frame;

		RouteCacheEntry& cacheEntry = routeCache.entries[slot];
		if (cacheEntry.expiresFrame > frame && cacheEntry.endpoints == endpoints) {
			if (!cacheEntry.requiresClamp)
				return false;

			guideY = cacheEntry.guideY;
			peakT = cacheEntry.peakT;
			return true;
		}

		const float horizontalLengthSq = path.x * path.x + path.z * path.z;
		const bool longPath = (horizontalLengthSq > gc.longPathThresholdSq);
		const std::size_t sampleCount = longPath ? gc.longSamples.size() : gc.shortSamples.size();

		guideY = -std::numeric_limits<float>::max();
		float maxPenetration = -std::numeric_limits<float>::max();
		peakT = 0.5f;

		for (std::size_t sample = 0; sample < sampleCount; ++sample) {
			const float t = longPath ? gc.longSamples[sample] : gc.shortSamples[sample];
			const float3 samplePos = startPos + path * t;
			const float groundY = GetGroundYMargin(samplePos.x, samplePos.z, frame);
			guideY = std::max(guideY, groundY);

			const float penetration = groundY - samplePos.y;
			if (penetration > maxPenetration) {
				maxPenetration = penetration;
				peakT = t;
			}
		}

		cacheEntry.endpoints = endpoints;
		cacheEntry.expiresFrame = frame + gc.routeCacheFrames;
		cacheEntry.requiresClamp = (maxPenetration > gc.smartDelta);

		if (!cacheEntry.requiresClamp)
			return false;

		cacheEntry.guideY = guideY;
		cacheEntry.peakT = peakT;
		return true;
	}

	/*
	 * Homing targets are shared: every particle in one builder's stream chases
	 * the same unit or nano piece, so resolving it once per frame collapses
	 * hundreds of unit lookups into one.
	 */
	struct HomingTargetCacheEntry {
		int frame = -1;
		int targetID = -1;
		std::int64_t targetSyncID = -1;
		int targetPiece = -2;
		float3 pos;
		bool valid = false;
	};

	std::vector<HomingTargetCacheEntry> homingTargetCache;

	bool GetHomingTargetPos(int targetID, std::int64_t targetSyncID, int targetPiece, float3& targetPos)
	{
		RECOIL_DETAILED_TRACY_ZONE;
		const HomingConfig& hc = GetConfig().homingParams;

		if (homingTargetCache.size() != hc.targetCacheSlots)
			homingTargetCache.assign(hc.targetCacheSlots, HomingTargetCacheEntry{});

		const std::size_t slot = (
			static_cast<std::uint32_t>(targetID) * HASH_X ^
			static_cast<std::uint32_t>(targetPiece + 2) * HASH_Y
		) & (hc.targetCacheSlots - 1);
		HomingTargetCacheEntry& entry = homingTargetCache[slot];
		const int frame = gs->frameNum;

		if (entry.frame == frame && entry.targetID == targetID && entry.targetSyncID == targetSyncID && entry.targetPiece == targetPiece) {
			if (entry.valid)
				targetPos = entry.pos;

			return entry.valid;
		}

		entry.frame = frame;
		entry.targetID = targetID;
		entry.targetSyncID = targetSyncID;
		entry.targetPiece = targetPiece;
		entry.valid = false;

		const CUnit* target = unitHandler.GetUnit(targetID);
		if (target == nullptr || target->GetSyncID() != targetSyncID || target->isDead || target->IsCrashing())
			return false;

		if (targetPiece >= 0) {
			if (!target->localModel.Initialized() || !target->localModel.HasPiece(targetPiece))
				return false;

			entry.pos = target->GetObjectSpacePos(target->localModel.GetRawPiecePos(targetPiece));
		} else {
			entry.pos = target->midPos;
		}

		entry.valid = true;
		targetPos = entry.pos;
		return true;
	}

	/*
	 * Whether a bound target has been lost, or has had its work finished. Every
	 * particle in a stream asks about the same unit, so the unit lookup and the
	 * health read are done once per target per frame and the particles share it.
	 */
	struct TargetStateCacheEntry {
		int frame = -1;
		int targetID = -1;
		std::int64_t targetSyncID = -1;
		/// Destroyed, cancelled, recycled, or crashing: nothing left to spray at.
		bool lost = false;
		/// Finished and at full health: the work this spray represents is done.
		bool complete = false;
	};

	std::vector<TargetStateCacheEntry> targetStateCache;

	const TargetStateCacheEntry& GetTargetState(int targetID, std::int64_t targetSyncID)
	{
		RECOIL_DETAILED_TRACY_ZONE;
		const TargetLostFadeConfig& fc = GetConfig().targetLostFadeParams;

		if (targetStateCache.size() != fc.cacheSlots)
			targetStateCache.assign(fc.cacheSlots, TargetStateCacheEntry{});

		const std::size_t slot = (static_cast<std::uint32_t>(targetID) * HASH_X) & (fc.cacheSlots - 1);
		TargetStateCacheEntry& entry = targetStateCache[slot];
		const int frame = gs->frameNum;

		if (entry.frame == frame && entry.targetID == targetID && entry.targetSyncID == targetSyncID)
			return entry;

		entry.frame = frame;
		entry.targetID = targetID;
		entry.targetSyncID = targetSyncID;

		const CUnit* target = unitHandler.GetUnit(targetID);

		entry.lost = (target == nullptr || target->GetSyncID() != targetSyncID || target->isDead || target->IsCrashing());
		entry.complete = !entry.lost && !target->beingBuilt && (target->health >= target->maxHealth);
		return entry;
	}

	/*
	 * LOS for particles belonging to another allyteam, cached per quantised
	 * position. Only used to decide what LuaUI is told about; the renderer does
	 * its own, tighter test against the LOS map.
	 */
	struct LosCacheEntry {
		int x = 0;
		int y = 0;
		int z = 0;
		int allyTeam = -1;
		int expiresFrame = -1;
		bool visible = false;
	};

	struct LosCache {
		std::vector<LosCacheEntry> entries;
		int lastFrame = -1;
	};

	LosCache losCache;

	void InvalidateLosCache()
	{
		for (LosCacheEntry& entry: losCache.entries)
			entry.expiresFrame = -1;
	}

	bool IsPosInLos(const float3& pos, int allyTeam)
	{
		const VisibilityConfig& vc = GetConfig().visibility;
		const int frame = gs->frameNum;

		if (losCache.entries.size() != vc.losCacheSlots)
			losCache.entries.assign(vc.losCacheSlots, LosCacheEntry{});

		if (frame < losCache.lastFrame) {
			for (LosCacheEntry& entry : losCache.entries)
				entry.expiresFrame = -1;
		}
		losCache.lastFrame = frame;

		const int x = Quantize(pos.x, vc.losCacheCellSize);
		const int y = Quantize(pos.y, vc.losCacheCellSize);
		const int z = Quantize(pos.z, vc.losCacheCellSize);
		const std::size_t slot = (
			static_cast<std::uint32_t>(x) * HASH_X ^
			static_cast<std::uint32_t>(y) * HASH_Y ^
			static_cast<std::uint32_t>(z) * HASH_Z ^
			static_cast<std::uint32_t>(allyTeam) * HASH_W
		) & (vc.losCacheSlots - 1);

		LosCacheEntry& entry = losCache.entries[slot];
		if (entry.expiresFrame > frame && entry.x == x && entry.y == y && entry.z == z && entry.allyTeam == allyTeam)
			return entry.visible;

		entry.x = x;
		entry.y = y;
		entry.z = z;
		entry.allyTeam = allyTeam;
		entry.expiresFrame = frame + vc.losCacheFrames;
		entry.visible = losHandler->InLos(pos, allyTeam);
		return entry.visible;
	}

	float3 PositionAt(const Particle& particle, int frame)
	{
		return particle.startPos + particle.velocity * static_cast<float>(frame - particle.baseFrame);
	}
} // namespace


void System::Init()
{
	RECOIL_DETAILED_TRACY_ZONE;
	projectileHandler.currentNanoParticles -= static_cast<int>(particles.size());

	particles.clear();
	particles.reserve(std::max(0, projectileHandler.maxNanoParticles));
	events.clear();
	events.reserve(GetConfig().luaUpdate.threadQueueReserve);

	nextParticleID = -1;
	generation = 1;
	luaSampleGeneration = 1;
	luaClientActive = false;
	// a fresh game must not leave a reloaded widget holding stale lightIDs
	luaResetPending = true;

	groundHeightCache = {};
	routeCache = {};
	homingTargetCache.clear();
	targetStateCache.clear();
	losCache = {};
}

void System::Kill()
{
	RECOIL_DETAILED_TRACY_ZONE;
	projectileHandler.currentNanoParticles -= static_cast<int>(particles.size());

	particles.clear();
	events.clear();
	homingTargetCache.clear();
	targetStateCache.clear();
	groundHeightCache = {};
	routeCache = {};
	losCache = {};
}

bool System::Enabled() const
{
	return GetConfig().enabled && renderer != nullptr && renderer->Available();
}

void System::ResetLuaUpdates()
{
	++luaSampleGeneration;
	luaResetPending = true;
}


bool System::AllowSpawn(bool highPriority) const
{
	const Config& cfg = GetConfig();
	const int maxParticles = projectileHandler.maxNanoParticles;

	if (maxParticles <= 0)
		return false;

	/* High-priority emissions (capture, reclaim bursts) get the whole budget;
	 * ordinary spray is held slightly below it so the two never starve. */
	const float budget = maxParticles * (highPriority ? 1.0f : NORMAL_SPRAY_BUDGET_FRACTION);
	const float used = particles.size() / std::max(1.0f, budget);

	if (used < cfg.emission.budgetSoftStart)
		return true;

	if (used >= 1.0f)
		return false;

	const float acceptProbability = (1.0f - used) / std::max(0.01f, 1.0f - cfg.emission.budgetSoftStart);
	return (guRNG.NextFloat() < acceptProbability);
}

void System::SpawnSpray(
	const float3& startPos,
	const float3& direction,
	float distance,
	float jitterFraction,
	const SColor& color,
	int teamNum,
	float builderBuildSpeed,
	bool inverse,
	const SpawnParams& params
) {
	RECOIL_DETAILED_TRACY_ZONE;
	const EmissionConfig& ec = GetConfig().emission;

	const float3 sprayDirection = direction + guRNG.NextVector() * (jitterFraction * ec.directionJitterScale);
	const int lifeTime = std::max(1, static_cast<int>(std::ceil(distance / ec.particleSpeed)));
	const int allyTeam = teamHandler.IsValidTeam(teamNum) ? teamHandler.AllyTeam(teamNum) : -1;

	// an inverse spray starts where a forward one would end, and runs backwards
	const float3 spawnPos = inverse ? (startPos + sprayDirection * distance) : startPos;
	const float3 velocity = (inverse ? -sprayDirection : sprayDirection) * ec.particleSpeed;

	Add(spawnPos, velocity, lifeTime, color, allyTeam, builderBuildSpeed, params);
}

void System::Add(
	const float3& startPos,
	const float3& velocity,
	int lifeTime,
	const SColor& color,
	int allyTeam,
	float builderBuildSpeed,
	const SpawnParams& params
) {
	RECOIL_DETAILED_TRACY_ZONE;
	const Config& cfg = GetConfig();
	const int frame = gs->frameNum;

	/* Per-particle speed spread. The endpoint is preserved and the lifetime
	 * adjusted to match, so a faster particle simply arrives sooner. */
	const float3 endPos = startPos + velocity * static_cast<float>(lifeTime);
	const float speedMultiplier = 1.0f + cfg.emission.speedVariation * (guRNG.NextFloat() * 2.0f - 1.0f);
	const int adjustedLifeTime = std::max(1, static_cast<int>(std::ceil(lifeTime / speedMultiplier)));

	if (nextParticleID == std::numeric_limits<int>::min())
		nextParticleID = -1;

	Particle particle;
	particle.startPos = startPos;
	particle.velocity = (endPos - startPos) / static_cast<float>(adjustedLifeTime);
	particle.color = color;
	particle.baseFrame = frame;
	particle.createFrame = frame;
	particle.deathFrame = frame + adjustedLifeTime;
	particle.arriveFrame = particle.deathFrame;
	particle.id = nextParticleID--;
	particle.allyTeam = allyTeam;
	particle.builderBuildSpeed = builderBuildSpeed;
	particle.updatePhase = static_cast<std::uint8_t>(static_cast<std::uint32_t>(-particle.id));

	if (params.target != nullptr) {
		/* Bound regardless of homing, so the target-lost fade can watch it. A
		 * piece that does not exist on the model falls back to the midpos. */
		particle.targetID = params.target->id;
		particle.targetSyncID = params.target->GetSyncID();
		particle.targetPiece = params.targetPiece;
		particle.fadeWhenTargetComplete = params.fadeWhenTargetComplete;

		if (particle.targetPiece >= 0 && (!params.target->localModel.Initialized() || !params.target->localModel.HasPiece(particle.targetPiece)))
			particle.targetPiece = -1;

		/* A unit still under construction has not left the factory pad yet;
		 * homing onto it would make the spray chase it as it rolls out. */
		if (cfg.homing && (params.inverse || !params.target->beingBuilt))
			InitHoming(particle, params.target, adjustedLifeTime);
	}

	particle.fadeFrames = cfg.appearance.fadeFrames;

	if (cfg.groundClamp)
		InitGroundClamp(particle, params.inverse, adjustedLifeTime);

	particle.luaSampleGeneration = luaSampleGeneration;
	particle.luaSelected = ShouldReportToLua(particle);

	particles.emplace_back(particle);
	projectileHandler.currentNanoParticles += 1;

	if (++generation == 0)
		generation = 1;
}


void System::InitHoming(Particle& particle, const CUnit* target, int lifeTime)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const int targetPiece = particle.targetPiece;

	float3 targetPos = (targetPiece >= 0)
		? target->GetObjectSpacePos(target->localModel.GetRawPiecePos(targetPiece))
		: static_cast<float3>(target->midPos);

	particle.homing = true;

	if (targetPiece < 0) {
		/* Aiming at the midpos would funnel every particle of a stream into one
		 * point. Keep the spread the emitter picked by tracking the offset from
		 * the midpos rather than the midpos itself. */
		const float3 initialEndPos = particle.startPos + particle.velocity * static_cast<float>(lifeTime);
		particle.homingOffset = initialEndPos - targetPos;
		targetPos += particle.homingOffset;

		const float initialSpeed = particle.velocity.Length();
		const float speedMultiplier = (target->unitDef != nullptr && target->unitDef->canfly)
			? HOMING_SPEED_LIMIT_MULT_AIR
			: HOMING_SPEED_LIMIT_MULT_GROUND;
		const float maxHomingSpeed = std::max(initialSpeed * speedMultiplier, 0.1f);
		particle.homingSpeedLimitSq = maxHomingSpeed * maxHomingSpeed;
	}

	Reaim(particle, particle.startPos, targetPos, particle.baseFrame, lifeTime);
}

void System::InitGroundClamp(Particle& particle, bool inverse, int lifeTime)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (lifeTime <= 1)
		return;

	const GroundClampConfig& gc = GetConfig().groundClampParams;
	const int frame = particle.baseFrame;

	particle.groundClampFinalPos = particle.startPos + particle.velocity * static_cast<float>(lifeTime);

	float guideY;
	float peakT;
	if (!EvaluateGroundClampRoute(particle.startPos, particle.groundClampFinalPos, guideY, peakT))
		return;

	particle.groundClamp = true;
	particle.groundClampNextFrame = frame + (particle.updatePhase % gc.recheckFramesHit);
	particle.groundClampFinalPos.y = std::max(
		particle.groundClampFinalPos.y,
		GetGroundYMargin(particle.groundClampFinalPos.x, particle.groundClampFinalPos.z, frame)
	);

	/* Reclaim-style particles converge on the builder, which is above ground by
	 * construction, so a lifted waypoint would only make them arc oddly. */
	if (inverse)
		return;

	peakT = std::clamp(peakT, GROUND_CLAMP_PEAK_MIN, GROUND_CLAMP_PEAK_MAX);

	const int firstLegFrames = std::clamp(static_cast<int>(lifeTime * peakT), 1, lifeTime - 1);
	particle.groundClampWaypointPos = particle.startPos + (particle.groundClampFinalPos - particle.startPos) * peakT;
	particle.groundClampWaypointPos.y = std::max(particle.groundClampWaypointPos.y, guideY);
	particle.groundClampWaypointFrame = frame + firstLegFrames;

	Reaim(particle, particle.startPos, particle.groundClampWaypointPos, frame, firstLegFrames);
}

void System::Reaim(Particle& particle, const float3& fromPos, const float3& targetPos, int frame, int remainingFrames)
{
	if (remainingFrames <= 0)
		return;

	particle.startPos = fromPos;
	particle.baseFrame = frame;
	particle.velocity = (targetPos - fromPos) / static_cast<float>(remainingFrames);
}

bool System::ResolveHomingTarget(Particle& particle, float3& targetPos) const
{
	if (!particle.homing)
		return false;

	if (!GetHomingTargetPos(particle.targetID, particle.targetSyncID, particle.targetPiece, targetPos)) {
		particle.homing = false;
		return false;
	}

	if (particle.targetPiece < 0)
		targetPos += particle.homingOffset;

	return true;
}

bool System::UpdateGroundClamp(Particle& particle, const float3& currentPos, int frame)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const Config& cfg = GetConfig();
	const GroundClampConfig& gc = cfg.groundClampParams;
	/* Paced against the arrival schedule: a fade may have pulled deathFrame
	 * earlier, and dividing the remaining path by that shortened window would
	 * accelerate the particle toward a target it is meant to dissolve short of. */
	const int remainingLife = particle.arriveFrame - frame;

	if (remainingLife <= 0)
		return false;

	bool reaimed = false;
	float3 pos = currentPos;

	// first leg done: turn toward the real endpoint
	if (particle.groundClampWaypointFrame >= 0 && frame >= particle.groundClampWaypointFrame) {
		particle.groundClampWaypointFrame = -1;

		float3 targetPos = particle.groundClampFinalPos;
		if (cfg.homing)
			ResolveHomingTarget(particle, targetPos);

		targetPos.y = std::max(targetPos.y, GetGroundYMargin(targetPos.x, targetPos.z, frame));
		Reaim(particle, pos, targetPos, frame, remainingLife);
		reaimed = true;
	}

	if (!cfg.groundClamp || frame < particle.groundClampNextFrame)
		return reaimed;

	// still sinking into terrain? lift back out and re-aim from there
	const float groundY = GetGroundYMargin(pos.x, pos.z, frame);
	if (pos.y >= groundY) {
		particle.groundClampNextFrame = frame + gc.recheckFramesMiss;
		return reaimed;
	}

	pos.y = groundY;

	const bool onFirstLeg = (particle.groundClampWaypointFrame >= 0);
	float3 targetPos = onFirstLeg ? particle.groundClampWaypointPos : particle.groundClampFinalPos;
	const int remainingFrames = onFirstLeg ? (particle.groundClampWaypointFrame - frame) : remainingLife;

	if (!onFirstLeg && cfg.homing)
		ResolveHomingTarget(particle, targetPos);

	targetPos.y = std::max(targetPos.y, GetGroundYMargin(targetPos.x, targetPos.z, frame));
	Reaim(particle, pos, targetPos, frame, remainingFrames);
	particle.groundClampNextFrame = frame + gc.recheckFramesHit;
	return true;
}

bool System::UpdateTargetLostFade(Particle& particle, int frame)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const Config& cfg = GetConfig();
	const TargetStateCacheEntry& state = GetTargetState(particle.targetID, particle.targetSyncID);

	if (!state.lost && !(particle.fadeWhenTargetComplete && state.complete))
		return false;

	const int remaining = particle.deathFrame - frame;
	if (remaining <= 0)
		return false;

	/* Each particle gets its own window so a stream dissolves unevenly instead
	 * of winking out on one frame. The window never extends a life, only
	 * shortens one, and the whole remainder becomes the ramp. */
	const TargetLostFadeConfig& fc = cfg.targetLostFadeParams;
	const float jitter = fc.jitterMin + (fc.jitterMax - fc.jitterMin) * guRNG.NextFloat();
	const int fadeFrames = std::clamp(static_cast<int>(fc.durationFrames * jitter), 1, remaining);

	particle.deathFrame = frame + fadeFrames;
	particle.fadeFrames = static_cast<float>(fadeFrames);
	particle.fading = true;
	/* Homing is left alone: a finished unit is still there to curve toward
	 * while the spray dissolves, and a destroyed one makes it give up on its
	 * own the next time it looks. */
	return true;
}

bool System::UpdateHoming(Particle& particle, const float3& currentPos, int frame)
{
	RECOIL_DETAILED_TRACY_ZONE;
	float3 targetPos;
	if (!ResolveHomingTarget(particle, targetPos))
		return false;

	// arriveFrame, not deathFrame: see UpdateGroundClamp
	const int remainingFrames = particle.arriveFrame - frame;
	if (remainingFrames <= 1)
		return false;

	const float3 newVelocity = (targetPos - currentPos) / static_cast<float>(remainingFrames);

	// target ran away faster than a nano particle can plausibly chase; let it go
	if (particle.homingSpeedLimitSq > 0.0f && newVelocity.SqLength() > particle.homingSpeedLimitSq) {
		particle.homing = false;
		return false;
	}

	if ((newVelocity - particle.velocity).SqLength() * remainingFrames * remainingFrames < HOMING_MIN_TARGET_MOVE_SQ)
		return false;

	particle.startPos = currentPos;
	particle.baseFrame = frame;
	particle.velocity = newVelocity;
	return true;
}


void System::Update()
{
	ZoneScopedN("NanoParticles::Update");
	RECOIL_DETAILED_TRACY_ZONE;

	const Config& cfg = GetConfig();
	const int frame = gs->frameNum;

	const bool clientActive = eventHandler.HasNanoParticleUpdateClients();
	if (clientActive != luaClientActive) {
		luaClientActive = clientActive;
		ResetLuaUpdates();
	}

	/* Retract everything when the local player's visibility changes: a reported
	 * light lives until its remainingLife runs out, so without this a particle
	 * that just went out of sight keeps travelling on the consumer's side. */
	const int visibilityAllyTeam = gu->myAllyTeam;
	const bool visibilityFullView = gu->spectatingFullView;
	const bool visibilityGlobalLos = teamHandler.IsValidAllyTeam(visibilityAllyTeam) && losHandler->GetGlobalLOS(visibilityAllyTeam);

	if (visibilityAllyTeam != luaVisibilityAllyTeam || visibilityFullView != luaVisibilityFullView || visibilityGlobalLos != luaVisibilityGlobalLos) {
		luaVisibilityAllyTeam = visibilityAllyTeam;
		luaVisibilityFullView = visibilityFullView;
		luaVisibilityGlobalLos = visibilityGlobalLos;

		// cached LOS answers were decided under the old basis
		InvalidateLosCache();
		ResetLuaUpdates();
	}

	const bool reportToLua = cfg.luaUpdates && luaClientActive;
	const std::uint32_t sampleGeneration = luaSampleGeneration;

	for (std::size_t i = 0; i < particles.size();) {
		Particle& particle = particles[i];

		if (frame >= particle.deathFrame) {
			Remove(i);
			continue;
		}

		const float3 currentPos = PositionAt(particle, frame);

		const bool clampDue = particle.groundClamp && (
			(particle.groundClampWaypointFrame >= 0 && frame >= particle.groundClampWaypointFrame) ||
			(cfg.groundClamp && frame >= particle.groundClampNextFrame)
		);
		/* Homing is staggered by particle so a large stream spreads its re-aims
		 * over the interval instead of spiking on one frame. */
		const bool homingDue = particle.homing
			&& cfg.homing
			&& particle.groundClampWaypointFrame < 0
			&& ((frame + (particle.updatePhase % cfg.homingParams.runEveryFrames)) % cfg.homingParams.runEveryFrames) == 0;

		bool reaimed = false;

		if (clampDue)
			reaimed = UpdateGroundClamp(particle, currentPos, frame);

		if (!reaimed && homingDue)
			reaimed = UpdateHoming(particle, currentPos, frame);

		/* Staggered the same way as homing. Once a particle is fading there is
		 * nothing further to decide, so it drops out of the check entirely. */
		const bool fadeCheckDue = !particle.fading
			&& particle.targetID >= 0
			&& cfg.targetLostFade
			&& ((frame + (particle.updatePhase % cfg.targetLostFadeParams.checkEveryFrames)) % cfg.targetLostFadeParams.checkEveryFrames) == 0;

		bool faded = false;

		if (fadeCheckDue)
			faded = UpdateTargetLostFade(particle, frame);

		if ((reaimed || faded) && ++generation == 0)
			generation = 1;

		if (reportToLua) {
			// re-sample after a config change so the reported subset stays consistent
			if (particle.luaSampleGeneration != sampleGeneration) {
				particle.luaSampleGeneration = sampleGeneration;
				particle.luaSelected = ShouldReportToLua(particle);
				particle.luaSpawnReported = false;
			}

			if (particle.luaSelected) {
				if (!particle.luaSpawnReported) {
					QueueEvent(particle, currentPos, EventType::Spawn);
					particle.luaSpawnReported = true;
				} else if (reaimed || faded) {
					QueueEvent(particle, currentPos, EventType::Update);
				}
			}
		}

		++i;
	}

	DispatchEvents();
}

void System::Remove(std::size_t index)
{
	particles[index] = particles.back();
	particles.pop_back();

	projectileHandler.currentNanoParticles -= 1;

	if (++generation == 0)
		generation = 1;
}


bool System::ShouldReportToLua(const Particle& particle) const
{
	const Config& cfg = GetConfig();
	const LuaUpdateConfig& lc = cfg.luaUpdate;

	/*
	 * Deferred-light widgets cannot afford one light per particle, so only a
	 * fraction is reported. The fraction scales with the emitter's throughput
	 * and the particle's speed, so a big builder still lights up more than a
	 * small one, and the pick itself is a hash of the particle id: stable
	 * across frames, and free of any per-particle RNG draw.
	 */
	const float sampleFraction = cfg.rate
		* lc.sampleRate
		* (std::max(0.0f, particle.builderBuildSpeed) / cfg.emission.referenceBuildSpeed)
		* (particle.velocity.Length() / cfg.emission.particleSpeed);

	if (sampleFraction <= 0.0f)
		return false;
	if (sampleFraction >= 1.0f)
		return true;

	const double hash = std::fmod(static_cast<double>(static_cast<std::uint32_t>(particle.id)) * lc.hashMultiplier, lc.hashRange);
	return hash < (static_cast<double>(sampleFraction) * lc.hashRange);
}

bool System::IsEventVisible(const Particle& particle, const float3& pos) const
{
	if (gu->spectatingFullView)
		return true;

	if (teamHandler.IsValidAllyTeam(particle.allyTeam) && teamHandler.Ally(particle.allyTeam, gu->myAllyTeam))
		return true;

	if (!teamHandler.IsValidAllyTeam(gu->myAllyTeam))
		return false;

	return IsPosInLos(pos, gu->myAllyTeam) || IsPosInLos(pos + particle.velocity, gu->myAllyTeam);
}

void System::QueueEvent(const Particle& particle, const float3& pos, EventType type)
{
	if (!IsEventVisible(particle, pos))
		return;

	Event event;
	event.type = type;
	event.lightID = particle.id;
	event.pos = pos;
	event.velocity = particle.velocity;
	event.remainingLife = static_cast<float>(std::max(particle.deathFrame - gs->frameNum, 0));
	event.color = float3(particle.color[0], particle.color[1], particle.color[2]) * (1.0f / 255.0f);
	event.builderBuildSpeed = particle.builderBuildSpeed;
	events.emplace_back(event);
}

void System::DispatchEvents()
{
	if (luaResetPending) {
		Event reset;
		reset.type = EventType::Reset;
		events.insert(events.begin(), reset);
		luaResetPending = false;
	}

	if (events.empty())
		return;

	{
		ZoneScopedN("NanoParticles::Update:LuaUI");
		eventHandler.NanoParticleUpdate(events);
	}

	events.clear();
}


namespace {
	/*
	 * Springsettings observer. Lives here rather than on the System or Renderer
	 * because a change can affect either, and both have to be told in one place.
	 */
	struct ConfigObserver {
		void ConfigNotify(const std::string& key, const std::string& /*value*/)
		{
			if (!ReloadConfigSetting(key))
				return;

			if (renderer != nullptr)
				renderer->ConfigChanged();

			system.ResetLuaUpdates();
		}
	};

	ConfigObserver configObserver;
} // namespace

void Init()
{
	InitConfig();
	configHandler->NotifyOnChange(&configObserver, GetObservedConfigKeys());

	system.Init();
	emitter.Init();
}

void Kill()
{
	configHandler->RemoveObserver(&configObserver);

	emitter.Kill();
	system.Kill();
}

} // namespace NanoParticles
