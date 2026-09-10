/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <cstdint>

#include "System/Color.h"
#include "System/float3.h"

class CUnit;

namespace NanoParticles {

/*
 * A nano particle is a pure render-side POD: it never enters the projectile
 * containers, has no id in the projectile handler, and is not visible to Lua as
 * a projectile. Motion is analytic, so the shader reconstructs the position
 * from `startPos + velocity * (frame - baseFrame)` and the CPU only touches a
 * particle when it homes or has to clear terrain.
 */
struct Particle {
	/// Position the particle occupied at `baseFrame`.
	float3 startPos;
	/// Elmos per frame.
	float3 velocity;
	SColor color;

	/*
	 * `baseFrame` moves forward whenever the particle is re-aimed, so the
	 * analytic position stays correct without replaying the whole path.
	 * `createFrame` never moves: it seeds the per-particle hash in the shader,
	 * so re-aiming does not make a particle visibly change size or tumble.
	 */
	int baseFrame = 0;
	int createFrame = 0;
	int deathFrame = 0;
	/*
	 * Frame the spray would land on its target. Homing and ground clamp pace
	 * their re-aims against this rather than deathFrame: a target-lost fade
	 * pulls deathFrame earlier, and pacing against that would make every
	 * subsequent re-aim speed the particle up to arrive before it dies.
	 */
	int arriveFrame = 0;

	/// Unique and negative, so it cannot be confused with a projectile id. Also the LuaUI light id.
	int id = -1;
	int allyTeam = -1;
	/// buildSpeed of the emitting builder; passed through to LuaUI for light sizing.
	float builderBuildSpeed = 0.0f;

	/*
	 * Frames the shader ramps alpha and size down over before `deathFrame`.
	 * Starts at the appearance default; a target-lost fade shortens the
	 * particle's life and widens this so the whole remainder is the ramp.
	 */
	float fadeFrames = 0.0f;

	// --- target (the unit this particle is bound to) -----------------------
	/*
	 * The workpiece for a forward spray, the builder for an inverse one. Homing
	 * re-aims at it when enabled; the target-lost fade watches it either way.
	 */
	int targetID = -1;
	std::int64_t targetSyncID = -1;
	/// Nano piece on the target to follow, or -1 for its midpos.
	int targetPiece = -1;

	// --- homing (only meaningful while `homing` is set) ------------------
	float homingSpeedLimitSq = 0.0f;
	float3 homingOffset;

	// --- ground clamp (only meaningful while `groundClamp` is set) -------
	float3 groundClampFinalPos;
	float3 groundClampWaypointPos;
	int groundClampWaypointFrame = -1;
	int groundClampNextFrame = -1;

	// --- LuaUI reporting -------------------------------------------------
	/// Sampling generation this particle's selection was decided under.
	std::uint32_t luaSampleGeneration = 0;
	/// Whether this particle was picked for the LuaUI update sample.
	bool luaSelected = false;
	/// Whether the Spawn event has already gone out for it.
	bool luaSpawnReported = false;

	/// Staggers the per-particle homing/clamp work across frames.
	std::uint8_t updatePhase = 0;
	bool homing = false;
	bool groundClamp = false;
	/// Also fade when the target is finished and at full health, i.e. the work is done.
	bool fadeWhenTargetComplete = false;
	/// Set once the target-lost fade has shortened this particle's life.
	bool fading = false;
};

/// Extra spawn context the emitter hands to the system; empty for plain sprays.
struct SpawnParams {
	/*
	 * Unit the spray is bound to: the workpiece for a forward spray, the
	 * builder for an inverse one. Homing follows it when enabled; losing it
	 * fades the particle out.
	 */
	const CUnit* target = nullptr;
	/// Nano piece on `target` to track, or -1 for its midpos.
	int targetPiece = -1;
	/// Reclaim-style particle: it travels from the target back to the builder.
	bool inverse = false;
	/*
	 * The spray represents work that ends when the target is finished and at
	 * full health - build and repair, but not capture, whose target is usually
	 * healthy from the start. Fades the particle out when that point is reached.
	 */
	bool fadeWhenTargetComplete = false;
};

/*
 * Mirrors the operation ids documented on the NanoParticleUpdate callin.
 *
 * `Remove` is reserved and never emitted: a particle's death frame is fixed at
 * spawn and reported as `remainingLife`, so a consumer can expire its own state
 * without a second event per particle. `Reset` covers the cases where that is
 * not enough - the sampling changed, or the effect was switched off.
 */
enum class EventType : std::uint8_t {
	Spawn  = 1,
	Update = 2,
	Remove = 3,
	Reset  = 4,
};

/// One entry of the batch handed to LuaUI each sim frame.
struct Event {
	EventType type = EventType::Update;
	int lightID = -1;
	float3 pos;
	float3 velocity;
	float remainingLife = 0.0f;
	float3 color;
	float builderBuildSpeed = 0.0f;
};

} // namespace NanoParticles
