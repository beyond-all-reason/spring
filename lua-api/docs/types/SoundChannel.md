---
layout: default
title: SoundChannel
parent: Lua API
permalink: lua-api/types/SoundChannel
---

# alias SoundChannel
---



```lua
(alias) SoundChannel = ("general"|"battle"|"sfx"|"unitreply"|"voice"|"userinterface"|"ui"|0|1|2...)
    | "general" -- 0
    | "battle" -- Same as `"sfx" | 1`
    | "sfx" -- Same as `"battle" | 1`
    | "unitreply" -- Same as `"voice" | 2`
    | "voice" -- Same as `"unitreply" | 2`
    | "userinterface" -- Same as "ui" | 3`
    | "ui" -- Same as "userinterface" | 3`
    | 0 -- General
    | 1 -- SFX
    | 2 -- Voice
    | 3 -- User interface

```




[<a href="https://github.com/beyond-all-reason/spring/blob/f0c829c6e793112ab93b1ec454b2e6f2a1767d4c/rts/Lua/LuaUnsyncedCtrl.cpp#L698-L711" target="_blank">source</a>]

