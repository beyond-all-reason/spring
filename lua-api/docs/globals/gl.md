---
layout: default
title: gl
parent: Lua API
permalink: lua-api/globals/gl
---

{% raw %}

# global gl




## methods


### gl.GetVAO

```lua
function gl.GetVAO() -> vao VAO?
```

@return `vao` - The VAO ref on success, else `nil`





Example:
```
local myVAO = gl.GetVAO()
if myVAO == nil then Spring.Echo("Failed to get VAO") end
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVAO.cpp#L75-L84" target="_blank">source</a>]


### gl.GetShaderLog

```lua
function gl.GetShaderLog() -> infoLog string
```





Returns the shader compilation error log. This is empty if the shader linking failed, in that case, check your in/out blocks and ensure they match.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaShaders.cpp#L198-L202" target="_blank">source</a>]


### gl.CreateShader

```lua
function gl.CreateShader(shaderParams: ShaderParams) -> shaderID integer
```





Create a shader.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaShaders.cpp#L621-L627" target="_blank">source</a>]


### gl.DeleteShader

```lua
function gl.DeleteShader(shaderID: integer)
```





Deletes a shader identified by shaderID

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaShaders.cpp#L789-L793" target="_blank">source</a>]


### gl.UseShader

```lua
function gl.UseShader(shaderID: integer) -> linked boolean
```





Binds a shader program identified by shaderID. Pass 0 to disable the shader. Returns whether the shader was successfully bound.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaShaders.cpp#L806-L811" target="_blank">source</a>]


### gl.ActiveShader

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

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaShaders.cpp#L839-L849" target="_blank">source</a>]


### gl.GetActiveUniforms

```lua
function gl.GetActiveUniforms(shaderID: integer) -> activeUniforms ActiveUniform[]
```





Query the active (actually used) uniforms of a shader and identify their
names, types (float, int, uint) and sizes (float, vec4, ...).

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaShaders.cpp#L931-L938" target="_blank">source</a>]


### gl.GetUniformLocation

```lua
function gl.GetUniformLocation(
  shaderID: integer,
  name: string
) -> locationID GL
```





Returns the locationID of a shaders uniform. Needed for changing uniform
values with function `gl.Uniform`.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaShaders.cpp#L966-L974" target="_blank">source</a>]


### gl.Uniform

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

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaShaders.cpp#L1045-L1055" target="_blank">source</a>]


### gl.UniformInt

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

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaShaders.cpp#L1086-L1096" target="_blank">source</a>]


### gl.UniformArray

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

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaShaders.cpp#L1156-L1166" target="_blank">source</a>]


### gl.UniformMatrix

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

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaShaders.cpp#L1205-L1216" target="_blank">source</a>]


### gl.UniformMatrix

```lua
function gl.UniformMatrix(
  locationID: (integer|string),
  matrix: number[]
)
```
@param `locationID` - uniformName

@param `matrix` - A 2x2, 3x3 or 4x4 matrix.






Sets the a uniform mat4 locationID for the currently active shader.

Shader must be activated before setting uniforms.

Can set one one common matrix like shadow, or by passing 16 additional
numbers for the matrix.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaShaders.cpp#L1218-L1229" target="_blank">source</a>]


### gl.GetEngineUniformBufferDef

```lua
function gl.GetEngineUniformBufferDef(index: number) -> glslDefinition string
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaShaders.cpp#L1306-L1314" target="_blank">source</a>]

Return the GLSL compliant definition of UniformMatricesBuffer(idx=0) or UniformParamsBuffer(idx=1) structure.


### gl.GetEngineModelUniformDataDef

```lua
function gl.GetEngineModelUniformDataDef(index: number) -> glslDefinition string
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaShaders.cpp#L1328-L1336" target="_blank">source</a>]

Return the GLSL compliant definition of ModelUniformData structure (per Unit/Feature buffer available on GPU)


### gl.SetGeometryShaderParameter

```lua
function gl.SetGeometryShaderParameter(
  shaderID: integer,
  param: number,
  number: number
) ->  nil
```





Sets the Geometry shader parameters for shaderID. Needed by geometry shader programs (check the opengl GL_ARB_geometry_shader4 extension for glProgramParameteri)

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaShaders.cpp#L1346-L1353" target="_blank">source</a>]


### gl.SetTesselationShaderParameter

```lua
function gl.SetTesselationShaderParameter(
  param: integer,
  value: integer
) ->  nil
```





Sets the tesselation shader parameters for `shaderID`.

Needed by tesselation shader programs. (Check the opengl
`GL_ARB_tessellation_shader` extension for `glProgramParameteri`).

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaShaders.cpp#L1374-L1384" target="_blank">source</a>]


### gl.CreateFBO

```lua
function gl.CreateFBO(fboDesc: FBODescription) -> fbo FBO
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaFBOs.cpp#L467-L471" target="_blank">source</a>]


### gl.DeleteFBO

```lua
function gl.DeleteFBO(fbo: FBO)
```





This doesn't delete the attached objects!

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaFBOs.cpp#L542-L547" target="_blank">source</a>]


### gl.IsValidFBO

```lua
function gl.IsValidFBO(
  fbo: FBO,
  target: GL?
)
 -> valid boolean
 -> status number?

```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaFBOs.cpp#L560-L566" target="_blank">source</a>]


### gl.ActiveFBO

```lua
function gl.ActiveFBO(
  fbo: FBO,
  func: fun(...),
  ...: any
)
```
@param `...` - args






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaFBOs.cpp#L601-L606" target="_blank">source</a>]


### gl.ActiveFBO

```lua
function gl.ActiveFBO(
  fbo: FBO,
  target: GL?,
  func: fun(...),
  ...: any
)
```
@param `...` - args






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaFBOs.cpp#L607-L613" target="_blank">source</a>]


### gl.RawBindFBO

```lua
function gl.RawBindFBO(
  fbo: nil,
  target: GL?,
  rawFboId: integer?
) ->  nil
```
@param `target` - (Default: `GL_FRAMEBUFFER_EXT`)

@param `rawFboId` - (Default: `0`)






Bind default or specified via rawFboId numeric id of FBO

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaFBOs.cpp#L677-L685" target="_blank">source</a>]


### gl.RawBindFBO

```lua
function gl.RawBindFBO(
  fbo: FBO,
  target: GL?
) -> previouslyBoundRawFboId number
```
@param `target` - (Default: `fbo.target`)






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaFBOs.cpp#L686-L691" target="_blank">source</a>]


### gl.BlitFBO

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

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaFBOs.cpp#L719-L732" target="_blank">source</a>]


### gl.BlitFBO

```lua
function gl.BlitFBO(
  fboSrc: FBO,
  x0Src: number,
  y0Src: number,
  x1Src: number,
  y1Src: number,
  fboDst: FBO,
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

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaFBOs.cpp#L733-L748" target="_blank">source</a>]


### gl.ClearAttachmentFBO

```lua
function gl.ClearAttachmentFBO(
  target: number?,
  attachment: (GL|Attachment),
  clearValue0: number?,
  clearValue1: number?,
  clearValue2: number?,
  clearValue3: number?
) -> success boolean
```
@param `target` - (Default: `GL.FRAMEBUFFER`)

@param `attachment` - (e.g. `"color0"` or `GL.COLOR_ATTACHMENT0`)

@param `clearValue0` - (Default: `0`)

@param `clearValue1` - (Default: `0`)

@param `clearValue2` - (Default: `0`)

@param `clearValue3` - (Default: `0`)






needs `Platform.glVersionNum >= 30`
Clears the "attachment" of the currently bound FBO type "target" with "clearValues"

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaFBOs.cpp#L818-L829" target="_blank">source</a>]


### gl.HasExtension

```lua
function gl.HasExtension(ext: string) ->  boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L1143-L1147" target="_blank">source</a>]


