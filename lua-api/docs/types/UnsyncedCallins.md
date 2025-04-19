---
layout: default
title: UnsyncedCallins
parent: Lua API
permalink: lua-api/types/UnsyncedCallins
---

{% raw %}

# class UnsyncedCallins





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaHandleSynced.cpp#L166-L169" target="_blank">source</a>]

Functions called by the Engine (Unsynced).



## methods


### UnsyncedCallins.RecvFromSynced

```lua
function UnsyncedCallins.RecvFromSynced(...: any)
```





Receives data sent via `SendToUnsynced` callout.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaHandleSynced.cpp#L177-L181" target="_blank">source</a>]


### UnsyncedCallins.DrawUnit

```lua
function UnsyncedCallins.DrawUnit(
  unitID: integer,
  drawMode: number
) -> suppressEngineDraw boolean
```





For custom rendering of units

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaHandleSynced.cpp#L208-L215" target="_blank">source</a>]


### UnsyncedCallins.DrawFeature

```lua
function UnsyncedCallins.DrawFeature(
  featureID: integer,
  drawMode: number
) -> suppressEngineDraw boolean
```





For custom rendering of features

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaHandleSynced.cpp#L245-L252" target="_blank">source</a>]


### UnsyncedCallins.DrawShield

```lua
function UnsyncedCallins.DrawShield(
  featureID: integer,
  weaponID: integer,
  drawMode: number
) -> suppressEngineDraw boolean
```





For custom rendering of shields.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaHandleSynced.cpp#L281-L289" target="_blank">source</a>]


### UnsyncedCallins.DrawProjectile

```lua
function UnsyncedCallins.DrawProjectile(
  projectileID: integer,
  drawMode: number
) -> suppressEngineDraw boolean
```





For custom rendering of weapon (& other) projectiles

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaHandleSynced.cpp#L320-L327" target="_blank">source</a>]


### UnsyncedCallins.DrawMaterial

```lua
function UnsyncedCallins.DrawMaterial(
  uuid: integer,
  drawMode: number
) -> suppressEngineDraw boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaHandleSynced.cpp#L358-L365" target="_blank">source</a>]






{% endraw %}