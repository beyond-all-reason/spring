---
layout: post
title: Release 105-861
parent: Changelogs
permalink: changelogs/changelog-105-861
author: sprunk
---

The changelog since the fork **until release 105-861**, which happened in February 2022.

## Caveats
* `wupget:GameProgress` interval 10 → 5 seconds
* basecontent now provides the `treetype0` to `treetype15` features with a model and some customparams
* buildings can be placed inside other buildings' open yardmap gaps
* demos are saved with UTC clock filename (could break filename parsing etc)

## Features and fixes

### Icons as UI
* new springsetting `UnitIconsAsUI` and an ingame `/iconsAsUI` command (all below have a `/command` variant without the "Unit"). Set to 1 to make radar icons behave like UI elements instead of in-world objects (e.g. no obscuring by terrain, distance fog, blurring or reflection in the water etc.
* new springsetting `UnitIconScaleUI`. Size scaling for icons when drawn as UI.
* new springsetting `UnitIconsHideWithUI`. Whether icons get hidden via F5.
* new springsettings `UnitIconFadeStart` and `UnitIconFadeVanish`, for making icons fade smoothly

### Grass
* `Spring.{Add,Get,Remove}Grass` now works even if grass rendering is disabled (also fixes desync)
* `Spring.AddGrass` now has a 3rd param, grass intensity (returned by `Get`; putting 0 equivalent to using `Remove`)

### Recursion
Allow shallow recursion (up to 16 calls deep) for the following callouts/callins:
* `Spring.{Create,Destroy}{Unit,Feature}`
* `Spring.GiveOrder{,Array}ToUnit{,Array,Map}`

Additionally:
* `Spring.SpawnExplosion` works inside `Projectile{Created,Destroyed}`

### Texture Atlases
* add `gl.CreateTextureAtlas(number x, number y, number? allocType, string? atlasName) → string texName`.
`x` and `y` have to be between 256 and the value of the springsetting `MaxTextureAtlasSizeX` (Y respectively).
`allocType` defaults to 0 if not specified. `atlasName` is an optional name for the atlas.
* add `gl.FinalizeTextureAtlas(string id) → bool success`.
* add `gl.DeleteTextureAtlas(string id) → bool success`.
* add `gl.AddAtlasTexture(string id, string luaTex, string? subAtlasTexName)`. Adds texture `luaTexStr` to sub-atlas `subAtlasTextureName` (defaulting to same name as `luaTex` if not specified) of atlas `id`. The lua tex has to be a regular 2D RGBA/UNORM texture. DDS textures can also be put into the atlas.
* add `gl.GetAtlasTexture(string texName, string luaTex) → number s, p, t, q`. Query an atlas texture for the UV coordinates (minU, maxU, minV, maxV) of a texture saved earlier with `Spring.AddAtlasTexture`.

### Particles
* CEGs from allied projectiles now visible in the fog
* add `/softParticles` toggle and `SoftParticles` springsetting to make particles (esp. groundflashes) clipping into the ground fade a bit instead of having a jagged edge
* add `Spring.SetNanoProjectileParams(r, v, a, randR, randV, randA) → nil`. All params are numbers, and default to 0 when not given. The first three are starting rotation in °, rotation speed in °/s, and rotation acceleration in °/s². The other three are the same but multiplied with a random (-1; +1) before being added to the former, rolled once per particle at the start of its lifetime.
* add `Spring.GetNanoProjectileParams() → r, v, a, randR, randV, randA`, returns the parameters as above.
* add `/drawOrderParticles` command which makes particles obey draw order, supplied in CEG definition under the `drawOrder` key
* add `rotParams` key to `CSimpleParticleSystem` and `CBitmapMuzzleFlame` CEGs, a vector of 3 floats: rotation speed °/s, rotation acceleration °/s², starting rotation in °

### Advanced GL4 (shaders, VAO, etc)
* add `gl.GetVAO` which returns an object with a bunch of methods.
* add `gl.GetVBO`.
* add `Spring.GetSelectedUnitsCount` to the shader container (use case: F2 pathmap replacement, can show a general slope map if 0 units selected)
* add `gl.{Unit,Feature}{,Shape}GL4` temporary functions
* added a ton of uniforms for shaders
* add LuaShaders::Set{Unit,Feature}BufferUniforms for GPU side per unit/feature "uniforms" (SSBO in fact)
* gl.Uniform/gl.UniformInt/etc don't require location as a first param, name is fine too.

### Lua constants for rendering
* add `Platform.glHave{AMD,NVidia,Intel,GLSL,GL4}` bools
* remove `Platform.glSupport24bitDepthBuffer` and add `Platform.glSupportDepthBufferBitDepth` instead
* add the following constants to `GL`: `LINE_STRIP_ADJACENCY`, `LINES_ADJACENCY`, `TRIANGLE_STRIP_ADJACENCY`, `TRIANGLES_ADJACENCY`, `PATCHES`
* add the following constants to `GL`: `[UNSIGNED_]BYTE`, `[UNSIGNED_]SHORT`, `[UNSIGNED_]INT[_VEC4]`, `FLOAT[_VEC4]`, `FLOAT_MAT4`
* add the following constants to `GL`: `ELEMENT_ARRAY_BUFFER`, `ARRAY_BUFFER`, `UNIFORM_BUFFER`, `SHADER_STORAGE_BUFFER`
* add the following constants to `GL`: `READ_ONLY`, `WRITE_ONLY`, `READ_WRITE`
* add a ton of image type specifiers (`RGBA32` etc) to `GL`
* add a ton of barrier bit constants to `GL`

### Other rendering-adjacent stuff
* most CEGs and weapon visuals are tested for texture validity such that sprites with invalid textures are not displayed. No more need for 1x1 empty texture
* fix skybox stretching and seams (in case it's represented by a cubemap)
* smoother area-command circle, vertices 20 → 100
* add `Spring.{Set,Get}{Unit,Feature}AlwaysUpdateMatrix` which makes the unit "always update matrix"
* add `/reloadTextures [lua, smf, s3o, ceg]` to reload textures, all groups if none specified.
* filling the nano wireframe works on the current model heights (meaning units with pieces hidden high above but moved downward work correctly)
* add `globalRenderingInfo.availableVideoModes `
* add `gl.GetFixedState` that returns a bunch of various GL constants and tables, it does a lot depending on args.
* `DeprecatedGLWarnLevel` springsetting default value 2 → 0 (to reduce spam)
* add `gl.AlphaToCoverage(bool enable, bool? force) → nil`, defined only if `GLEW_ARB_multisample` is supported by the platform, and otherwise the func itself is nil; `force` makes it apply even if MSAA level is under 4
* improve AMD gfx card detection (looks for more strings, eg. "radeon", and in more places)
* add `gl.ClipDistance(number clipID, bool enabled) → nil`, `clipID` is 1 or 2.
* change `TextureMemPoolSize` springsetting default value 256 → 512 MB
* expose `windSpeed` to `Spring.{Get,Set}WaterParams`
* new bumpwater params (defaults): waveOffsetFactor (0.0), waveLength (0.15), waveFoamDistortion (0.05), waveFoamIntensity (0.5), causticsResolution (75.0), causticsStrength (0.08)
* fix sky reflections on the ground (`skyReflectModTex`)

### Missile smoke trails
* weapon defs: add bool `smokeTrailCastShadow`, number `smokePeriod` (default 8), number `smokeTime` (default 60), number `smokeSize` (default 7), and number `smokeColor` (default 0.7, single value for all three RGB channels tint) to things with smoke trails
* weapon defs: add bool `castShadow`, for the projectile itself

### Camera tweaks
* add integer springsetting `SmoothTimeOffset`. This attempts to smooth out the TimeOffset parameter that is used to calculated the tweened draw frames. Default 0, old behaviour. Recommended value of 2, this attempts to keep the actual timeoffset within 90% of the true time. Best with vsync on.
* add boolean springsetting `CamFrameTimeCorrection`. Default false is the current behaviour, use true to get better interpolation during high load.

### Builders
* new yardmap options for "stacked" buildings
* add `Spring.{Set,Get}UnitBuildParams(unitID, "buildDistance"/"buildRange3D", value) → nil` about unit build range (number) and whether it's spherical (bool)
* add `Spring.GetUnitInBuildStance(unitID) → bool`
* add the `gadget:AllowUnitCaptureStep(capturerID, capturerTeamID, victimID, victimTeamID, progressDiff) → bool` callin
* fix seaplanes being unbuildable on water if they were set to float rather than submerge

### Other additions to Lua API
* `math.random()` now correctly accepts arguments when parsing defs
* allow `nil` as the first arg to `Spring.SetCameraState`
* `Spring.SetWMIcon` new 2nd arg, bool `force`: ignores the 32x32 icon restriction on Windows
* allow empty argument for `Spring.GetKeyBindings` to return all keybindings
* add `Spring.GetUnitsInScreenRectangle`
* added `Spring.SetWindowGeometry` for easy window positioning.
* add `Spring.ForceTesselationUpdate(bool normalMesh = true, bool shadowMesh = false) → bool isROAM` for ROAM
* parameterless `Spring.GetUnitFlanking()` now has an 8th return value, collected flanking bonus mobility

### Misc features
* selection keys: add an `IdMatches` filter to use the internal name
* the default pathfinder (set via 0 in modrules) is now multi-threaded. In release 861, the old single-threaded pathfinder is temporarily accessible by setting the modrule to 2, but this gets removed later on in release 1544 when MT issues are fixed.
* add `/setSpeed x` to set game speed directly, still obeys min/max
* add `/endGraph 2` to bring the player back to the menu (1 still quits)
* keybinds: same action with different arguments now differentiated
* added a Load Game button to the raw spring.exe menu
* command cache increased 1024 → 2048
* native AI interface: add `Feature_getResurrectDef` and `Feature_getBuildingFacing`
* native AI interface: add `spherical` param to `Get{Friendly,Enemy,Neutral,}{Units,Features}{In,}`

### Misc fixes
* fix CEGs from allied projectiles: now visible in the fog
* fix disk read failures (files that get read failure errors are retried multiple times before giving up)
* fix enabling globallos not revealing unseen terrain changes immediately
* fix height bounds calculation, it now reflects the actual heights rather than the historical min/max (for example if the highest point went 100 → 90 → 110 → 100, previously `Spring.GetGroundExtremes` would return 100 → 100 → 110 → 110)
* fix `Spring.SpawnExplosion` crashing on some valid data
* fixed aircraft using smoothmesh to update if terrain was modified pre-game
* multiple fixes to save/load
* fix spring-dedicated not correctly showing output on Windows when not redirected (spring-headless already worked correctly)
* fix a crash when trying to add enemy units to a control group (e.g. selected via /godmode)
* fix a desync caused by locale-dependent string sorting
* fix a crash trying to load a save done on a map with LuaGaia when currently playing on a map without it
* fix resources sometimes getting into the negative (on the order of 1e-15) due to float inaccuracies; in particular this caused units that cost 0 resources to shoot to be unable to fire due to insufficient resources
* fix "zombie torpedoes" bouncing under the map
* fix projectiles going through units sometimes
* fix the "unload dead unit" synced crash
* probably works on Linux Wayland (implementation doesn't support hardware cursor yet; added in a later release)