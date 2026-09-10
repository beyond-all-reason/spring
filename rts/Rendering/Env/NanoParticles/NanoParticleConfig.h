/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace NanoParticles {

/*
 * Every tunable of the standalone nano particle effect lives here, in one
 * struct, so the look and feel can be iterated on without hunting through the
 * renderer/system sources for bare literals.
 *
 * The handful of knobs a player is expected to touch are springsettings, listed
 * in NanoParticleConfig.cpp and live-reloadable through ConfigNotify. Everything
 * else is a compiled-in default: tuned here, in one place, rather than exposed
 * as content configuration.
 */

/// Knobs that decide how many particles are emitted and how fast they travel.
struct EmissionConfig {
	/// Elmos per frame a nano particle covers. Legacy nano projectiles use 3.
	float particleSpeed = 4.0f;
	/// Per-particle speed spread, +/- this fraction. Also shortens/extends lifetime to match.
	float speedVariation = 0.14f;
	/// Emission is proportional to (buildSpeed * buildPower) / this reference buildSpeed.
	float referenceBuildSpeed = 100.0f;
	/*
	 * Scales the emission spread the caller asked for. 1.0 reproduces legacy
	 * nano spray exactly; the 3D shapes read better as discrete chunks with a
	 * somewhat tighter stream, so games may want to pull this down.
	 */
	float directionJitterScale = 1.0f;
	/*
	 * If a builder has build power but its proportional rate is too low to have
	 * produced a particle for this many frames, force one out so the player still
	 * gets feedback. The forced emit is debited from the accumulator, so the
	 * long-run rate stays proportional.
	 */
	int feedbackEmitMinGap = 60;
	/// The gap above is stated at this rate; it is rescaled when NanoParticlesRate differs.
	float feedbackEmitReferenceRate = 0.32f;
	/// Emitter bookkeeping for a unit is dropped after this many idle frames.
	int emitterStateMaxIdleFrames = 300;
	/*
	 * Fraction of the particle budget that may be spent before spawns start
	 * being rejected.
	 *
	 * Legacy nano spray throttles in proportion to how much of the budget is
	 * already used, starting from the very first particle. That makes the live
	 * count settle at T*E*L / (T + E*L) for a request rate E and lifetime L, so
	 * it approaches the budget T hyperbolically and NanoParticlesRate stops
	 * doing much well before the budget is full. Holding the gate fully open up
	 * to this fraction keeps the response linear across the useful range, and
	 * only ramps rejection in over the last stretch so a full budget is still
	 * shared between emitters rather than taken by whoever asks first.
	 */
	float budgetSoftStart = 0.85f;
};

/// One-shot burst fired when a unit finishes being reclaimed.
struct ReclaimBurstConfig {
	/// Particles each contributing builder emits regardless of the reclaimee's cost.
	int base = 1;
	/// How quickly each builder's share grows with the reclaimee's metal cost.
	float logK = 40.0f;
	/// Metal cost at or below which a builder emits roughly `base` particles.
	float logNorm = 250.0f;
	/// Sub-linear exponent on contributor count: total = perBuilder * count^exponent.
	float builderExponent = 0.5f;
	/// Hard ceiling on the total particle count across all contributors.
	int maxParticles = 1500;
	/// Burst spawns inside this fraction of the reclaimee's collision volume.
	float volumeFraction = 0.55f;
	/// Direction jitter for burst particles, as a fraction of their travel length.
	float directionJitter = 0.10f;
	/// A builder counts as a contributor if it poured reclaim within this many frames.
	int contributorMaxAge = 30;
};

/// Particles curving toward a moving target (unit midpos, or a builder nano piece).
struct HomingConfig {
	/// Re-aim every Nth frame. Particles move little between frames, so this is invisible.
	int runEveryFrames = 4;
	/// Slots in the per-frame target-position cache. Power of two.
	std::uint32_t targetCacheSlots = 256;
};

/// Routing particles over terrain that would otherwise swallow them.
struct GroundClampConfig {
	/// Particles are held this far above the ground height.
	float margin = 11.0f;
	/// Only clamp when the path dips more than this far below the margin.
	float smartDelta = 4.0f;
	/// Re-evaluate the route this many frames after a clamp was needed.
	int recheckFramesHit = 6;
	/// Re-evaluate the route this many frames after a clamp was not needed.
	int recheckFramesMiss = 12;
	/// Quantisation of the ground-height cache, in elmos. Power of two.
	float heightCacheCellSize = 16.0f;
	/// Slots in the ground-height cache. Power of two.
	std::uint32_t heightCacheSlots = 1024;
	/// Quantisation of the route cache endpoints, in elmos.
	float routeCacheCellSize = 64.0f;
	/// Route-cache entries stay valid for this many frames.
	int routeCacheFrames = 45;
	/// Slots in the route cache. Power of two.
	std::uint32_t routeCacheSlots = 1024;
	/// Horizontal length squared above which the denser sample set is used.
	float longPathThresholdSq = 4096.0f;
	/// Path fractions sampled for short hops.
	std::array<float, 3> shortSamples = {0.35f, 0.50f, 0.65f};
	/// Path fractions sampled for long hops.
	std::array<float, 8> longSamples = {0.12f, 0.22f, 0.35f, 0.50f, 0.65f, 0.78f, 0.90f, 0.96f};
};

/*
 * Fading a particle out when the unit it was bound to is lost - destroyed,
 * cancelled mid-build, crashing - or, for build and repair spray, when the work
 * is done. Rather than vanishing or flying on into nothing, the trailing spray
 * keeps its course and dissolves: alpha and size ramp down over a per-particle
 * window, and the particle dies when the window closes.
 */
struct TargetLostFadeConfig {
	/// Base fade duration, in frames.
	float durationFrames = 30.0f;
	/// Check each particle's target every Nth frame, staggered per particle.
	int checkEveryFrames = 4;
	/*
	 * Per-particle fade duration is durationFrames scaled by a random factor in
	 * [jitterMin, jitterMax), so a stream dissolves unevenly rather than
	 * winking out on one frame.
	 */
	float jitterMin = 0.75f;
	float jitterMax = 1.25f;
	/// Slots in the per-frame target-state cache. Power of two.
	std::uint32_t cacheSlots = 256;
};

/// Line-of-sight filtering of particles belonging to other allyteams.
struct VisibilityConfig {
	/// An LOS answer is reused for this many frames.
	int losCacheFrames = 7;
	/// Quantisation of the LOS cache, in elmos.
	float losCacheCellSize = 64.0f;
	/// Slots in the LOS cache. Power of two.
	std::uint32_t losCacheSlots = 1024;
};

/// Buffer management and culling on the render side.
struct RenderConfig {
	/// Rebuild the persistent (own/allied) vertex buffer at most every Nth frame.
	int bufferSyncIntervalFrames = 4;
	/// Enemy particles are binned into cells of this size for frustum rejection.
	float enemyCellSize = 128.0f;
	/// Conservative per-particle radius used for frustum tests, in elmos.
	float cullRadius = 22.0f;
	/*
	 * Minimap streak length, expressed as frames of travel. One frame is what
	 * legacy nano projectiles draw, but at ~4 elmos that is well under a pixel
	 * on a normal-sized minimap of a large map, so the spray is only visible
	 * magnified. 0 draws a point per particle instead, which is always at least
	 * one pixel.
	 */
	float minimapStreakFrames = 10.0f;
	/// Draw a point at the head of each streak so particles stay visible when zoomed out.
	bool minimapPoints = true;
	/*
	 * Whether to draw in the water reflection and refraction passes.
	 *
	 * Off by default: those two passes double the number of times the effect is
	 * drawn per frame, and each one pays a full set of state changes and uniform
	 * uploads before a single particle is submitted. Nano spray is small and
	 * bright, so what it contributes to a reflection is marginal next to that.
	 */
	bool drawInWaterPasses = false;
};

/*
 * Purely visual knobs. These are uploaded to the shaders as uniforms rather
 * than baked in as GLSL constants, so both the geometry and the no-geometry
 * path read the same numbers and a tweak needs no shader edit.
 */
struct AppearanceConfig {
	/// Half-extent of the particle shape, in elmos. The cube spans ~2x this.
	float drawRadius = 1.5f;
	/// Per-particle size spread, +/- this fraction.
	float sizeVariation = 0.3f;
	/// Base alpha of a particle, 0-1.
	float baseAlpha = 50.0f / 255.0f;
	/// Per-particle alpha spread, +/- this fraction.
	float alphaVariation = 2.5f;
	/// End-of-life alpha/size ramp, in frames.
	float fadeFrames = 4.0f;
	/// Additive halo size, as a multiple of the shape size.
	float glowScale = 11.0f;
	/// Additive halo brightness.
	float glowIntensity = 0.35f;
	/// Additive halo radial falloff exponent. Higher is tighter.
	float glowFalloff = 9.5f;
	/// Team-color brightness equalisation strength, 0 = raw team color, 1 = full.
	float colorEqualize = 0.7f;
	/// Luma the equalisation aims for.
	float colorTargetLuma = 0.55f;
	/// Per-particle hue wobble.
	float hueJitter = 0.1f;
	/// Face shading multiplier. Kept modest so dark faces still read as solid.
	float coreBoost = 0.3f;
	/// View-dependent face shading: 0 = flat, higher = back faces visible but dimmed.
	float showInside = 4.0f;
	/// Internal noise amplitude.
	float noiseAmount = 6.0f;
	/// Internal noise scroll speed, in units per second.
	float noiseSpeed = 25.0f;
	/// Internal noise spatial frequency.
	float noiseScale = 1.75f;
	/// Strength of the white hot spots the noise punches through.
	float whiteHotspot = 1.5f;
	/// Noise value above which a hot spot starts to appear.
	float whiteHotspotThreshold = 0.6f;
	/// Base tumble angle range, in degrees: [-rotationRange, +rotationRange].
	float rotationRange = 180.0f;
	/// Tumble rate range, in degrees per second: [-rotationRate, +rotationRate].
	float rotationRate = 40.0f;
};

/// Sampling of the batched NanoParticleUpdate callin LuaUI receives.
struct LuaUpdateConfig {
	/// Multiplier folded into the per-particle selection fraction.
	float sampleRate = 0.25f;
	/// Multiplier of the integer-hash used to pick which particles are reported.
	double hashMultiplier = 2654435761.0;
	/// Range the hash is folded into before comparing against the sample fraction.
	double hashRange = 1000000.0;
	/// Initial capacity of each per-thread event queue.
	std::size_t threadQueueReserve = 64;
};

struct Config {
	// --- springsettings (see NanoParticleConfig.cpp) ---------------------
	/// Master switch. When off, nano spray falls through to legacy CNanoProjectile.
	bool enabled = false;
	/// Force the instanced no-geometry-shader path even where a geometry shader works.
	bool forceNoGeometryShader = false;
	/// Curve particles toward moving targets.
	bool homing = true;
	/// Route particles above intervening terrain.
	bool groundClamp = true;
	/// Fire a burst when reclaiming a unit finishes.
	bool reclaimBurst = false;
	/// Send batched lifecycle updates to LuaUI (used by deferred-lighting widgets).
	bool luaUpdates = false;
	/// Global emission multiplier, 0-1.
	float rate = 0.32f;
	/// Dissolve particles whose target is lost; see TargetLostFadeConfig.
	bool targetLostFade = true;

	// --- compiled-in defaults, tuned here ---------------------------------
	EmissionConfig emission;
	ReclaimBurstConfig reclaimBurstParams;
	HomingConfig homingParams;
	GroundClampConfig groundClampParams;
	TargetLostFadeConfig targetLostFadeParams;
	VisibilityConfig visibility;
	RenderConfig render;
	AppearanceConfig appearance;
	LuaUpdateConfig luaUpdate;

	/// Bumped whenever anything changes, so caches keyed on it can invalidate.
	std::uint32_t generation = 1;
};

/// The single live instance. Read-only outside of this translation unit.
const Config& GetConfig();

/// Reads the springsettings. Called once during startup.
void InitConfig();

/// Re-reads the springsettings after a live change. Returns true if anything changed.
bool ReloadConfigSetting(const std::string& key);

/// Names of the springsettings the effect wants ConfigNotify callbacks for.
const std::vector<std::string>& GetObservedConfigKeys();

} // namespace NanoParticles
