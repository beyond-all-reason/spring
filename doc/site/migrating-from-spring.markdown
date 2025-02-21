---
layout: default
title: Migrating from Spring
permalink: /migrating-from-spring/
nav_order: 5
---

# Migrating from Spring
{: .no_toc }

While Recoil is mostly compatible with Spring 105 some divergences naturally
developed over time.

Here we list the most relevant breaking changes and how to fix them when
migrating from Spring.

<details open markdown="block">
  <summary>
    Table of contents
  </summary>
  {: .text-delta }
- TOC
{:toc}
</details>

## Callouts

### Font rendering

Rendering fonts now obeys GL color state. This means that sometimes text will
not be the same color as previously. To get back previous behaviour, you might
need to add
[`gl.Color`](https://beyond-all-reason.github.io/spring/ldoc/modules/OpenGL.html#gl.Text)
in front of
[`gl.Text`](https://beyond-all-reason.github.io/spring/ldoc/modules/OpenGL.html#gl.Color)
calls. Alternatively, you can add inline colour codes (`\255\rrr\ggg\bbb`).

### Spring.Marker usage

All three [Spring.Marker] functions (Point, Line, Erase) no longer accept
numerical 0 as false for `onlyLocal` (now follows regular Lua language rules
where 0 is true). To get back old behaviour, change `0` to `false`, e.g.:

```diff
-Spring.MarkerAddPoint(x, y, z, text, 0)
+Spring.MarkerAddPoint(x, y, z, text, false)
```

```diff
-Spring.MarkerAddLine(x1, y1, z1, x2, y2, z2, 0, playerId)
+Spring.MarkerAddLine(x1, y1, z1, x2, y2, z2, false, playerId)
```

```diff
-Spring.MarkerErasePosition(x, y, z, noop, 0, playerId)
+Spring.MarkerErasePosition(x, y, z, noop, false, playerId)
```

### Spring.GetConfig usage

[`Spring.GetConfigInt`](https://beyond-all-reason.github.io/spring/ldoc/modules/UnsyncedRead.html#Spring.GetConfigInt),
[`Spring.GetConfigFloat`](https://beyond-all-reason.github.io/spring/ldoc/modules/UnsyncedRead.html#Spring.GetConfigFloat) and
[`Spring.GetConfigString`](https://beyond-all-reason.github.io/spring/ldoc/modules/UnsyncedRead.html#Spring.GetConfigString)
now accept nil as the second argument (what to return by default if not set).
Previously it was treated as 0 (Int and Float) or "" (String).
To get back previous behaviour:

```lua
local originalGetConfigInt = Spring.GetConfigInt
Spring.GetConfigInt = function(key, def)
 return originalGetConfigInt(key, def) or 0
end
```

### Spring.GetSelectedUnits{Sorted,Counts}

The tables returned by
[`Spring.GetSelectedUnitsSorted`](https://beyond-all-reason.github.io/spring/ldoc/modules/UnsyncedRead.html#Spring.GetSelectedUnitsSorted) and
[`Spring.GetSelectedUnitsCounts`](https://beyond-all-reason.github.io/spring/ldoc/modules/UnsyncedRead.html#Spring.GetSelectedUnitsCounts)
no longer have an additional `n` key with corresponding value containing the
number of unitdefs.

Instead use the second return value, e.g.:

```diff
- local counts = Spring.GetSelectedUnitsCounts()
- local unitDefsCount = counts.n
- counts.n = nil
+ local counts, unitDefsCount = Spring.GetSelectedUnitsCounts()
```

## Stop command on manual share
Manually shared units no longer receive the Stop command.
Replicate the previous behaviour via the `UnitGiven` callin:
```lua
function wupget:UnitGiven(unitID, unitDefID, newTeam)
	if newTeam == Spring.GetMyTeamID() then -- if doing in unsynced
		Spring.GiveOrderToUnit(unitID, CMD.STOP, 0, 0)
	end
end
```

## Defs

- Hovercraft and ships brought out of water no longer forced to be upright.
To get back previous behaviour, put the `upright = true` tag in all unit defs
whose move def is of the hovercraft or ship type.
- Units with `useFootPrintCollisionVolume` but no `collisionVolumeScales` set
will now use the footprint volume (previously mistakenly used the model's sphere).

  To keep the old hitvolume, set `useFootPrintCollisionVolume` to false for units
  with no `collisionVolumeScales`. Assuming you apply `lowerkeys` to unit defs,
  this can also be achieved by putting the following in unit defs post-processing:

  ```lua
  for unitDefID, unitDef in pairs(UnitDefs) do
    if not unitDef.collisionvolumescales then
      unitDef.usefootprintcollisionvolume = nil
    end
  end
  ```
- Tree feature defs `treetype0` through `treetype16` are now provided by the
basecontent archive instead of the engine.

  No known games ship their own basecontent and they would know what to do if so.

- The `firestarter` weapon tag no longer capped at 10000 in defs (which
becomes 100 in Lua WeaponDefs after rescale), now uncapped.

  To get back previous behaviour, put the following in weapon defs
  post-processing:

  ```lua
  for weaponDefID, weaponDef in pairs(WeaponDefs) do
    if weaponDef.firestarter then
      weaponDef.firestarter = math.min(weaponDef.firestarter, 10000)
    end
  end
  ```

- It is heavily recommended to replace `acceleration` and `brakeRate`
with `maxAcc` and `maxDec` respectively (possibly in post-processing).
While the old spelling still works the way it always did, at some point
in the future it will be changed from elmo/frame to elmo/second.

## Camera modifiers

The following keyboard modifiers were unhardcoded from engine:

- On spring camera: rotating on the x (pitch) or y (yaw) axis with ALT and
middle mouse button pressed while moving the cursor.
- Resetting camera settings on ALT pressed and mousewheelscroll down.
- Rotating on the x axis (pitch) with CTRL pressed and mousewheelscroll.

If games and players do not change engine defaults, no action is needed,
however to enable these modifiers as previously the following keybindings must
be set:

```
bind Any+ctrl movetilt   // rotates the camera over the x axis on mousewheel move
bind Any+alt  movereset  // resets camera state to maxzoom/minzoom on mousewheel move, additionally resets tilt on Overhead cam
bind Any+alt  moverotate // rotates the camera in x and y axis on mmb move (Spring cam)
```

## Resurrecting units

Resurrecting units no longer overrides their health to 5%.
To get back the old behaviour define a gadget with the following callin:

```lua
function gadget:UnitCreated(unitID, unitDefID, teamID, builderID)
  if builderID and Spring.GetUnitWorkerTask(builderID) == CMD.RESURRECT then
    Spring.SetUnitHealth(unitID, Spring.GetUnitHealth(unitID) * 0.05)
  end
end
```

## VFS mapping API

`Spring.MapArchive` and `Spring.UnmapArchive` have been temporarily removed due to sync unsafety.
In the meantime, use `Spring.UseArchive`. These functions are going to come back at some point,
but there is no concrete timeline for that yet.

## Iterating synced proxy tables

Functions for iterating synced proxy tables: `snext`, `spairs` and `sipairs` have been removed.
These have been able to be replaced by the regular `next`, `pairs` and `ipairs` for some time
already (so the change can be done before migrating):
```diff
 local syncedProxyTable = SYNCED.foo
-for key, value in spairs(syncedProxyTable) do
+for key, value in  pairs(syncedProxyTable) do
```

## General

- Paletted image files are no longer accepted. Convert your images not to be paletted.
- The return value from the
[`UnitUnitCollision`](https://beyond-all-reason.github.io/spring/ldoc/modules/LuaHandle.html#UnitUnitCollision)
callin is now ignored and there is only one event for each collision.
There is no way to get back the old behaviour for now,
but if someone needs it it could be arranged.
- Removed the following constants:
  - `Platform.glSupport16bitDepthBuffer`
  - `Platform.glSupport24bitDepthBuffer`
  - `Platform.glSupport32bitDepthBuffer`

  To get back previous behaviour, replace with
  ```lua
  Platform.glSupportDepthBufferBitDepth >= 16 -- or 24, or 32, respectively
  ```
- Removed LOD rendering (2D unit billboards when zoomed out far), including the
`/distdraw` command and the `UnitLodDist` springsettings entry
- Removed the `AdvSky` springsetting and the `/dynamicsky` command,
which made clouds move across the sky. You cannot easily get back
previous behaviour, though you can probably achieve something similar
by rendering moving clouds yourself.
- The deprecated `Game.allowTeamColors`, whose value was always `true`, has been removed. Note that this inverts logic if you used it like a bool.
- The default `/screenshot` format was changed to PNG. Check any automated processing you do on these.
- Screenshots and replay demo files now have a different filename format, with an UTC timestamp. Check any automated file parsing you might have.

[Spring.Marker]: https://beyond-all-reason.github.io/spring/ldoc/modules/UnsyncedCtrl.html#Markers
