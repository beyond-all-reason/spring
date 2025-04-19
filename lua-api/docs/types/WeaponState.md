---
layout: default
title: WeaponState
parent: Lua API
permalink: lua-api/types/WeaponState
---

{% raw %}

# class WeaponState





Parameter for weapon states

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaSyncedCtrl.cpp#L2234-L2257" target="_blank">source</a>]





## fields


### WeaponState.reloadState

```lua
WeaponState.reloadState : integer?
```




### WeaponState.reloadFrame

```lua
WeaponState.reloadFrame : integer?
```



Alias for `reloadState`.


### WeaponState.reloadTime

```lua
WeaponState.reloadTime : number?
```




### WeaponState.reaimTime

```lua
WeaponState.reaimTime : integer?
```




### WeaponState.accuracy

```lua
WeaponState.accuracy : number?
```




### WeaponState.sprayAngle

```lua
WeaponState.sprayAngle : number?
```




### WeaponState.range

```lua
WeaponState.range : number?
```



If you change the range of a weapon with dynamic damage make sure you use `SetUnitWeaponDamages` to change dynDamageRange as well.


### WeaponState.projectileSpeed

```lua
WeaponState.projectileSpeed : number?
```




### WeaponState.burst

```lua
WeaponState.burst : integer?
```




### WeaponState.burstRate

```lua
WeaponState.burstRate : number?
```




### WeaponState.projectiles

```lua
WeaponState.projectiles : integer?
```




### WeaponState.salvoLeft

```lua
WeaponState.salvoLeft : integer?
```




### WeaponState.nextSalvo

```lua
WeaponState.nextSalvo : integer?
```




### WeaponState.aimReady

```lua
WeaponState.aimReady : number?
```



Set to `true` if a non-zero value is passed, `false` is zero is passed.


### WeaponState.forceAim

```lua
WeaponState.forceAim : integer?
```




### WeaponState.avoidFlags

```lua
WeaponState.avoidFlags : integer?
```




### WeaponState.collisionFlags

```lua
WeaponState.collisionFlags : integer?
```




### WeaponState.ttl

```lua
WeaponState.ttl : number?
```



How many seconds the projectile should live




{% endraw %}