---
layout: post
title: Release 105-2511
parent: Changelogs
permalink: changelogs/changelog-105-2511
author: sprunk
---

This is the changelog since version 2314 **until release 2511**, which happened on 2024-06-07.

# Caveats
These are the entries which may require special attention when migrating:
* some animations are now multi-threaded. It shouldn't cause desyncs, but an `AnimationMT` springsetting has been provided to disable it, just in case. See below.
* when building the engine via CMake, only native C++ AIs are now built by default.
* unit def `trackStretch` values now treated reciprocally compared to previous, i.e. a stretch factor of 2 now means the track is stretched x2 longer (was squeezed x0.5 shorter previously).
* ground decals may behave a bit different because there's a new implementation. There are a handful of known issues, see the decals section above.
* ground decals may no longer work on potato hardware.
* instead of 4 default explosion decals in basecontent (`bitmaps/scars/scarN.bmp`, 1-4), there's 2 new normal-mapped ones (`bitmaps/scars/scarN.tga` and `bitmaps/scars/scarN_normal.tga`, 1-2).
Note the format change from BMP to TGA.
You might want to check your `gamedata/resources.lua` to see if you're referencing the old default scars (either explicitly or e.g. autogenerating from the `bitmaps/scars` folder).
You might also want to produce more to counteract the reduced variety.
* default targeting priority for typed units no longer has a ±30% random component. Use `gadget:AllowWeaponTarget` to get back the previous behaviour.
* engine line-move formations now use unit type power rather than a function of cost (60M+E) to distribute units around the formation. Power defaults to 60M+E and all known games use Lua customformations.
* burst weapons can now be made to respect firing angle restrictions. See the burst section below.

