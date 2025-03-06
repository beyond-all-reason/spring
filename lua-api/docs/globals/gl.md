---
layout: default
title: gl
parent: Lua API
permalink: lua-api/globals/gl
---

# global gl


---

## methods
---

### gl.ActiveFBO
---
```lua
function gl.ActiveFBO(
  fbo: Fbo,
  target: GL?,
  identities: boolean?,
  lua_function: function?,
  arg1: any?,
  arg2: any?,
  argn: any?
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaFBOs.cpp#L569-L578" target="_blank">source</a>]


### gl.ActiveShader
---
```lua
function gl.ActiveShader(
  shaderID: integer,
  func: function,
  ...: any
)
```
@param `...` - Arguments






Binds a shader program identified by shaderID, and calls the Lua func with
the specified arguments.

Can be used in NON-drawing events (to update uniforms etc.)!

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaShaders.cpp#L839-L849" target="_blank">source</a>]


### gl.AddFallbackFont
---
```lua
function gl.AddFallbackFont(filePath: string) -> success boolean
```
@param `filePath` - VFS path to the file, for example "fonts/myfont.ttf". Uses VFS.RAW_FIRST access mode.






Adds a fallback font for the font rendering engine.

Fonts added first will have higher priority.
When a glyph isn't found when rendering a font, the fallback fonts
will be searched first, otherwise os fonts will be used.

The application should listen for the unsynced 'FontsChanged' callin so
modules can clear any already reserved display lists or other relevant
caches.

Note the callin won't be executed at the time of calling this method,
but later, on the Update cycle (before other Update and Draw callins).

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaFonts.cpp#L210-L226" target="_blank">source</a>]


### gl.BlitFBO
---
```lua
function gl.BlitFBO(
  x0Src: number,
  y0Src: number,
  x1Src: number,
  y1Src: number,
  x0Dst: number,
  y0Dst: number,
  x1Dst: number,
  y1Dst: number,
  mask: number?,
  filter: number?
)
```
@param `mask` - (Default: GL_COLOR_BUFFER_BIT)

@param `filter` - (Default: GL_NEAREST)






needs `GLAD_GL_EXT_framebuffer_blit`

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaFBOs.cpp#L684-L697" target="_blank">source</a>]


### gl.ClearAttachmentFBO
---
```lua
function gl.ClearAttachmentFBO(
  target: number?,
  attachment: (GL|string),
  clearValue0: number,
  clearValue1: number,
  clearValue2: number,
  clearValue3: number
)
```
@param `target` - (Default: GL.FRAMEBUFFER)

@param `attachment` - (e.g. `"color0"` or `GL.COLOR_ATTACHMENT0`)






needs `Platform.glVersionNum >= 30`
Clears the "attachment" of the currently bound FBO type "target" with "clearValues"

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaFBOs.cpp#L783-L793" target="_blank">source</a>]


### gl.ClearFallbackFonts
---
```lua
function gl.ClearFallbackFonts() ->  nil
```





Clears all fallback fonts.

See the note at 'AddFallbackFont' about the 'FontsChanged' callin,
it also applies when calling this method.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaFonts.cpp#L238-L245" target="_blank">source</a>]


### gl.Color
---
```lua
function gl.Color(
  r: number,
  g: number,
  b: number,
  a: number?
)
```
@param `r` - Red

@param `g` - Green

@param `b` - Blue

@param `a` - Alpha (Default: 1.0f)






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaOpenGL.cpp#L2527-L2533" target="_blank">source</a>]


### gl.CreateFBO
---
```lua
function gl.CreateFBO(fbo: Fbo)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaFBOs.cpp#L435-L438" target="_blank">source</a>]


