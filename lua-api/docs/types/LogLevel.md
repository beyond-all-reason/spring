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
    | "debug"  --LOG.DEBUG
    | "info"  --LOG.INFO
    | "notice"  --LOG.NOTICE (engine default)
    | "warning"  --LOG.WARNING
    | "deprecated"  --LOG.DEPRECATED
    | "error"  --LOG.ERROR
    | "fatal"  --LOG.FATAL

```




[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L487-L497" target="_blank">source</a>]

