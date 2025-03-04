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
    | "general"  --0
    | "battle"  --Same as `"sfx" | 1`
    | "sfx"  --Same as `"battle" | 1`
    | "unitreply"  --Same as `"voice" | 2`
    | "voice"  --Same as `"unitreply" | 2`
    | "userinterface"  --Same as "ui" | 3`
    | "ui"  --Same as "userinterface" | 3`
    | 0  --General
    | 1  --SFX
    | 2  --Voice
    | 3  --User interface

```




[<a href="https://github.com/beyond-all-reason/spring/blob/f30aea20ae92a148009e573d844c20fa0bb31671/rts/Lua/LuaUnsyncedCtrl.cpp#L697-L710" target="_blank">source</a>]

