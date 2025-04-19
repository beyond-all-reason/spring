---
layout: default
title: ShaderParams
parent: Lua API
permalink: lua-api/types/ShaderParams
---

{% raw %}

# class ShaderParams





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaShaders.cpp#L570-L619" target="_blank">source</a>]





## fields


### ShaderParams.vertex

```lua
ShaderParams.vertex : string?
```



The "Vertex" or vertex-shader is your GLSL-Code as string, its written in a
C-Dialect.  This shader is busy deforming the geometry of a unit but it can
not create new polygons. Use it for waves, wobbling surfaces etc.


### ShaderParams.tcs

```lua
ShaderParams.tcs : string?
```



The "TCS" or Tesselation Control Shader controls how much tessellation a
particular patch gets; it also defines the size of a patch, thus allowing it
to augment data. It can also filter vertex data taken from the vertex shader.
The main purpose of the TCS is to feed the tessellation levels to the
Tessellation primitive generator stage, as well as to feed patch data (as its
output values) to the Tessellation Evaluation Shader stage.


### ShaderParams.tes

```lua
ShaderParams.tes : string?
```



The "TES" or Tesselation Evaluation Shader takes the abstract patch generated
by the tessellation primitive generation stage, as well as the actual vertex
data for the entire patch, and generates a particular vertex from it. Each
TES invocation generates a single vertex. It can also take per-patch data
provided by the Tessellation Control Shader.


### ShaderParams.geometry

```lua
ShaderParams.geometry : string?
```



The "Geometry" or Geometry-shader can create new vertices and vertice-stripes
from points.


### ShaderParams.fragment

```lua
ShaderParams.fragment : string?
```



The "Fragment" or Fragment-shader (sometimes called pixel-Shader) is post
processing the already rendered picture (for example drawing stars on the
sky).

Remember textures are not always 2 dimensional pictures. They can contain
information about the depth, or the third value marks areas and the strength
at which these are processed.


### ShaderParams.uniform

```lua
ShaderParams.uniform : UniformParam<number>?
```




### ShaderParams.uniformInt

```lua
ShaderParams.uniformInt : UniformParam<integer>?
```




### ShaderParams.uniformFloat

```lua
ShaderParams.uniformFloat : UniformParam<number>?
```




### ShaderParams.uniformMatrix

```lua
ShaderParams.uniformMatrix : UniformParam<number>?
```




### ShaderParams.geoInputType

```lua
ShaderParams.geoInputType : integer?
```



inType


### ShaderParams.geoOutputType

```lua
ShaderParams.geoOutputType : integer?
```



outType


### ShaderParams.geoOutputVerts

```lua
ShaderParams.geoOutputVerts : integer?
```



maxVerts


### ShaderParams.definitions

```lua
ShaderParams.definitions : string?
```



string of shader #defines"




{% endraw %}