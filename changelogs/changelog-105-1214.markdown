---
layout: post
title: Release 105-1214
parent: Changelogs
permalink: changelogs/changelog-105-1214
author: sprunk
---

The changelog since release 105-966 **until release 105-1214**, which happened in October 2022.

## Caveats
* there is now only one `UnitUnitCollision` event for each unit pair, and return values from UnitUnitCollision are now ignored!
* `vsync` springsettng now defaults to -1
* ships now obey `upright = false` when pulled out of water (still always upright when floating).
* killed most of ARB shader fallbacks

## Features and fixes

### Unit collisions
* multi-threaded unit steering and collisions.
* there is now only one `UnitUnitCollision` event for each unit pair, and return values from UnitUnitCollision are now ignored!
* added modrule, `movement.groundUnitCollisionAvoidanceUpdateRate`, for controlling steering performance vs quality tradeoff load. Reduce to get better quality at the cost of perf. Default value is 3 for cycling through all units over 3 sim frames.
* added modrule, `movement.unitQuadPositionUpdateRate`. Similar perf tradeoff to the above, but affects collision accuracy (incl. with projectiles). In particular, if this value isn't 1 then sometimes beamlasers/lightning can visibly ghost through units. Defaults to 3.
* added modrule, `movement.maxCollisionPushMultiplier`, limits the speed (as a multiplier relative to their base speed) units can push each other at. Defaults to infinity/unlimited.

### Particles
* add animated "flipbook" CEGs: `animParams = "numX, numY, animLength"`. Enabled for BitmapMuzzleFlame, HeatCloudProjectile, SimpleParticleSystem and GroundFlash.
* some semi-transparent objects can cast colored shadows
* fixed smoke-trails with a non-standard smoke period
* `/dumpatlas` for now only works with `proj` argument to dump projectiles atlases to disk

### Units on slopes
* ships now obey `upright = false` when pulled out of water (still always upright when floating).
* added `upDirSmoothing` to unit def so units could update their upward vector smoother than by default
* floating mobiles can be built over seafloor slopes

### Bugger off!
* add `Spring.SetFactoryBuggerOff(unitID, bool? active, number? distanceFromFactoryCenter, number? radius, number? relativeHeading, bool? spherical, bool? forced) → bool active`. Forced means it also asks push-resistant units to move. Everything is optional.
* add `Spring.GetFactoryBuggerOff(unitID) → active, distance, radius, relativeHeading, spherical, forced`, same units as above.
* add `Spring.BuggerOff(x, y = groundHeight, z, radius, teamID, spherical = true, forced = true, excludeUnitID = nil, excludeUnitDefIDArray = nil) → nil`, for standalone bugger-off in-world (like happens when units stand on a construction site).
* `/debugcolvol` now shows factory bugger off, in cyan. Standalone bugger off not shown.

### Key press scan codes
* add `Spring.GetPressedScans() → {[scanCode] = true, [scanName] = true, ...}`, similar to `Spring.GetPressedKeys` but with scancodes
* add `Spring.GetKeyFromScanSymbol(scanSymbol) → string keyName` that receives a scancode and returns the user's correspondent key in the current keyboard layout
* add `wupget:KeyMapChanged() → nil` callin for when the user switches keyboard layouts, for example switching input method language.

### Selection box
* add `Spring.SetBoxSelectionByEngine(bool) → nil`. If set to false, engine won't apply selection when you release the selection box.
* add `Spring.GetSelectionBox() → minX, maxY, maxX, minY` (sic) returning the screen coordinates of the selection box. Nil if not dragging a box.

### Rendering internals
* lots of engine internals (in particular, fonts) use modern approaches to rendering. Expect speed-ups.
* applied more robust algorithm of culling shadow projection matrix. The net result is that shadows should not disappear any longer on small maps, at oblique camera angles, etc.
* allow skipping the use of Texture Memory Pool to save ~512 MB (by default) of statically allocated memory. Note this might be detrimental due to possible memory fragmentation. See `TextureMemPoolSize` springsettings
* removed most of the sanity checks from window positioning code (reverted back to 105.0 like code). Now the engine won't resist to resizing the window as the user/widget wants. There are still some rules in place, but they shouldn't be too restrictive.
* FontConfig should be more robust upon failure and don't crash that often on FC errors
* fix multiple camera issues.
* fix the hardware cursor on Linux/Wayland.
* fixed incorrect cubemap texture mapping

### Rendering API
* add `DrawGroundDeferred` callin such that bound g-buffers could be updated from say game side decals renderer.
* projectiles transformation matrices are now available through LuaVBO

Added the following `GL.` constants:
* `TEXTURE_{1,2,3}D`
* `TEXTURE_CUBE_MAP`
* `TEXTURE_2D_MULTISAMPLE`
* `COLOR_ATTACHMENT{0...15}{,_EXT}`
* `DEPTH_ATTACHMENT{,_EXT}`
* `STENCIL_ATTACHMENT{,_EXT}`

### Miscellaneous
* add `Spring.GetSyncedGCInfo(bool forceCollectionBeforeReturning = false) → number usage in KiB`. Use from unsynced to query the status of synced memory occupation and force garbage collection
* introduced `RapidTagResolutionOrder` option to control priority of domains when resolving rapid tags in case multiple Rapid mirrors are used at the same time.
* `/dumprng` switch to tap out the state of synced RNG, works similar to `/dumpstate`
* skirmish AI interface: add FeatureDef_isAutoreclaimable
* fix the 1 second pause/freeze players experience when a player or spectator disconnects from the match.
* fix SaveLoad hang in case when AllyTeams > 2 and one or more teams died.
* fix SaveLoad memory leaks.

## pr-downloader
* add options to override rapid and springfiles url.
* add option to download from Rapid without using `streamer.cgi`, but files directly. See https://github.com/beyond-all-reason/pr-downloader/pull/9
* improve file IO performance by multithreading it (big performance boost on Windows)
* fix handling of unicode paths on Windows
* fix finding and loading of SSL certificates on multiple Linux distributions (on Windows also switches to system certificate storage)