### gl.GetNumber

```lua
function gl.GetNumber(
  pname: GL,
  count: integer?
) ->  number...
```
@param `count` - (Default: `1`) Number of values to return, in range [1, 64].






Get the value or values of a selected parameter.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L1155-L1162" target="_blank">source</a>]


### gl.GetString

```lua
function gl.GetString(pname: GL)
```





Get a string describing the current OpenGL connection.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L1180-L1184" target="_blank">source</a>]


### gl.GetScreenViewTrans

```lua
function gl.GetScreenViewTrans()
 -> x number
 -> y number
 -> z number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L1200-L1205" target="_blank">source</a>]


### gl.GetViewSizes

```lua
function gl.GetViewSizes()
 -> x number
 -> y number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L1215-L1219" target="_blank">source</a>]


### gl.GetViewRange

```lua
function gl.GetViewRange()
 -> nearPlaneDist number
 -> farPlaneDist number
 -> minViewRange number
 -> maxViewRange number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L1228-L1234" target="_blank">source</a>]


### gl.SetSlaveMode

```lua
function gl.SetSlaveMode(newMode: boolean)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L1250-L1253" target="_blank">source</a>]


### gl.ConfigMiniMap

```lua
function gl.ConfigMiniMap(
  px: integer,
  py: integer,
  sx: integer,
  sy: integer
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L1265-L1271" target="_blank">source</a>]


### gl.DrawMiniMap

```lua
function gl.DrawMiniMap(defaultTransform: boolean?)
```
@param `defaultTransform` - (Default: `true`)






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L1287-L1290" target="_blank">source</a>]


### gl.BeginText

```lua
function gl.BeginText()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L1327-L1329" target="_blank">source</a>]


### gl.EndText

```lua
function gl.EndText()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L1338-L1340" target="_blank">source</a>]


### gl.Text

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






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L1348-L1373" target="_blank">source</a>]


### gl.GetTextWidth

```lua
function gl.GetTextWidth(text: string) -> width number
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L1429-L1433" target="_blank">source</a>]


### gl.GetTextHeight

```lua
function gl.GetTextHeight(text: string)
 -> height number
 -> descender number
 -> lines integer

```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L1444-L1450" target="_blank">source</a>]


### gl.Unit

```lua
function gl.Unit(
  unitID: integer,
  doRawDraw: boolean?,
  useLuaMat: integer?,
  noLuaCall: boolean?,
  fullModel: boolean?
)
```
@param `doRawDraw` - (Default: `false`)

@param `noLuaCall` - (Default: `false`) Skip the `DrawUnit` callin.

@param `fullModel` - (Default: `true`)






Draw the unit, applying transform.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L1645-L1654" target="_blank">source</a>]


### gl.UnitRaw

```lua
function gl.UnitRaw(
  unitID: integer,
  doRawDraw: boolean?,
  useLuaMat: integer?,
  noLuaCall: boolean?,
  fullModel: boolean?
)
```
@param `doRawDraw` - (Default: `false`)

@param `noLuaCall` - (Default: `true`) Skip the `DrawUnit` callin.

@param `fullModel` - (Default: `true`)






Draw the unit without applying transform.

Also skips the `DrawUnit` callin by default so any
recursion is blocked.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L1657-L1669" target="_blank">source</a>]


### gl.UnitTextures

```lua
function gl.UnitTextures(
  unitID: integer,
  push: boolean
)
```
@param `push` - If `true`, push the render state; if `false`, pop it.






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L1672-L1676" target="_blank">source</a>]


### gl.UnitShape

```lua
function gl.UnitShape(
  unitDefID: integer,
  teamID: integer,
  rawState: boolean?,
  toScreen: boolean?,
  opaque: boolean?
)
```
@param `rawState` - (Default: `true`)

@param `toScreen` - (Default: `false`)

@param `opaque` - (Default: `true`) If `true`, draw opaque; if `false`, draw alpha.






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L1684-L1691" target="_blank">source</a>]


### gl.UnitShapeTextures

```lua
function gl.UnitShapeTextures(
  unitDefID: integer,
  push: boolean
)
```
@param `push` - If `true`, push the render state; if `false`, pop it.






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L1699-L1703" target="_blank">source</a>]


### gl.UnitMultMatrix

