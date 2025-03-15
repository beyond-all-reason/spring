---
layout: default
title: AtmosphereParams
parent: Lua API
permalink: lua-api/types/AtmosphereParams
---

# class AtmosphereParams





[<a href="https://github.com/beyond-all-reason/spring/blob/f0c829c6e793112ab93b1ec454b2e6f2a1767d4c/rts/Lua/LuaUnsyncedCtrl.cpp#L3918-L3926" target="_blank">source</a>]

---



## fields
---

### AtmosphereParams.cloudColor
---
```lua
AtmosphereParams.cloudColor : rgba {
    a: number,
    b: number,
    g: number,
    r: number,
}
```




### AtmosphereParams.fogEnd
---
```lua
AtmosphereParams.fogEnd : number
```




### AtmosphereParams.fogStart
---
```lua
AtmosphereParams.fogStart : number
```




### AtmosphereParams.skyAxisAngle
---
```lua
AtmosphereParams.skyAxisAngle : xyzw {
    w: number,
    x: number,
    y: number,
    z: number,
}
```



rotation axis and angle in radians of skybox orientation


### AtmosphereParams.skyColor
---
```lua
AtmosphereParams.skyColor : rgba {
    a: number,
    b: number,
    g: number,
    r: number,
}
```




### AtmosphereParams.sunColor
---
```lua
AtmosphereParams.sunColor : rgba {
    a: number,
    b: number,
    g: number,
    r: number,
}
```




