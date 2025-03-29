---
layout: default
title: SendToUnsynced
parent: Lua API
permalink: lua-api/globals/SendToUnsynced
---

# global SendToUnsynced


```lua
function SendToUnsynced(...: (nil|boolean|number|string))
```
@param `...` - Arguments. Typically the first argument is the name of a function to call.







Invoke `UnsyncedCallins:RecvFromSynced` callin with the given arguments.

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaHandleSynced.cpp#L1961-L1967" target="_blank">source</a>]
 See: UnsyncedCallins:RecvFromSynced


