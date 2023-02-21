---
layout: default
title: Migrating from Spring
permalink: /migrating-from-spring/
nav_order: 4
---

# Migrating from Spring

While ReCoil is mostly compatible with Spring 105 some divergences naturally
developed over time.

Here we list the most relevant breaking changes and how to fix them when
migrating from Spring.

## Camera modifiers

The following keyboard modifiers were unhardcoded from engine:

- On spring camera: rotating on the y axis (yaw) with ALT and middle mouse
button pressed while moving the cursor.
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
  if builderID and Spring.GetUnitCurrentCommand(builderID) == CMD.RESURRECT then
    Spring.SetUnitHealth(unitID, Spring.GetUnitHealth(unitID) * 0.05)
  end
end
```

## Spring.Marker usage

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

## Spring.GetConfig usage

`Spring.GetConfigInt`, `Spring.GetConfigFloat` and `Spring.GetConfigString` now
accept nil as the second argument (what to return by default if not set).
Previously it was treated as 0 (Int and Float) or "" (String).
To get back previous behaviour:

```lua
local originalGetConfigInt = Spring.GetConfigInt
Spring.GetConfigInt = function(key, def)
 return originalGetConfigInt(key, def) or 0
end
```

[Spring.Marker]: https://beyond-all-reason.github.io/spring/ldoc/modules/UnsyncedCtrl.html#Markers
