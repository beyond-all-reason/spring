---
layout: default
title: gl
parent: Lua API
permalink: lua-api/globals/gl
---

# global gl




## methods


### gl.HasExtension

```lua
function gl.HasExtension(ext: string) ->  boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L1147-L1151" target="_blank">source</a>]


### gl.GetNumber

```lua
function gl.GetNumber(
  pname: GL,
  count: integer?
) ->  number...
```
@param `count` - (Default: `1`) Number of values to return, in range [1, 64].






Get the value or values of a selected parameter.

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L1159-L1166" target="_blank">source</a>]


### gl.GetString

```lua
function gl.GetString(pname: GL)
```





Get a string describing the current OpenGL connection.

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L1184-L1188" target="_blank">source</a>]


### gl.GetScreenViewTrans

```lua
function gl.GetScreenViewTrans()
 -> x number
 -> y number
 -> z number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L1204-L1209" target="_blank">source</a>]


### gl.GetViewSizes

```lua
function gl.GetViewSizes()
 -> x number
 -> y number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L1219-L1223" target="_blank">source</a>]


### gl.GetViewRange

```lua
function gl.GetViewRange()
 -> nearPlaneDist number
 -> farPlaneDist number
 -> minViewRange number
 -> maxViewRange number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L1232-L1238" target="_blank">source</a>]


### gl.SetSlaveMode

