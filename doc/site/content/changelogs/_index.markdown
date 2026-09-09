+++
title = "Running changelog"
[cascade]
  [cascade.params]
    type = "docs"
+++

This is the bleeding-edge changelog since version 2026.07, for **pre-release 2026.09**. This is a minor release that contains mostly capabilities for efficient rendering and a bunch of fixes.

## Caveats

 * LuaGaia entry points (`LuaGaia/main.lua`, `LuaGaia/draw.lua`) are now loaded by the engine with `VFS.GAME .. VFS.MAP` load mode, from `VFS.MAP`.
If you have these files in your game archive they will start being loaded, possibly breaking maps that ship some LuaGaia logic (but also giving you control).

### Deprecation notices

 * basecontent's `LuaHandler/Utilities/keysym.lua` and the loose `LuaUI/Headers/keysym.h.lua‎` distributed with engine installs are deprecated and will be removed.
For now they work unchanged, but print lots of helpful migration suggestions when the `KEYSYMS` table is used.

## Features

### Key bindings
 * added `wupget:KeyBindingsChanged()` unsynced callin. Called whenever a valid `/bind` or similar command is called.
 * `Spring.GetKeyCode` now supports most of keys defined in `keysyms.lua`, except a few that SDL2 dropped.
 * fixed `/unbind` also removing longer chains that share the last key.

### "Spring" camera smoothness

The "spring" camera received changes aimed at improving the smoothmesh tracking mode (`CamSpringTrackMapHeightMode = 2`). The aim is to keep the advantages of smoothmesh when zoomed out and not have the camera get thrown around by every little bump when zoomed in.

 * added `CamSpringSmoothMeshBlendMinDist`, default 150 (in elmos). Below this height above ground, the smoothmesh tracking mode will follow the raw terrain. Takes precedence over the "max" below.
 * added `CamSpringSmoothMeshBlendMaxDist`, default 600 (in elmos). Above this height above ground, the smoothmesh tracking mode will follow the smoothmesh.
 * between these two heights, the camera will track an interpolation between the two relevant meshes.
 * tracking now applied for camera movement via zooming, dragging the window on the minimap, and Lua-based movement (previously only edge scrolling).
 * fixed interpolation issues with the existing tracking (over- and under-shooting, framerate dependence, visible jolts).

### Unit icon draw ordering
 * add optional `drawOrder` number (default 0) to `gamedata/icontypes.lua` entries; icons with higher `drawOrder` are drawn on top
of lower ones in the world, screen ("icons as UI") and minimap draw paths. Icons sharing a `drawOrder` keep their stable creation order.
Games that define no `drawOrder` values skip sorting entirely.
 * add `UnitIconsSortedByDepth` config bool (default false). When enabled, overlapping icons are additionally ordered back-to-front
by view depth within equal `drawOrder`; this makes overlap stacking change as units and the camera move.
 * add optional 10th parameter `drawOrder` (default 0) to `Spring.AddUnitIcon`.
 * `Spring.GetUnitIconData`, `Spring.GetIconData` and `Spring.GetAllIconDataArray` now include `drawOrder` when called with `fullData`.

### Built-in GL4 nanospray

Ported BAR's GL4 nanospray widget to be native.

 * added `NanoParticlesGL4` boolean springsetting, default false. Renders nano particles as
  shader-generated 3D shapes with an additive halo instead of textured billboards.
 * added `NanoParticlesNoGeometryShader` boolean springsetting, default false. Forces the instanced
no-geometry-shader renderer. The effect already falls back to it automatically where geometry
shaders are unavailable.
 * added `NanoParticlesRate` numerical springsetting, default 0.32, range 0-1. Emission multiplier.
Emission is proportional to the emitter's `buildSpeed * buildPower` rather than one particle per
work tick, so a builder's spray tracks the work it is actually doing rather than its nano piece
count.
 * added `NanoParticlesHoming` boolean springsetting, default true. Particles follow moving unit
targets, and reclaim/capture particles follow the builder's nano piece.
 * added `NanoParticlesGroundClamp` boolean springsetting, default true. Routes particles above
intervening terrain instead of letting them sink through it.
 * added `NanoParticlesReclaimBurst` boolean springsetting, default false. Emits a one-shot burst
when reclaiming a unit finishes, sized by the unit's metal cost and split across the builders
that contributed.
 * added `NanoParticlesTargetLostFade` boolean springsetting, default true. When the unit a spray
was aimed at is destroyed, cancelled mid-build, or crashing - or, for build and repair spray,
finished and at full health - the particles already in flight keep their course (still homing if
the unit is merely finished) and dissolve, shrinking and fading to nothing over a per-particle
window of roughly 30 frames, instead of flying on into nothing. Reclaim spray does the same when
its builder dies.
 * added `NanoParticlesUpdateLuaUI` boolean springsetting, default false. Sends batched particle
lifecycle events to LuaUI, for deferred-lighting widgets.
 * added `NanoParticlesUpdateLuaUISampleRate` numerical springsetting, default 0.25, range 0-1.
