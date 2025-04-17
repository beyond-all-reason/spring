---
layout: post
title: Release 105-1354
parent: Changelogs
permalink: changelogs/changelog-105-1354
author: sprunk
---

The changelog since release 105-1214 **until release 105-1354**, which happened in November 2022.

## Caveats
* Ctrl and Alt as camera movement modifiers were unhardcoded and turned into bindings. The engine binds them same as before by default, but if you wipe bindings and handle them on your own then you may want to handle rebinding them.
* `/dynamicSky` command and `AdvSky` springsetting were removed.
* resurrection no longer sets health to 5%. See the "migrating from Spring" guide for a replacement. Note that the Lua function to do this 100% correctly only got added in a further release.
* respect `gl.Color()` in `gl.Text()`. May require taking care that the appropriate colour is set, possibly via inline color codes.

## Features and fixes

### Minimap
* new boolean springsetting `MiniMapCanFlip`, default 0. If enabled, flips minimap coordinates when camera rotation is between 90 and 270 degrees (with rotating cameras, or when flipping via `/viewtaflip` with the default non-rotating camera).
* add `Spring.GetMiniMapRotation() → rot`, currently only 0, or π if flipped.
* minimap now allows making labels when cursor is inside, transposed to map location
* new boolean springsetting: `MiniMapCanDraw`, defaults to 0. Enable the other draw events (line/erase) to happen via minimap

### Camera control
* add `Spring.GetCameraRotation() → rotX, rotY, rotZ`. Also useful for minimap flipping detection.
* Ctrl and Alt as camera movement modifiers were unhardcoded and turned into bindings. The engine binds them same as before by default, but if you wipe bindings and handle them on your own then you may want to handle rebinding them.
* add `movetilt`, `movereset` and `moverotate` actions for previously hardcoded behavior, engine defaults to `Any+ctrl` and `Any+alt` respectively (i.e. previous behaviour).
* make the `InvertMouse` springsetting respected when inverting the "hold middle mouse button to pan camera" functionality
 * add configurability of movefast and moveslow speed via added config values (`CameraMoveFastMult`, `CameraMoveSlowMult`) and scaling factors for specific cameras (`CamSpringFastScaleMouseMove`, `CamSpringFastScaleMousewheelMove`, `CamOverheadFastScale`)
* fix `movefast` and `moveslow` actions not being respected in some circumstances.

### Keys
* accept `keyreload` and `keyload` commands in uikeys
* add `/keydefaults` command, loads the defaults
* accept filename arguments for `keyload`, `keyreload` and `keysave`

### Skybox
* allow loading skybox from equirectangular 2D texture
* deprecate `AdvSky` springsetting
* remove the obsolete `/dynamicsky` option

### Miscellaneous
* add `Platform.cpuLogicalCores` and `Platform.cpuPhysicalCores` constants
* new action: `group unset`, removes any group assignment to selected units
* add `Spring.GetTeamAllyTeamID(teamID) → allyTeamID`
* add `Spring.GetProjectileAllyTeamID(projectileID) → allyTeamID`
* add `Spring.SetWindow{Min,Max}imized() → bool success` (also in LuaIntro)
* add `Spring.LoadModelTextures(string modelName) -> bool success` to preload model textures. Returns false if model not found or is 3do.
* add `damage.debris` modrule for debris damage, default 50 (same as previous).
* add `reclaim.unitDrainHealth` modrule, whether reclaim drains health. Mostly for the reverse wireframe method
* add 5th and 6th params to `Spring.MarkerErasePosition`, onlyLocal and playerID. Makes the erase happen locally, optionally as if done by given player, same as the existing interface for MarkerAddPoint and MarkerAddLine. NOTE: 4th arg is currently unused (reserved for radius)!
* fix functionality for `DualScreenMode` setting, it draws on the window area of the left or rightmost display depending on `DualScreenMiniMapOnLeft` setting. Fix engine related issues with view positioning and adds `DualScreenMiniMapAspectRatio` to draw minimap with preserved aspect ratio.
* fixed a crash caused by kicking a player
