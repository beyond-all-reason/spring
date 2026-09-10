/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "NanoParticleDefs.h"

namespace NanoParticles {

/*
 * Owns the live nano particles.
 *
 * The system is deliberately not a projectile container: particles have no
 * projectile id, take no part in collision or quadfield work, are never handed
 * to Lua as projectiles, and are not serialised. They exist only between the
 * emitter that spawns them and the renderer that draws them.
 *
 * Update() runs once per sim frame, straight after the projectile handler, and
 * is single-threaded: the whole live set is a flat vector of PODs, and at the
 * default MaxNanoParticles a full pass is far cheaper than the synchronisation
 * a parallel one would need.
 */
class System {
public:
	void Init();
	void Kill();

	/// True when the effect is configured on and the renderer can actually draw it.
	bool Enabled() const;

	/*
	 * Adds one particle to a spray.
	 *
	 * `direction` is a unit vector along the un-jittered path and `jitterFraction`
	 * the spread the caller asked for, as a fraction of `distance`; the effect
	 * applies its own scale on top. `inverse` sprays run from the far end back
	 * toward `startPos`, which is how reclaim and capture read.
	 */
	void SpawnSpray(
		const float3& startPos,
		const float3& direction,
		float distance,
		float jitterFraction,
		const SColor& color,
		int teamNum,
		float builderBuildSpeed,
		bool inverse,
		const SpawnParams& params = {}
	);

	/*
	 * Whether the budget has room for another particle. Stays fully open until
	 * `emission.budgetSoftStart` of the budget is spent, then ramps rejection in
	 * over the remainder, so NanoParticlesRate scales close to linearly instead
	 * of asymptoting the way legacy's proportional throttle does.
	 */
	bool AllowSpawn(bool highPriority) const;

	/// Per sim frame: re-aim homing/clamped particles, retire expired ones, tell LuaUI.
	void Update();

	/// Live particles, in no particular order. Read by the renderer.
	const std::vector<Particle>& GetParticles() const { return particles; }

	/// Bumped on every add, removal and re-aim, so the renderer can skip re-uploads.
	std::uint32_t GetGeneration() const { return generation; }

	/// Invalidates every LuaUI light and re-samples which particles are reported.
	void ResetLuaUpdates();

private:
	void Add(const float3& startPos, const float3& velocity, int lifeTime, const SColor& color, int allyTeam, float builderBuildSpeed, const SpawnParams& params);
	void InitHoming(Particle& particle, const CUnit* target, int lifeTime);
	void InitGroundClamp(Particle& particle, bool inverse, int lifeTime);
	bool ResolveHomingTarget(Particle& particle, float3& targetPos) const;
	bool UpdateGroundClamp(Particle& particle, const float3& currentPos, int frame);
	bool UpdateHoming(Particle& particle, const float3& currentPos, int frame);
	bool UpdateTargetLostFade(Particle& particle, int frame);
	static void Reaim(Particle& particle, const float3& fromPos, const float3& targetPos, int frame, int remainingFrames);

	bool ShouldReportToLua(const Particle& particle) const;
	bool IsEventVisible(const Particle& particle, const float3& pos) const;
	void QueueEvent(const Particle& particle, const float3& pos, EventType type);
	void DispatchEvents();

	void Remove(std::size_t index);

	std::vector<Particle> particles;
	std::vector<Event> events;

	/// Lua light ids count down from -1 so they can never collide with a unit id.
	int nextParticleID = -1;
	std::uint32_t generation = 1;
	/// Bumped when the sampling changes; particles re-evaluate their selection.
	std::uint32_t luaSampleGeneration = 1;
	/// Whether any client is actually subscribed to the callin.
	bool luaClientActive = false;
	/// Set while a Reset event is pending, so it leads the next batch.
	bool luaResetPending = false;

	/*
	 * What the local player could see when the current LuaUI reports were made.
	 * A consumer expires a light from the `remainingLife` it was given, so
	 * nothing retracts a light on its own; when visibility changes underneath
	 * it - spectator toggle, allyteam change, global LOS toggle - the whole set
	 * has to be reset and re-reported, or lights keep travelling for particles
	 * that just became invisible.
	 */
	int luaVisibilityAllyTeam = -2;
	bool luaVisibilityFullView = false;
	bool luaVisibilityGlobalLos = false;
};

extern System system;

/*
 * Brings the effect up and down as a whole: config, particle store, emitter
 * state and the springsettings observer. The renderer is separate, because it
 * needs a GL context and therefore a different point in the load order.
 */
void Init();
void Kill();

} // namespace NanoParticles
