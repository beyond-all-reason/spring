---
layout: default
title: UniformParam
parent: Lua API
permalink: lua-api/types/UniformParam
---

# class UniformParam


- supers: { , [string]: (T|T[]) }




A table of uniform name to value.

The Uniforms are the values you send along with the shader-program. To use
them in the shader-program declare them like this: `uniform float frame;`

Specify a Lua array to initialize GLSL arrays.

The engine will automatically fill in an appropriately named uniform for team
colour if it is declared;

```glsl
uniform vec4 teamColor;
```

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaShaders.cpp#L552-L568" target="_blank">source</a>]




