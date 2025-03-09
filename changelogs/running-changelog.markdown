---
layout: post
title: Running changelog
parent: Changelogs
permalink: changelogs/running-changelog
author: sprunk
---

This is the changelog **since version 2025.01.6**.

# Caveats
These are the entries which may require special attention when migrating:
* network protocol: scribbling-related (draw, point, erase) messages now send coordinates as `uint32` instead of `int16`.
This may break replay parsing.
* uniform location 5 in vertex shaders for models extended from uvec2 (boneID, boneWeight) to uvec3 (boneID low byte, boneWeight, boneID high byte).
Existing shaders should generally keep working.
* missiles now obey `myGravity` when expired.
* `math.clamp` now errors if the lower bound is higher than the upper bound.
* added minimap rotation API which can screw up the minimap (see below), set it to nil at LuaUI entry point if you don't want to handle it.
* server no longer automatically forcestarts the game if there is nobody connected after 30s.
* fixed the `SPRING_LOG_SECTIONS` environment var, it no longer requires a comma in front.
* `gl.GetAtmosphere("skyDir")` now returns nil. The skyDir was never actually used for anything.
* updated Tracy to v0.11.1, see the changelog at https://github.com/wolfpld/tracy/releases
* archive cache was reworked. Should be much faster to process, but will take up more disk space and it's not yet known how stable it is.


