---
layout: post
title: Running changelog
parent: Changelogs
permalink: changelogs/running-changelog
author: sprunk
---

This is the changelog **since version 2314**.

# Caveats
These are the entries which may require special attention when migrating: (none so far)

# Features
* added a new optional boolean parameter to `Spring.GetUnitHeading`, default false. If true, the value returned is in radians instead of the TA 16-bit angular unit.
* added a new callin, `GameFramePost(number frame)`. This is the last callin in a sim frame (regular `GameFrame` is the first).
Use for batching events that happened during the frame to be sent to unsynced for use in draw frames before the next sim frame.
* added `Spring.GetTeamMaxUnits(teamID) -> number maxUnits, number? currentUnits`. The second value is only returned if you have read access to that team (max is public).
There is currently no corresponding Set.

### Water
* add `Spring.GetWaterLevel(x, z) -> number waterHeight`. Similar to `Spring.GetGroundHeight` except returns the height of water at that spot.
Currently water height is 0 everywhere. Use where appropriate to be future-proof for when Recoil gets dynamic water, or just to give a name to the otherwise magic constant.
* add `Spring.GetWaterPlaneLevel() -> number waterPlaneHeight`. Ditto, except encodes that you expect the water to be a flat plane.
Use as above but where you have no x/z coordinates.

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

# Fixes
* inserting (via `CMD.INSERT`) a "build unit" command to a factory with a SHIFT and/or CTRL modifier (i.e. x5/20) will now work correctly (previously ignored and went x1).
* `Spring.GetUnitWeaponState(unitID, "burstRate")` now correctly returns fractional values (was only full integers before).