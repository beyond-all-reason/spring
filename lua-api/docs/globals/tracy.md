---
layout: default
title: tracy
parent: Lua API
permalink: lua-api/globals/tracy
---

# global tracy


---

## methods
---

### tracy.LuaTracyPlot
---
```lua
function tracy.LuaTracyPlot(
  plotName: string,
  plotValue: number
)
```
@param `plotName` - Which LuaPlot should be updated

@param `plotValue` - the number to show on the Tracy plot






Update a Tracy plot with a value

[<a href="https://github.com/beyond-all-reason/spring/blob/f30aea20ae92a148009e573d844c20fa0bb31671/rts/Lua/LuaTracyExtra.cpp#L67-L72" target="_blank">source</a>]


### tracy.LuaTracyPlotConfig
---
```lua
function tracy.LuaTracyPlotConfig(
  plotName: string,
  plotFormatType: ("Number"|"Percentage"|"Memory"|nil),
  stepwise: boolean?,
  fill: boolean?,
  color: integer?
)
```
@param `plotName` - name of the plot to customize

@param `plotFormatType` - (Default: `"Number"`)

@param `stepwise` - (Default: `true`) stepwise chart

@param `fill` - (Default: `false`) whether to fill color

@param `color` - (Default: `0xFFFFFF`) uint32 number as BGR color






Configure custom appearance for a Tracy plot for use in debugging or profiling

[<a href="https://github.com/beyond-all-reason/spring/blob/f30aea20ae92a148009e573d844c20fa0bb31671/rts/Lua/LuaTracyExtra.cpp#L37-L45" target="_blank">source</a>]




