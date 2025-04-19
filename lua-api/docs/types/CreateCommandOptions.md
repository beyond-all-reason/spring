---
layout: default
title: CreateCommandOptions
parent: Lua API
permalink: lua-api/types/CreateCommandOptions
---

{% raw %}

# alias CreateCommandOptions
---



```lua
(alias) CreateCommandOptions = (CommandOptionName[]|table<CommandOptionName,boolean>|CommandOptionBit|integer)
    | CommandOptionName[] -- An array of option names.
    | table<...> -- A map of command names to booleans, considered held when `true`.
    | CommandOptionBit -- A specific integer value for a command option.
    | integer -- A bit mask combination of `CommandOptionBit` values. Pass `0` for no options.

```




[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaUtils.cpp#L999-L1005" target="_blank">source</a>]


{% endraw %}