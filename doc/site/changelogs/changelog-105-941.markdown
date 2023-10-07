---
layout: post
title: Release 105-941
parent: Changelogs
permalink: changelogs/changelog-105-941
author: sprunk
---

The changelog since release 105-902 **until minor release 105-941**, which happened in May 2022.

### Drawing
* enable 3D and cubemaps for luatextures
* `Spring.GetRender{Units,Features}` now support GL4 CUS.
* add `Spring.Clear{Units,Features}PreviousDrawFlag() → nil`
* add `Spring.GetRender{Units,Features}DrawFlagChanged(bool returnMasks = false) → changedIDs[, changedMasks]`

### Timers
 * Add new `Spring.GetTimerMicros() → number`, which gets timers in usec precision
 * `Spring.DiffTimers` now takes a fourth arg, if you want usec precision in millisecond numbers, and passed in microsec level timers

### Miscellaneous
* added a new 'b' designator for yardmaps to declare an area that is buildable, but is not walkable. This allows for the current method of upgradable buildings to create a locking pattern that won't break pathing.
* add `group [subcommand] N` action handlers where subcommand is one of `add`, `set`, `selecttoggle`, `selectadd`, or `selectclear`. This allows for more granular group binding control than `groupN` (which checks for keyboard modifiers in the action). When subcommand is not present, select group N.
* Skirmish AI: call `PostLoad()` after load event and only if `loadSupported != yes`
* accept an integer quality argument in `screenshot` action, as in `screenshot [format] [quality]`.
