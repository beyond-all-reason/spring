---
layout: post
title: Release 105-966
parent: Changelogs
permalink: changelogs/changelog-105-966
author: sprunk
---

The changelog since release 105-941 **until minor release 105-966**, which happened in June 2022.

## Caveats
* removed LOD unit rendering (far 2d sprites) including `/distdraw` command and `UnitLodDist` springconfig entry
* hovercraft are no longer forced to be upright regardless on the terrain slope they are travelling over, they now obey the `upright` unit def.
* the `Up+` modifier in keybinds is now deprecated, a warning is displayed if attempted to bind. No change in behaviour though.

## Features and fixes

### Keybinds
* `wupget:KeyPress` and `wupget:KeyRelease` callins receive an additional scanCode parameter.
* `sc_foo` keysets are introduced for scancodes and handled in conjunction with keycode bindings preserving the order on which the actions where bound. `Any+` order is preserved (`Any+` bindings are at the bottom)
* `/keysave` and `/keyprint` now preserve the binding order, providing a better output
* extend `Spring.GetKeyBindings` to receive an additional keyset string to retrieve scancode actions
* add `Spring.GetScanSymbol` callout in a similar fashion to `Spring.GetKeySymbol`
* basecontent wupget handlers are modified to support scancodes. Games that wish to support lua actions via scancodes are recommended to adopt these changes, otherwise behavior should be identical to before.
* fix issue where keysets could be bound to duplicate actions when the action line contained different comments
* fix issue where name retrieval for keycodes would never return a non hex value, this affected the label parameter of KeyPress and KeyRelease callins
* fix issue where `Spring.GetKeyBindings` would return incorrect actions when keychains were bound, e.g. if actions bound to m,n and b,n both would be returned on GetKeyBindings('n') regardless of the previous pressed key

### Rendering
* add `gl.GetEngineAtlasTextures("$explosions" | "$groundfx") → { texName = {x1, y1, x2, y2}, texName = {...}, ... }`.
* atlasses now have a deterministic order given an identical alphabetical ordering of identically sized textures. This allows Lua construction of pairs of atlasses, e.g. pairs of diffuse and normal maps for multiple decals.

### Aircraft smooth mesh
* The Smooth Height Mesh will now respond to changes in the height map and recalculate itself over the impacted area. This means aircraft using the smooth mesh will continue to be able to navigate appropriately after extended battles have modified the terrain significantly.
* smooth mesh can now be disabled with the boolean modrule `system.enableSmoothMesh`.

### Original heightmap
Lua can now change what is considered the "original" heightmap, mostly for random map generators. These new interfaces work the same as the existing ones for the current heightmap.
* `Spring.AdjustOriginalHeightMap(x1, z1[, x2, z2], height) → nil`, adds height.
* Spring.LevelOriginalHeightMap(x1, z1[, x2, z2], height) → nil`, sets height.
* `Spring.RevertOriginalHeightMap(x1, z1[, x2, z2], factor) → nil`, reverts to the "original original" heightmap as defined in the map file; `factor` is a 0-1 value that interpolates between the current and the map file height.
* `Spring.SetOriginalHeightMapFunc(func, arg1, arg2, ...) → number totalChange`, calls a function that can call one of the two functions below.
* `Spring.AddOriginalHeightMap(x, z, height) → newHeight`, can only be called from `Spring.SetOriginalHeightMapFunc` and adds height at given co-ordinates.
* `Spring.SetOriginalHeightMap(x, z, height[, factor= 1]) → heightChange`, can only be called from `Spring.SetOriginalHeightMapFunc` and interpolates height at given co-ordinates towards the given value with given factor.

Usage example:
```lua
Spring.SetOriginalHeightMapFunc(function(amplitude, period)
	for z = 0, Game.mapSizeZ, Game.squareSize do
	for x = 0, Game.mapSizeX, Game.squareSize do
		Spring.SetOriginalHeightMap(x, z, 100 + amplitude * math.cos((x + z) / period))
	end end
end, 123, 456) -- args to pass (in this case amplitude and period)
```

### Miscellaneous
* add `Spring.AddUnitExperience(unitID, delta_xp)`. Can subtract, but the result will be clamped to 0 if negative.
* fixed the `/crash` command to crash properly.