### Deprecation notice
* the metal view automatically toggling when building a metal extractor is now deprecated.
Disable it via `Spring.SetAutoShowMetal` (see below) and reimplement manually. A replacement
example has been provided at [`(basecontent)/examples/Widgets/gui_autoshowmetal.lua`](https://github.com/beyond-all-reason/spring/blob/master/cont/examples/Widgets/gui_autoshowmetal.lua).

# Features

### Lua language server support
LDoc has been replaced by [Lua Language Server](https://luals.github.io/) compatible annotations. This allows for language server support when editing Lua code (namely autocompletion and type checking).
Type definitions can be found in the [Lua library repo](https://github.com/beyond-all-reason/recoil-lua-library). This is intended to be included as a submodule in projects that use the engine.
[Lua API docs]({{ site.baseurl }}{% link lua-api/docs/index.md %}) are now generated from LLS definitions instead of LDoc. This has caused a regression in docs quality, with all docs on a single page and some docs missing information. Improvements to the docs are being considered.
For more information see the [Lua Language Server guide]({{ site.baseurl }}{% link guides/lua-language-server.markdown %}).

### Unit groups
* units no longer removed from groups at the start of their death animation.
* added `Spring.SetUnitNoGroup(unitID, bool noGroup) → nil`, whether a unit can be added to groups.
* added `Spring.GetUnitNoGroup(unitID) → bool noGroup`.

### Infra-adjacent
* added `--calc-checksum "Archive Name"` CLI param, writes a single archive's checksum to archive cache. Use from a lobby for preloading content, though be wary not to call it in paralllel.
* added `--only-local` CLI param to `spring.exe`, prevents a replay from opening a connection. Use for mass replay parsing.
* fixed the `SPRING_LOG_SECTIONS` environment var, it no longer requires a comma in front.
* server no longer automatically forcestarts the game if there is nobody connected after 30s.

### More model pieces
* models now support 65534 pieces, up from 255.
* this tends to have terrible performance for even hundred-piece models, so consider it a crutch and avoid it if possible.
* add `Engine.FeatureSupport.maxPiecesPerModel` constant you can read to get the piece number.
* uniform location 5 in vertex shaders for models extended from uvec2 (boneID, boneWeight) to uvec3 (boneID low byte, boneWeight, boneID high byte).
Existing shaders should generally keep working.

### Automatic metalmap view
* added `Spring.SetAutoShowMetal(bool) → nil`. If you set it to false then selecting a "build mex" command won't automatically enable
the metal view (use the widget from `(basecontent)/examples/Widgets/gui_autoshowmetal.lua` to replicate it). If you leave it at true (default),
engine will keep automatically enabling the metal view but also spam deprecation warnings. This function will be removed at some point in the future.
* added `Engine.FeatureSupport.noAutoShowMetal`, false. At some point in the future this will change to `true` at which point the engine
will stop automatically enabling the metal view for mexes (and remove `Spring.SetAutoShowMetal`).

### Minimap 90°/270° rotations
* minimap can now be rotated to 90° and 270°, and can be rotated manually.
* added `Spring.SetMiniMapRotation(number angle) → nil`, sets the minimap angle (in radians). Snaps to cardinal directions.
Only works if the springsetting `MiniMapCanFlip` is set to 0 (if it's 1, engine automatically flips vertically as previously).
Geometry is kept, so it may end up stretched - readjust it manually via `gl.ConfigMiniMap`.
* `Spring.GetMiniMapRotation() → number angle` handles the new rotations (i.e. can return `π/2` and `3π/2`).

### Misc rendering
* add `Platform.glVersionNum`, 1/10th the version of glsl (e.g. GL 3.0 → 30).
* added `gl.ClearAttachmentFBO(number? target = GL.FRAMEBUFFER, string|number attachment, clearValue0, cv1, cv2, cv3) → bool ok`. Clears the "attachment" of the currently bound FBO type "target" with "clearValues".
Attachment can be either string ("color0" .. "color15", "depth", or "stencil") or the equivalent `GL.COLOR_ATTACHMENT#` constant.
* minimap is sharper with super-sampling anti-aliasing enabled
* added 3D noise to basecontent bitmaps archive, under `bitmaps/noise/recoil_noise_2025_p5_p3_w6_w4_64x64x64_RGBA.dds` and `bitmaps/noise/recoil_noise_2025_p5_p3_w6_w4_128x128x128_RGBA.dds`.
This allows for reliable generation of cheaper 3D noise in shaders.

### Praise the sun!
* the shading texture (used by minimap, water, and grass) now tracks sun position changes immediately.
* the "modern" sky renderer now uses the actual sun color instead of hardcoded RGB 253/251/211.

### Spinning skybox
* added new tag to `Spring.SetAtmosphere`, `skyAxisAngle`, array of 4 numbers: X, Y, Z (defining an axis) and rotation around that axis.
Lets the skybox be drawn at an angle and possibly spin. Complex rotations involve doing math yourself.
* added `skyAxisAngle` to `gl.GetAtmosphere`, same format.
* added `atmosphere.skyAxisAngle` to mapinfo, same format.
* removed `atmosphere.skyDir` from mapinfo, it didn't actually affect anything.
* `gl.GetAtmosphere("skyDir")` now returns nil.

### Debugging for rendering
* added `VBO:GetID() → number`, gets the internal OpenGL ID for use in debugging.
* added a second return value to `gl.CreateShader`, now also returns the internal ID for use in debugging.
* added `gl.ObjectLabel(objType, objID, string identifier) → nil`. Type is one of the new GL object type constants (`GL.BUFFER`, `GL.PROGRAM_PIPELINE` etc., see the list at the bottom).
Adds a text label to an object for debugging tools. Conditionally available based on gfx drivers etc (do a nil check).
* added `gl.PushDebugGroup(number ID, string message, bool isThirdParty) → nil`. Conditionally available. Puts a group (FIXME what exactly is a group? just a scope same as tracy? and how do I get its ID?)
with given message onto the debug stack. Meant for debugging tools, seems to be specifically for "nVidia nSight 2024.04". Apparently "does not seem to work" when FBOs are raw bound (FIXME is that a crash, corrupt data, just no-op?).
* added `gl.PopDebugGroup() → nil`, conditionally available, pops a previously pushed debug group from the stack.
* the engine now natively pushes some GL debug groups in relevant scopes, similar to tracy.

### Unit rotations
* `Spring.GetUnitDirection` and `Spring.GetFeatureDirection` now return 9 values (from 3).
The initial three stay as the XYZ of the "front" direction in unit space, the new ones are XYZ of the "right" direction
and the XYZ of the "up" direction.
```diff
- local frontX, frontY, frontZ = Spring.GetUnitDirection(unitID)
+ local frontX, frontY, frontZ, rightX, rightY, rightZ, upX, upY, upZ = Spring.GetUnitDirection(unitID)
```
* `Spring.SetUnitDirection` and `Spring.SetFeatureDirection` now accept up to 6 args (from 3). The new ones
are the XYZ for the "right" direction (since just the "front" is ambiguous) and are optional (same behaviour
as previous, i.e. some direction based on ground). Note there is no way to specify the "up" dir (but given
the "right" dir it is unambiguous; and if you have front+up dirs you can unambiguously find the right dir).
```diff
  Spring.SetUnitDirection(unitID, frontX, frontY, frontZ) -- still works
+ Spring.SetUnitDirection(unitID, frontX, frontY, frontZ, rightX, rightY, rightZ) -- new
```

### Engine feature support tags
Added a bunch of feature support entries to the `Engine.FeatureSupport` tags. These are for the future.
Most likely you don't need to worry about those since games will receive patches if there is a change.
* number `gunshipCruiseAltitudeMultiplier`, currently `1.5`. Right now gunship cruise altitude is multiplied by this.
* bool `noRefundForConstructionDecay`, currently `false`. At some point refunds for construction decay will be handed over to Lua.
* bool `noRefundForFactoryCancel`, currently `false`. At some point refunds for factory cancel will be handed over to Lua.
* bool `noOffsetForFeatureID`, currently `false`. At some point featureIDs in mixed contexts (e.g. target ID for reclaim) won't require to be offset.

### Misc
* add `Spring.GetSoundDevices() → { { name = "...", }, { name = "...", }, ... }`.
May be extended with more info than just name in the future.
* add `wupget:ActiveCommandChanged(cmdID?, cmdType?) → nil`.
* the `Spring.GiveOrder` family of functions now accept `nil` as params (same as `{}`) and options (same as `0`).
* add `Spring.SetUnitStorage(unitID, "m"|"e", value) → nil`.
* add `Spring.GetUnitStorage(unitID) → numbers metal, energy`.
* added `Game.buildGridResolution`, number which is currently 2. This means that buildings created via native build orders
are aligned to 2 squares.
* add `Platform.totalRAM`, in megabytes.
* added `Engine.gameSpeed` and `Engine.textColorCodes`, same as the existing entries in `Game.`.
The practical effect is that the Engine table is available in some LuaParser environments that Game isn't.
* missiles now obey `myGravity` when expired.
* NaN and infinity coming from Lua is now sometimes rejected. Coverage isn't yet comprehensive.
* `socket.lua` moved from being a loosely distributed file under `./socket.lua` to basecontent `./LuaSocket/socket.lua`.
* add `experience.experienceGrade` number modrule, same as calling Spring.SetExperienceGrade.
* the `allowHoverUnitStrafing` modrule now defaults to `false`. Previously it defaulted to `false` for HAPFS and `true` for QTPFS.
* bumpmapped water (aka `/water 4`) now has a different default texture.

### Fixes
* fix `Spring.SetUnitHealth(build < 1)` not reverting the unit into a nanoframe.
* fix CPU pinning, no longer tries to pin itself to bad choices (efficiency cores,
hyperthreads on the same physical core, performance cores on a dedicated server)
* fixed `Spring.ShareResources(teamID, "units", nil)` breaking due to the explicit nil.
* fix scribblings and labels breaking on maps larger than 64xN.
* fix basecontent `actions.lua` providing an incorrect `KeyAction` handler.
* fix height of buildings under construction not updating properly.
* fix landed aircraft starting to levitate when EMPed.
* fix units being stuck if an overlapping push-resistant unit stops.
* fix the "modern" sky renderer not adjusting to changes via `Spring.SetAtmosphere`.

### GL object type constants
For use with the new `gl.ObjectLabel` (see above):
* `GL.BUFFER`
* `GL.SHADER`
* `GL.PROGRAM`
* `GL.VERTEX_ARRAY`
* `GL.QUERY`
* `GL.PROGRAM_PIPELINE`
* `GL.TRANSFORM_FEEDBACK`
* `GL.RENDERBUFFER`
* `GL.FRAMEBUFFER`
