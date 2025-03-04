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

[<a href="https://github.com/beyond-all-reason/spring/blob/f30aea20ae92a148009e573d844c20fa0bb31671/rts/Lua/LuaSyncedCtrl.cpp#L6940-L6959" target="_blank">source</a>]

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




