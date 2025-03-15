---
layout: default
title: WeaponState
parent: Lua API
permalink: lua-api/types/WeaponState
---

# class WeaponState





Parameter for weapon states

[<a href="https://github.com/beyond-all-reason/spring/blob/f0c829c6e793112ab93b1ec454b2e6f2a1767d4c/rts/Lua/LuaSyncedCtrl.cpp#L2221-L2243" target="_blank">source</a>]

---



## fields
---

### WeaponState.accuracy
---
```lua
WeaponState.accuracy : number?
```




### WeaponState.aimReady
---
```lua
WeaponState.aimReady : number?
```



Set to `true` if a non-zero value is passed, `false` is zero is passed.


### WeaponState.avoidFlags
---
```lua
WeaponState.avoidFlags : integer?
```




### WeaponState.burst
---
```lua
WeaponState.burst : integer?
```




### WeaponState.burstRate
---
```lua
WeaponState.burstRate : number?
```




### WeaponState.collisionFlags
---
```lua
WeaponState.collisionFlags : integer?
```




### WeaponState.forceAim
---
```lua
WeaponState.forceAim : integer?
```




### WeaponState.nextSalvo
---
```lua
WeaponState.nextSalvo : integer?
```




### WeaponState.projectileSpeed
---
```lua
WeaponState.projectileSpeed : number?
```




### WeaponState.projectiles
---
```lua
WeaponState.projectiles : integer?
```




### WeaponState.range
---
```lua
WeaponState.range : number?
```



If you change the range of a weapon with dynamic damage make sure you use `SetUnitWeaponDamages` to change dynDamageRange as well.


### WeaponState.reaimTime
---
```lua
WeaponState.reaimTime : integer?
```




### WeaponState.reloadFrame
---
```lua
WeaponState.reloadFrame : integer?
```



Alias for `reloadState`.


### WeaponState.reloadState
---
```lua
WeaponState.reloadState : integer?
```




### WeaponState.reloadTime
---
```lua
WeaponState.reloadTime : number?
```




### WeaponState.salvoLeft
---
```lua
WeaponState.salvoLeft : integer?
```




### WeaponState.sprayAngle
---
```lua
WeaponState.sprayAngle : number?
```




