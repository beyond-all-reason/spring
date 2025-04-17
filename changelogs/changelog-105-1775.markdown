---
layout: post
title: Release 105-1775
parent: Changelogs
permalink: changelogs/changelog-105-1775
author: sprunk
---

The changelog since release 105-1544 **until release 105-1775**, which happened in June 2023.

## Caveats
* `Spring.GetSelectedUnits{Sorted,Counts}` no longer returns the `n` inside the table, it's now the 2nd return value from the function
* LuaUI can receive attacker info in `UnitDamaged` and similar, when appropriate. Set them to `nil` in the widget handler (or just don't pass them to widgets) if you don't want this behaviour.
* remove the deprecated `Game.allowTeamColors` entry that was always `true`.
* PNG is now the default `/screenshot` format.
* new Recoil icons and loadscreens.
* Due to the introduction of skinning/bones (see below), the maximum number of pieces per model is limited by 254 pieces
* unit shader changes required related to skinning and bones. This requires updates of code that calls `vbo:ModelsVBO()` such that you'd replace:
 ```
 layout (location = 5) in uint pieceIndex;
 ```
 with
 ```
layout (location = 5) in uvec2 bonesInfo; // boneIDs, boneWeights
#define pieceIndex (bonesInfo.x & 0x000000FFu)
```

## Features and fixes

### Catching up performance
* CPU usage reserved for drawing and minimum frames draw by second are now configurable via springsettings: `MinSimDrawBalance` which controls the minimum CPU fraction dedicated to drawing; more means more FPS but slower catch-up (defaults to 0.15) and `MinDrawFPS` which controls the minimum desired FPS below which it won't go (defaults to 2).
* fixed how this drawing/simulating proportion interacts with the "slow the game for laggers" feature.

### Modelling
* added skinning and bones. Not supported for `.s3o` or `.3do`, supported by `.dae` (the other formats supported by the engine are fairly uncommon and untested, but those that can carry bone information should work as well). No extra work needed, bones created by an industry standard tool (for example Blender) should work out of the box.
* shader change is needed, see above in the caveats section.
* allow instanced rendering without instance VBO attached. `gl_InstanceID` can be used to index into an SSBO or used to produce instance attributes algorithmically.
* bump model pool from 2560 to 3840 and log when it gets exhausted
* bump up 3do atlas size (4k^2)
* change default `MaxTextureAtlasSize{X,Y}` springsettings from 2048 to 4096.
* fix 3do shattered pieces rendering (no texture)
* fix AMD / Windows issues with invalid "under-construction" model rendering
* fix invalid asset checksum calculations

### Terrain rendering
* add the `wupget:DrawShadowPassTransparent() → nil` and `wupget:DrawWaterPost() → nil` callins
* infotexture shaders (F1, F2 etc) are more tolerant to potato drivers
* improved terrain shadow quality and visibility checks.
* add `/debugVisibility` for debugging the visible quadfield quads (for engine devs, mostly useless otherwise)
* add `/debugShadowFrustum` to draw the shadow frustum on the minimap (ditto; also beware, doesn't exactly correspond to the actual camera frustum so probably doesn't have much use outside shadows specifically due to minor bugs)
* fix the overview camera not being centered in 2nd and subsequent games if the terrain is different
* fix bump water and terrain rendering on Intel

### Selecting units
 * add `InGroup_N` filter for selections, to select given control group. For example: `Visible+_InGroup_1+_ClearSelection_SelectAll`
 * add `group select` and `group focus` group subactions. Note that the previous `bind X group N` is equivalent to `bind X group select N` + `bind X,X group focus N`. The old `group N` still works.

### Unit defs: weapon
 * add `weaponAimAdjustPriority` numerical tag to weapon (note, NOT weapon def! This is the table inside a unit def, where e.g. target categories are). This is a multiplier for the importance of picking targets in the direction the weapon is already aiming at. Defaults to 1.
 * and `fastAutoRetargeting` boolean tag to weapon. Makes the unit acquire a new target if its current ones dies (the tradeoff is the perf cost, and a buff if applies to existing units). Defaults to false (target acquisition check every 0.5s).

### Unit heading
 * changed how unit rotation works. This should make cases like spiders pathing across near-vertical peaks preserve heading better
 * add `Spring.Set{Unit,Feature}HeadingAndUpDir(thingID, heading, x, y, z) → nil`, note that the previously existing `Spring.SetUnitDirection` only receives one vector so does not allow setting an object's rotation unambiguously.

### Pathing
* TKPFS merged into the default HAPFS.
* add a new modrule, `system.pathFinderUpdateRateScale` (default 1.0), to control pathfinder update rate. Note that it gets removed sometime after 1775.
* the pathing overlay (F2) update rate increased x4.
* engine line-move now tries to maintain relative unit position to some extent (so equivalent units don't cross each other).
* fix various pathing quality issues and improve performance.

### Miscellaneous API changes
* allow shallow recursion in `Spring.TransferUnit` (16 levels deep, same as for other callouts)
* add `Game.footprintScale`, the footprint multiplier compared to defs
* add `Game.buildSquareSize`, the building alignment grid size
* add `system.LuaAllocLimit` modrule to control global Lua alloc limit, default 1536 (in megabytes)
* `Spring.GetPlayerInfo` returns a 12th value, boolean isDesynced. Keep in mind the 11th will be `nil` if you opt out of the customkeys table (i.e. the new 12th value does NOT get "pushed back" to fill the "missing" spot)
* add `Spring.GetFeaturesInScreenRectangle(x1, y1, x2, z2) → {featureID, featureID, ...}`, similar to the existing one for units
* `Spring.SetProjectileCEG` now accepts numerical cegID in addition to CEG name
* add `Game.demoPlayName` to get the filename of the running replay.
* add 7th boolean param to `Spring.MarkerErasePosition`, which makes the command erase any marker when `localOnly` and the current player is spectating. This allows spectators to erase players markers which otherwise they can't do outside of `/clearmapmarks`.
* `Spring.SetActiveCommand(nil)` now cancels the command (previous method of `-1` still works)
* add `StoreDefaultSettings` springsetting, default false. If enabled, settings equal to their defaults will still be saved. This means that `Spring.GetConfigInt` et al does not need to do a nil check for the defaults, but also the defaults won't get automatically updated if they change since they already have a concrete value.
* reimplement Lua mem pools, controlled by the `UseLuaMemPools` springsettings.

### Miscellaneous
* added Tracy support for debugging.
* default minimum sim speed changed from 0.3 to 0.1
* add `--list-[un]synced-commands` flag to dump the command list when running the exe directly
* fixed `/iconsAsUI 1` leaking information about dead ghosted buildings.
* fixed piece projectiles (debris flying off from exploding units), and weapon projectiles with `collideFireBase` set to true and no other `collisionX` set to false, failing to collide with units.

## pr-downloader
* improved performance of verifying if there is an update needed: it now takes ~0.25s on all OSes
* introduce custom X-Prd-Retry-Num to inform server at which retry are we when fetching files
* fixed sdp downloading behavior when all files are already present
* fixed progress bar in presence of redirects and when total download size is >2GiB
* fixed `--disable-all-logging` to actually do that
* fixed the handling of Unicode env vars on Windows