```lua
function gl.SetSlaveMode(newMode: boolean)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L1254-L1257" target="_blank">source</a>]


### gl.ConfigMiniMap

```lua
function gl.ConfigMiniMap(
  px: integer,
  py: integer,
  sx: integer,
  sy: integer
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L1269-L1275" target="_blank">source</a>]


### gl.DrawMiniMap

```lua
function gl.DrawMiniMap(defaultTransform: boolean?)
```
@param `defaultTransform` - (Default: `true`)






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L1291-L1294" target="_blank">source</a>]


### gl.BeginText

```lua
function gl.BeginText()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L1331-L1333" target="_blank">source</a>]


### gl.EndText

```lua
function gl.EndText()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L1342-L1344" target="_blank">source</a>]


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






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L1352-L1377" target="_blank">source</a>]


### gl.GetTextWidth

```lua
function gl.GetTextWidth(text: string) -> width number
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L1433-L1437" target="_blank">source</a>]


### gl.GetTextHeight

```lua
function gl.GetTextHeight(text: string)
 -> height number
 -> descender number
 -> lines integer

```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L1448-L1454" target="_blank">source</a>]


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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L1649-L1658" target="_blank">source</a>]


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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L1661-L1673" target="_blank">source</a>]


### gl.UnitGL4

```lua
function gl.UnitGL4()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L1676-L1679" target="_blank">source</a>]


### gl.UnitTextures

```lua
function gl.UnitTextures(
  unitID: integer,
  push: boolean
)
```
@param `push` - If `true`, push the render state; if `false`, pop it.






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L1685-L1689" target="_blank">source</a>]


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






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L1697-L1704" target="_blank">source</a>]


### gl.UnitShapeGL4

```lua
function gl.UnitShapeGL4()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L1712-L1715" target="_blank">source</a>]


### gl.UnitShapeTextures

```lua
function gl.UnitShapeTextures(
  unitDefID: integer,
  push: boolean
)
```
@param `push` - If `true`, push the render state; if `false`, pop it.






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L1721-L1725" target="_blank">source</a>]


### gl.UnitMultMatrix

```lua
function gl.UnitMultMatrix(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L1734-L1737" target="_blank">source</a>]


### gl.UnitPiece

```lua
function gl.UnitPiece(
  unitID: integer,
  pieceID: integer
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L1752-L1756" target="_blank">source</a>]


### gl.UnitPieceMatrix

```lua
function gl.UnitPieceMatrix(
  unitID: integer,
  pieceID: integer
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L1763-L1767" target="_blank">source</a>]


### gl.UnitPieceMultMatrix

```lua
function gl.UnitPieceMultMatrix(
  unitID: integer,
  pieceID: integer
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L1770-L1774" target="_blank">source</a>]


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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L1835-L1843" target="_blank">source</a>]


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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L1846-L1856" target="_blank">source</a>]


### gl.FeatureGL4

```lua
function gl.FeatureGL4()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L1859-L1862" target="_blank">source</a>]


### gl.FeatureTextures

```lua
function gl.FeatureTextures(
  featureID: integer,
  push: boolean
)
```
@param `push` - If `true`, push the render state; if `false`, pop it.






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L1868-L1872" target="_blank">source</a>]


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






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L1880-L1887" target="_blank">source</a>]


### gl.FeatureShapeGL4

```lua
function gl.FeatureShapeGL4()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L1895-L1898" target="_blank">source</a>]


### gl.FeatureShapeTextures

```lua
function gl.FeatureShapeTextures(
  featureDefID: integer,
  push: boolean
)
```
@param `push` - If `true`, push the render state; if `false`, pop it.






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L1904-L1908" target="_blank">source</a>]


### gl.FeatureMultMatrix

```lua
function gl.FeatureMultMatrix(featureID: integer)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L1917-L1920" target="_blank">source</a>]


### gl.FeaturePiece

```lua
function gl.FeaturePiece(
  featureID: integer,
  pieceID: integer
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L1935-L1939" target="_blank">source</a>]


### gl.FeaturePieceMatrix

```lua
function gl.FeaturePieceMatrix(
  featureID: integer,
  pieceID: integer
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L1947-L1951" target="_blank">source</a>]


### gl.FeaturePieceMultMatrix

```lua
function gl.FeaturePieceMultMatrix(
  featureID: integer,
  pieceID: integer
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L1955-L1959" target="_blank">source</a>]


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






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L1972-L1984" target="_blank">source</a>]


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






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L2029-L2035" target="_blank">source</a>]


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





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L2072-L2079" target="_blank">source</a>]


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





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L2080-L2090" target="_blank">source</a>]


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






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L2131-L2139" target="_blank">source</a>]


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






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L2140-L2151" target="_blank">source</a>]


### gl.Shape

```lua
function gl.Shape(
  type: GL,
  vertices: VertexData[]
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L2340-L2344" target="_blank">source</a>]


### gl.BeginEnd

```lua
function gl.BeginEnd(
  primMode: GL,
  fun: unknown,
  ...: any
)
```
@param `...` - Arguments passed to function.






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L2386-L2391" target="_blank">source</a>]


### gl.Vertex

```lua
function gl.Vertex(v: xy)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L2423-L2426" target="_blank">source</a>]


### gl.Vertex

```lua
function gl.Vertex(v: xyz)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L2427-L2430" target="_blank">source</a>]


### gl.Vertex

```lua
function gl.Vertex(v: xyzw)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L2431-L2434" target="_blank">source</a>]


### gl.Vertex

```lua
function gl.Vertex(
  x: number,
  y: number,
  z: number?,
  w: number?
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L2435-L2441" target="_blank">source</a>]


### gl.Normal

```lua
function gl.Normal(v: xyz)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L2505-L2508" target="_blank">source</a>]


### gl.Normal

```lua
function gl.Normal(
  x: number,
  y: number,
  z: number
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L2509-L2514" target="_blank">source</a>]


### gl.TexCoord

```lua
function gl.TexCoord(coord: (number))
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L2553-L2556" target="_blank">source</a>]


### gl.TexCoord

```lua
function gl.TexCoord(coord: xy)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L2557-L2560" target="_blank">source</a>]


### gl.TexCoord

```lua
function gl.TexCoord(coord: xyz)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L2561-L2564" target="_blank">source</a>]


### gl.TexCoord

```lua
function gl.TexCoord(coord: xyzw)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L2565-L2568" target="_blank">source</a>]


### gl.TexCoord

```lua
function gl.TexCoord(
  s: number,
  t: number?,
  r: number?,
  q: number?
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L2569-L2575" target="_blank">source</a>]


### gl.MultiTexCoord

```lua
function gl.MultiTexCoord(
  texNum: integer,
  coord: (number)
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L2644-L2648" target="_blank">source</a>]


### gl.MultiTexCoord

```lua
function gl.MultiTexCoord(
  texNum: integer,
  coord: xy
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L2649-L2653" target="_blank">source</a>]


### gl.MultiTexCoord

```lua
function gl.MultiTexCoord(
  texNum: integer,
  coord: xyz
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L2654-L2658" target="_blank">source</a>]


### gl.MultiTexCoord

```lua
function gl.MultiTexCoord(
  texNum: integer,
  coord: xyzw
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L2659-L2663" target="_blank">source</a>]


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





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L2664-L2671" target="_blank">source</a>]


### gl.SecondaryColor

```lua
function gl.SecondaryColor(color: rgb)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L2746-L2749" target="_blank">source</a>]


### gl.SecondaryColor

```lua
function gl.SecondaryColor(
  r: number,
  g: number,
  b: number
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L2750-L2755" target="_blank">source</a>]


### gl.FogCoord

```lua
function gl.FogCoord(coord: number)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L2794-L2797" target="_blank">source</a>]


### gl.EdgeFlag

```lua
function gl.EdgeFlag(flag: boolean)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L2809-L2812" target="_blank">source</a>]


### gl.Rect

```lua
function gl.Rect(
  x1: number,
  y1: number,
  x2: number,
  y2: number
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L2827-L2833" target="_blank">source</a>]


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





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L2846-L2854" target="_blank">source</a>]


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





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L2855-L2865" target="_blank">source</a>]


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






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L2921-L2927" target="_blank">source</a>]


### gl.MemoryBarrier

```lua
function gl.MemoryBarrier(barriers: integer?)
```
@param `barriers` - (Default: `4`)






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L2960-L2963" target="_blank">source</a>]


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






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L2981-L2987" target="_blank">source</a>]


### gl.Color

```lua
function gl.Color(color: rgba)
```
@param `color` - Color with alpha.






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L2988-L2991" target="_blank">source</a>]


### gl.Color

```lua
function gl.Color(color: rgb)
```
@param `color` - Color.






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L2992-L2995" target="_blank">source</a>]


### gl.Material

```lua
function gl.Material(material: Material)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3043-L3046" target="_blank">source</a>]


### gl.ResetState

```lua
function gl.ResetState()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3116-L3118" target="_blank">source</a>]


### gl.ResetMatrices

```lua
function gl.ResetMatrices()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3127-L3129" target="_blank">source</a>]


### gl.Lighting

```lua
function gl.Lighting(enable: boolean)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3145-L3148" target="_blank">source</a>]


### gl.ShadeModel

```lua
function gl.ShadeModel(model: GL)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3162-L3165" target="_blank">source</a>]


### gl.Scissor

```lua
function gl.Scissor(enable: boolean)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3174-L3177" target="_blank">source</a>]


### gl.Scissor

```lua
function gl.Scissor(
  x: integer,
  y: integer,
  w: integer,
  h: integer
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3178-L3184" target="_blank">source</a>]


### gl.Viewport

```lua
function gl.Viewport(
  x: integer,
  y: integer,
  w: integer,
  h: integer
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3215-L3221" target="_blank">source</a>]


### gl.ColorMask

```lua
function gl.ColorMask(rgba: boolean)
```





Enable or disable writing of frame buffer color components.

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3238-L3242" target="_blank">source</a>]


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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3243-L3250" target="_blank">source</a>]


### gl.DepthMask

```lua
function gl.DepthMask(enable: boolean)
```





Enable or disable writing into the depth buffer.

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3274-L3278" target="_blank">source</a>]


### gl.DepthTest

```lua
function gl.DepthTest(enable: boolean)
```





Enable or disable depth test.

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3291-L3295" target="_blank">source</a>]


### gl.DepthTest

```lua
function gl.DepthTest(depthFunction: GL)
```
@param `depthFunction` - Symbolic constants `GL.NEVER`, `GL.LESS`, `GL.EQUAL`, `GL.LEQUAL`,
`GL.GREATER`, `GL.NOTEQUAL`, `GL.GEQUAL`, and `GL.ALWAYS` are accepted.
The initial value is `GL.LESS`.






Enable depth test and specify the depth comparison function.

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3296-L3305" target="_blank">source</a>]


### gl.DepthClamp

```lua
function gl.DepthClamp(enable: boolean)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3333-L3336" target="_blank">source</a>]


### gl.Culling

```lua
function gl.Culling(enable: boolean)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3350-L3353" target="_blank">source</a>]


### gl.Culling

```lua
function gl.Culling(mode: GL)
```
@param `mode` - Specifies whether front- or back-facing facets are candidates for culling.
Symbolic constants `GL.FRONT`, `GL.BACK`, and `GL.FRONT_AND_BACK` are accepted. The
initial value is `GL.BACK`.






Enable culling and set culling mode.

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3354-L3362" target="_blank">source</a>]


### gl.LogicOp

```lua
function gl.LogicOp(enable: boolean)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3391-L3394" target="_blank">source</a>]


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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3395-L3407" target="_blank">source</a>]


### gl.Fog

```lua
function gl.Fog(enable: boolean)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3435-L3438" target="_blank">source</a>]


### gl.Blending

```lua
function gl.Blending(enable: boolean)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3453-L3456" target="_blank">source</a>]


### gl.Blending

```lua
function gl.Blending(mode: ("add"|"alpha_add"|"alpha"|"reset"|"color"|"modulate"|"disable"))
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3457-L3460" target="_blank">source</a>]


### gl.Blending

```lua
function gl.Blending(
  src: GL,
  dst: GL
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3461-L3465" target="_blank">source</a>]


### gl.BlendEquation

```lua
function gl.BlendEquation(mode: GL)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3528-L3531" target="_blank">source</a>]


### gl.BlendFunc

```lua
function gl.BlendFunc(
  src: GL,
  dst: GL
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3541-L3545" target="_blank">source</a>]


### gl.BlendEquationSeparate

```lua
function gl.BlendEquationSeparate(
  modeRGB: GL,
  modeAlpha: GL
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3556-L3560" target="_blank">source</a>]


### gl.BlendFuncSeparate

```lua
function gl.BlendFuncSeparate(
  srcRGB: GL,
  dstRGB: GL,
  srcAlpha: GL,
  dstAlpha: GL
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3571-L3577" target="_blank">source</a>]


### gl.AlphaTest

```lua
function gl.AlphaTest(enable: boolean)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3590-L3593" target="_blank">source</a>]


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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3594-L3607" target="_blank">source</a>]


### gl.AlphaToCoverage

```lua
function gl.AlphaToCoverage(enable: boolean)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3631-L3634" target="_blank">source</a>]


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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3652-L3668" target="_blank">source</a>]


### gl.PolygonOffset

```lua
function gl.PolygonOffset(enable: boolean)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3679-L3682" target="_blank">source</a>]


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






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3683-L3694" target="_blank">source</a>]


### gl.StencilTest

```lua
function gl.StencilTest(enable: boolean)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3726-L3729" target="_blank">source</a>]


### gl.StencilMask

```lua
function gl.StencilMask(mask: integer)
```
@param `mask` - Specifies a bit mask to enable and disable writing of individual bits in the stencil planes. Initially, the mask is all `1`'s.






Control the front and back writing of individual bits in the stencil planes.

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3743-L3747" target="_blank">source</a>]


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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3757-L3763" target="_blank">source</a>]


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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3775-L3781" target="_blank">source</a>]


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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3793-L3798" target="_blank">source</a>]


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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3809-L3816" target="_blank">source</a>]


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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3829-L3836" target="_blank">source</a>]


### gl.LineStipple

```lua
function gl.LineStipple(enable: boolean)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3851-L3854" target="_blank">source</a>]


### gl.LineStipple

```lua
function gl.LineStipple(ignoredString: string)
```
@param `ignoredString` - The value of this string is ignored, but it still does something.






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3855-L3858" target="_blank">source</a>]


### gl.LineStipple

```lua
function gl.LineStipple(
  factor: integer,
  pattern: integer,
  shift: integer?
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3859-L3864" target="_blank">source</a>]


### gl.LineWidth

```lua
function gl.LineWidth(width: number)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3914-L3917" target="_blank">source</a>]


### gl.PointSize

```lua
function gl.PointSize(size: number)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3928-L3931" target="_blank">source</a>]


### gl.PointSprite

```lua
function gl.PointSprite(
  enable: boolean,
  enableCoordReplace: boolean?,
  coordOrigin: boolean?
)
```
@param `coordOrigin` - `true` for upper left, `false` for lower left, otherwise no change.






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3942-L3947" target="_blank">source</a>]


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





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L3976-L3984" target="_blank">source</a>]


### gl.Texture

```lua
function gl.Texture(
  texNum: integer,
  enable: boolean?
) ->  boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L4012-L4017" target="_blank">source</a>]


### gl.Texture

```lua
function gl.Texture(enable: boolean) ->  boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L4018-L4022" target="_blank">source</a>]


### gl.Texture

```lua
function gl.Texture(
  texNum: integer,
  image: string
) ->  boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L4023-L4028" target="_blank">source</a>]


### gl.Texture

```lua
function gl.Texture(image: string) ->  boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L4029-L4033" target="_blank">source</a>]


### gl.CreateTexture

```lua
function gl.CreateTexture(
  xsize: integer,
  ysize: integer,
  texture: Texture
) -> texName string?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L4162-L4168" target="_blank">source</a>]


### gl.CreateTexture

```lua
function gl.CreateTexture(
  xsize: integer,
  ysize: integer,
  zsize: integer,
  texture: Texture
) -> texName string?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L4169-L4176" target="_blank">source</a>]


### gl.ChangeTextureParams

```lua
function gl.ChangeTextureParams(
  texName: string,
  params: Texture
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L4244-L4248" target="_blank">source</a>]


### gl.DeleteTexture

```lua
function gl.DeleteTexture(texName: string) ->  boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L4275-L4279" target="_blank">source</a>]


### gl.DeleteTextureFBO

```lua
function gl.DeleteTextureFBO(texName: string) ->  boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L4296-L4300" target="_blank">source</a>]


### gl.TextureInfo

```lua
function gl.TextureInfo(texName: string) -> textureInfo TextureInfo
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L4321-L4325" target="_blank">source</a>]


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





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L4347-L4358" target="_blank">source</a>]


### gl.RenderToTexture

```lua
function gl.RenderToTexture(
  texName: string,
  fun: unknown,
  ...: any
)
```
@param `...` - Arguments to the function.






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L4395-L4400" target="_blank">source</a>]


### gl.GenerateMipmap

```lua
function gl.GenerateMipmap(texName: string)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L4449-L4452" target="_blank">source</a>]


### gl.ActiveTexture

```lua
function gl.ActiveTexture(
  texNum: integer,
  func: function,
  ...: any
)
```
@param `...` - Arguments to the function.






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L4474-L4479" target="_blank">source</a>]


### gl.TextEnv

```lua
function gl.TextEnv(
  target: GL,
  pname: GL,
  value: number
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L4508-L4513" target="_blank">source</a>]


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






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L4514-L4522" target="_blank">source</a>]


### gl.MultiTexEnv

```lua
function gl.MultiTexEnv(
  texNum: integer,
  target: GL,
  pname: GL,
  value: number
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L4552-L4558" target="_blank">source</a>]


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






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L4559-L4568" target="_blank">source</a>]


### gl.TexGen

```lua
function gl.TexGen(
  target: GL,
  state: boolean
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L4619-L4623" target="_blank">source</a>]


### gl.TexGen

```lua
function gl.TexGen(
  target: GL,
  pname: GL,
  value: number
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L4624-L4629" target="_blank">source</a>]


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






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L4630-L4638" target="_blank">source</a>]


### gl.MultiTexGen

```lua
function gl.MultiTexGen(
  texNum: integer,
  target: GL,
  state: boolean
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L4677-L4682" target="_blank">source</a>]


### gl.MultiTexGen

```lua
function gl.MultiTexGen(
  texNum: integer,
  target: GL,
  pname: GL,
  value: number
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L4683-L4689" target="_blank">source</a>]


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






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L4690-L4699" target="_blank">source</a>]


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





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L4748-L4756" target="_blank">source</a>]


### gl.CreateTextureAtlas

```lua
function gl.CreateTextureAtlas(
  xsize: integer,
  ysize: integer,
  allocType: integer?
) -> texName string
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L4868-L4874" target="_blank">source</a>]


### gl.FinalizeTextureAtlas

```lua
function gl.FinalizeTextureAtlas(texName: string) ->  boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L4899-L4903" target="_blank">source</a>]


### gl.DeleteTextureAtlas

```lua
function gl.DeleteTextureAtlas(texName: string) ->  boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L4919-L4923" target="_blank">source</a>]


### gl.AddAtlasTexture

```lua
function gl.AddAtlasTexture(
  texName: string,
  subAtlasTexName: string
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L4935-L4939" target="_blank">source</a>]


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





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L4986-L4994" target="_blank">source</a>]


### gl.GetEngineAtlasTextures

```lua
function gl.GetEngineAtlasTextures(atlasName: ("$explosions"|"$groundfx")) -> atlasTextures table<string,float4>
```

@return `atlasTextures` - Table of x1,x2,y1,y2 coordinates by texture name.





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L5019-L5023" target="_blank">source</a>]


### gl.Clear

```lua
function gl.Clear(
  bits: GL,
  val: number
)
```
@param `bits` - `GL.DEPTH_BUFFER_BIT` or `GL.STENCIL_BUFFER_BIT`.






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L5073-L5077" target="_blank">source</a>]


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






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L5078-L5085" target="_blank">source</a>]


### gl.SwapBuffers

```lua
function gl.SwapBuffers()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L5124-L5126" target="_blank">source</a>]


### gl.Translate

```lua
function gl.Translate(
  x: number,
  y: number,
  z: number
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L5141-L5146" target="_blank">source</a>]


### gl.Scale

```lua
function gl.Scale(
  x: number,
  y: number,
  z: number
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L5159-L5164" target="_blank">source</a>]


### gl.Rotate

```lua
function gl.Rotate(
  r: number,
  x: number,
  y: number,
  z: number
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L5177-L5183" target="_blank">source</a>]


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





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L5197-L5205" target="_blank">source</a>]


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





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L5221-L5229" target="_blank">source</a>]


### gl.Billboard

```lua
function gl.Billboard()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L5245-L5247" target="_blank">source</a>]


### gl.Light

```lua
function gl.Light(
  light: integer,
  enable: boolean
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L5259-L5263" target="_blank">source</a>]


### gl.Light

```lua
function gl.Light(
  light: integer,
  pname: GL,
  param: GL
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L5264-L5269" target="_blank">source</a>]


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





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L5270-L5278" target="_blank">source</a>]


### gl.ClipPlane

```lua
function gl.ClipPlane(
  plane: integer,
  enable: boolean
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L5329-L5333" target="_blank">source</a>]


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





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L5334-L5341" target="_blank">source</a>]


### gl.ClipDistance

```lua
function gl.ClipDistance(
  clipId: integer,
  enable: boolean
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L5371-L5375" target="_blank">source</a>]


### gl.MatrixMode

```lua
function gl.MatrixMode(mode: GL)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L5404-L5407" target="_blank">source</a>]


### gl.LoadIdentity

```lua
function gl.LoadIdentity()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L5420-L5422" target="_blank">source</a>]


### gl.LoadMatrix

```lua
function gl.LoadMatrix(matrix: string)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L5457-L5460" target="_blank">source</a>]


### gl.LoadMatrix

```lua
function gl.LoadMatrix(matrix: Matrix4x4)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L5461-L5464" target="_blank">source</a>]


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





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L5465-L5483" target="_blank">source</a>]


### gl.MultMatrix

```lua
function gl.MultMatrix(matrixName: string)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L5516-L5519" target="_blank">source</a>]


### gl.MultMatrix

```lua
function gl.MultMatrix(matrix: Matrix4x4)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L5520-L5523" target="_blank">source</a>]


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





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L5524-L5542" target="_blank">source</a>]


### gl.PushMatrix

```lua
function gl.PushMatrix()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L5575-L5577" target="_blank">source</a>]


### gl.PopMatrix

```lua
function gl.PopMatrix()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L5596-L5598" target="_blank">source</a>]


### gl.PushPopMatrix

```lua
function gl.PushPopMatrix(
  matMode1: GL,
  func: fun(),
  ...: any
)
```
@param `...` - Arguments to the function.






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L5616-L5621" target="_blank">source</a>]


### gl.PushPopMatrix

```lua
function gl.PushPopMatrix(
  func: fun(),
  ...: any
)
```
@param `...` - Arguments to the function.






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L5622-L5626" target="_blank">source</a>]


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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L5674-L5681" target="_blank">source</a>]


### gl.GetMatrixData

```lua
function gl.GetMatrixData(type: GL) -> The Matrix4x4
```
@param `type` - Matrix type (`GL.PROJECTION`, `GL.MODELVIEW`, `GL.TEXTURE`).


@return `The` - matrix.





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L5682-L5686" target="_blank">source</a>]


### gl.GetMatrixData

```lua
function gl.GetMatrixData(index: integer) -> The number
```
@param `index` - Matrix index in range `[1, 16]`.


@return `The` - value.





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L5687-L5691" target="_blank">source</a>]


### gl.GetMatrixData

```lua
function gl.GetMatrixData(name: MatrixName) -> The Matrix4x4
```
@param `name` - The matrix name.


@return `The` - matrix.





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L5692-L5696" target="_blank">source</a>]


### gl.PushAttrib

```lua
function gl.PushAttrib(mask: GL?)
```
@param `mask` - (Default: `GL.ALL_ATTRIB_BITS`)






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L5759-L5762" target="_blank">source</a>]


### gl.PopAttrib

```lua
function gl.PopAttrib()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L5776-L5778" target="_blank">source</a>]


### gl.UnsafeState

```lua
function gl.UnsafeState(
  state: GL,
  func: fun(),
  ...: any
)
```
@param `...` - Arguments to the function.






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L5787-L5792" target="_blank">source</a>]


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






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L5793-L5799" target="_blank">source</a>]


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






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L5826-L5832" target="_blank">source</a>]


### gl.CreateList

```lua
function gl.CreateList(
  func: fun(),
  ...: any
)
```
@param `...` - Arguments to the function.






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L6103-L6107" target="_blank">source</a>]


### gl.CallList

```lua
function gl.CallList(listIndex: integer)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L6155-L6158" target="_blank">source</a>]


### gl.DeleteList

```lua
function gl.DeleteList(listIndex: integer)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L6179-L6182" target="_blank">source</a>]


### gl.Flush

```lua
function gl.Flush()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L6202-L6204" target="_blank">source</a>]


### gl.Finish

```lua
function gl.Finish()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L6213-L6215" target="_blank">source</a>]


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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L6271-L6280" target="_blank">source</a>]


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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L6281-L6290" target="_blank">source</a>]


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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L6291-L6300" target="_blank">source</a>]


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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L6301-L6310" target="_blank">source</a>]


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





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L6390-L6399" target="_blank">source</a>]


### gl.CreateQuery

```lua
function gl.CreateQuery() -> query any
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L6473-L6476" target="_blank">source</a>]


### gl.DeleteQuery

```lua
function gl.DeleteQuery(query: any)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L6497-L6500" target="_blank">source</a>]


### gl.RunQuery

```lua
function gl.RunQuery(query: any)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L6524-L6527" target="_blank">source</a>]


### gl.GetQuery

```lua
function gl.GetQuery(query: any) -> count integer
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L6564-L6568" target="_blank">source</a>]


### gl.GetGlobalTexNames

```lua
function gl.GetGlobalTexNames() -> List string[]
```

@return `List` - of texture names.





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L6591-L6594" target="_blank">source</a>]


### gl.GetGlobalTexCoords

```lua
function gl.GetGlobalTexCoords()
 -> xstart number
 -> ystart number
 -> xend number
 -> yend number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L6610-L6616" target="_blank">source</a>]


### gl.GetShadowMapParams

```lua
function gl.GetShadowMapParams()
 -> x number
 -> y number
 -> z number
 -> w number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L6633-L6639" target="_blank">source</a>]


### gl.GetAtmosphere

```lua
function gl.GetAtmosphere()
 -> lightDirX number
 -> lightDirY number
 -> lightDirZ number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L6649-L6654" target="_blank">source</a>]


### gl.GetAtmosphere

```lua
function gl.GetAtmosphere(param: ("fogStart"|"fogEnd"|"pos"|"fogColor"|"skyColor"|"sunColor"|"cloudColor"|"skyAxisAngle")) ->  any...
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L6655-L6659" target="_blank">source</a>]


### gl.GetSun

```lua
function gl.GetSun()
 -> lightDirX number
 -> lightDirY number
 -> lightDirZ number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L6712-L6717" target="_blank">source</a>]


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






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L6718-L6725" target="_blank">source</a>]


### gl.GetWaterRendering

```lua
function gl.GetWaterRendering(key: string) -> value any...
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L6791-L6795" target="_blank">source</a>]


### gl.GetMapRendering

```lua
function gl.GetMapRendering(key: string) -> value any...
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L6973-L6977" target="_blank">source</a>]


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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L7017-L7023" target="_blank">source</a>]


### gl.PushDebugGroup

```lua
function gl.PushDebugGroup(
  id: integer,
  message: string,
  sourceIsThirdParty: boolean
) ->  nil
```
@param `id` - A numeric identifier for the group.

@param `message` - A human-readable string describing the debug group.

@param `sourceIsThirdParty` - Set the source tag, true for GL_DEBUG_SOURCE_THIRD_PARTY, false for GL_DEBUG_SOURCE_APPLICATION. default false






Pushes a debug marker for nVidia nSight 2024.04, does not seem to work when
FBO's are raw bound.

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L7053-L7062" target="_blank">source</a>]


### gl.PopDebugGroup

```lua
function gl.PopDebugGroup() ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaOpenGL.cpp#L7084-L7087" target="_blank">source</a>]


### gl.CreateRBO

```lua
function gl.CreateRBO(
  xsize: integer,
  ysize: integer,
  data: CreateRBOData
) ->  RBO
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaRBOs.cpp#L157-L163" target="_blank">source</a>]


### gl.DeleteRBO

```lua
function gl.DeleteRBO(rbo: RBO)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaRBOs.cpp#L233-L236" target="_blank">source</a>]


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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVAO.cpp#L75-L84" target="_blank">source</a>]


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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaFonts.cpp#L210-L226" target="_blank">source</a>]


### gl.ClearFallbackFonts

```lua
function gl.ClearFallbackFonts() ->  nil
```





Clears all fallback fonts.

See the note at 'AddFallbackFont' about the 'FontsChanged' callin,
it also applies when calling this method.

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaFonts.cpp#L238-L245" target="_blank">source</a>]


### gl.CreateFBO

```lua
function gl.CreateFBO(fboDesc: FBODescription) -> fbo FBO
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaFBOs.cpp#L467-L471" target="_blank">source</a>]


### gl.DeleteFBO

```lua
function gl.DeleteFBO(fbo: FBO)
```





This doesn't delete the attached objects!

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaFBOs.cpp#L542-L547" target="_blank">source</a>]


### gl.IsValidFBO

```lua
function gl.IsValidFBO(
  fbo: FBO,
  target: GL?
)
 -> valid boolean
 -> status number?

```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaFBOs.cpp#L560-L566" target="_blank">source</a>]


### gl.ActiveFBO

```lua
function gl.ActiveFBO(
  fbo: FBO,
  func: fun(...),
  ...: any
)
```
@param `...` - args






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaFBOs.cpp#L601-L606" target="_blank">source</a>]


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






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaFBOs.cpp#L607-L613" target="_blank">source</a>]


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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaFBOs.cpp#L677-L685" target="_blank">source</a>]


### gl.RawBindFBO

```lua
function gl.RawBindFBO(
  fbo: FBO,
  target: GL?
) -> previouslyBoundRawFboId number
```
@param `target` - (Default: `fbo.target`)






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaFBOs.cpp#L686-L691" target="_blank">source</a>]


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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaFBOs.cpp#L719-L732" target="_blank">source</a>]


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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaFBOs.cpp#L733-L748" target="_blank">source</a>]


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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaFBOs.cpp#L818-L829" target="_blank">source</a>]


### gl.GetShaderLog

```lua
function gl.GetShaderLog() -> infoLog string
```





Returns the shader compilation error log. This is empty if the shader linking failed, in that case, check your in/out blocks and ensure they match.

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaShaders.cpp#L198-L202" target="_blank">source</a>]


### gl.CreateShader

```lua
function gl.CreateShader(shaderParams: ShaderParams) -> shaderID integer
```





Create a shader.

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaShaders.cpp#L621-L627" target="_blank">source</a>]


### gl.DeleteShader

```lua
function gl.DeleteShader(shaderID: integer)
```





Deletes a shader identified by shaderID

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaShaders.cpp#L789-L793" target="_blank">source</a>]


### gl.UseShader

```lua
function gl.UseShader(shaderID: integer) -> linked boolean
```





Binds a shader program identified by shaderID. Pass 0 to disable the shader. Returns whether the shader was successfully bound.

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaShaders.cpp#L806-L811" target="_blank">source</a>]


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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaShaders.cpp#L839-L849" target="_blank">source</a>]


### gl.GetActiveUniforms

```lua
function gl.GetActiveUniforms(shaderID: integer) -> activeUniforms ActiveUniform[]
```





Query the active (actually used) uniforms of a shader and identify their
names, types (float, int, uint) and sizes (float, vec4, ...).

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaShaders.cpp#L931-L938" target="_blank">source</a>]


### gl.GetUniformLocation

```lua
function gl.GetUniformLocation(
  shaderID: integer,
  name: string
) -> locationID GL
```





Returns the locationID of a shaders uniform. Needed for changing uniform
values with function `gl.Uniform`.

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaShaders.cpp#L966-L974" target="_blank">source</a>]


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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaShaders.cpp#L1045-L1055" target="_blank">source</a>]


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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaShaders.cpp#L1086-L1096" target="_blank">source</a>]


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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaShaders.cpp#L1156-L1166" target="_blank">source</a>]


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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaShaders.cpp#L1205-L1216" target="_blank">source</a>]


### gl.UniformMatrix

```lua
function gl.UniformMatrix(
  locationID: (number|string),
  matrix: number[]
)
```
@param `locationID` - uniformName

@param `matrix` - A 2x2, 3x3 or 4x4 matrix.






Sets the a uniform mat4 locationID for the currently active shader.

Shader must be activated before setting uniforms.

Can set one one common matrix like shadow, or by passing 16 additional
numbers for the matrix.

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaShaders.cpp#L1218-L1229" target="_blank">source</a>]


### gl.GetEngineUniformBufferDef

```lua
function gl.GetEngineUniformBufferDef(index: number) -> glslDefinition string
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaShaders.cpp#L1306-L1314" target="_blank">source</a>]

Return the GLSL compliant definition of UniformMatricesBuffer(idx=0) or UniformParamsBuffer(idx=1) structure.


### gl.GetEngineModelUniformDataDef

```lua
function gl.GetEngineModelUniformDataDef(index: number) -> glslDefinition string
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaShaders.cpp#L1328-L1336" target="_blank">source</a>]

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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaShaders.cpp#L1346-L1353" target="_blank">source</a>]


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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaShaders.cpp#L1374-L1384" target="_blank">source</a>]


### gl.GetVBO

```lua
function gl.GetVBO(
  bufferType: GLBufferType?,
  freqUpdated: boolean?
) -> VBO VBO?
```
@param `bufferType` - (Default: GL.ARRAY_BUFFER)

Use `GL.ARRAY_BUFFER` for vertex data and
`GL.ELEMENT_ARRAY_BUFFER` for vertex indices.

@param `freqUpdated` - (Default: `true`)

`true` to updated frequently, `false` to update only once.


@return `VBO` - The VBO ref on success, or nil if not supported or an error occurred.





Example:

```lua
local myVBO = gl.GetVBO()
if myVBO == nil then Spring.Echo("Failed to get VBO") end
```

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVBO.cpp#L132-L156" target="_blank">source</a>]
 See: GL.OpenGL_Buffer_Types





