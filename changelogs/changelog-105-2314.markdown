---
layout: post
title: Release 105-2314
parent: Changelogs
permalink: changelogs/changelog-105-2314
author: sprunk
---

This is the changelog since release 1775 **until release 2314**, which happened on 2024-02-13.

# Caveats
These are the entries which may require special attention when migrating:

### Removals
* removed `gl.GetMatrix` and the interface accessible from the returned matrix object. Apparently these were unused, so we offer no replacement guide at the moment.
* removed `spairs`, `sipairs` and `snext`. These have been equivalent to the regular `pairs`, `ipairs` and `next` for years now, use the regular versions instead.
You can replace these functions before migrating, and known existing games have already received patches to do so.
* removed `VFS.MapArchive` and `VFS.UnmapArchive`. They were very sync-unsafe. Hopefully they will be back at some point, but no timeline is available yet. Use `VFS.UseArchive` in the meantime.
* removed LuaUI's access to `Script.LuaRules` and `Script.LuaGaia`.
* removed the recently added `Spring.MakeGLDBQuery` and `Spring.GetGLDBQuery`.

### Behaviour changes
* QTPFS had a major overhaul, with multiple modrule changes and behaviour changes. See the section below.
* many invalid def entries now cause the unit to be rejected; on the other hand, many (in particular, metal cost and weapon damage) can now be 0. Watch out for division by 0!
Check the "def validity checks" section below for details.
* nanoturret (immobile builder) build-range now only needs to reach the edge of the buildee's radius instead of its center. Mobile builders already worked this way.
* raw move via `Spring.SetUnitMoveGoal` is now considered completed not only when the unit reaches its goal but also when it touches another unit who is at the goal. As of 105-2314 there is no easy way to detect that this happened.
* screenshots are postfixed with UTC timestamp instead of number.
* add `SMFTextureStreaming` boolean springsetting, defaults to false. If true, dynamically load and unload SMF Diffuse textures, which saves VRAM, but worse performance and image quality.
Previous behaviour was equivalent to `true`, so if you get VRAM issues try changing it.
* it's now possible to play fixed and random start positions with more teams than the map specifies.
The extras are considered to start in the (0, 0) corner and it is now up to the game to handle this case correctly.
* the `movement.allowGroundUnitGravity` mod rule now defaults to `false`. All known games have an explicit value set, so this should only affect new games.
* `/ally` no longer announces this to unrelated players via a console message. The affected players still see one.
Use the `TeamChanged` call-in to make a replacement if you want it to be public.
* manually shared units no longer receive the Stop command. Use the `UnitGiven` callin to get back the previous behaviour.
* `DumpGameStateOnDesync` springsetting is now enabled by default

### Deprecation
No changes yet, but these will happen in the future and possibly break things.

* the `acceleration` and `brakeRate` unit def entries are scheduled for a unit change from elmo/frame to elmo/second. There is no change yet,
but if you prefer not to have to add processing later you might want to change to `maxAcc` and `maxDec` respectively (which will stay elmo/frame).
* the `CSphereParticleSpawner` (alias `simpleparticlespawner`) CEG class is scheduled for removal. It can be entirely drop-in replaced with `CSimpleParticleSystem` (alias `simpleparticlesystem`)
since it has always had the same behaviour, just different internal implementation. Known games using the class will receive PRs before this happens.
* there are now explicit facilities for generating a blank map (see below). Existing "random" map generator (that always produced a blank map) stays as-is,
but may be repurposed into a "real" random map generator sometime in the future (not immediately planned though).
* `UsePBO` springsetting is now deprecated. It already did nothing though.

# QTPFS

The QTPFS pathfinder has received a large overhaul. There are major improvements in both quality (fewer cases of units getting stuck, cutting corners etc.)
and performance (processing speed, memory use, even disk usage). Every facet of QTPFS should generally work better. Try it out!

