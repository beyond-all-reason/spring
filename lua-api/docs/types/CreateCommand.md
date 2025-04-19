---
layout: default
title: CreateCommand
parent: Lua API
permalink: lua-api/types/CreateCommand
---

{% raw %}

# class CreateCommand





Used when assigning multiple commands at once.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaUtils.cpp#L1151-L1159" target="_blank">source</a>]





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




{% endraw %}