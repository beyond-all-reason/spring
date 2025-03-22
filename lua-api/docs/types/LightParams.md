---
layout: default
title: LightParams
parent: Lua API
permalink: lua-api/types/LightParams
---

# class LightParams





Parameters for lighting

[<a href="https://github.com/beyond-all-reason/spring/blob/4d8d14a2d6cb762d118e3e50f41c850673f13ca9/rts/Lua/LuaUnsyncedCtrl.cpp#L1567-L1593" target="_blank">source</a>]





## fields


### LightParams.position

```lua
LightParams.position : { py: number,px: number,pz: number }
```




### LightParams.direction

```lua
LightParams.direction : { dy: number,dx: number,dz: number }
```




### LightParams.ambientColor

```lua
LightParams.ambientColor : { green: number,blue: number,red: number }
```




### LightParams.diffuseColor

```lua
LightParams.diffuseColor : { blue: number,red: number,green: number }
```




### LightParams.specularColor

```lua
LightParams.specularColor : { green: number,blue: number,red: number }
```




### LightParams.intensityWeight

```lua
LightParams.intensityWeight : { diffuseWeight: number,specularWeight: number,ambientWeight: number }
```




### LightParams.ambientDecayRate

```lua
LightParams.ambientDecayRate : { ambientGreenDecay: number,ambientBlueDecay: number,ambientRedDecay: number }
```



Per-frame decay of `ambientColor` (spread over TTL frames)


### LightParams.diffuseDecayRate

```lua
LightParams.diffuseDecayRate : { diffuseGreenDecay: number,diffuseBlueDecay: number,diffuseRedDecay: number }
```



Per-frame decay of `diffuseColor` (spread over TTL frames)


### LightParams.specularDecayRate

```lua
LightParams.specularDecayRate : { specularBlueDecay: number,specularRedDecay: number,specularGreenDecay: number }
```



Per-frame decay of `specularColor` (spread over TTL frames)


### LightParams.decayFunctionType

```lua
LightParams.decayFunctionType : { specularDecayType: number,ambientDecayType: number,diffuseDecayType: number }
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