* debug path drawer now draws into the minimap, showing the map damage updates waiting to be processed. The more intense the colour, the more layers (MoveTypes) that still need to process the change.
* added modrule, `system.pfRepathDelayInFrames`, which controls how many frames at least must pass between checks for whether a unit is making enough progress to its current waypoint
or whether a new path should be requested. Defaults to 60 (2 seconds). Adjust to find the right balance between how quickly units can get unstuck and how much CPU power is used.
Smaller intervals increase the chance of slow units triggering unnecessary re-pathing requests, but reduces the chance that players may believe a unit is getting stuck and is not handling itself well.
* added modrule, `system.pfRepathMaxRateInFrames`, which controls the minimum amount of frames that must pass before a unit is allowed to request a new path. By default, it is 150 frames (5 seconds).
This is mostly for rate limiting and prevent excessive CPU wastage, because processing path requests are not cheap.
* added modrule, `system.pfUpdateRateScale`. This is a multiplier for the update rate and defaults to 1. Increase to get faster updates but more CPU usage.
* added modrule, `system.pfRawMoveSpeedThreshold`. Controls the speed modifier (which includes typemap boosts and up/down hill modifiers) under which units will never do raw move,
regardless of distance etc. Defaults to 0, which means units will not try to raw-move into unpathable terrain (e.g. typemapped lava, cliffs, water). You can set it to some positive
value to make them avoid pathable but very slow terrain (for example if you set it to 0.2 then they will not raw-move across terrain where they move at 20% speed or less, and will use
normal pathing instead - which may still end up taking them through that path).
* added modrule, `system.qtMaxNodesSearched`, can be used to limit the absolute number of nodes searched.
* added modrule, `system.qtMaxNodesSearchedRelativeToMapOpenNodes`, can be used to limit the number of nodes searched relative to the number of pathable quads on the map.
The final limit will be the larger between the relative and the absolute limits.
* added modrule, `system.pfHcostMult`, a float value between 0 and 2, defaults to 0.2. Controls how aggressively the pathing search prioritizes nodes going in the direction of the goal.
Higher values mean pathing is cheaper, but can start producing degenerate paths where the unit goes straight at the goal and then has to hug a wall.
* added modrule, `system.qtRefreshPathMinDist`, can be used to configure the minimum size a path has to be in order to be eligible for an automatic incomplete path repath.
* removed modrules: `system.pfForceUpdateSingleThreaded` and `system.pfForceSingleThreaded`. Multithreading has shown itself stable.
* removed modrule: `system.pathFinderUpdateRate`, since `system.pfUpdateRateScale` now serves the same general role but has different units.
* added **map** config entry, set in mapinfo under `(info).pfs.qtpfsConstants` subtable: `maxNodesSearched`, controls the absolute limit for how many nodes to search.
Can only increase the modrule limit, not reduce it. Defaults to 0. Use for maps with large but unconnected areas (think lava & two hills).
* added map config entry, `maxRelativeNodesSearched`. Controls the relative limit compared to total nodes on the map.
Can only increase the modrule limit, not reduce it. Defaults to 0.

# Defs unification

Unit defs (i.e. `/units/*.lua`) and `UnitDefs` (in wupgets) referring to the same thing under different names and sometimes even different units of measurement has always been a point of confusion.
Some of this has been alleviated, with a unified name being available for many mismatched keys. Usually it's one already existing on either "side" of the divide.

## New def keys
The following unit def keys now accept the same spelling as the ones exposed via `UnitDefs`.
The old spelling still works (old → new).
* metalUse → metalUpkeep
* energyUse → energyUpkeep
* buildCostMetal → metalCost
* buildCostEnergy → energyCost
* unitRestricted → maxThisUnit
* name → humanName

These two also accept a new spelling, and both the old and new spellings are in elmo/frame.
However, consider migrating to the new spellings ASAP because the original spellings will be
changed to use elmo/s sometime in the future.
* acceleration → maxAcc
* brakeRate → maxDec

The following unit def keys now accept a spelling and measurement unit
as the one exposed via `UnitDefs` (old → new). The old spelling still works,
and is still in the old measurement unit.
* maxVelocity (elmo/frame) → speed (elmo/second)
* maxReverseVelocity (elmo/frame) → rSpeed (elmo/second)

The following unit def keys now accept a new spelling, which hasn't been previously
available in `UnitDefs`, but which has also been added there in this update.
Old spelling still works.
* losEmitHeight → sightEmitHeight
* cruiseAlt → cruiseAltitude

Added `fastQueryPointUpdate` to weapon (note, this is the entry inside a unit def that also sets target categories and direction; NOT weapon def!).
When enabled, the `QueryWeapon` family of functions in the script is called every frame (instead of every 15 in slow update). This fixes friendly fire for rapid-fire multi-barrel weapons.

