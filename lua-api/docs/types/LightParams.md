---
layout: default
title: LightParams
parent: Lua API
permalink: lua-api/types/LightParams
---

# class LightParams





Parameters for lighting

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaUnsyncedCtrl.cpp#L1514-L1540" target="_blank">source</a>]





## fields


### LightParams.position

```lua
LightParams.position : { px: number,py: number,pz: number }
```




### LightParams.direction

```lua
LightParams.direction : { dx: number,dy: number,dz: number }
```




### LightParams.ambientColor

```lua
LightParams.ambientColor : { blue: number,red: number,green: number }
```




### LightParams.diffuseColor

```lua
LightParams.diffuseColor : { blue: number,green: number,red: number }
```




### LightParams.specularColor

```lua
LightParams.specularColor : { blue: number,green: number,red: number }
```




### LightParams.intensityWeight

```lua
LightParams.intensityWeight : { diffuseWeight: number,ambientWeight: number,specularWeight: number }
```




### LightParams.ambientDecayRate

```lua
LightParams.ambientDecayRate : { ambientBlueDecay: number,ambientRedDecay: number,ambientGreenDecay: number }
```



Per-frame decay of `ambientColor` (spread over TTL frames)


### LightParams.diffuseDecayRate

```lua
LightParams.diffuseDecayRate : { diffuseBlueDecay: number,diffuseRedDecay: number,diffuseGreenDecay: number }
```



Per-frame decay of `diffuseColor` (spread over TTL frames)


### LightParams.specularDecayRate

```lua
LightParams.specularDecayRate : { specularGreenDecay: number,specularBlueDecay: number,specularRedDecay: number }
```



Per-frame decay of `specularColor` (spread over TTL frames)


### LightParams.decayFunctionType

```lua
LightParams.decayFunctionType : { ambientDecayType: number,diffuseDecayType: number,specularDecayType: number }
```



If value is `0.0` then the `*DecayRate` values will be interpreted as linear, otherwise exponential.


### LightParams.radius

```lua
LightParams.radius : number
```




### LightParams.fov

```lua
LightParams.fov : number
```




### LightParams.ttl

```lua
LightParams.ttl : number
```




### LightParams.priority

```lua
LightParams.priority : number
```




### LightParams.ignoreLOS

```lua
LightParams.ignoreLOS : boolean
```