# Features
* The `select` action now composes `IdMatches` filters as *OR* statements see [The select command]({{ site.baseurl }}{% link articles/select-command.markdown %}#idmatches_string) for further reference.
* added a new optional boolean parameter to `Spring.GetUnitHeading`, default false. If true, the value returned is in radians instead of the TA 16-bit angular unit.
* added a new callin, `GameFramePost(number frame)`. This is the last callin in a sim frame (regular `GameFrame` is the first).
* added a new callin, `UnitArrivedAtGoal(unitID, unitDefID, teamID)`, for when a unit arrives at its movement goal (incl. raw move).
Use for batching events that happened during the frame to be sent to unsynced for use in draw frames before the next sim frame.
* added `Spring.GetTeamMaxUnits(teamID) -> number maxUnits, number? currentUnits`. The second value is only returned if you have read access to that team (max is public).
There is currently no corresponding Set.
* added `GAME/ShowServerName` startscript entry. If not empty, the initial connection screen's "Connecting to: X" message will display the value of that option instead of the host's IP.
* added `Spring.GetModOption(string key) -> string? value`. Returns a single modoption; replaces the `Spring.GetModOptions().foo` pattern for greater performance.
* added `Spring.GetMapOption(string key) -> string? value`, ditto for a single mapoption.
* add a `Patrolling` selection filter. Applies to units that have a Patrol command among the first 4 commands (remember Patrol prepends Fight, which itself prepends Attack).
* the `SpringDataRoot` springsetting now accepts multiple data root paths. Split them via ';' (on Windows) or ':' (elsewhere).
* `Spring.GetUnitWorkerTask` now works on factories.
* major performance (incl. loading time and Lua memory usage) improvements.
* further Tracy instrumentation.

### New ground decals
* added normal mapping to all decals (explosion scars, building plates, tank tracks).
Name the normalmap the same as the base decal but with `_normal` at the end., e.g. when you use `scar1.tga` as a decal diffuse/alpha, use `scar1_normal.tga` as a normal map texture.
* alpha channel of normal map is now used for explosion decal glow. Scales with weapon damage. Full opacity (255) represents the hottest part.
* added a Lua interface to create and edit decals. Check the [control](https://beyond-all-reason.github.io/spring/ldoc/modules/UnsyncedCtrl.html#Decals)
and [read](https://beyond-all-reason.github.io/spring/ldoc/modules/UnsyncedRead.html#Decals) parts in the API listings.
* added a shader interface for decal rendering. No documentation of uniforms/attributes/etc seems to exist at the moment,
but you can look up the default shader implementation ([fragment](https://github.com/beyond-all-reason/spring/blob/BAR105/cont/base/springcontent/shaders/GLSL/GroundDecalsFragProg.glsl), [vertex](https://github.com/beyond-all-reason/spring/blob/BAR105/cont/base/springcontent/shaders/GLSL/GroundDecalsVertProg.glsl)).
* replaced scar bitmaps in basecontent. See the caveats section above.
* unit def `trackStretch` values now treated reciprocally compared to previous, i.e. a stretch factor of 2 now means the track is stretched x2 longer (was squeezed x0.5 shorter previously).
* ground decals may no longer work on potato hardware.
* known issue: tracks/footprints no longer maintain the texture offset on stutter-step.
* known issue: building decals may not render correctly underwater.

### Bursts and firing angles
* added a new weapon (not weapon def, the weapon table in a unit def) numerical tag, `burstControlWhenOutOfArc`. By setting it to 1 or 2, ongoing bursts (from 2nd shot onwards) can now be made to respect firing cones coming from `mainDir` + `maxAngleDif` (or equivalently from `turret = false` + `tolerance`), and also the other cone from `fireTolerance` compared to the aim-from piece direction.
* if 0, further shots don't respect the cones. This is the default and the previous behaviour.
* if 1, the weapon will fail to produce shots aimed at targets out of the cone, wasting them. Aiming in the cone again will resume producing projectiles (but not "refund" the wasted ones, the overall timing of the burst is kept). The `EndBurst` script event still runs even if the ending shot was wasted, but per-shot events don't run for wasted shots.
* if 2, the weapon will just ignore attempts to aim outside the cone and keep firing in whatever direction its aim-from piece was pointing at.

### More interfaces in `defs.lua`
The following functions are now available in the `defs.lua` phase:
* `Spring.GetMapOption` (new, see above)
* `Spring.GetModOption` (new, see above)
* `Spring.GetTeamLuaAI`
* `Spring.GetTeamList`
* `Spring.GetGaiaTeamID`
* `Spring.GetPlayerList`
* `Spring.GetAllyTeamList`
* `Spring.GetTeamInfo`
* `Spring.GetAllyTeamInfo`
* `Spring.GetAIInfo`
* `Spring.GetTeamAllyTeamID`
* `Spring.AreTeamsAllied`
* `Spring.ArePlayersAllied`
* `Spring.GetSideData`

### Water
* add `Spring.GetWaterLevel(x, z) -> number waterHeight`. Similar to `Spring.GetGroundHeight` except returns the height of water at that spot.
Currently water height is 0 everywhere. Use where appropriate to be future-proof for when Recoil gets dynamic water, or just to give a name to the otherwise magic constant.
* add `Spring.GetWaterPlaneLevel() -> number waterPlaneHeight`. Ditto, except encodes that you expect the water to be a flat plane.
Use as above but where you have no x/z coordinates.

### Interpolated game seconds
* added `Spring.GetGameSecondsInterpolated() -> number` function to unsynced Lua.
Unlike `GetGameSeconds` it flows during rendering. Unlike `GetDrawSeconds` its flow reflects gamespeed (incl. stopping when paused).
And unlike `GetGameFrame` and `GetFrameTimeOffset` it is in a natural unit instead of the technical frame abstraction.
* shaders: changed the `timeInfo.z` uniform from draw frame number to interpolated game seconds.

### Skidding
* units will skid if hit with impulses sufficiently large in the direction opposite their movement vector. Previously units would only skid on large impulses that hit their sides.
* added `Spring.SetUnitPhysicalStateBit(number unitID, number stateBit) -> nil`, for setting a unit's physical state bit. Gotta use magic constants for bits at the moment.
Use for example to unattach units from the grounds and trigger skidding.
* added unit def: `rollingResistanceCoefficient`, used to reduce a unit's speed when exceeding their normal max speed. Defaults to 0.05.
* added unit def: `groundFrictionCoefficient`, used to reduce a unit's speed when skidding. Defaults to 0.01.
* added unit def: `atmosphericDragCoefficient`, reduces a unit's speed when skidding and exceeding speed maximum. Defaults to 1.0.

### Debugging tools
* `/debugcolvol` now also draws the selection volume, in green.
* `/track 1 unitID unitID unitID` lets you specify units to track via the command. If no unitID is given it still behaves the old way and uses the current selection.
* added an `AnimationMT` boolean springsetting. Defaults to true. Set to false if there's desync problems. Will be removed after some time, when MT animations prove to be sync-safe.
* COB piece errors say the culprit's unit def name (since the same script can be shared by many units, with different piece lists).

# Fixes
* inserting (via `CMD.INSERT`) a "build unit" command to a factory with a SHIFT and/or CTRL modifier (i.e. x5/20) will now work correctly (previously ignored and went x1).
* fix an issue where a unit that kills something via `SFX.FIRE_WEAPON` would sometimes continue to shoot at the location it was standing at at the time.
* fixed `VFS` functions that deal with file paths being exceedingly slow.
* fixed `Spring.GetTeamUnitsByDefs` revealing much more information than it should.
* `Spring.GetUnitWeaponState(unitID, "burstRate")` now correctly returns fractional values (was only full integers before).
* fixed camera rotation via the middle mouse button canceling unit tracking mode.
* fixed timed out clients sometimes managing to send packets and claim a desync.
