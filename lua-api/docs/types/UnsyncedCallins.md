---
layout: default
title: UnsyncedCallins
parent: Lua API
permalink: lua-api/types/UnsyncedCallins
---

# class UnsyncedCallins





[<a href="https://github.com/beyond-all-reason/spring/blob/4d8d14a2d6cb762d118e3e50f41c850673f13ca9/rts/Lua/LuaHandleSynced.cpp#L165-L168" target="_blank">source</a>]

Functions called by the Engine (Unsynced).



## methods


### UnsyncedCallins.RecvFromSynced

```lua
function UnsyncedCallins.RecvFromSynced(
  arg1: any,
  arg2: any,
  argn: any
)
```





Receives data sent via `SendToUnsynced` callout.

[<a href="https://github.com/beyond-all-reason/spring/blob/4d8d14a2d6cb762d118e3e50f41c850673f13ca9/rts/Lua/LuaHandleSynced.cpp#L176-L182" target="_blank">source</a>]


### UnsyncedCallins.DrawUnit

```lua
function UnsyncedCallins.DrawUnit(
  unitID: integer,
  drawMode: number
) -> suppressEngineDraw boolean
```





For custom rendering of units

[<a href="https://github.com/beyond-all-reason/spring/blob/4d8d14a2d6cb762d118e3e50f41c850673f13ca9/rts/Lua/LuaHandleSynced.cpp#L209-L215" target="_blank">source</a>]


### UnsyncedCallins.DrawFeature

```lua
function UnsyncedCallins.DrawFeature(
  featureID: integer,
  drawMode: number
) -> suppressEngineDraw boolean
```





For custom rendering of features

[<a href="https://github.com/beyond-all-reason/spring/blob/4d8d14a2d6cb762d118e3e50f41c850673f13ca9/rts/Lua/LuaHandleSynced.cpp#L245-L251" target="_blank">source</a>]


### UnsyncedCallins.DrawShield

```lua
function UnsyncedCallins.DrawShield(
  featureID: integer,
  weaponID: integer,
  drawMode: number
) -> suppressEngineDraw boolean
```





For custom rendering of shields.

[<a href="https://github.com/beyond-all-reason/spring/blob/4d8d14a2d6cb762d118e3e50f41c850673f13ca9/rts/Lua/LuaHandleSynced.cpp#L280-L287" target="_blank">source</a>]


### UnsyncedCallins.DrawProjectile

```lua
function UnsyncedCallins.DrawProjectile(
  projectileID: integer,
  drawMode: number
) -> suppressEngineDraw boolean
```





For custom rendering of weapon (& other) projectiles

[<a href="https://github.com/beyond-all-reason/spring/blob/4d8d14a2d6cb762d118e3e50f41c850673f13ca9/rts/Lua/LuaHandleSynced.cpp#L318-L324" target="_blank">source</a>]


### UnsyncedCallins.DrawMaterial

```lua
function UnsyncedCallins.DrawMaterial(
  uuid: number,
  drawMode: number
) -> suppressEngineDraw boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/4d8d14a2d6cb762d118e3e50f41c850673f13ca9/rts/Lua/LuaHandleSynced.cpp#L355-L361" target="_blank">source</a>]




