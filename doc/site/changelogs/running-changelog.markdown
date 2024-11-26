---
layout: post
title: Running changelog
parent: Changelogs
permalink: changelogs/running-changelog
author: sprunk
---

This is the changelog **since version 2590**.

# Caveats
These are the entries which may require special attention when migrating:
* missing models crash the game in order to avoid desyncs (for real this time).
No pop-up, must look for error messages in the infolog.
* a bunch of changes in Lua events for damage and death, see below.
* moved sky rendering right after the terrain rendering and before `DrawWorldPreUnit`.
This will affect things drawn in the pre-unit layer by wupgets.
* changing camera mode (TA, rotatable overhead etc) now tries to keep rotation/position
instead of going back to the rotation/position it was last in when in that mode.
* allow the attack command onto units out of map. Ground-targeted still not allowed.
* allow the guard command when either the guard or the guardee are out of map.
* weapons with `groundBounce = true` will now bounce even if `numBounces`
is left undefined. This is because `numBounces = -1`, which is the default
value, will now result in infinite bounces as intended instead of 0.
* explicitly specified weapon def `scarTTL` value is now the actual TTL.
Previously it was multiplied by the ground decals level springsetting,
which defaults to 3, so if you had tweaked scars they may turn out to have
significantly different TTL now.
* live mobile units' footprint is now the size of their movedef. This affects
things like responding to bugger-off, blocking construction, or the size of their
built-in selection square. Note that unit defs are unaffected and that individual
units adjust if their movedef changes at runtime.
* support for non-power-of-2 textures, float textures, and frame buffer objects
is now mandatory. Apparently these were all common 15 years ago already so should
be safe even for relative potatoes.
* removed `tdfID` from weapon defs, including the WeaponDefs table. Any remaining
interfaces receive the weaponDefID instead.


# Features

