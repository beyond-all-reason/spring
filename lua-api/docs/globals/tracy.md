---
layout: default
title: tracy
parent: Lua API
permalink: lua-api/globals/tracy
---

{% raw %}

# global tracy




## methods


### tracy.ZoneBegin

```lua
function tracy.ZoneBegin(name: string)
```






### tracy.ZoneBeginN

```lua
function tracy.ZoneBeginN(name: string)
```






### tracy.ZoneBeginS

```lua
function tracy.ZoneBeginS(name: string)
```






### tracy.ZoneBeginNS

```lua
function tracy.ZoneBeginNS(name: string)
```






### tracy.ZoneEnd

```lua
function tracy.ZoneEnd()
```






### tracy.ZoneText

```lua
function tracy.ZoneText(text: string)
```






### tracy.ZoneName

```lua
function tracy.ZoneName(name: string)
```






### tracy.Message

```lua
function tracy.Message(text: string)
```






### tracy.LuaTracyPlotConfig

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

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaTracyExtra.cpp#L32-L40" target="_blank">source</a>]


### tracy.LuaTracyPlot

```lua
function tracy.LuaTracyPlot(
  plotName: string,
  plotValue: number
)
```
@param `plotName` - Which LuaPlot should be updated

@param `plotValue` - the number to show on the Tracy plot






Update a Tracy plot with a value

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaTracyExtra.cpp#L62-L67" target="_blank">source</a>]






{% endraw %}