## New `UnitDefs` members
The following `UnitDefs` keys now accept the same spelling as the ones
accepted for unit def files. The old spelling still works (old → new).
* tooltip → description
* wreckName → corpse
* buildpicname → buildPic
* canSelfD → canSelfDestruct
* selfDCountdown → selfDestructCountdown
* losRadius → sightDistance
* airLosRadius → airSightDistance
* radarRadius → radarDistance
* jammerRadius → radarDistanceJam
* sonarRadius → sonarDistance
* sonarJamRadius → sonarDistanceJam
* seismicRadius → seismicDistance
* kamikazeDist → kamikazeDistance
* targfac → isTargetingUpgrade

The following keys receive a new spelling which hasn't previously been
available for unit defs, but which has also been added in this update.
Old spellings still work.
* losHeight → sightEmitHeight
* wantedHeight → cruiseAltitude

Added the missing `radarEmitHeight` to UnitDefs. The unit def file key was also already `radarEmitHeight`.

## Def validity checks

Some invalid and/or missing defs are now handled differently.

* negative values for health, (reverse) speed, and metal/energy/buildtime
now cause the unit def to be rejected; previously each was clamped to 0.1
* negative values for acceleration and brake rate now cause the unit def to
be rejected; previously the absolute value was taken
* values (0; 0.1) now allowed for health and buildtime (0 still prohibited)
* values [0; 0.1) now allowed for metal cost (0 now allowed)
* undefined metal cost now defaults to 0 instead of 1
* undefined health and buildtime now each default to 100 instead of 0.1
* weapon `edgeEffectiveness` can now be 1 (previously capped at 0.999)
* unit armor multiplier (aka `damageModifier`) can now be 0 (previously capped at 0.0001)
* damage in weapon defs can now be 0 (previously capped at 0.0001)
* damage and armor can also be negative again (so that the target is healed),
but keep in mind weapons will still always target enemies and never allies,
so avoid using it outside of manually-triggered contexts, death explosions, and such

### Deprecated UnitDefs removal

All deprecated UnitDefs keys (who returned zero and produced a warning) have been removed, listed below:
* techLevel
* harvestStorage
* extractSquare
* canHover
* drag
* isAirBase
* cloakTimeout
* minx
* miny
* minz
* maxx
* maxy
* maxz
* midx
* midy
* midz

# Features and fixes

### Builder behaviour
* nanoturret (immobile builder) build-range now only needs to reach the edge of the buildee's radius instead of its center. Mobile builders already worked this way.
* added `buildeeBuildRadius` unit def entry, the radius for the purposes of being built (placing the nanoframe and then nanolathing).
Negative values make it use the model radius (this is the default). Set to zero to require builders to reach the center.
* added `Spring.Get/SetUnitBuildeeRadius(unitID)` for controlling the above dynamically.
* fixed builders not placing nanoframes from their maximum range.
* units vacating a build area (aka "bugger off") will now try to use the fastest route out.
* bugger off now applies correctly to units sitting outside the area, but with a large enough footprint to block construction.
* added `Spring.GetUnitWorkerTask(unitID) → cmdID, targetID`.  Similar to `Spring.GetUnitCurrentCommand`, but shows what the unit is actually doing,
so will differ when the unit is guarding or out of range. Also resolves Build vs Repair. Only shows worker tasks (i.e. things related to nanolathing).
* `gadget:AllowUnitCreation` now has two return values. The first one is still a boolean on whether to allow creating the unit (no change here).
The new second value is a boolean, if the creation was not allowed, whether to drop the order (defaults to true, which is the previous behaviour).
If set to false, the builder or factory will keep retrying.
* added `Spring.GetUnitEffectiveBuildRange(unitID[, buildeeDefID]) → number`. Returns the effective build range for given builder towards the center of the prospective buildee,
i.e. the same way engine measures build distance. Useful for setting the goal radius for raw move orders.
This doesn't solve all known cases yet (doesn't handle features, or terraform) which are pending a solution; for now, the function returns just the build range if `buildeeDefID` is nil.
* added `Spring.GetUnitIsBeingBuilt(unitID) → bool beingBuilt, number buildProgress`. Note that this doesn't bring new _capability_ because `buildProgress` was already available
from the 5th return of `Spring.GetUnitHealth`, and `beingBuilt` from the 3rd return of `Spring.GetUnitIsStunned`, but it wasn't terribly convenient or intuitive.
* fixed it being possible to place a unit that requires a geothermal vent anywhere.

