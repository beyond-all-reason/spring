---
layout: default
title: Platform
parent: Lua API
permalink: lua-api/globals/Platform
---

{% raw %}

# global Platform






## fields


### Platform.gpu

```lua
Platform.gpu : string
```



Full GPU device name


### Platform.gpuVendor

```lua
Platform.gpuVendor : ("Nvidia"|"Intel"|"ATI"|"Mesa"|"Unknown")
```




### Platform.glVersionShort

```lua
Platform.glVersionShort : string
```



`major.minor.buildNumber`


### Platform.glslVersionShort

```lua
Platform.glslVersionShort : string
```



`major.minor`


### Platform.glVersion

```lua
Platform.glVersion : string
```



Full version


### Platform.glVendor

```lua
Platform.glVendor : string
```




### Platform.glRenderer

```lua
Platform.glRenderer : string
```




### Platform.glslVersion

```lua
Platform.glslVersion : string
```



Full version


### Platform.gladVersion

```lua
Platform.gladVersion : string
```




### Platform.osName

```lua
Platform.osName : string
```



full name of the OS


### Platform.osFamily

```lua
Platform.osFamily : ("Windows"|"Linux"|"MacOSX"|"FreeBSD"|"Unknown")
```




### Platform.numDisplays

```lua
Platform.numDisplays : number
```




### Platform.gpuMemorySize

```lua
Platform.gpuMemorySize : number
```



Size of total GPU memory in MBs; only available for "Nvidia", (rest are 0)


### Platform.sdlVersionCompiledMajor

```lua
Platform.sdlVersionCompiledMajor : number
```




### Platform.sdlVersionCompiledMinor

```lua
Platform.sdlVersionCompiledMinor : number
```




### Platform.sdlVersionCompiledPatch

```lua
Platform.sdlVersionCompiledPatch : number
```




### Platform.sdlVersionLinkedMajor

```lua
Platform.sdlVersionLinkedMajor : number
```




### Platform.sdlVersionLinkedMinor

```lua
Platform.sdlVersionLinkedMinor : number
```




### Platform.sdlVersionLinkedPatch

```lua
Platform.sdlVersionLinkedPatch : number
```




### Platform.totalRAM

```lua
Platform.totalRAM : number
```



Total physical system RAM in MBs.


### Platform.glSupportNonPowerOfTwoTex

```lua
Platform.glSupportNonPowerOfTwoTex : boolean
```




### Platform.glSupportTextureQueryLOD

```lua
Platform.glSupportTextureQueryLOD : boolean
```




### Platform.glSupport24bitDepthBuffer

```lua
Platform.glSupport24bitDepthBuffer : boolean
```




### Platform.glSupportRestartPrimitive

```lua
Platform.glSupportRestartPrimitive : boolean
```




### Platform.glSupportClipSpaceControl

```lua
Platform.glSupportClipSpaceControl : boolean
```




### Platform.glSupportFragDepthLayout

```lua
Platform.glSupportFragDepthLayout : boolean
```






{% endraw %}