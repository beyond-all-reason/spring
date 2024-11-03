---
layout: post
title: Release 105-2590
parent: Changelogs
permalink: changelogs/changelog-105-2590
author: sprunk
---

This is the changelog since version 2511 **until release 2590**, which happened on 2024-09-30.

# Caveats
These are the entries which may require special attention when migrating:
* removed the `movement.allowDirectionalPathing` modrule. Pathing is now always directional. All known games had directional pathing enabled already.
* unitdef `waterline` is now ignored if the unit has a movedef, and is taken from the movedef via its new `waterline` def, unless the movedef sets the new `overrideUnitWaterline` boolean def to false.

# Features

### Movement
* removed the `movement.allowDirectionalPathing` modrule. Pathing is now always directional, i.e. going downhill does not incur a speed penalty. All known games had directional pathing enabled already.
* added `allowDirectionalPathing` boolean movedef entry, default false. Allows the HAPFS (aka legacy) pathfinder to take direction into account. If false, it will avoid going downhill to the same extent as climbing that slope uphill (note that this is the previous behaviour). Does not apply to QTPFS, which cannot use directional pathing.
* added `preferShortestPath` boolean movedef entry, default false. Makes the QTPFS pathfinder ignore speed modifiers (from any source: typemap, slope, water) when deciding the path. Still avoids 0% modifiers (i.e. won't try to move across completely unpathable terrain). Does not apply to HAPFS, which cannot use this and will always take speed modifiers into account.
* added `waterline` numerical entry to movedef - how deep the unit sits in water, in elmos, similar to the existing unit def waterline. Defaults to 1 for ships, to the unit width (according to the movedef footprint, converted to elmos) for submarines, and 0 for everything else. Overrides the existing `waterline` from the unit def by default.
* added `overrideUnitWaterline` boolean entry to movedef, defaults to true. If set to false, it will use the unit def waterline after all.
* added `separationDistance` numerical movedef entry, default 0. Treated as extra radius for the purposes of colliding with other mobiles during movement (i.e. doesn't apply to impulse-based collisions or to pathing near buildings/terrain). Use to loosen up tight formations without affecting where the unit can path.

### Yardmaps
* added 'u' yardmap tile: not buildable, but pathable. Good replacement for indoor 'y'.
* added 'e' yardmap tile: not buildable, exit-only. Units cannot path from a normal tile to an exit-only tile (but pathing between adjacent exit-only tiles is fine).
Decent for factory construction areas, but keep in mind it only affects ground units (not aircraft, not wrecks, etc) and even ground units can still enter such tiles via non-pathing means (e.g. via impulse).

### Smooth mesh
* added `Spring.RebuildSmoothMesh() → nil` synced callout to immediately rebuild the smooth mesh.
* added `system.smoothMeshResDivider` numerical modrule, default 2. Reduces the resolution of the smoothmesh. Increase to get better performance at the cost of worse accuracy.
* added `system.smoothMeshSmoothRadius` numerical modrule, default 40. The radius for smoothing, in elmos.

### Lua wupget API
* added `Spring.RebuildSmoothMesh() → nil`, see above.
* added `Spring.GetUnitPhysicalState(unitID) → number bitmask`. See engine source for the meanings of bits. Only available to synced.
* added `Spring.GetUnitArrayCentroid({unitID, unitID, ...}) → numbers x, y, z`. Returns the centroid (average position), or nil if the array is empty.
* added `Spring.GetUnitMapCentroid({[unitID] = any, [unitID] = any, ...}) → numbers x, y, z`. Ditto but the unitIDs are keys instead of values in the accepted table.
* added `Spring.GetUnitCosts(unitID) → number buildTime, number metal, number energy`.
* added `Spring.GetUnitCostTable(unitID) → { metal = number, energy = number }, number buildTime`. Note that buildtime is not a regular resource and is returned separately.
* added `Spring.GetTeamDamageStats(teamID) → number damageDealt, number damageReceived`. Same as the values already available from `Spring.GetTeamStatsHistory`, but without most of the overhead.

### Misc and fixes
* something happened to terraforming rate (via restore command, or ground flattening before construction).
* lots of general performance improvements.
* fixed the stack warning spam if `wupget:UnitArrivedAtGoal` was defined.
* fixed factory UnitDefs being able to have `canAssist` set to true
* fixed mouse not warping correctly (when using `Spring.WarpMouse`) on Unix/Wayland
* fixed invalid corpse names being left unsanitized in UnitDefs `corpse`
