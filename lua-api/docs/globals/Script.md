---
layout: default
title: Script
parent: Lua API
permalink: lua-api/globals/Script
---

{% raw %}

# global Script




## methods


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





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaUtils.cpp#L527-L533" target="_blank">source</a>]


### Script.Kill

```lua
function Script.Kill(killMessage: string?)
```
@param `killMessage` - Kill message.






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaHandle.cpp#L214-L217" target="_blank">source</a>]


### Script.UpdateCallin

```lua
function Script.UpdateCallin(name: string)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaHandle.cpp#L641-L644" target="_blank">source</a>]


### Script.GetName

```lua
function Script.GetName() -> name string
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaHandle.cpp#L4000-L4003" target="_blank">source</a>]


### Script.GetSynced

```lua
function Script.GetSynced() -> synced boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaHandle.cpp#L4011-L4014" target="_blank">source</a>]


### Script.GetFullCtrl

```lua
function Script.GetFullCtrl() -> fullCtrl boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaHandle.cpp#L4022-L4025" target="_blank">source</a>]


### Script.GetFullRead

```lua
function Script.GetFullRead() -> fullRead boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaHandle.cpp#L4033-L4036" target="_blank">source</a>]


### Script.GetCtrlTeam

```lua
function Script.GetCtrlTeam() -> teamID integer
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaHandle.cpp#L4044-L4047" target="_blank">source</a>]


### Script.GetReadTeam

```lua
function Script.GetReadTeam() -> teamID integer
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaHandle.cpp#L4055-L4058" target="_blank">source</a>]


### Script.GetReadAllyTeam

```lua
function Script.GetReadAllyTeam() -> allyTeamID integer
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaHandle.cpp#L4066-L4069" target="_blank">source</a>]


### Script.GetSelectTeam

```lua
function Script.GetSelectTeam() -> teamID integer
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaHandle.cpp#L4077-L4080" target="_blank">source</a>]


### Script.GetGlobal

```lua
function Script.GetGlobal() -> global integer?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaHandle.cpp#L4088-L4091" target="_blank">source</a>]


### Script.GetRegistry

```lua
function Script.GetRegistry() -> registry integer?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaHandle.cpp#L4102-L4105" target="_blank">source</a>]


### Script.DelayByFrames

```lua
function Script.DelayByFrames(
  frameDelay: integer,
  fun: unknown
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaHandle.cpp#L4122-L4126" target="_blank">source</a>]





## fields


### Script.NO_ACCESS_TEAM

```lua
Script.NO_ACCESS_TEAM: integer = -1
```



[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaHandle.cpp#L3979-L3979" target="_blank">source</a>]


### Script.ALL_ACCESS_TEAM

```lua
Script.ALL_ACCESS_TEAM: integer = -2
```



[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaHandle.cpp#L3981-L3981" target="_blank">source</a>]




{% endraw %}