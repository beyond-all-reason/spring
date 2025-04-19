---
layout: default
title: WaterParams
parent: Lua API
permalink: lua-api/types/WaterParams
---

{% raw %}

# class WaterParams





Water params

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaUnsyncedCtrl.cpp#L4312-L4354" target="_blank">source</a>]





## fields


### WaterParams.absorb

```lua
WaterParams.absorb : rgb {
    [1]: number,
    [2]: number,
    [3]: number,
}
```




### WaterParams.baseColor

```lua
WaterParams.baseColor : rgb {
    [1]: number,
    [2]: number,
    [3]: number,
}
```




### WaterParams.minColor

```lua
WaterParams.minColor : rgb {
    [1]: number,
    [2]: number,
    [3]: number,
}
```




### WaterParams.surfaceColor

```lua
WaterParams.surfaceColor : rgb {
    [1]: number,
    [2]: number,
    [3]: number,
}
```




### WaterParams.diffuseColor

```lua
WaterParams.diffuseColor : rgb {
    [1]: number,
    [2]: number,
    [3]: number,
}
```




### WaterParams.specularColor

```lua
WaterParams.specularColor : rgb {
    [1]: number,
    [2]: number,
    [3]: number,
}
```




### WaterParams.planeColor

```lua
WaterParams.planeColor : rgb {
    [1]: number,
    [2]: number,
    [3]: number,
}
```




### WaterParams.texture

```lua
WaterParams.texture : string
```



file


### WaterParams.foamTexture

```lua
WaterParams.foamTexture : string
```



file


### WaterParams.normalTexture

```lua
WaterParams.normalTexture : string
```



file


### WaterParams.damage

```lua
WaterParams.damage : number
```




### WaterParams.repeatX

```lua
WaterParams.repeatX : number
```




### WaterParams.repeatY

```lua
WaterParams.repeatY : number
```




### WaterParams.surfaceAlpha

```lua
WaterParams.surfaceAlpha : number
```




### WaterParams.ambientFactor

```lua
WaterParams.ambientFactor : number
```




### WaterParams.diffuseFactor

```lua
WaterParams.diffuseFactor : number
```




### WaterParams.specularFactor

```lua
WaterParams.specularFactor : number
```




### WaterParams.specularPower

```lua
WaterParams.specularPower : number
```




### WaterParams.fresnelMin

```lua
WaterParams.fresnelMin : number
```




### WaterParams.fresnelMax

```lua
WaterParams.fresnelMax : number
```




### WaterParams.fresnelPower

```lua
WaterParams.fresnelPower : number
```




### WaterParams.reflectionDistortion

```lua
WaterParams.reflectionDistortion : number
```




### WaterParams.blurBase

```lua
WaterParams.blurBase : number
```




### WaterParams.blurExponent

```lua
WaterParams.blurExponent : number
```




### WaterParams.perlinStartFreq

```lua
WaterParams.perlinStartFreq : number
```




### WaterParams.perlinLacunarity

```lua
WaterParams.perlinLacunarity : number
```




### WaterParams.perlinAmplitude

```lua
WaterParams.perlinAmplitude : number
```




### WaterParams.windSpeed

```lua
WaterParams.windSpeed : number
```




### WaterParams.waveOffsetFactor

```lua
WaterParams.waveOffsetFactor : number
```




### WaterParams.waveLength

```lua
WaterParams.waveLength : number
```




### WaterParams.waveFoamDistortion

```lua
WaterParams.waveFoamDistortion : number
```




### WaterParams.waveFoamIntensity

```lua
WaterParams.waveFoamIntensity : number
```




### WaterParams.causticsResolution

```lua
WaterParams.causticsResolution : number
```




### WaterParams.causticsStrength

```lua
WaterParams.causticsStrength : number
```




### WaterParams.numTiles

```lua
WaterParams.numTiles : integer
```




### WaterParams.shoreWaves

```lua
WaterParams.shoreWaves : boolean
```




### WaterParams.forceRendering

```lua
WaterParams.forceRendering : boolean
```




### WaterParams.hasWaterPlane

```lua
WaterParams.hasWaterPlane : boolean
```






{% endraw %}