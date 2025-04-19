---
layout: default
title: AtmosphereParams
parent: Lua API
permalink: lua-api/types/AtmosphereParams
---

{% raw %}

# class AtmosphereParams





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaUnsyncedCtrl.cpp#L3859-L3867" target="_blank">source</a>]





## fields


### AtmosphereParams.fogStart

```lua
AtmosphereParams.fogStart : number
```




### AtmosphereParams.fogEnd

```lua
AtmosphereParams.fogEnd : number
```




### AtmosphereParams.sunColor

```lua
AtmosphereParams.sunColor : rgba {
    [1]: number,
    [2]: number,
    [3]: number,
    [4]: number,
}
```




### AtmosphereParams.skyColor

```lua
AtmosphereParams.skyColor : rgba {
    [1]: number,
    [2]: number,
    [3]: number,
    [4]: number,
}
```




### AtmosphereParams.cloudColor

```lua
AtmosphereParams.cloudColor : rgba {
    [1]: number,
    [2]: number,
    [3]: number,
    [4]: number,
}
```




### AtmosphereParams.skyAxisAngle

```lua
AtmosphereParams.skyAxisAngle : xyzw {
    [1]: number,
    [2]: number,
    [3]: number,
    [4]: number,
}
```



rotation axis and angle in radians of skybox orientation




{% endraw %}