+++
title = "Release 2026.07"
aliases = ['/changelogs/changelog-2026-07']
+++

This is the changelog since version 2025.07 until **version 2025.07.04**, which was released on 2026-08-04.

## Caveats
- removed Java bindings for Skirmish AI.
- Lua environment sandboxing changes, each has a caveat. See below.
- builders now perform an extra block check immediately when a build command reaches the front of the queue, in addition to the existing periodic check. This can result in an event spam e.g. if a builder is on repeat between multiple queued buildings and they're all blocked.
- mouse4 and mouse5 ignore mouse ownership and produce MousePress/MouseRelease events even if another mouse button is already pressed. Make sure your wupget handlers handle this correctly if they manage wupget mouse ownership.
- sonar jamming now jams sonar only; no longer blocks regular radar for units in water, i.e. surface ships. No easy replacement.
- errors when loading modrules are no longer silently ignored, rather there's an error popup. Wrap everything in pcall if needed.
- removed `LimitDgun` from being listed in `EngineOptions.lua`, add it to modoptions if you used it as such.
- removed some basecontent gadgets that used `LimitDgun` and other long-removed engineoptions. Copypaste them from older basecontent if needed.

## Features

### Lua environment sandboxing
- LuaSocket no longer inits before Lua sandboxing. Anything that relied on the previous order of execution won't work.
- unsynced LuaRules (incl. unsynced LuaGaia) now has access to `io` and `os` libraries. Watch out when loading maps etc.
- unsynced LuaRules (incl. unsynced LuaGaia) now has access to the `debug` library by default (no longer requires devmode).

### Replay path getters
- add `Spring.GetReplayFilePath() → string?`, returns path of replay being watched.
- add `Spring.GetReplayRecordingFilePath() → string?`, returns path of replay to be produced. Note that this is just a prospective file path (nothing is written until the match ends), and that it possible to record a replay of a replay.

### Build commands
- builders now perform an extra block check immediately when a build command reaches the front of the queue. This is in addition to the existing periodic (≈ 0.4 Hz) block check. A block check cancels a build command if the build site is hard-blocked ("red squares", as opposed to "yellow squares" with reclaimables/mobiles).
- add `Spring.SetEngineBuildSquareRendering(bool) → nil`, for disabling the native rendering of the footprint grid when a build command is selected.
- add `wupget:DrawBuildSquare(unitDefID, x, z, facing, statuses) → nil` unsynced callin, fires when a build command is selected. Statuses is a 1D array for the status of each tile: 0 blocked (red), 1 occupied (yellow), 2 reclaimable (yellow), 3 open (green). This fires even if native drawing is enabled.

### Lua trace ray
- adds `Spring.TraceRayBetweenPositions(xA, yA, zA, xB, yB, zB, type) -> {{dist, objID, type}, ...}`
- adds `Spring.TraceRayInDirection(x, y, z, dx, dy, dz, length, type) -> {{dist, objID, type}, ...}`
- type is a string, "unit", "feature", or (for input only) "both"
- the returned array is sorted by distance, starting from closest.

### Blank map splats

Blank map generator now interprets some mapoptions. These are for filling entries in the generated `mapinfo.lua` file.

- `blank_map_splatdetailtex`, string with the path
- `blank_map_splatdistr`, string with the path
- `blank_map_splattexscale1 .. 4`, number
- `blank_map_splattexmult1 .. 4`, number
- `blank_map_splatdetailnormaltex1 .. 4`, string with the path
- `blank_map_splatdetailnormaldiffusealpha`, bool

### Other runtime map texturing stuff
- fix `Spring.SetSkyBoxTexture` not working if the map didn't have a skybox from the start
- fix map shaders having stale uniforms and being completely broken for forward rendering. This fix is signalled by the `Engine.FeatureSupport.reliableLuaMapShaders` flag.

### Input emulation

Added a bunch of `debug.emulateFoo` functions that emulate input. Useful for automated testing of UI. Buttons are considered pressed from any source for edge-based events (i.e. pressing a "real" button when it is already pressed via emulation, or vice versa, will not produce a KeyPressed event; ditto release if it is still pressed from the other source).

- `debug.emulateKeyPress(keycode)`. The event will have a scancode based on the current keyboard layout (i.e. possibly "unknown" if no keyboard, such as a headless VM).
- `debug.emulateKeyRelease(keycode)`
- `debug.emulateMousePress(button)`
- `debug.emulateMouseRelease(button)`
- `debug.emulateMouseMove(dx, dy)`. Does not actually move the mouse, so getters won't reflect it, but you can `Spring.WarpMouse` alongside it.
- `debug.emulateMouseWheel(number delta)`. Note that this accepts fractions, but most physical mice produce integer deltas (+1, -1).
- `debug.clearEmulatedInput()`. Releases all emulated presses.