### gl.CreateRBO
---
```lua
function gl.CreateRBO(
  xsize: integer,
  ysize: integer,
  data: CreateRBOData
) ->  RBO
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaRBOs.cpp#L157-L163" target="_blank">source</a>]


### gl.CreateShader
---
```lua
function gl.CreateShader(shaderParams: ShaderParams) -> shaderID integer
```





Create a shader.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaShaders.cpp#L621-L627" target="_blank">source</a>]


### gl.DeleteFBO
---
```lua
function gl.DeleteFBO(fbo: Fbo)
```





This doesn't delete the attached objects!

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaFBOs.cpp#L509-L514" target="_blank">source</a>]


### gl.DeleteRBO
---
```lua
function gl.DeleteRBO(rbo: RBO)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaRBOs.cpp#L233-L236" target="_blank">source</a>]


### gl.DeleteShader
---
```lua
function gl.DeleteShader(shaderID: integer)
```





Deletes a shader identified by shaderID

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaShaders.cpp#L789-L793" target="_blank">source</a>]


### gl.GetActiveUniforms
---
```lua
function gl.GetActiveUniforms(shaderID: integer) -> activeUniforms ActiveUniform[]
```





Query the active (actually used) uniforms of a shader and identify their
names, types (float, int, uint) and sizes (float, vec4, ...).

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaShaders.cpp#L931-L938" target="_blank">source</a>]


### gl.GetEngineModelUniformDataDef
---
```lua
function gl.GetEngineModelUniformDataDef(index: number) -> glslDefinition string
```





Return the GLSL compliant definition of ModelUniformData structure (per Unit/Feature buffer available on GPU)

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaShaders.cpp#L1328-L1336" target="_blank">source</a>]


### gl.GetEngineUniformBufferDef
---
```lua
function gl.GetEngineUniformBufferDef(index: number) -> glslDefinition string
```





Return the GLSL compliant definition of UniformMatricesBuffer(idx=0) or UniformParamsBuffer(idx=1) structure.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaShaders.cpp#L1306-L1314" target="_blank">source</a>]


### gl.GetShaderLog
---
```lua
function gl.GetShaderLog() -> infoLog string
```





Returns the shader compilation error log. This is empty if the shader linking failed, in that case, check your in/out blocks and ensure they match.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaShaders.cpp#L198-L202" target="_blank">source</a>]


### gl.GetUniformLocation
---
```lua
function gl.GetUniformLocation(
  shaderID: integer,
  name: string
) -> locationID GL
```





Returns the locationID of a shaders uniform. Needed for changing uniform
values with function `gl.Uniform`.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaShaders.cpp#L966-L974" target="_blank">source</a>]


### gl.GetVAO
---
```lua
function gl.GetVAO() -> vao VAO?
```

@return `vao` - The VAO ref on success, else `nil`





Example:
```
local myVAO = gl.GetVAO()
if myVAO == nil then Spring.Echo("Failed to get VAO") end
```

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaVAO.cpp#L75-L84" target="_blank">source</a>]


### gl.GetVBO
---
```lua
function gl.GetVBO(
  bufferType: GLBufferType?,
  freqUpdated: boolean?
) -> VBO VBO?
```
@param `bufferType` - (Default: GL.ARRAY_BUFFER)

Use `GL.ARRAY_BUFFER` for vertex data and
`GL.ELEMENT_ARRAY_BUFFER` for vertex indices.

@param `freqUpdated` - (Default: true)

`true` to updated frequently, `false` to update only once.


@return `VBO` - The VBO ref on success, or nil if not supported or an error occurred.





Example:

```lua
local myVBO = gl.GetVBO()
if myVBO == nil then Spring.Echo("Failed to get VBO") end
```

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaVBO.cpp#L132-L156" target="_blank">source</a>]