### Rules params
* added player rules params. Controlled by the new interfaces: `Spring.SetPlayerRulesParam`, `GetPlayerRulesParam` and `GetPlayerRulesParams`,
similar to other existing rules params. There's currently two visibility levels, public and private. A notable difference is that the private level
is only visible to that player, not his allyteam, and not even his (comsharing) team; this is partially for technical reasons and can be changed if need be.
Synced and specs see everything. Not yet available to the Skirmish AI interface.
* added boolean value support to rules params, including the new player rules params.
Skirmish AI and the unit rules param selection filter can read them via existing numerical interface as 0 and 1.

### Map textures
* added `SMFTextureStreaming` boolean springsetting, defaults to false. If true, dynamically load and unload SMF Diffuse textures, which saves VRAM, but worse performance and image quality.
Previous behaviour was equivalent to `true`, so if you get VRAM issues try changing it.
* added `SMFTextureLodBias` numerical springsetting, defaults to 0. In case `SMFTextureStreaming = false`, this parameter controls the sampling lod bias applied to diffuse texture.
* added a 5th integer param to `Spring.GetMapSquareTexture(x, y, lodMin, texName[, lodMax = lodMin])`. It controls the max lod and defaults to the 3rd parameter,
which is now the minimum (instead of being the final value).

### FFA support
* it's now possible to play fixed and random start positions with more teams than the map specifies.
The extras are considered to start in the (0, 0) corner and it is now up to the game to handle this case correctly.
* `/ally` no longer announces this to unrelated players via a console message. The affected players still see one.
Use the `TeamChanged` call-in to make a replacement if you want it to be public.
* added `system.allowEnginePlayerlist`, defaults to true. If false, the built-in `/info` playerlist won't display.
Use for "anonymous players" modes in conjunction with `Spring.GetPlayerInfo` poisoning, or just to prevent ugliness.
* added `Spring.SetAllyTeamStartBox(allyTeamID, xMin, zMin, xMax, zMax) → nil`, sets that allyteam's startbox edges, in elmos.

### VFS
* added a 4th boolean parameter to `VFS.DirList` and `VFS.SubDirs`, defaults to false. If set to true, the search is recursive.
* fixed `VFS.SubDirs` applying the passed pattern to the whole paths instead of just the individual folder names in `VFS.RAW` mode.

### Unit selection
* added `Spring.DeselectUnit(unitID) → nil`.
* added `Spring.SelectUnit(unitID[, bool append]]) → nil`, a single-unit version of `Spring.SelectUnit{Array,Map}` that doesn't require a table.
The unitID can be nil.
* added `Spring.DeselectUnitArray({[any] = unitID, [any] = unitID, ...}) → nil` and `Spring.DeselectUnitMap({[unitID] = any, [unitID] = any, ...}) → nil`.
These are the counterparts to the existing `Spring.SelectUnitArray` and `Spring.SelectUnitMap`.
* the table in `Spring.SelectUnitArray` can now have arbitrary keys. Previously they had to be numbers, but the table did not actually have to be an array.

### Root pieces
* added `Spring.GetModelRootPiece(modelName) → number pieceID` which returns the root piece.
* added `Spring.GetUnitRootPiece(unitID) → number pieceID` and `Spring.GetFeatureRootPiece(featureID) → number pieceID`, likewise.

### Colored text
* added an inline colour code `\254`, followed by 8 bytes: RGBARGBA, where the first four describe the following text colour and the next four the text's outline.
* added the `Game.textColorCodes` table, containing the constants `Color` (`\255`), `ColorAndOutline` (the newly added `\254`), and `Reset` (`\008`).

