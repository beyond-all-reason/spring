---
layout: default
title: LogLevel
parent: Lua API
permalink: lua-api/types/LogLevel
---

# alias LogLevel
---



```lua
(alias) LogLevel = (integer|"debug"|"info"|"notice"|"warning"|"deprecated"|"error"|"fatal")
    | integer
    | "debug" -- LOG.DEBUG
    | "info" -- LOG.INFO
    | "notice" -- LOG.NOTICE (engine default)
    | "warning" -- LOG.WARNING
    | "deprecated" -- LOG.DEPRECATED
    | "error" -- LOG.ERROR
    | "fatal" -- LOG.FATAL

```




[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaUtils.cpp#L1430-L1440" target="_blank">source</a>]

