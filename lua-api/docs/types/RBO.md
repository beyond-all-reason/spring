---
layout: default
title: RBO
parent: Lua API
permalink: lua-api/types/RBO
---

{% raw %}

# class RBO





User Data RBO

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaRBOs.cpp#L139-L148" target="_blank">source</a>]





## fields


### RBO.target

```lua
RBO.target : GL {
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




### RBO.format

```lua
RBO.format : GL {
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




### RBO.xsize

```lua
RBO.xsize : integer
```




### RBO.ysize

```lua
RBO.ysize : integer
```




### RBO.valid

```lua
RBO.valid : boolean
```




### RBO.samples

```lua
RBO.samples : integer
```



will return globalRendering->msaaLevel for multisampled RBO or 0 otherwise




{% endraw %}