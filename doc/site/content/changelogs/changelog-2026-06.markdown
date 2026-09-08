+++
title = "Release 2026.06"
aliases = ['/changelogs/changelog-2026-06']
+++

This is the changelog since version 2025.06 until **version 2026.06.12**, which was released on 2026-07-14.

## Caveats

- UTF-8 file paths are now supported.
- some file accesses are now case-sensitive.
- rmlUI version used 6.0 → 6.2
- ARM64 architecture builds now have nominal support.
- `/aicontrol` is now blocked by default. Call `/aiCtrl PlayerName` or `/aiCtrlByNum 123` to enable.
- `script:AimWeapon` now receives unit-relative heading and pitch, rather than world-space. This means units angled on slopes will receive different values.
- heading cast to radians will now return \[-pi; +pi) rather than \[0; tau).
- minor Lua env sandboxing changes, see the "Lua environment sandboxing" section below.
- always output logs to stdout.
- removed `CSphereParticleSpawner` particle class. Identical to `CSimpleParticleSystem`.
- archive cache version 20 → 21.

## Features

### RmlUi

- rmlUI version used 6.0 → 6.2
- add datamodel support for pairs: `pairs(dm_handle)`
- add datamodel support for ipairs: `dm_handle:__ipairs()`
- support for accessing the underlying datamodel table with `dm_handle.__raw()`
- allow datamodel self-referential assignments such as `dm_handle.property = dm_handle.another_property`
- support for retrieving datamodel property length: `dm_handle.property.__len()`
- fix datamodel array access
- fix `data-value` binds in rml elements
- added `RmlUi.GetDocumentPathRequests(string docPath) -> {"filePath", "filePath", ...}` which tracks all of the files opened by an RmlUi LoadDocument call
- added `RmlUi.ClearDocumentPathRequests(string docPath) -> nil` to clear tracked LoadDocument files

### Radar icon Lua API
- added `Spring.SetUnitIcon(unitID, string? iconName)`. Pass `nil` to reset to default.
- added `Spring.GetUnitIcon(unitID) → string iconName`.

### Minimap callins
- added `wupget:MiniMapRotationChanged(rotation, previousRotation)` unsynced callin. In radians.
- added `wupget:MiniMapGeometryChanged(x, y, sizeX, sizeY, prevX, prevY, prevSizeX, prevSizeY)` unsynced callin. In pixels.
- added `wupget:MiniMapStateChanged(isMinimized, isMaximized, isSlaved)` unsynced callin.

### Misc

- UTF-8 file paths are now supported.
- some file accesses are now case-sensitive.
- `/aicontrol` is now blocked by default. Call `/aiCtrl PlayerName` or `/aiCtrlByNum 123` to enable.
- `script:AimWeapon` now receives unit-relative heading and pitch, rather than world-space. This means units angled on slopes will receive different values.
- heading cast to radians will now return \[-pi; +pi) rather than \[0; tau).
- add `Spring.SetCheatingEnabled(bool)`.
- add `Spring.SetGodMode(bool? controlAllies, bool? controlEnemies)`.
- large QTPFS perf improvements.
- always output logs to stdout.
- add boolean `Platform.isHeadless`.
- archive cache version 20 → 21.

## Fixes

- fix logs sometimes not getting flushed on exit
- fix `CMD[20]` and `CMD[105]` returning legacy aliases for those commands rather than their standard names.
- fix an archive scanner pool crash.
- fix AI commands not being validated.
- fix Windows reporting hardware incorrectly.
