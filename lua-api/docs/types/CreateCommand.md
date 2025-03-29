---
layout: default
title: CreateCommand
parent: Lua API
permalink: lua-api/types/CreateCommand
---

# class CreateCommand





Used when assigning multiple commands at once.

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaUtils.cpp#L1151-L1159" target="_blank">source</a>]





## fields


### CreateCommand.[1]

```lua
CreateCommand.[1] : (CMD|integer)
```



Command ID.


### CreateCommand.[2]

```lua
CreateCommand.[2] : CreateCommandParams?
```



Parameters for the given command.


### CreateCommand.[3]

```lua
CreateCommand.[3] : CreateCommandOptions?
```



Command options.


### CreateCommand.[4]

```lua
CreateCommand.[4] : integer?
```



Timeout.