### rmlUI
RmlUI is here! It is a GUI framework that will let you create widgets using web technologies (html, css and all that nonsense).
See [the reference](https://mikke89.github.io/RmlUiDoc/pages/lua_manual/api_reference.html) for the Lua API; it is available to LuaUI under the `RmlUi` global.
Differences compared to upstream:
* `context:LoadDocument(filepath, widget)` takes a second argument that is the widget itself,
so that inline event handlers can call its functions like so: `onclick="widget:OnReloadClick()"`.
* `document` is available too.
* our data model is reactive only on the first depth level.
All keys for reactivity are expected to exist at creation.
Behaviour is unspecified if the same data model is "opened" repeatedly.
* there is a custom html tag called `texture` that provides access to engine textures, e.g. `src="$heightmap"`

Look for upcoming examples in games, especially BAR.
The API is currently v1 and may be subject to change, use the new `Engine.FeatureSupport.rmlUiApiVersion` var for compat (see below).

### Feature support table

There is a new `Engine.FeatureSupport` table containing various feature support checks.
Use for engine version compatibility, similar to existing `Script.IsEngineMinVersion`,
except self-documenting and does not assume linear availability / commit numbering.

So far contains two vars:
* `rmlUiApiVersion` = `1`
* `hasExitOnlyYardmaps` = `true`

More will be added in the future as new features are added.

### Death events
* `wupget:UnitDestroyed` will pass the builder as the killer if a unit gets reclaimed. Note that
reclaim still does not generate `UnitDamaged` events.
* `wupget:UnitDestroyed` now receives a 7th argument, weaponDefID, with the cause of death.
Widgets receive this info regardless of LoS on the attacker (no new hax, `UnitDamaged` already did this).
* the weaponDefID above is never `nil`, all causes of death are attributable including things like
"was being built in a factory, got cancelled" or "died automatically due to `isFeature` def tag".
Added a bunch of `Game.envDamageTypes` constants for this purpose. See the table at the bottom of the page.
* the 1000000 damage that applies to units being transported in a non-`releaseHeld` transport when it
dies for non-selfD reasons will now be attributed to the new `TransportKilled` damage type in `UnitDamaged`,
previously was `Killed`.
* construction decay now produces an event: `wupget:UnitConstructionDecayed(unitID, unitDefID, unitTeam, timeSinceLastBuild, iterationPeriod, part)`.
Time and iteration period are in seconds, part is a fraction.

### Dolly camera
Added a dolly camera that follows a predetermined path, activated via Lua.
Added the following functions to the `Spring` table, check the main Lua API docs for their signatures and what they do:
* `RunDollyCamera`
* `PauseDollyCamera`
* `ResumeDollyCamera`
* `SetDollyCameraMode`
* `SetDollyCameraPosition`
* `SetDollyCameraCurve`
* `SetDollyCameraLookCurve`
* `SetDollyCameraLookPosition`
* `SetDollyCameraLookUnit`
* `SetDollyCameraRelativeMode`
* `SolveNURBSCurve`

### Other camera work
* add `CamTransitionMode` integer springsetting to control how discrete camera movement is interpolated.
0 - exponential decay, 1 - spring dampened, 2 - spring dampened timed, 3 - linear. Defaults to 0 - exponential decay.
* add `CamSpringHalflife` float springsetting, the time in milliseconds at which the timed spring dampened interpolation mode should be approximately halfway towards the goal.
* add `OverheadMinZoomDistance` and `CamSpringMinZoomDistance` float springsettings to set minimum camera zoom distances for their respective cameras. Note that this is the distance in the looking direction of the frustum and not height.
* changing camera mode (TA, rotatable overhead etc) now tries to keep rotation/position
instead of going back to the rotation/position it was last in when in that mode.

### Out of map orders
* allow centering area commands out of map.
* allow the attack command onto units out of map. Ground-targeted still not allowed
* allow the guard command when either the guard or the guardee are out of map

### Lua microoptimisation
* add `math.normalize(x1, x2, ...) → numbers xn1, xn2, ...`. Normalizes a vector. Can have any dimensions (pass and receive each as a separate value).
Returns a zero vector if passed a zero vector.
* add `Spring.AllocateTable(arraySlots, hashSlots) → {}`. Returns an empty table with more space allocated.
Use as a microoptimisation when you have a big table which you are going to populate with a known number of elements, for example `#UnitDefs`.
* `Script.LuaXYZ.Foo()` no longer produces a warning if there is no such function in the LuaXYZ environment. In practice this means you
can avoid calling `Script.LuaXYZ("Foo")` each time, but it can still be a good idea e.g. to avoid needless calculation or to warn manually.
* add variadic variants of LUS `Turn`, `Move`, `Spin`, `StopSpin`, `Explode`, and `SetPieceVisibility`, each has the same name with "Multi" prepended (so `MultiTurn` etc).
These accept multiple full sets of arguments compared to the regular function so you can avoid extra function calls.

### Rendering
* engine now draws the sky before the `wupget:DrawWorldPreUnit` layer.
* `wupget:DrawWorldPreParticles` now has four boolean parameters depending on which phase is being drawn: above water, below water, reflection, refraction.
They aren't mutually exclusive.
* drawing now occurs once every 30s when minimized (was intended already but didn't actually happen).
* add `MinSampleShadingRate` springsetting, float between 0 and 1, default 0. A value of 1 indicates that each sample in the framebuffer should be independently shaded. A value of 0 effectively allows rendering to ignore sample rate shading. Any value between 0 and 1 allows the GL to shade only a subset of the total samples within each covered fragment.
* add new 'm' character to Lua texture options, which disables trilinear mipmap filtering for both DDS and uncompressed textures.
* particles (incl. ground decals) now have 4 levels of mipmaps.

### Defs
* add `windup` weapon def tag. Delay in seconds before the first projectile of a salvo appears.
Has the same mechanics as burst (obeys the recent out-of-arc tags and the delay can be set/read via burst Lua API).
* live mobile units' footprint is now the size of their movedef. This affects
things like responding to bugger-off, blocking construction, or the size of their
built-in selection square. Note that unit defs are unaffected and that individual
units adjust if their movedef changes at runtime.
* weapon defs now have a new `animParamsN` (N = 1-4) tag for flipbook animations for given texture 1-4, same format as CEGs (three numbers: sprite count X, Y, and duration).
* removed `tdfID` from weapons.

### Misc
* add `SelectThroughGround` float springsetting. Controls how far through ground you can single-click a unit. Default is 200, in elmos (same behaviour as previous).
* add `Spring.ForceUnitCollisionUpdate(unitID) → nil`. Forces a unit to have correct collisions. Normally, collisions are updated according
to the `unitQuadPositionUpdateRate` modrule, which may leave them unable to be hit by some weapons when moving. Call this for targets of important
weapons (e.g. in `script.FireWeapon` if it's hitscan) if the modrule has a value greater than 1 to ensure reliable hit detection.
* built-in endgame graphs have a toggle for log scale instead of linear.
* `gl.SaveImage` can now save in the `.hdr` format (apparently).
* `pairs()` now looks at the `__pairs` metamethod in tables, same as in Lua 5.2.

## Fixes
* fix draw position for asymmetric models, they no longer disappear when not appropriate.
* fix streaming very small sound files.
* fix `Spring.SetCameraTarget` reverting to the old position.
* fix the `/iconsHideWithUI` command not saving the relevant springsetting properly.
* fix `Spring.GetFeatureSelectionVolumeData` missing from the API since about 105-800ish.
* fix `/groundDecals` not enabling decals if they were disabled at engine startup.
* fix rightclicking and area-commands sometimes failing to include radar dots if they wobbled too far from real position.
* fix runtime metalmap adjustments not being saved/loaded (would use map default).
* fix metal extractors not being saved/loaded correctly (would allow duplicates).
* fix an issue where sometimes `C:\a\_temp\msys\msys64\var\cache\fontconfig` would be created on the user's disk.
* the `quadFieldQuadSizeInElmos` modrule now only accepts powers of two instead of breaking at the edges of the map.

### Death damage types listing

| Game.envDamageTypes.??? | Description                                                                                                                                                       |
|-------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| AircraftCrashed         | Aircraft hitting the ground                                                                                                                                       |
| Kamikaze                | Unit exploding due to its kamikaze ability                                                                                                                        |
| SelfD                   | Unit exploding after a self-D command and countdown                                                                                                               |
| ConstructionDecay       | Abandoned nanoframe disappearing (the process itself is HP removal)                                                                                               |
| Reclaimed               | Killed via reclaim (the process itself is HP removal)                                                                                                             |
| TurnedIntoFeature       | Unit dying on completion, without explosion, due to `isFeature` def tag                                                                                           |
| TransportKilled         | Unit was in transport which had no `releaseHeld`. If the transport was not self-destructed, the unit also receives 1000000 damage of this type before dying.      |
| FactoryKilled           | Unit was being built in a factory which died. No direct way to discern how exactly the factory died currently, you'll have to wait for the factory's death event. |
| FactoryCancel           | Unit was being built in a factory but the order was cancelled.                                                                                                    |
| UnitScript              | COB unit script ordered the unit's death. Note that LUS has access to normal kill/damage interfaces instead.                                                      |
| SetNegativeHealth       | A unit had less than 0 health for non-damage reasons (e.g. Lua set it so).                                                                                        |
| OutOfBounds             | A unit was thrown way out of map bounds.                                                                                                                          |
| KilledByCheat           | The `/remove` or `/destroy` commands were used.                                                                                                                   |
| KilledByLua             | Default cause of death when using `Spring.DestroyUnit`.                                                                                                           |

`KilledByLua` is guaranteed to be the "last" value, so you can define your own custom damage types for Lua mechanics with a guarantee of no overlap via e.g.
```
Game.envDamageTypes.CullingStrike      = Game.KilledByLua - 1
Game.envDamageTypes.SummonTimerExpired = Game.KilledByLua - 2
```
