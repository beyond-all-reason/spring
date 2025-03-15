---
layout: default
title: ExplosionParams
parent: Lua API
permalink: lua-api/types/ExplosionParams
---

# class ExplosionParams





Parameters for explosion.

Please note the explosion defaults to 1 damage regardless of what it's defined in the weaponDef.
The weapondefID is only used for visuals and for passing into callins like UnitDamaged.

[<a href="https://github.com/beyond-all-reason/spring/blob/f0c829c6e793112ab93b1ec454b2e6f2a1767d4c/rts/Lua/LuaSyncedCtrl.cpp#L6958-L6977" target="_blank">source</a>]

---



## fields
---

### ExplosionParams.craterAreaOfEffect
---
```lua
ExplosionParams.craterAreaOfEffect : number
```




### ExplosionParams.damageAreaOfEffect
---
```lua
ExplosionParams.damageAreaOfEffect : number
```




### ExplosionParams.damageGround
---
```lua
ExplosionParams.damageGround : boolean
```




### ExplosionParams.edgeEffectiveness
---
```lua
ExplosionParams.edgeEffectiveness : number
```




### ExplosionParams.explosionSpeed
---
```lua
ExplosionParams.explosionSpeed : number
```




### ExplosionParams.gfxMod
---
```lua
ExplosionParams.gfxMod : number
```




### ExplosionParams.hitFeature
---
```lua
ExplosionParams.hitFeature : number
```




### ExplosionParams.hitUnit
---
```lua
ExplosionParams.hitUnit : number
```




### ExplosionParams.ignoreOwner
---
```lua
ExplosionParams.ignoreOwner : boolean
```




### ExplosionParams.impactOnly
---
```lua
ExplosionParams.impactOnly : boolean
```




### ExplosionParams.owner
---
```lua
ExplosionParams.owner : number
```




### ExplosionParams.weaponDef
---
```lua
ExplosionParams.weaponDef : number
```




