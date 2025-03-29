---
layout: default
title: Script
parent: Lua API
permalink: lua-api/globals/Script
---

# global Script




## methods


### Script.Kill

```lua
function Script.Kill(killMessage: string?)
```
@param `killMessage` - Kill message.






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaHandle.cpp#L214-L217" target="_blank">source</a>]


### Script.UpdateCallin

```lua
function Script.UpdateCallin(name: string)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaHandle.cpp#L641-L644" target="_blank">source</a>]


### Script.GetName

```lua
function Script.GetName() -> name string
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaHandle.cpp#L3969-L3972" target="_blank">source</a>]


### Script.GetSynced

```lua
function Script.GetSynced() -> synced boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaHandle.cpp#L3980-L3983" target="_blank">source</a>]


### Script.GetFullCtrl

```lua
function Script.GetFullCtrl() -> fullCtrl boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaHandle.cpp#L3991-L3994" target="_blank">source</a>]


### Script.GetFullRead

```lua
function Script.GetFullRead() -> fullRead boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaHandle.cpp#L4002-L4005" target="_blank">source</a>]


### Script.GetCtrlTeam

```lua
function Script.GetCtrlTeam() -> teamID integer
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaHandle.cpp#L4013-L4016" target="_blank">source</a>]


### Script.GetReadTeam

```lua
function Script.GetReadTeam() -> teamID integer
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaHandle.cpp#L4024-L4027" target="_blank">source</a>]


### Script.GetReadAllyTeam

```lua
function Script.GetReadAllyTeam() -> allyTeamID integer
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaHandle.cpp#L4035-L4038" target="_blank">source</a>]


### Script.GetSelectTeam

```lua
function Script.GetSelectTeam() -> teamID integer
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaHandle.cpp#L4046-L4049" target="_blank">source</a>]


### Script.GetGlobal

```lua
function Script.GetGlobal() -> global integer?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaHandle.cpp#L4057-L4060" target="_blank">source</a>]


### Script.GetRegistry

```lua
function Script.GetRegistry() -> registry integer?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaHandle.cpp#L4071-L4074" target="_blank">source</a>]


### Script.DelayByFrames

```lua
function Script.DelayByFrames(
  frameDelay: integer,
  fun: unknown
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaHandle.cpp#L4091-L4095" target="_blank">source</a>]


### Script.IsEngineMinVersion

```lua
function Script.IsEngineMinVersion(
  minMajorVer: integer,
  minMinorVer: integer?,
  minCommits: integer?
) -> satisfiesMin boolean
```
@param `minMinorVer` - (Default: `0`)

@param `minCommits` - (Default: `0`)


@return `satisfiesMin` - `true` if the engine version is greater or equal to the specified version, otherwise `false`.





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaUtils.cpp#L527-L533" target="_blank">source</a>]





## fields


### Script.NO_ACCESS_TEAM

```lua
Script.NO_ACCESS_TEAM: integer = -1
```



[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaHandle.cpp#L3948-L3948" target="_blank">source</a>]


### Script.ALL_ACCESS_TEAM

```lua
Script.ALL_ACCESS_TEAM: integer = -2
```



[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaHandle.cpp#L3950-L3950" target="_blank">source</a>]


