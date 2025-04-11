---
layout: default
title: SaveImageOptions
parent: Lua API
permalink: lua-api/types/SaveImageOptions
---

{% raw %}

# class SaveImageOptions





[<a href="https://github.com/beyond-all-reason/spring/blob/625902d539f43871ceb4ecdc45fc539e91a71b55/rts/Lua/LuaOpenGL.cpp#L6342-L6348" target="_blank">source</a>]





## fields


### SaveImageOptions.alpha

```lua
SaveImageOptions.alpha : boolean
```



(Default: `false`)


### SaveImageOptions.yflip

```lua
SaveImageOptions.yflip : boolean
```



(Default: `true`)


### SaveImageOptions.grayscale16bit

```lua
SaveImageOptions.grayscale16bit : boolean
```



(Default: `false`)


### SaveImageOptions.readbuffer

```lua
SaveImageOptions.readbuffer : GL {
    cmd: integer,
    POINTS: integer,
    LINES: integer,
    LINE_LOOP: integer,
    LINE_STRIP: integer,
    TRIANGLES: integer,
    TRIANGLE_STRIP: integer,
    TRIANGLE_FAN: integer,
    QUADS: integer,
    QUAD_STRIP: integer,
    POLYGON: integer,
    LINE_STRIP_ADJACENCY: integer,
    ...
}
```



(Default: current read buffer)




{% endraw %}