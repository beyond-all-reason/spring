---
layout: default
title: AtmosphereParams
parent: Lua API
permalink: lua-api/types/AtmosphereParams
---

{% raw %}

# class AtmosphereParams





[<a href="https://github.com/beyond-all-reason/spring/blob/625902d539f43871ceb4ecdc45fc539e91a71b55/rts/Lua/LuaUnsyncedCtrl.cpp#L3863-L3871" target="_blank">source</a>]





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