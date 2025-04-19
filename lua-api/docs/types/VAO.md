---
layout: default
title: VAO
parent: Lua API
permalink: lua-api/types/VAO
---

{% raw %}

# class VAO





Vertex Array Object

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVAOImpl.cpp#L18-L25" target="_blank">source</a>]



## methods


### VAO.Delete

```lua
function VAO.Delete() ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVAOImpl.cpp#L39-L43" target="_blank">source</a>]


### VAO.AttachVertexBuffer

```lua
function VAO.AttachVertexBuffer(vbo: VBO) ->  nil
```





Attaches a VBO to be used as a vertex buffer

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVAOImpl.cpp#L93-L98" target="_blank">source</a>]


### VAO.AttachInstanceBuffer

```lua
function VAO.AttachInstanceBuffer(vbo: VBO) ->  nil
```





Attaches a VBO to be used as an instance buffer

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVAOImpl.cpp#L105-L110" target="_blank">source</a>]


### VAO.AttachIndexBuffer

```lua
function VAO.AttachIndexBuffer(vbo: VBO) ->  nil
```





Attaches a VBO to be used as an index buffer

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVAOImpl.cpp#L117-L122" target="_blank">source</a>]


### VAO.DrawArrays

```lua
function VAO.DrawArrays(
  glEnum: number,
  vertexCount: number?,
  vertexFirst: number?,
  instanceCount: number?,
  instanceFirst: number?
) ->  nil
```
@param `glEnum` - primitivesMode






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVAOImpl.cpp#L367-L376" target="_blank">source</a>]


### VAO.DrawElements

```lua
function VAO.DrawElements(
  glEnum: number,
  drawCount: number?,
  baseIndex: number?,
  instanceCount: number?,
  baseVertex: number?,
  baseInstance: number?
) ->  nil
```
@param `glEnum` - primitivesMode






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVAOImpl.cpp#L404-L414" target="_blank">source</a>]


### VAO.AddUnitsToSubmission

```lua
function VAO.AddUnitsToSubmission(unitIDs: (number|number[])) -> submittedCount number
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVAOImpl.cpp#L466-L471" target="_blank">source</a>]


### VAO.AddFeaturesToSubmission

```lua
function VAO.AddFeaturesToSubmission(featureIDs: (number|number[])) -> submittedCount number
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVAOImpl.cpp#L476-L481" target="_blank">source</a>]


### VAO.AddUnitDefsToSubmission

```lua
function VAO.AddUnitDefsToSubmission(unitDefIDs: (number|number[])) -> submittedCount number
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVAOImpl.cpp#L486-L491" target="_blank">source</a>]


### VAO.AddFeatureDefsToSubmission

```lua
function VAO.AddFeatureDefsToSubmission(featureDefIDs: (number|number[])) -> submittedCount number
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVAOImpl.cpp#L496-L501" target="_blank">source</a>]


### VAO.RemoveFromSubmission

```lua
function VAO.RemoveFromSubmission(index: number) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVAOImpl.cpp#L506-L511" target="_blank">source</a>]


### VAO.Submit

```lua
function VAO.Submit() ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVAOImpl.cpp#L529-L533" target="_blank">source</a>]






{% endraw %}