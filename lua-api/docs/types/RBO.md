---
layout: default
title: RBO
parent: Lua API
permalink: lua-api/types/RBO
---

# class RBO





User Data RBO

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaRBOs.cpp#L139-L148" target="_blank">source</a>]

---



## fields
---

### RBO.format
---
```lua
RBO.format : GL {
    ACCUM_BUFFER_BIT: number,
    ALL_ATTRIB_BITS: number,
    ALWAYS: number,
    AMBIENT: number,
    AND: number,
    AND_INVERTED: number,
    AND_REVERSE: number,
    ARRAY_BUFFER: number,
    BACK: number,
    BLEND: number,
    BYTE: number,
    CLAMP: number,
    ...
}
```




### RBO.samples
---
```lua
RBO.samples : integer
```



will return globalRendering->msaaLevel for multisampled RBO or 0 otherwise


### RBO.target
---
```lua
RBO.target : GL {
    ACCUM_BUFFER_BIT: number,
    ALL_ATTRIB_BITS: number,
    ALWAYS: number,
    AMBIENT: number,
    AND: number,
    AND_INVERTED: number,
    AND_REVERSE: number,
    ARRAY_BUFFER: number,
    BACK: number,
    BLEND: number,
    BYTE: number,
    CLAMP: number,
    ...
}
```




### RBO.valid
---
```lua
RBO.valid : boolean
```




### RBO.xsize
---
```lua
RBO.xsize : integer
```




### RBO.ysize
---
```lua
RBO.ysize : integer
```