### gl.IsValidFBO
---
```lua
function gl.IsValidFBO(
  fbo: Fbo,
  target: GL?
)
 -> valid boolean
 -> status number?

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaFBOs.cpp#L527-L533" target="_blank">source</a>]


### gl.RawBindFBO
---
```lua
function gl.RawBindFBO(
  fbo: nil,
  target: GL?,
  rawFboId: integer?
) ->  nil
```
@param `target` - (Default: `GL_FRAMEBUFFER_EXT`)

@param `rawFboId` - (Default: 0)






Bind default or specified via rawFboId numeric id of FBO

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaFBOs.cpp#L642-L650" target="_blank">source</a>]


### gl.SetGeometryShaderParameter
---
```lua
function gl.SetGeometryShaderParameter(
  shaderID: integer,
  param: number,
  number: number
) ->  nil
```





Sets the Geometry shader parameters for shaderID. Needed by geometry shader programs (check the opengl GL_ARB_geometry_shader4 extension for glProgramParameteri)

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaShaders.cpp#L1346-L1353" target="_blank">source</a>]


### gl.SetTesselationShaderParameter
---
```lua
function gl.SetTesselationShaderParameter(
  param: integer,
  value: integer
) ->  nil
```





Sets the tesselation shader parameters for `shaderID`.

Needed by tesselation shader programs. (Check the opengl
`GL_ARB_tessellation_shader` extension for `glProgramParameteri`).

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaShaders.cpp#L1374-L1384" target="_blank">source</a>]


### gl.Text
---
```lua
function gl.Text(
  text: string,
  x: number,
  y: number,
  size: number,
  options: string?
) ->  nil
```
@param `options` - concatenated string of option characters.

- horizontal alignment:
- 'c' = center
- 'r' = right
- vertical alignment:
- 'a' = ascender
- 't' = top
- 'v' = vertical center
- 'x' = baseline
- 'b' = bottom
- 'd' = descender
- decorations:
- 'o' = black outline
- 'O' = white outline
- 's' = shadow
- other:
- 'n' = don't round vertex coords to nearest integer (font may get blurry)






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaOpenGL.cpp#L1291-L1316" target="_blank">source</a>]


### gl.Uniform
---
```lua
function gl.Uniform(
  locationID: (GL|string),
  f1: number,
  f2: number?,
  f3: number?,
  f4: number?
)
```
@param `locationID` - uniformName






Sets the uniform float value at the locationID for the currently active
shader. Shader must be activated before setting uniforms.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaShaders.cpp#L1045-L1055" target="_blank">source</a>]


### gl.UniformArray
---
```lua
function gl.UniformArray(
  locationID: (integer|string),
  type: UniformArrayType,
  uniforms: number[]
)
```
@param `locationID` - uniformName

@param `uniforms` - Array up to 1024 elements






Sets the an array of uniform values at the locationID for the currently
active shader.

Shader must be activated before setting uniforms.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaShaders.cpp#L1156-L1166" target="_blank">source</a>]


### gl.UniformInt
---
```lua
function gl.UniformInt(
  locationID: (integer|string),
  int1: integer,
  int2: integer?,
  int3: integer?,
  int4: integer?
)
```
@param `locationID` - uniformName






Sets the uniform int value at the locationID for the currently active shader.
Shader must be activated before setting uniforms.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaShaders.cpp#L1086-L1096" target="_blank">source</a>]


### gl.UniformMatrix
---
```lua
function gl.UniformMatrix(
  locationID: (integer|string),
  matrix: ("shadows"|"camera"|"caminv"|"camprj")
)
```
@param `locationID` - uniformName

@param `matrix` - Name of common matrix.






Sets the a uniform mat4 locationID for the currently active shader.

Shader must be activated before setting uniforms.

Can set one one common matrix like shadow, or by passing 16 additional
numbers for the matrix.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaShaders.cpp#L1205-L1216" target="_blank">source</a>]


### gl.UseShader
---
```lua
function gl.UseShader(shaderID: integer) -> linked boolean
```





Binds a shader program identified by shaderID. Pass 0 to disable the shader. Returns whether the shader was successfully bound.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaShaders.cpp#L806-L811" target="_blank">source</a>]