### Miscellaneous additions
* add `Spring.IsPosInMap(x, z) → bool inPlayArea, bool inMap`. Currently, both of the returned values are the same and just check whether the position
is in the map's rectangle. Perhaps in the future, or if a game overrides the function, there will be cases of limited play area (think SupCom singleplayer
map extension; 0 A.D. circular maps; or just an external decoration area).
* add `Spring.GetFacingFromHeading(number heading) → number facing` and `Spring.GetHeadingFromFacing(number facing) → number heading` for unit conversion.
* added `wupget:Unit{Entered,Left}Underwater(unitID, unitDefID, teamID) → nil`, similar to existing UnitEnteredWater.
Note that EnteredWater happens when the unit dips its toes into the water while EnteredUnderwater is when it becomes completely submerged.
* add new `/remove` cheat-only command, it removes selected units similar to `/destroy` except the units are just removed (no wreck, no death explosion).
* added new startscript entry: `FixedRNGSeed`. Defaults to 0 which means to generate a random seed for synced RNG (current behaviour).
Otherwise, given value is used as the seed. Use for reproducible runs (benchmarks, mission cutscenes...).
* added `Script.DelayByFrames(frameDelay, function, args...)`. **Beware**, it's `Script`, not `Spring`! Runs `function(args...)` after a delay of the specified number of frames (at least 1).
Multiple functions can be queued onto the same frame and run in the order they were added, just before that frame's `GameFrame` call-in. Use to avoid manual tracking in GameFrame.
* added `Spring.GetUnitSeismicSignature(unitID) → number` and `Spring.SetUnitSeismicSignature(unitID, number newSignature) → nil`.
* added `Spring.SetUnitShieldRechargeDelay(unitID, [weaponNum], [seconds]) → nil`. Resets a unit's shield regeneration delay.
The weapon number is optional if the unit has a single shield. The timer value is also optional: if you leave it nil it will emulate a weapon hit.
Note that a weapon hit (both via `nil` here, and "real" hits) will never decrease the remaining timer, though it can increase it.
An explicit numerical value always sets the timer to that many seconds.
* added `MaxFontTries` numerical springsetting, defaults to 5. Represents the maximum number of attempts to search for a glyph replacement when rendering text (lower = foreign glyphs may fail to render, higher = searching for foreign glyphs can lag the game). The search is optimized to look in the "best" places so the chance of finding the glyph does not increase linearly alongside search time.
* added `GL.DEPTH_COMPONENT{16,24,32,32F}` constants.
* added the following `GL` constants for use in `gl.BlendEquation`: `FUNC_ADD`, `FUNC_SUBTRACT`, `FUNC_REVERSE_SUBTRACT`, `MIN` and `MAX`.
* added `Spring.GetWindowDisplayMode() → number width, number height, number bitsPerPixel, number refreshRateHz, string pixelFormatName`.
The pixel format name is something like, for example, "SDL_PIXELFORMAT_RGB565".
* added `/dumpatlas 3do`.

### Blank map generation
* new way to specify blank map colour: `blank_map_color_{r,g,b}`, number 0-255.
* a new start-script entry, `InitBlank` (at root level). Alias for existing `MapSeed`.
* new built-in mapoptions, `blank_map_x` and `blank_map_y`, aliases for existing `new_map_x/y`.

### Weapon fixes
* fix `Cannon` type weapons aiming at the (0, 0) corner of the world if they can't find a physical firing solution due to target leading
* fix `Cannon` type weapons being too lenient in friendly-fire avoidance when firing over allies
* fix `MissileLauncher` weapons with high `trajectoryHeight` not performing ground and ally avoidance correctly
* fix `MissileLauncher` weapons with high `trajectoryHeight`, zero `turnRate` and high `wobble` having an unstable trajectory and prematurely falling down when firing onto higher elevations
* fix `DGun` weapon type projectile direction (previously shot at an angle that would be valid from the `AimFromWeapon` piece and not the `QueryWeapon` piece)

### Basecontent fixes
* moved the `cursornormal` cursor from the "Spring cursors" archive to basecontent. The significance
of this is that `modinfo.lua` is now sufficient for an archive to be a valid Recoil game that doesn't crash.
* fixed the initial spawn gadget.
* fixed action handler key press/release events.

### Miscellaneous fixes
* fixed hovercraft/ship movement types being able to encroach onto land across sheer cliffs.
* fixed skirmish AI API getTeamResourcePull (used to return max storage instead).
* `Spring.SetSunDirection` no longer causes broken shadows if you pass an unnormalized vector.
* fixed being unable to drag-select units with `/specfullview 0`.
* fixed weirdly-boned Assimp (`.dae`) models being loaded incorrectly.
* fixed COB `SetMaxReloadTime` receiving a value 10% smaller than it was supposed to.
* fix screenshots saved as PNG having an inflated file size via a redundant fully-opaque alpha channel.
* fix clicking during loadscreen sometimes registering as startbox placement later.
* fix a crash on self-attach via `Spring.UnitAttach`.
* fix `Spring.MoveCtrl.SetMoveDef` not working with numerical movedef IDs.