### Keybind-related work
- add `cancelcommand` action for binding, cancels the currently selected command. The hardcoded Escape key binding still works.
- mouse4 and mouse5 ignore mouse ownership and produce MousePress/Release events even if another mouse button is already pressed.
- fix `sc_nonusbackslash` scancode name (was `sc_nonusbacklash`)
- fix `/unbindaction` not clearing scancode bindings
- fix `/unbind` not working with keychains longer than 1 key
- fix stale returns from `Spring.GetActionHotKeys`

### Resourcing

- add `gadget:ResourceExcess({[teamID] = {m, e, ...}}) -> bool handledGameside`. Runs every frame. If you return false, the engine will do the existing behaviour where the excess is accumulated until a slowupdate and shared to teammates if possible.
- add `Spring.AddTeamResourceExcessStats(teamID, resourcetype, amount)`. Adjusts the stats for endgame graphs purposes (does not do anything to the actual resources). Useful for when you handle excess yourself with the callin above.

### Custom teamcolor palette
- add `Spring.SetCustomPaletteColor(paletteID, r, g, b) → nil`. Sets a palette color for shader use. See below.
- add `Spring.GetCustomPaletteColor(paletteID) → r, g, b`.
- add `Engine.maxCustomPaletteID`, the highest available palette ID.
- add `Spring.SetUnitPaletteIndex(unitID, paletteID?) → nil`. Assigns a paletteID to a unit. Use nil to reset to the default palette.
- add `Spring.GetUnitPaletteIndex(unitID) → paletteID?`.
- add `Spring.SetFeaturePaletteIndex(featureID, paletteID?)`.
- add `Spring.GetFeaturePaletteIndex(featureID) → paletteID?`.
- the `teamColor` array in shaders now has much more room and contains both teamcolors and colors of the custom palette, as per above.
- the actual teamID is now available as the fifth byte (first byte of the second 4-byte composite) in model uniform data.
- note that the value of the palette index is different than what is seen from Lua. Units with no custom paletteID have the index point to an entry that contains their team color.
- the basecontent teamcolor shader takes the above changes into account. Existing custom shaders that use `instData.z & 0xFF` for teamID should keep working as long as the palette feature isn't used, to support it properly the constant needs to be `0x7FF` instead.
- ghosts of out-of-sight buildings now draw with the unit's last seen palette color. This also fixes the bug where the teamcolor updated to always reflect the unit's real team even if it changed out of sight.
- projectiles and building ghosts in constructor queues unchanged, they still just draw teamcolor naively and cannot use a shader.

### Building ghosts
- building ghosts now use the custom palette teamcolor.
- fix building ghosts always showing their "true" teamcolor, now they stick to their last known teamcolor (including custom palette).
- fix full-view spectators seeing a ground decal wherever their spectated team sees a ghost with decal.

### Misc
- removed Java bindings for Skirmish AI.
- entries returned by `VFS.GetAvailableAIs()` now have a new `isLuaAI` boolean (skirmish AI otherwise).
- errors when loading modrules are no longer silently ignored, rather there's an error popup.
- add `Spring.GetPrevFrameSyncChecksum() -> string` and `Platform.hasSyncChecksums`. Useful for correctness checks. Note that the checksum looks like a number but is NOT convertible to one in Lua (checksum is a 32-bit number but Lua numbers have 24-bit precision).
- add string `Platform.architecture`. Usually "x86_64", with some ongoing work to support "arm64".
- add `Spring.GetClosestEnemyUnit(x, y, z, range = inf) → unitID?` to LuaUI.
- add `Spring.GetClosestEnemyUnit(x, y, z, range = inf, allyTeamID, bool useLoS = true, bool spherical = false, bool requireEnemyToSeePos = false) → unitID?` to LuaRules.
- add `Engine.FeatureSupport.reliableLuaMapShaders` bool, means issues with map shaders having stale uniforms and being completely broken for forward rendering are fixed.
- the `/dumpAtlas` command now accepts a file format (e.g. `/dumpatlas proj tga`), defaults to the existing `png`.
- Barbarian AI bundled with the engine updated to version 1.6.28. Just maintenance, no new features.

### Fixes
- fix LuaSocket initialisation for LuaMenu.
- fix unitsync not running functions passed to `Spring.TimeCheck`.
- fix Skirmish AI API compilation issues due to `AIFloat3` having a non-trivial constructor.
- attempt to fix the lack of `wupget:GameProgress` calls when initially catching up.
- fix Lua `VBO::CopyTo` to copy VBO data CPU side in additon to GPU side. Formerly it only copied data GPU side.
- fix QTPFS path cleanup when an immediate path search fails