```lua
function gl.UnitMultMatrix(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L1712-L1715" target="_blank">source</a>]


### gl.UnitPiece

```lua
function gl.UnitPiece(
  unitID: integer,
  pieceID: integer
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L1730-L1734" target="_blank">source</a>]


### gl.UnitPieceMatrix

```lua
function gl.UnitPieceMatrix(
  unitID: integer,
  pieceID: integer
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L1741-L1745" target="_blank">source</a>]


### gl.UnitPieceMultMatrix

```lua
function gl.UnitPieceMultMatrix(
  unitID: integer,
  pieceID: integer
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L1748-L1752" target="_blank">source</a>]


### gl.Feature

```lua
function gl.Feature(
  featureID: integer,
  doRawDraw: boolean?,
  useLuaMat: integer?,
  noLuaCall: boolean?
)
```
@param `doRawDraw` - (Default: `false`)

@param `noLuaCall` - (Default: `false`) Skip the `DrawFeature` callin.






Draw the feature, applying transform.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L1813-L1821" target="_blank">source</a>]


### gl.FeatureRaw

```lua
function gl.FeatureRaw(
  featureID: integer,
  doRawDraw: boolean?,
  useLuaMat: integer?,
  noLuaCall: boolean?
)
```
@param `doRawDraw` - (Default: `false`)

@param `noLuaCall` - (Default: `true`) Skip the `DrawFeature` callin.






Draw the unit without applying transform.

Also skips the `DrawFeature` callin by default so any
recursion is blocked.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L1824-L1834" target="_blank">source</a>]


### gl.FeatureTextures

```lua
function gl.FeatureTextures(
  featureID: integer,
  push: boolean
)
```
@param `push` - If `true`, push the render state; if `false`, pop it.






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L1837-L1841" target="_blank">source</a>]


### gl.FeatureShape

```lua
function gl.FeatureShape(
  featureDefID: integer,
  teamID: integer,
  rawState: boolean?,
  toScreen: boolean?,
  opaque: boolean?
)
```
@param `rawState` - (Default: `true`)

@param `toScreen` - (Default: `false`)

@param `opaque` - (Default: `true`) If `true`, draw opaque; if `false`, draw alpha.






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L1849-L1856" target="_blank">source</a>]


### gl.FeatureShapeTextures

```lua
function gl.FeatureShapeTextures(
  featureDefID: integer,
  push: boolean
)
```
@param `push` - If `true`, push the render state; if `false`, pop it.






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L1864-L1868" target="_blank">source</a>]


### gl.FeatureMultMatrix

```lua
function gl.FeatureMultMatrix(featureID: integer)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L1877-L1880" target="_blank">source</a>]


### gl.FeaturePiece

```lua
function gl.FeaturePiece(
  featureID: integer,
  pieceID: integer
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L1895-L1899" target="_blank">source</a>]


### gl.FeaturePieceMatrix

```lua
function gl.FeaturePieceMatrix(
  featureID: integer,
  pieceID: integer
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L1907-L1911" target="_blank">source</a>]


### gl.FeaturePieceMultMatrix

```lua
function gl.FeaturePieceMultMatrix(
  featureID: integer,
  pieceID: integer
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L1915-L1919" target="_blank">source</a>]


### gl.DrawListAtUnit

```lua
function gl.DrawListAtUnit(
  unitID: integer,
  listIndex: integer,
  useMidPos: boolean?,
  scaleX: number?,
  scaleY: number?,
  scaleZ: number?,
  degrees: number?,
  rotX: number?,
  rotY: number?,
  rotZ: number?
)
```
@param `useMidPos` - (Default: `true`)

@param `scaleX` - (Default: `1.0`)

@param `scaleY` - (Default: `1.0`)

@param `scaleZ` - (Default: `1.0`)

@param `degrees` - (Default: `0.0`)

@param `rotX` - (Default: `0.0`)

@param `rotY` - (Default: `1.0`)

@param `rotZ` - (Default: `0.0`)






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L1932-L1944" target="_blank">source</a>]


### gl.DrawFuncAtUnit

```lua
function gl.DrawFuncAtUnit(
  unitID: integer,
  useMidPos: boolean?,
  fun: unknown,
  ...: any
)
```
@param `useMidPos` - (Default: `true`)

@param `...` - Arguments passed to function.






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L1989-L1995" target="_blank">source</a>]


### gl.DrawGroundCircle

```lua
function gl.DrawGroundCircle(
  posX: number,
  posY: number,
  posZ: number,
  radius: number,
  resolution: integer
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L2032-L2039" target="_blank">source</a>]


### gl.DrawGroundCircle

```lua
function gl.DrawGroundCircle(
  posX: number,
  posY: number,
  posZ: number,
  radius: number,
  resolution: integer,
  slope: number,
  gravity: number,
  weaponDefID: integer
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L2040-L2050" target="_blank">source</a>]


### gl.DrawGroundCircle

```lua
function gl.DrawGroundCircle(
  x0: number,
  z0: number,
  x1: number,
  z1: number,
  useNorm: nil,
  useTxcd: boolean?
)
```
@param `useNorm` - No longer used.

@param `useTxcd` - (Default: `false`)






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L2091-L2099" target="_blank">source</a>]


### gl.DrawGroundCircle

```lua
function gl.DrawGroundCircle(
  x0: number,
  z0: number,
  x1: number,
  z1: number,
  useNorm: nil,
  tu0: number,
  tv0: number,
  tu1: number,
  tv1: number
)
```
@param `useNorm` - No longer used.






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L2100-L2111" target="_blank">source</a>]


### gl.Shape

```lua
function gl.Shape(
  type: GL,
  vertices: VertexData[]
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L2300-L2304" target="_blank">source</a>]


### gl.BeginEnd

```lua
function gl.BeginEnd(
  primMode: GL,
  fun: unknown,
  ...: any
)
```
@param `...` - Arguments passed to function.






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L2346-L2351" target="_blank">source</a>]


### gl.Vertex

```lua
function gl.Vertex(v: xy)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L2383-L2386" target="_blank">source</a>]


### gl.Vertex

```lua
function gl.Vertex(v: xyz)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L2387-L2390" target="_blank">source</a>]


### gl.Vertex

```lua
function gl.Vertex(v: xyzw)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L2391-L2394" target="_blank">source</a>]


### gl.Vertex

```lua
function gl.Vertex(
  x: number,
  y: number,
  z: number?,
  w: number?
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L2395-L2401" target="_blank">source</a>]


### gl.Normal

```lua
function gl.Normal(v: xyz)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L2465-L2468" target="_blank">source</a>]


### gl.Normal

```lua
function gl.Normal(
  x: number,
  y: number,
  z: number
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L2469-L2474" target="_blank">source</a>]


### gl.TexCoord

```lua
function gl.TexCoord(coord: (number))
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L2513-L2516" target="_blank">source</a>]


### gl.TexCoord

```lua
function gl.TexCoord(coord: xy)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L2517-L2520" target="_blank">source</a>]


### gl.TexCoord

```lua
function gl.TexCoord(coord: xyz)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L2521-L2524" target="_blank">source</a>]


### gl.TexCoord

```lua
function gl.TexCoord(coord: xyzw)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L2525-L2528" target="_blank">source</a>]


### gl.TexCoord

```lua
function gl.TexCoord(
  s: number,
  t: number?,
  r: number?,
  q: number?
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L2529-L2535" target="_blank">source</a>]


### gl.MultiTexCoord

```lua
function gl.MultiTexCoord(
  texNum: integer,
  coord: (number)
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L2604-L2608" target="_blank">source</a>]


### gl.MultiTexCoord

```lua
function gl.MultiTexCoord(
  texNum: integer,
  coord: xy
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L2609-L2613" target="_blank">source</a>]


### gl.MultiTexCoord

```lua
function gl.MultiTexCoord(
  texNum: integer,
  coord: xyz
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L2614-L2618" target="_blank">source</a>]


### gl.MultiTexCoord

```lua
function gl.MultiTexCoord(
  texNum: integer,
  coord: xyzw
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L2619-L2623" target="_blank">source</a>]


### gl.MultiTexCoord

```lua
function gl.MultiTexCoord(
  texNum: integer,
  s: number,
  t: number?,
  r: number?,
  q: number?
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L2624-L2631" target="_blank">source</a>]


### gl.SecondaryColor

```lua
function gl.SecondaryColor(color: rgb)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L2706-L2709" target="_blank">source</a>]


### gl.SecondaryColor

```lua
function gl.SecondaryColor(
  r: number,
  g: number,
  b: number
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L2710-L2715" target="_blank">source</a>]


### gl.FogCoord

```lua
function gl.FogCoord(coord: number)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L2754-L2757" target="_blank">source</a>]


### gl.EdgeFlag

```lua
function gl.EdgeFlag(flag: boolean)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L2769-L2772" target="_blank">source</a>]


### gl.Rect

```lua
function gl.Rect(
  x1: number,
  y1: number,
  x2: number,
  y2: number
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L2787-L2793" target="_blank">source</a>]


### gl.Rect

```lua
function gl.Rect(
  x1: number,
  y1: number,
  x2: number,
  y2: number,
  flipSCoords: boolean?,
  flipTCoords: boolean?
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L2806-L2814" target="_blank">source</a>]


### gl.Rect

```lua
function gl.Rect(
  x1: number,
  y1: number,
  x2: number,
  y2: number,
  s1: number,
  t1: number,
  s2: number,
  t2: number
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L2815-L2825" target="_blank">source</a>]


### gl.DispatchCompute

```lua
function gl.DispatchCompute(
  numGroupX: integer,
  numGroupY: integer,
  numGroupZ: integer,
  barriers: integer?
)
```
@param `barriers` - (Default: `4`)






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L2881-L2887" target="_blank">source</a>]


### gl.MemoryBarrier

```lua
function gl.MemoryBarrier(barriers: integer?)
```
@param `barriers` - (Default: `4`)






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L2920-L2923" target="_blank">source</a>]


### gl.Color

```lua
function gl.Color(
  r: number,
  g: number,
  b: number,
  a: number?
)
```
@param `r` - Red.

@param `g` - Green.

@param `b` - Blue.

@param `a` - (Default: `1.0`) Alpha.






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L2941-L2947" target="_blank">source</a>]


### gl.Color

```lua
function gl.Color(color: rgba)
```
@param `color` - Color with alpha.






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L2948-L2951" target="_blank">source</a>]


### gl.Color

```lua
function gl.Color(color: rgb)
```
@param `color` - Color.






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L2952-L2955" target="_blank">source</a>]


### gl.Material

```lua
function gl.Material(material: Material)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3003-L3006" target="_blank">source</a>]


### gl.ResetState

```lua
function gl.ResetState()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3076-L3078" target="_blank">source</a>]


### gl.ResetMatrices

```lua
function gl.ResetMatrices()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3087-L3089" target="_blank">source</a>]


### gl.Lighting

```lua
function gl.Lighting(enable: boolean)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3105-L3108" target="_blank">source</a>]


### gl.ShadeModel

```lua
function gl.ShadeModel(model: GL)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3122-L3125" target="_blank">source</a>]


### gl.Scissor

```lua
function gl.Scissor(enable: boolean)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3134-L3137" target="_blank">source</a>]


### gl.Scissor

```lua
function gl.Scissor(
  x: integer,
  y: integer,
  w: integer,
  h: integer
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3138-L3144" target="_blank">source</a>]


### gl.Viewport

```lua
function gl.Viewport(
  x: integer,
  y: integer,
  w: integer,
  h: integer
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3175-L3181" target="_blank">source</a>]


### gl.ColorMask

```lua
function gl.ColorMask(rgba: boolean)
```





Enable or disable writing of frame buffer color components.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3198-L3202" target="_blank">source</a>]


### gl.ColorMask

```lua
function gl.ColorMask(
  red: boolean,
  green: boolean,
  blue: boolean,
  alpha: boolean
)
```





Enable or disable writing of frame buffer color components.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3203-L3210" target="_blank">source</a>]


### gl.DepthMask

```lua
function gl.DepthMask(enable: boolean)
```





Enable or disable writing into the depth buffer.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3234-L3238" target="_blank">source</a>]


### gl.DepthTest

```lua
function gl.DepthTest(enable: boolean)
```





Enable or disable depth test.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3251-L3255" target="_blank">source</a>]


### gl.DepthTest

```lua
function gl.DepthTest(depthFunction: GL)
```
@param `depthFunction` - Symbolic constants `GL.NEVER`, `GL.LESS`, `GL.EQUAL`, `GL.LEQUAL`,
`GL.GREATER`, `GL.NOTEQUAL`, `GL.GEQUAL`, and `GL.ALWAYS` are accepted.
The initial value is `GL.LESS`.






Enable depth test and specify the depth comparison function.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3256-L3265" target="_blank">source</a>]


### gl.DepthClamp

```lua
function gl.DepthClamp(enable: boolean)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3293-L3296" target="_blank">source</a>]


### gl.Culling

```lua
function gl.Culling(enable: boolean)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3310-L3313" target="_blank">source</a>]


### gl.Culling

```lua
function gl.Culling(mode: GL)
```
@param `mode` - Specifies whether front- or back-facing facets are candidates for culling.
Symbolic constants `GL.FRONT`, `GL.BACK`, and `GL.FRONT_AND_BACK` are accepted. The
initial value is `GL.BACK`.






Enable culling and set culling mode.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3314-L3322" target="_blank">source</a>]


### gl.LogicOp

```lua
function gl.LogicOp(enable: boolean)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3351-L3354" target="_blank">source</a>]


### gl.LogicOp

```lua
function gl.LogicOp(opCode: GL)
```
@param `opCode` - Specifies a symbolic constant that selects a logical operation. The following
symbols are accepted: `GL.CLEAR`, `GL.SET`, `GL.COPY`, `GL.COPY_INVERTED`,
`GL.NOOP`, `GL.INVERT`, `GL.AND`, `GL.NAND`, `GL.OR`, `GL.NOR`, `GL.XOR`,
`GL.EQUIV`, `GL.AND_REVERSE`, `GL.AND_INVERTED`, `GL.OR_REVERSE`, and
`GL.OR_INVERTED`.  The initial value is `GL.COPY`.






Specify a logical pixel operation for rendering.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3355-L3367" target="_blank">source</a>]


### gl.Fog

```lua
function gl.Fog(enable: boolean)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3395-L3398" target="_blank">source</a>]


### gl.Blending

```lua
function gl.Blending(enable: boolean)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3413-L3416" target="_blank">source</a>]


### gl.Blending

```lua
function gl.Blending(mode: ("add"|"alpha_add"|"alpha"|"reset"|"color"|"modulate"|"disable"))
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3417-L3420" target="_blank">source</a>]


### gl.Blending

```lua
function gl.Blending(
  src: GL,
  dst: GL
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3421-L3425" target="_blank">source</a>]


### gl.BlendEquation

```lua
function gl.BlendEquation(mode: GL)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3488-L3491" target="_blank">source</a>]


### gl.BlendFunc

```lua
function gl.BlendFunc(
  src: GL,
  dst: GL
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3501-L3505" target="_blank">source</a>]


### gl.BlendEquationSeparate

```lua
function gl.BlendEquationSeparate(
  modeRGB: GL,
  modeAlpha: GL
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3516-L3520" target="_blank">source</a>]


### gl.BlendFuncSeparate

```lua
function gl.BlendFuncSeparate(
  srcRGB: GL,
  dstRGB: GL,
  srcAlpha: GL,
  dstAlpha: GL
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3531-L3537" target="_blank">source</a>]


### gl.AlphaTest

```lua
function gl.AlphaTest(enable: boolean)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3550-L3553" target="_blank">source</a>]


### gl.AlphaTest

```lua
function gl.AlphaTest(
  func: GL,
  ref: number
)
```
@param `func` - Specifies the alpha comparison function. Symbolic constants `GL.NEVER`,
`GL.LESS`, `GL.EQUAL`, `GL.LEQUAL`, `GL.GREATER`, `GL.NOTEQUAL`, `GL.GEQUAL`,
and `GL.ALWAYS` are accepted. The initial value is `GL.ALWAYS`.

@param `ref` - Specifies the reference value that incoming alpha values are compared to.
This value is clamped to the range `[0, 1]`, where `0` represents the lowest
possible alpha value and `1` the highest possible value. The initial reference
value is `0`.






Specify the alpha test function.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3554-L3567" target="_blank">source</a>]


### gl.AlphaToCoverage

```lua
function gl.AlphaToCoverage(enable: boolean)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3591-L3594" target="_blank">source</a>]


### gl.PolygonMode

```lua
function gl.PolygonMode(
  face: GL,
  mode: GL
)
```
@param `face` - Specifies the polygons that mode applies to. Must be `GL.FRONT` for
front-facing polygons, `GL.BACK` for back-facing polygons, or `GL.FRONT_AND_BACK`
for front- and back-facing polygons.

@param `mode` - Specifies how polygons will be rasterized. Accepted values are `GL.POINT`,
`GL.LINE`, and `GL.FILL`. The initial value is `GL.FILL` for both front- and
back-facing polygons.






Select polygon rasterization mode.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3612-L3628" target="_blank">source</a>]


### gl.PolygonOffset

```lua
function gl.PolygonOffset(enable: boolean)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3639-L3642" target="_blank">source</a>]


### gl.PolygonOffset

```lua
function gl.PolygonOffset(
  factor: number,
  units: number
)
```
@param `factor` - Specifies a scale factor that is used to create a variable depth offset for
each polygon. The initial value is `0`.

@param `units` - Is multiplied by an implementation-specific value to create a constant depth
offset. The initial value is `0`.






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3643-L3654" target="_blank">source</a>]


### gl.StencilTest

```lua
function gl.StencilTest(enable: boolean)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3686-L3689" target="_blank">source</a>]


### gl.StencilMask

```lua
function gl.StencilMask(mask: integer)
```
@param `mask` - Specifies a bit mask to enable and disable writing of individual bits in the stencil planes. Initially, the mask is all `1`'s.






Control the front and back writing of individual bits in the stencil planes.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3703-L3707" target="_blank">source</a>]


### gl.StencilFunc

```lua
function gl.StencilFunc(
  func: GL,
  ref: integer,
  mask: integer
)
```
@param `func` - Specifies the test function. Eight symbolic constants are valid: `GL.NEVER`, `GL.LESS`, `GL.EQUAL`, `GL.LEQUAL`, `GL.GREATER`, `GL.NOTEQUAL`, `GL.GEQUAL`, and `GL.ALWAYS`. The initial value is `GL.ALWAYS`.

@param `ref` - Specifies the reference value for the stencil test. `ref` is clamped to the range `[0, 2^n - 1]`, where `n` is the number of bitplanes in the stencil buffer. The initial value is `0`.

@param `mask` - Specifies a mask that is ANDed with both the reference value and the stored stencil value when the test is done. The initial value is all `1`'s.






Set front and back function and reference value for stencil testing.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3717-L3723" target="_blank">source</a>]


### gl.StencilOp

```lua
function gl.StencilOp(
  fail: GL,
  zfail: GL,
  zpass: GL
)
```
@param `fail` - Specifies the action to take when the stencil test fails. Eight symbolic constants are valid: `GL.KEEP`, `GL.ZERO`, `GL.REPLACE`, `GL.INCR`, `GL.INCR_WRAP`, `GL.DECR`, `GL.DECR_WRAP`, and `GL.INVERT`. The initial value is `GL.KEEP`.

@param `zfail` - Specifies the stencil action when the stencil test passes, but the depth test fails. The initial value is `GL.KEEP`.

@param `zpass` - Specifies the stencil action when both the stencil test and the depth test pass, or when the stencil test passes and either there is no depth buffer or depth testing is not enabled. The initial value is `GL.KEEP`.






Set front and back stencil test actions.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3735-L3741" target="_blank">source</a>]


### gl.StencilMaskSeparate

```lua
function gl.StencilMaskSeparate(
  face: GL,
  mask: integer
)
```
@param `face` - Specifies whether the front and/or back stencil writemask is updated. Three symbolic constants are accepted: `GL.FRONT`, `GL.BACK`, and `GL.FRONT_AND_BACK`. The initial value is `GL.FRONT_AND_BACK`.

@param `mask` - Specifies a bit mask to enable and disable writing of individual bits in the stencil planes. Initially, the mask is all `1`'s.






Control the front and back writing of individual bits in the stencil planes.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3753-L3758" target="_blank">source</a>]


### gl.StencilFuncSeparate

```lua
function gl.StencilFuncSeparate(
  face: GL,
  func: GL,
  ref: integer,
  mask: integer
)
```
@param `face` - Specifies whether front and/or back stencil state is updated. Three symbolic constants are accepted: `GL.FRONT`, `GL.BACK`, and `GL.FRONT_AND_BACK`. The initial value is `GL.FRONT_AND_BACK`.

@param `func` - Specifies the test function. Eight symbolic constants are valid: `GL.NEVER`, `GL.LESS`, `GL.EQUAL`, `GL.LEQUAL`, `GL.GREATER`, `GL.NOTEQUAL`, `GL.GEQUAL`, and `GL.ALWAYS`. The initial value is `GL.ALWAYS`.

@param `ref` - Specifies the reference value for the stencil test. `ref` is clamped to the range `[0, 2^n - 1]`, where `n` is the number of bitplanes in the stencil buffer. The initial value is `0`.

@param `mask` - Specifies a mask that is ANDed with both the reference value and the stored stencil value when the test is done. The initial value is all `1`'s.






Set front and/or back function and reference value for stencil testing.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3769-L3776" target="_blank">source</a>]


### gl.StencilOpSeparate

```lua
function gl.StencilOpSeparate(
  face: GL,
  fail: GL,
  zfail: GL,
  zpass: GL
)
```
@param `face` - Specifies whether front and/or back stencil state is updated. Three symbolic constants are accepted: `GL.FRONT`, `GL.BACK`, and `GL.FRONT_AND_BACK`. The initial value is `GL.FRONT_AND_BACK`.

@param `fail` - Specifies the action to take when the stencil test fails. Eight symbolic constants are valid: `GL.KEEP`, `GL.ZERO`, `GL.REPLACE`, `GL.INCR`, `GL.INCR_WRAP`, `GL.DECR`, `GL.DECR_WRAP`, and `GL.INVERT`. The initial value is `GL.KEEP`.

@param `zfail` - Specifies the stencil action when the stencil test passes, but the depth test fails. The initial value is `GL.KEEP`.

@param `zpass` - Specifies the stencil action when both the stencil test and the depth test pass, or when the stencil test passes and either there is no depth buffer or depth testing is not enabled. The initial value is `GL.KEEP`.






Set front and/or back stencil test actions.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3789-L3796" target="_blank">source</a>]


### gl.LineStipple

```lua
function gl.LineStipple(enable: boolean)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3811-L3814" target="_blank">source</a>]


### gl.LineStipple

```lua
function gl.LineStipple(ignoredString: string)
```
@param `ignoredString` - The value of this string is ignored, but it still does something.






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3815-L3818" target="_blank">source</a>]


### gl.LineStipple

```lua
function gl.LineStipple(
  factor: integer,
  pattern: integer,
  shift: integer?
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3819-L3824" target="_blank">source</a>]


### gl.LineWidth

```lua
function gl.LineWidth(width: number)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3874-L3877" target="_blank">source</a>]


### gl.PointSize

```lua
function gl.PointSize(size: number)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3888-L3891" target="_blank">source</a>]


### gl.PointSprite

```lua
function gl.PointSprite(
  enable: boolean,
  enableCoordReplace: boolean?,
  coordOrigin: boolean?
)
```
@param `coordOrigin` - `true` for upper left, `false` for lower left, otherwise no change.






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3902-L3907" target="_blank">source</a>]


### gl.PointParameter

```lua
function gl.PointParameter(
  atten0: number,
  atten1: number,
  atten2: number,
  sizeMin: number?,
  sizeMax: number?,
  sizeFade: number?
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3936-L3944" target="_blank">source</a>]


### gl.Texture

```lua
function gl.Texture(
  texNum: integer,
  enable: boolean?
) ->  boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3972-L3977" target="_blank">source</a>]


### gl.Texture

```lua
function gl.Texture(enable: boolean) ->  boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3978-L3982" target="_blank">source</a>]


### gl.Texture

```lua
function gl.Texture(
  texNum: integer,
  image: string
) ->  boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3983-L3988" target="_blank">source</a>]


### gl.Texture

```lua
function gl.Texture(image: string) ->  boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L3989-L3993" target="_blank">source</a>]


### gl.CreateTexture

```lua
function gl.CreateTexture(
  xsize: integer,
  ysize: integer,
  texture: Texture
) -> texName string?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L4122-L4128" target="_blank">source</a>]


### gl.CreateTexture

```lua
function gl.CreateTexture(
  xsize: integer,
  ysize: integer,
  zsize: integer,
  texture: Texture
) -> texName string?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L4129-L4136" target="_blank">source</a>]


### gl.ChangeTextureParams

```lua
function gl.ChangeTextureParams(
  texName: string,
  params: Texture
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L4204-L4208" target="_blank">source</a>]


### gl.DeleteTexture

```lua
function gl.DeleteTexture(texName: string) ->  boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L4235-L4239" target="_blank">source</a>]


### gl.DeleteTextureFBO

```lua
function gl.DeleteTextureFBO(texName: string) ->  boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L4256-L4260" target="_blank">source</a>]


### gl.TextureInfo

```lua
function gl.TextureInfo(texName: string) -> textureInfo TextureInfo
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L4281-L4285" target="_blank">source</a>]


### gl.CopyToTexture

```lua
function gl.CopyToTexture(
  texName: string,
  xoff: integer,
  yoff: integer,
  x: integer,
  y: integer,
  w: integer,
  h: integer,
  target: GL?,
  level: GL?
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L4307-L4318" target="_blank">source</a>]


### gl.RenderToTexture

```lua
function gl.RenderToTexture(
  texName: string,
  fun: unknown,
  ...: any
)
```
@param `...` - Arguments to the function.






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L4355-L4360" target="_blank">source</a>]


### gl.GenerateMipmap

```lua
function gl.GenerateMipmap(texName: string)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L4409-L4412" target="_blank">source</a>]


### gl.ActiveTexture

```lua
function gl.ActiveTexture(
  texNum: integer,
  func: function,
  ...: any
)
```
@param `...` - Arguments to the function.






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L4434-L4439" target="_blank">source</a>]


### gl.TextEnv

```lua
function gl.TextEnv(
  target: GL,
  pname: GL,
  value: number
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L4468-L4473" target="_blank">source</a>]


### gl.TextEnv

```lua
function gl.TextEnv(
  target: GL,
  pname: GL,
  r: number?,
  g: number?,
  b: number?,
  a: number?
)
```
@param `r` - (Default: `0.0`)

@param `g` - (Default: `0.0`)

@param `b` - (Default: `0.0`)

@param `a` - (Default: `0.0`)






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L4474-L4482" target="_blank">source</a>]


### gl.MultiTexEnv

```lua
function gl.MultiTexEnv(
  texNum: integer,
  target: GL,
  pname: GL,
  value: number
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L4512-L4518" target="_blank">source</a>]


### gl.MultiTexEnv

```lua
function gl.MultiTexEnv(
  texNum: integer,
  target: GL,
  pname: GL,
  r: number?,
  g: number?,
  b: number?,
  a: number?
)
```
@param `r` - (Default: `0.0`)

@param `g` - (Default: `0.0`)

@param `b` - (Default: `0.0`)

@param `a` - (Default: `0.0`)






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L4519-L4528" target="_blank">source</a>]


### gl.TexGen

```lua
function gl.TexGen(
  target: GL,
  state: boolean
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L4579-L4583" target="_blank">source</a>]


### gl.TexGen

```lua
function gl.TexGen(
  target: GL,
  pname: GL,
  value: number
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L4584-L4589" target="_blank">source</a>]


### gl.TexGen

```lua
function gl.TexGen(
  target: GL,
  pname: GL,
  r: number?,
  g: number?,
  b: number?,
  a: number?
)
```
@param `r` - (Default: `0.0`)

@param `g` - (Default: `0.0`)

@param `b` - (Default: `0.0`)

@param `a` - (Default: `0.0`)






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L4590-L4598" target="_blank">source</a>]


### gl.MultiTexGen

```lua
function gl.MultiTexGen(
  texNum: integer,
  target: GL,
  state: boolean
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L4637-L4642" target="_blank">source</a>]


### gl.MultiTexGen

```lua
function gl.MultiTexGen(
  texNum: integer,
  target: GL,
  pname: GL,
  value: number
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L4643-L4649" target="_blank">source</a>]


### gl.MultiTexGen

```lua
function gl.MultiTexGen(
  texNum: integer,
  target: GL,
  pname: GL,
  r: number?,
  g: number?,
  b: number?,
  a: number?
)
```
@param `r` - (Default: `0.0`)

@param `g` - (Default: `0.0`)

@param `b` - (Default: `0.0`)

@param `a` - (Default: `0.0`)






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L4650-L4659" target="_blank">source</a>]


### gl.BindImageTexture

```lua
function gl.BindImageTexture(
  unit: integer,
  texID: string?,
  level: integer?,
  layer: integer?,
  access: integer?,
  format: integer?
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L4708-L4716" target="_blank">source</a>]


### gl.CreateTextureAtlas

```lua
function gl.CreateTextureAtlas(
  xsize: integer,
  ysize: integer,
  allocType: integer?
) -> texName string
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L4828-L4834" target="_blank">source</a>]


### gl.FinalizeTextureAtlas

```lua
function gl.FinalizeTextureAtlas(texName: string) ->  boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L4859-L4863" target="_blank">source</a>]


### gl.DeleteTextureAtlas

```lua
function gl.DeleteTextureAtlas(texName: string) ->  boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L4879-L4883" target="_blank">source</a>]


### gl.AddAtlasTexture

```lua
function gl.AddAtlasTexture(
  texName: string,
  subAtlasTexName: string
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L4895-L4899" target="_blank">source</a>]


### gl.GetAtlasTexture

```lua
function gl.GetAtlasTexture(
  texName: string,
  subAtlasTexName: string
)
 -> x1 number
 -> x2 number
 -> y1 number
 -> y2 number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L4946-L4954" target="_blank">source</a>]


### gl.GetEngineAtlasTextures

```lua
function gl.GetEngineAtlasTextures(atlasName: ("$explosions"|"$groundfx")) -> atlasTextures table<string,float4>
```

@return `atlasTextures` - Table of x1,x2,y1,y2 coordinates by texture name.





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L4979-L4983" target="_blank">source</a>]


### gl.Clear

```lua
function gl.Clear(
  bits: GL,
  val: number
)
```
@param `bits` - `GL.DEPTH_BUFFER_BIT` or `GL.STENCIL_BUFFER_BIT`.






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L5033-L5037" target="_blank">source</a>]


### gl.Clear

```lua
function gl.Clear(
  bits: GL,
  r: number,
  g: number,
  b: number,
  a: number
)
```
@param `bits` - `GL.COLOR_BUFFER_BIT` or `GL.ACCUM_BUFFER_BIT`.






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L5038-L5045" target="_blank">source</a>]


### gl.SwapBuffers

```lua
function gl.SwapBuffers()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L5084-L5086" target="_blank">source</a>]


### gl.Translate

```lua
function gl.Translate(
  x: number,
  y: number,
  z: number
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L5101-L5106" target="_blank">source</a>]


### gl.Scale

```lua
function gl.Scale(
  x: number,
  y: number,
  z: number
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L5119-L5124" target="_blank">source</a>]


### gl.Rotate

```lua
function gl.Rotate(
  r: number,
  x: number,
  y: number,
  z: number
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L5137-L5143" target="_blank">source</a>]


### gl.Ortho

```lua
function gl.Ortho(
  left: number,
  right: number,
  bottom: number,
  top: number,
  near: number,
  far: number
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L5157-L5165" target="_blank">source</a>]


### gl.Frustum

```lua
function gl.Frustum(
  left: number,
  right: number,
  bottom: number,
  top: number,
  near: number,
  far: number
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L5181-L5189" target="_blank">source</a>]


### gl.Billboard

```lua
function gl.Billboard()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L5205-L5207" target="_blank">source</a>]


### gl.Light

```lua
function gl.Light(
  light: integer,
  enable: boolean
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L5219-L5223" target="_blank">source</a>]


### gl.Light

```lua
function gl.Light(
  light: integer,
  pname: GL,
  param: GL
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L5224-L5229" target="_blank">source</a>]


### gl.Light

```lua
function gl.Light(
  light: integer,
  pname: GL,
  r: number,
  g: number,
  b: number,
  a: number?
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L5230-L5238" target="_blank">source</a>]


### gl.ClipPlane

```lua
function gl.ClipPlane(
  plane: integer,
  enable: boolean
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L5289-L5293" target="_blank">source</a>]


### gl.ClipPlane

```lua
function gl.ClipPlane(
  plane: integer,
  equation0: number,
  equation1: number,
  equation2: number,
  equation3: number
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L5294-L5301" target="_blank">source</a>]


### gl.ClipDistance

```lua
function gl.ClipDistance(
  clipId: integer,
  enable: boolean
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L5331-L5335" target="_blank">source</a>]


### gl.MatrixMode

```lua
function gl.MatrixMode(mode: GL)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L5364-L5367" target="_blank">source</a>]


### gl.LoadIdentity

```lua
function gl.LoadIdentity()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L5380-L5382" target="_blank">source</a>]


### gl.LoadMatrix

```lua
function gl.LoadMatrix(matrix: string)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L5417-L5420" target="_blank">source</a>]


### gl.LoadMatrix

```lua
function gl.LoadMatrix(matrix: Matrix4x4)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L5421-L5424" target="_blank">source</a>]


### gl.LoadMatrix

```lua
function gl.LoadMatrix()
 -> m11 number
 -> m12 number
 -> m13 number
 -> m14 number
 -> m21 number
 -> m22 number
 -> m23 number
 -> m24 number
 -> m31 number
 -> m32 number
 -> m33 number
 -> m34 number
 -> m41 number
 -> m42 number
 -> m43 number
 -> m44 number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L5425-L5443" target="_blank">source</a>]


### gl.MultMatrix

```lua
function gl.MultMatrix(matrixName: string)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L5476-L5479" target="_blank">source</a>]


### gl.MultMatrix

```lua
function gl.MultMatrix(matrix: Matrix4x4)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L5480-L5483" target="_blank">source</a>]


### gl.MultMatrix

```lua
function gl.MultMatrix(
  m11: number,
  m12: number,
  m13: number,
  m14: number,
  m21: number,
  m22: number,
  m23: number,
  m24: number,
  m31: number,
  m32: number,
  m33: number,
  m34: number,
  m41: number,
  m42: number,
  m43: number,
  m44: number
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L5484-L5502" target="_blank">source</a>]


### gl.PushMatrix

```lua
function gl.PushMatrix()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L5535-L5537" target="_blank">source</a>]


### gl.PopMatrix

```lua
function gl.PopMatrix()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L5556-L5558" target="_blank">source</a>]


### gl.PushPopMatrix

```lua
function gl.PushPopMatrix(
  matMode1: GL,
  func: fun(),
  ...: any
)
```
@param `...` - Arguments to the function.






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L5576-L5581" target="_blank">source</a>]


### gl.PushPopMatrix

```lua
function gl.PushPopMatrix(
  func: fun(),
  ...: any
)
```
@param `...` - Arguments to the function.






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L5582-L5586" target="_blank">source</a>]


### gl.GetMatrixData

```lua
function gl.GetMatrixData(
  type: GL,
  index: integer
) -> The number
```
@param `type` - Matrix type (`GL.PROJECTION`, `GL.MODELVIEW`, `GL.TEXTURE`).

@param `index` - Matrix index in range `[1, 16]`.


@return `The` - value.





Get value at index of matrix.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L5634-L5641" target="_blank">source</a>]


### gl.GetMatrixData

```lua
function gl.GetMatrixData(type: GL) -> The Matrix4x4
```
@param `type` - Matrix type (`GL.PROJECTION`, `GL.MODELVIEW`, `GL.TEXTURE`).


@return `The` - matrix.





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L5642-L5646" target="_blank">source</a>]


### gl.GetMatrixData

```lua
function gl.GetMatrixData(index: integer) -> The number
```
@param `index` - Matrix index in range `[1, 16]`.


@return `The` - value.





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L5647-L5651" target="_blank">source</a>]


### gl.GetMatrixData

```lua
function gl.GetMatrixData(name: MatrixName) -> The Matrix4x4
```
@param `name` - The matrix name.


@return `The` - matrix.





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L5652-L5656" target="_blank">source</a>]


### gl.PushAttrib

```lua
function gl.PushAttrib(mask: GL?)
```
@param `mask` - (Default: `GL.ALL_ATTRIB_BITS`)






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L5719-L5722" target="_blank">source</a>]


### gl.PopAttrib

```lua
function gl.PopAttrib()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L5736-L5738" target="_blank">source</a>]


### gl.UnsafeState

```lua
function gl.UnsafeState(
  state: GL,
  func: fun(),
  ...: any
)
```
@param `...` - Arguments to the function.






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L5747-L5752" target="_blank">source</a>]


### gl.UnsafeState

```lua
function gl.UnsafeState(
  state: GL,
  reverse: boolean,
  func: fun(),
  ...: any
)
```
@param `...` - Arguments to the function.






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L5753-L5759" target="_blank">source</a>]


### gl.GetFixedState

```lua
function gl.GetFixedState(
  param: string,
  toStr: boolean?
)
 -> enabled boolean
 -> values any...

```
@param `toStr` - (Default: `false`)






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L5786-L5792" target="_blank">source</a>]


### gl.CreateList

```lua
function gl.CreateList(
  func: fun(),
  ...: any
)
```
@param `...` - Arguments to the function.






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L6063-L6067" target="_blank">source</a>]


### gl.CallList

```lua
function gl.CallList(listIndex: integer)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L6115-L6118" target="_blank">source</a>]


### gl.DeleteList

```lua
function gl.DeleteList(listIndex: integer)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L6139-L6142" target="_blank">source</a>]


### gl.Flush

```lua
function gl.Flush()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L6162-L6164" target="_blank">source</a>]


### gl.Finish

```lua
function gl.Finish()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L6173-L6175" target="_blank">source</a>]


### gl.ReadPixels

```lua
function gl.ReadPixels(
  x: integer,
  y: integer,
  w: 1,
  h: 1,
  format: GL?
) -> Color number...
```
@param `format` - (Default: `GL.RGBA`)


@return `Color` - values (color size based on format).





Get single pixel.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L6231-L6240" target="_blank">source</a>]


### gl.ReadPixels

```lua
function gl.ReadPixels(
  x: integer,
  y: integer,
  w: 1,
  h: integer,
  format: GL?
) -> Column number[][]
```
@param `format` - (Default: `GL.RGBA`)


@return `Column` - of color values (color size based on format).





Get column of pixels.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L6241-L6250" target="_blank">source</a>]


### gl.ReadPixels

```lua
function gl.ReadPixels(
  x: integer,
  y: integer,
  w: integer,
  h: 1,
  format: GL?
) -> Row number[][]
```
@param `format` - (Default: `GL.RGBA`)


@return `Row` - of color values (color size based on format).





Get row of pixels.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L6251-L6260" target="_blank">source</a>]


### gl.ReadPixels

```lua
function gl.ReadPixels(
  x: integer,
  y: integer,
  w: integer,
  h: integer,
  format: GL?
) -> Array number[][][]
```
@param `format` - (Default: `GL.RGBA`)


@return `Array` - of columns of color values (color size based on format).





Get row of pixels.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L6261-L6270" target="_blank">source</a>]


### gl.SaveImage

```lua
function gl.SaveImage(
  x: integer,
  y: integer,
  width: integer,
  height: integer,
  filename: string,
  options: SaveImageOptions?
) -> success boolean?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L6350-L6359" target="_blank">source</a>]


### gl.CreateQuery

```lua
function gl.CreateQuery() -> query any
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L6433-L6436" target="_blank">source</a>]


### gl.DeleteQuery

```lua
function gl.DeleteQuery(query: any)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L6457-L6460" target="_blank">source</a>]


### gl.RunQuery

```lua
function gl.RunQuery(query: any)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L6484-L6487" target="_blank">source</a>]


### gl.GetQuery

```lua
function gl.GetQuery(query: any) -> count integer
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L6524-L6528" target="_blank">source</a>]


### gl.GetGlobalTexNames

```lua
function gl.GetGlobalTexNames() -> List string[]
```

@return `List` - of texture names.





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L6551-L6554" target="_blank">source</a>]


### gl.GetGlobalTexCoords

```lua
function gl.GetGlobalTexCoords()
 -> xstart number
 -> ystart number
 -> xend number
 -> yend number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L6570-L6576" target="_blank">source</a>]


### gl.GetShadowMapParams

```lua
function gl.GetShadowMapParams()
 -> x number
 -> y number
 -> z number
 -> w number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L6593-L6599" target="_blank">source</a>]


### gl.GetAtmosphere

```lua
function gl.GetAtmosphere()
 -> lightDirX number
 -> lightDirY number
 -> lightDirZ number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L6609-L6614" target="_blank">source</a>]


### gl.GetAtmosphere

```lua
function gl.GetAtmosphere(param: ("fogStart"|"fogEnd"|"pos"|"fogColor"|"skyColor"|"sunColor"|"cloudColor"|"skyAxisAngle")) ->  any...
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L6615-L6619" target="_blank">source</a>]


### gl.GetSun

```lua
function gl.GetSun()
 -> lightDirX number
 -> lightDirY number
 -> lightDirZ number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L6672-L6677" target="_blank">source</a>]


### gl.GetSun

```lua
function gl.GetSun(
  param: ("pos"|"dir"|"specularExponent"|"shadowDensity"|"diffuse"|"ambient"|"specular"),
  mode: ("ground"|"unit")
)
 -> data1 number?
 -> data2 number?
 -> data3 number?

```
@param `mode` - (Default: `"ground"`)






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L6678-L6685" target="_blank">source</a>]


### gl.GetWaterRendering

```lua
function gl.GetWaterRendering(key: string) -> value any...
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L6751-L6755" target="_blank">source</a>]


### gl.GetMapRendering

```lua
function gl.GetMapRendering(key: string) -> value any...
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L6933-L6937" target="_blank">source</a>]


### gl.ObjectLabel

```lua
function gl.ObjectLabel(
  objectTypeIdentifier: GL,
  objectID: integer,
  label: string
)
```
@param `objectTypeIdentifier` - Specifies the type of object being labeled.

@param `objectID` - Specifies the name or ID of the object to label.

@param `label` - A string containing the label to be assigned to the object.






Labels an object for use with debugging tools.
May be unavailable and `nil` if the platform doesn't support the feature.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L6977-L6985" target="_blank">source</a>]


### gl.PushDebugGroup

```lua
function gl.PushDebugGroup(
  id: integer,
  message: string,
  sourceIsThirdParty: boolean
) ->  nil
```
@param `id` - A numeric identifier for the group, can be any unique number.

@param `message` - A human-readable string describing the debug group. Will be truncated if longer than driver-specific limit

@param `sourceIsThirdParty` - Set the source tag, true for GL_DEBUG_SOURCE_THIRD_PARTY, false for GL_DEBUG_SOURCE_APPLICATION. default false






Pushes a debug marker for debugging tools such as `nVidia nSight 2024.04`,
see https://registry.khronos.org/OpenGL-Refpages/gl4/html/glPushDebugGroup.xhtml .

May be unavailable and `nil` if the platform doesn't support the feature.

Groups are basically named scopes similar to tracy's, and are pushed/popped independently
from GL attribute/matrix push/pop (though of course makes sense to put them together).

Tools are known to struggle to see the annotation for FBOs if they are raw bound.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L7014-L7030" target="_blank">source</a>]


### gl.PopDebugGroup

```lua
function gl.PopDebugGroup() ->  nil
```





Pops the most recent GL debug group from the stack (does NOT take the numerical ID from push).
May be unavailable and `nil` if the platform doesn't support the feature.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaOpenGL.cpp#L7052-L7059" target="_blank">source</a>]


### gl.AddFallbackFont

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

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaFonts.cpp#L210-L226" target="_blank">source</a>]


### gl.ClearFallbackFonts

```lua
function gl.ClearFallbackFonts() ->  nil
```





Clears all fallback fonts.

See the note at 'AddFallbackFont' about the 'FontsChanged' callin,
it also applies when calling this method.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaFonts.cpp#L238-L245" target="_blank">source</a>]


### gl.CreateRBO

```lua
function gl.CreateRBO(
  xsize: integer,
  ysize: integer,
  data: CreateRBOData
) ->  RBO
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaRBOs.cpp#L157-L163" target="_blank">source</a>]


### gl.DeleteRBO

```lua
function gl.DeleteRBO(rbo: RBO)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaRBOs.cpp#L233-L236" target="_blank">source</a>]


### gl.GetVBO

```lua
function gl.GetVBO(
  bufferType: GL?,
  freqUpdated: boolean?
) -> VBO VBO?
```
@param `bufferType` - (Default: `GL.ARRAY_BUFFER`) The buffer type to use.

Accepts the following:
- `GL.ARRAY_BUFFER` for vertex data.
- `GL.ELEMENT_ARRAY_BUFFER` for vertex indices.
- `GL.UNIFORM_BUFFER`
- `GL.SHADER_STORAGE_BUFFER`

@param `freqUpdated` - (Default: `true`)

`true` to updated frequently, `false` to update only once.


@return `VBO` - The VBO ref on success, or nil if not supported or an error occurred.





Example:

```lua
local myVBO = gl.GetVBO()
if myVBO == nil then Spring.Echo("Failed to get VBO") end
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVBO.cpp#L124-L151" target="_blank">source</a>]
 See: GL.OpenGL_Buffer_Types







{% endraw %}