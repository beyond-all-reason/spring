---
layout: default
title: LightParams
parent: Lua API
permalink: lua-api/types/LightParams
---

# class LightParams





Parameters for lighting

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L1566-L1592" target="_blank">source</a>]

---



## fields
---

### LightParams.ambientColor
---
```lua
LightParams.ambientColor : { red: number,green: number,blue: number }
```




### LightParams.ambientDecayRate
---
```lua
LightParams.ambientDecayRate : { ambientBlueDecay: number,ambientRedDecay: number,ambientGreenDecay: number }
```



Per-frame decay of `ambientColor` (spread over TTL frames)



### LightParams.decayFunctionType
---
```lua
LightParams.decayFunctionType : { ambientDecayType: number,diffuseDecayType: number,specularDecayType: number }
```



If value is `0.0` then the `*DecayRate` values will be interpreted as linear, otherwise exponential.



### LightParams.diffuseColor
---
```lua
LightParams.diffuseColor : { blue: number,green: number,red: number }
```




### LightParams.diffuseDecayRate
---
```lua
LightParams.diffuseDecayRate : { diffuseBlueDecay: number,diffuseGreenDecay: number,diffuseRedDecay: number }
```



Per-frame decay of `diffuseColor` (spread over TTL frames)



### LightParams.direction
---
```lua
LightParams.direction : { dy: number,dz: number,dx: number }
```




### LightParams.fov
---
```lua
LightParams.fov : number
```




### LightParams.ignoreLOS
---
```lua
LightParams.ignoreLOS : boolean
```




### LightParams.intensityWeight
---
```lua
LightParams.intensityWeight : { ambientWeight: number,diffuseWeight: number,specularWeight: number }
```




### LightParams.position
---
```lua
LightParams.position : { px: number,py: number,pz: number }
```




### LightParams.priority
---
```lua
LightParams.priority : number
```




### LightParams.radius
---
```lua
LightParams.radius : number
```




### LightParams.specularColor
---
```lua
LightParams.specularColor : { green: number,blue: number,red: number }
```




### LightParams.specularDecayRate
---
```lua
LightParams.specularDecayRate : { specularRedDecay: number,specularBlueDecay: number,specularGreenDecay: number }
```



Per-frame decay of `specularColor` (spread over TTL frames)


### LightParams.ttl
---
```lua
LightParams.ttl : number
```