Fraction multiplier for how many particles are reported to LuaUI.
 * added `wupget:NanoParticleUpdate(events, eventCount, gameFrame)` unsynced callin. Batched nano
particle lifecycle events, as one flat numeric array of 13-entry records
`{operation, lightID, px, py, pz, vx, vy, vz, remainingLife, r, g, b, builderBuildSpeed}`.
Operations are 1 = spawn, 2 = update, 3 = remove, 4 = reset. Only fires while both
`NanoParticlesGL4` and `NanoParticlesUpdateLuaUI` are on.
 * added `Engine.FeatureSupport.nanoParticleUpdateCallin` boolean, to detect the above.
 * added `Engine.FeatureSupport.nanoParticlesGL4` boolean, so games can detect that the engine has
the standalone nano particle effect and retire their own Lua implementation of it.

### Efficient rendering
 * add `gl.DrawMiniMapIcons(left, top, right, bottom, iconSize[, allyTeam = local[, fullView = local[, highlightSelected = true]]])`, which draws all unit icons visible
to the given perspective (team colors, radar dots, LOS rules, ghost dimming etc. exactly as on the minimap) for a map-space rectangle, mapped onto the
unit square under the current transform. Composes with `gl.RenderToTexture`/FBOs, so widgets (e.g. picture-in-picture views) can render icon layers
for any camera rect and zoom without maintaining their own icon pipeline.
 * add `gl.CopyImageSubData(name, sourceMipLvl, sx, sy, sz, destinationMipLvl, dx, dy, dz, width, height, depth)` for efficient copying of data
from textures into a Lua texture.
 * add `ProjectileReflectionMinRadius` springsetting, number in elmos, default 0. Particle reflections with a radius smaller than this in elmos are not drawn in water reflections (saves perf).
 * add `ProjectileDrawThreadedFill` boolean springsetting, default true. Adds multi-threading to particle drawing. Toggleable mostly just in case it causes threading trouble.
 * add `ProjectileDrawReuseWaterPasses` boolean springsetting, default true. Reuses below- and above water particle rendering passes, since they contain the same geometry. Toggleable mostly just in case it causes trouble.
 * support loading DX10 DDS textures (BC4, BC5, BC7).

### Misc
 * LuaGaia entry points (`LuaGaia/main.lua`, `LuaGaia/draw.lua`) are now loaded by the engine with `VFS.GAME .. VFS.MAP` load mode, from `VFS.MAP`.
 * add `system.forcedMapGravityStrength` modrule. If set, it overrides the map's `gravity` (same units, elmo/s²)
everywhere the engine uses it, including `Game.gravity`. Unset (nil) keeps the map value.
 * Barbarian AI shipped with the engine updated to v1.6.29, contains a fix for island maps.

### Fixes
 * fixed that attempting loading a save would overflow MacOS' default stack size.
 * fixed `/skip` not doing anything before game start.
 * fixed `pr-downloader` triggering a refresh of rapid repos when downloading non-rapid items like maps or engines.
 * fixed replays waiting for connections from some of the original participants at start (affected replays which contained mid-joining specs).
 * fixed `Spring.SetSunDirection` not notifying lighting consumers, leaving rendered terrain stale until another lighting update.
 * fixed grass not respecting `Spring.SetSunDirection` at all.
 * fixed `MultiScale` and `WaitForScale` missing from the LUS header.
 * fixed `Platform.glVersionNum` having an incorrect value.
 * fixed corrupted/incompatible saves causing a crash at load (still interrupts load, now with an error popup).
 * fixed `Spring.GetProfilerTimeRecord` crashing if given the 2nd boolean argument.
 * fixed the cursor sometimes disappearing after being warped (on Linux/Wayland).
 * fixed impulse applied at an angle causing ground units to lose velocity and stop instead.
 * fixed ships getting incorrect pathmap blocking zones with underwater structures in shallow water.
 * fixed terrain speed bonuses (typemap, e.g. roads or ice) being cancelled by overspeed drag that used the unit's base max speed instead of the terrain-modified effective max speed.
 * fixed BombDropper weapons with long burst sometimes failing to notice they completed a bombing run and making another run at the same target.
 * fixed `/unbind` also removing longer chains that share the last key.
 * fixed some inaccuracies in the website documentation.
 * fixed `unitsync` leaking memory and possibly other resources on shutdown (Lua state was not cleaned up).
 * fixed `VAO:RemoveFromSubmission` having a huge overhead, no longer dependent on how many indices there are in the submission.
 * fixed atlas-related log spam with a minimized window.

### Internals relevant for engine devs
 * enforce power-of-2 memory alignment for analyzing the engine with Address Sanitizer.
 * CMake: make engine test setup optional via new `BUILD_TESTING` option, default enabled.
 * `DemoTool` now builds again.
 * fix various build issues on OpenBSD and Windows.
 * fix some failing unit tests on aarch64. Involves a creg change for loading yielded Lua coroutines.
 * updated EnTT library from v3.10.2 to v3.16.0
