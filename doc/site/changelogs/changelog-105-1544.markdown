---
layout: post
title: Release 105-1544
parent: Changelogs
permalink: changelogs/changelog-105-1544
author: sprunk
---

The changelog since release 105-1354 **until release 105-1544**, which happened in February 2023.

## Caveats
* rebranded the fork to Recoil. Everything where a rename would have technical consequences (`spring.exe` filename, `Spring.Foo` Lua API etc) has been kept as-is.
* `Spring.GetConfig{Int,Float,String}` second parameter can now be `nil`, which means it will return `nil` if the requested setting is not set. Formerly returned `0`, `0`, or `""` respectively. Note that for settings used by the engine the value is never nil (so this is only for custom ones). To get back previous behaviour:
```lua
local originalGetConfigInt = Spring.GetConfigInt
Spring.GetConfigInt = function (key, def)
   return originalGetConfigInt(key, def) or 0
end
-- ditto for Float and String
```

## Features and fixes

### General multi-threading improvements
* main thread is now allowed to move between cores when hyperthreading is unavailable. This helps when a server is running multiple headless or opengl instances.
* `LoadingMT` springsetting no longer accepts -1 for autodetect (which meant 0 for Intel/Mesa graphics drivers and 1 for others)
* add `Spring.Yield() → bool hintShouldKeepCallingYield` to give back OpenGL context from game-loading thread back to main thread to process events and draw with LuaIntro. It's safe to call this regardless of LoadingMT. In practice this means it should be called from unsynced wupget initializers periodically as long as the returned hint value is true, the common implementation is to call it after a wupget is loaded like so:
```lua
local wantYield = true
for wupget in wupgetsToLoad do
  loadWupget(wupget)
  wantYield = wantYield and Spring.Yield()
end
```

### Multi-threading modrules
Note from the future, these were removed after release 1775.
* added `system.pfForceSingleThreaded` to force pathing requests to be sent single threaded.
* added `system.pfForceUpdateSingleThreaded` to force pathing vertex updates to be processed single threaded.
* added `movement.forceCollisionsSingleThreaded` to force collisions system to use only a single thread.
* added `movement.forceCollisionAvoidanceSingleThreaded` to force collision avoidance system to use only a single thread.

### Debugging improvements
* added boolean springsetting `DumpGameStateOnDesync`. When set to true on the server, it will request all clients to produce a local copy of the current game state (effectively calling `/dumpstate`). Doesn't do anything if set client-side.
* add `/debug reset` to reset the collected debug stats
* added the `/desync` command for non-debug builds
* fixed issue where desyncs could not be detected before the 300th sim frame (10 seconds)

### Standalone model piece getters
Unlike similar existing functions these don't require a unit/feature instance to exist.
* `Spring.GetModelPieceMap(string modelName) → { body = 0, turret = 1, ... }`
* `Spring.GetModelPieceList(string modelName) → { "body", "turret", ... }`

### New selectkeys filters
* `Buildoptions` - for units that have a non-zero number of build options (meaning constructors and factories)
* `Resurrect` - for units that can resurrect
* `Cloaked` - for units that are currently cloaked
* `Cloak` - for units of a type that can cloak (regardless of current cloak state)
* `Stealth` - for units of a type that is stealth by default (regardless of current stealth state)
* (note that there is no filter for "currently stealthed" like there is for cloak)

### Drawing related changes
* add `wupget:DrawPreDecals()` callin
* add `AlwaysSendDrawGroundEvents` boolean springsetting (default false) to always send `DrawGround{Pre,Post}{Forward,Deferred}` events, otherwise/previously they are skipped when non-Lua rendering pipeline was in use.
* add `/drawSky` which toggles drawing of the sky (mostly for performance measurements now)
* Maps with missing/invalid details texture no longer get tint with red. Improved reporting of missing map textures.
* Shadows color buffer can be made greyscale with springsettings `ShadowColorMode = 0` or `/shadows` switches.
* fix Bumpwater rendering on non-NVIDIA drivers

### Miscellaneous features
* add `Game.metalMapSquareSize` for use with API like `Spring.GetMetalAmount` etc
* add mirrored looping of animated CEG sprites (animates backwards until it reaches the start and then bounces again), enabled by making the animation speed parameter negative (e.g. `animParams = "4,4,-30"`).
Available for `BitmapMuzzleFlame`, `HeatCloudProjectile`, `SimpleParticleSystem` and `GroundFlash` CEG classes.
* add `Spring.GiveOrderArrayToUnit`, accepts a single unitID and an order array and otherwise works the same as the existing `GiveOrderXYZ` family of functions

### Miscellaneous fixes
* fix `Spring.GetSelectionBox` not returning nil when there's an active mouse owner (activeReceiver)
* fix multiple issues with movement and pathing quality and performance
* fix a crash on loading a game with Circuit/Barbarian AI.
* when scanning archives (at early loading), the window should no longer be considered frozen/unresponsive by the OS
* `PreloadModels = 0` (non-default) should work correctly now, though it comes with its own set of compromises.

## pr-downloader
* implement concurrent fetching of multiple mods and maps with single tool invocation
* replace potentially racy `If-Modified-Since` caching with ETags.
* harden downloading rapid packages by saving sdp file only after full successful download
* implement writing `md5sum` files for assets downloaded from springfiles to allow future full game
files validation
* drop lsl from the repo and separate RapidTools into separate repo
* fix resolving of dependent rapid archives
* fix compilation issues under MSVC making some parts of codebase more platform agnostic
