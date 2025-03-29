---
layout: default
title: CreateCommandOptions
parent: Lua API
permalink: lua-api/types/CreateCommandOptions
---

# alias CreateCommandOptions
---



```lua
(alias) CreateCommandOptions = (CommandOptionName[]|table<CommandOptionName,boolean>|CommandOptionBit|integer)
    | CommandOptionName[] -- An array of option names.
    | table<...> -- A map of command names to booleans, considered held when `true`.
    | CommandOptionBit -- A specific integer value for a command option.
    | integer -- A bit mask combination of `CommandOptionBit` values. Pass `0` for no options.

```




[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaUtils.cpp#L999-L1005" target="_blank">source</a>]

