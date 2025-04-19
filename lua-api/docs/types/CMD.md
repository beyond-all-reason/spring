---
layout: default
title: CMD
parent: Lua API
permalink: lua-api/types/CMD
---

{% raw %}

# enum CMD
---



Command constants.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L12-L15" target="_blank">source</a>]


### OPT_ALT

```lua
CMD.OPT_ALT = 128
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L19-L19" target="_blank">source</a>]

### OPT_CTRL

```lua
CMD.OPT_CTRL = 64
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L21-L21" target="_blank">source</a>]

### OPT_SHIFT

```lua
CMD.OPT_SHIFT = 32
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L23-L23" target="_blank">source</a>]

### OPT_RIGHT

```lua
CMD.OPT_RIGHT = 16
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L25-L25" target="_blank">source</a>]

### OPT_INTERNAL

```lua
CMD.OPT_INTERNAL = 8
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L27-L27" target="_blank">source</a>]

### OPT_META

```lua
CMD.OPT_META = 4
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L29-L29" target="_blank">source</a>]

### MOVESTATE_NONE

```lua
CMD.MOVESTATE_NONE = -1
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L32-L32" target="_blank">source</a>]

### MOVESTATE_HOLDPOS

```lua
CMD.MOVESTATE_HOLDPOS = 0
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L34-L34" target="_blank">source</a>]

### MOVESTATE_MANEUVER

```lua
CMD.MOVESTATE_MANEUVER = 1
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L36-L36" target="_blank">source</a>]

### MOVESTATE_ROAM

```lua
CMD.MOVESTATE_ROAM = 2
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L38-L38" target="_blank">source</a>]

### FIRESTATE_NONE

```lua
CMD.FIRESTATE_NONE = -1
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L41-L41" target="_blank">source</a>]

### FIRESTATE_HOLDFIRE

```lua
CMD.FIRESTATE_HOLDFIRE = 0
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L43-L43" target="_blank">source</a>]

### FIRESTATE_RETURNFIRE

```lua
CMD.FIRESTATE_RETURNFIRE = 1
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L45-L45" target="_blank">source</a>]

### FIRESTATE_FIREATWILL

```lua
CMD.FIRESTATE_FIREATWILL = 2
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L47-L47" target="_blank">source</a>]

### FIRESTATE_FIREATNEUTRAL

```lua
CMD.FIRESTATE_FIREATNEUTRAL = 3
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L49-L49" target="_blank">source</a>]

### WAITCODE_TIME

```lua
CMD.WAITCODE_TIME = 1
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L52-L52" target="_blank">source</a>]

### WAITCODE_DEATH

```lua
CMD.WAITCODE_DEATH = 2
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L54-L54" target="_blank">source</a>]

### WAITCODE_SQUAD

```lua
CMD.WAITCODE_SQUAD = 3
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L56-L56" target="_blank">source</a>]

### WAITCODE_GATHER

```lua
CMD.WAITCODE_GATHER = 4
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L58-L58" target="_blank">source</a>]

### STOP

```lua
CMD.STOP = 0
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L63-L63" target="_blank">source</a>]

### INSERT

```lua
CMD.INSERT = 1
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L65-L65" target="_blank">source</a>]

### REMOVE

```lua
CMD.REMOVE = 2
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L67-L67" target="_blank">source</a>]

### WAIT

```lua
CMD.WAIT = 5
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L69-L69" target="_blank">source</a>]

### TIMEWAIT

```lua
CMD.TIMEWAIT = 6
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L71-L71" target="_blank">source</a>]

### DEATHWAIT

```lua
CMD.DEATHWAIT = 7
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L73-L73" target="_blank">source</a>]

### SQUADWAIT

```lua
CMD.SQUADWAIT = 8
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L75-L75" target="_blank">source</a>]

### GATHERWAIT

```lua
CMD.GATHERWAIT = 9
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L77-L77" target="_blank">source</a>]

### MOVE

```lua
CMD.MOVE = 10
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L79-L79" target="_blank">source</a>]

### PATROL

```lua
CMD.PATROL = 15
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L81-L81" target="_blank">source</a>]

### FIGHT

```lua
CMD.FIGHT = 16
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L83-L83" target="_blank">source</a>]

### ATTACK

```lua
CMD.ATTACK = 20
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L85-L85" target="_blank">source</a>]

### AREA_ATTACK

```lua
CMD.AREA_ATTACK = 21
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L87-L87" target="_blank">source</a>]

### GUARD

```lua
CMD.GUARD = 25
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L89-L89" target="_blank">source</a>]

### GROUPSELECT

```lua
CMD.GROUPSELECT = 35
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L91-L91" target="_blank">source</a>]

### GROUPADD

```lua
CMD.GROUPADD = 36
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L93-L93" target="_blank">source</a>]

### GROUPCLEAR

```lua
CMD.GROUPCLEAR = 37
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L95-L95" target="_blank">source</a>]

### REPAIR

```lua
CMD.REPAIR = 40
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L97-L97" target="_blank">source</a>]

### FIRE_STATE

```lua
CMD.FIRE_STATE = 45
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L99-L99" target="_blank">source</a>]

### MOVE_STATE

```lua
CMD.MOVE_STATE = 50
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L101-L101" target="_blank">source</a>]

### SETBASE

```lua
CMD.SETBASE = 55
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L103-L103" target="_blank">source</a>]

### INTERNAL

```lua
CMD.INTERNAL = 60
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L105-L105" target="_blank">source</a>]

### SELFD

```lua
CMD.SELFD = 65
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L107-L107" target="_blank">source</a>]

### LOAD_UNITS

```lua
CMD.LOAD_UNITS = 75
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L109-L109" target="_blank">source</a>]

### LOAD_ONTO

```lua
CMD.LOAD_ONTO = 76
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L111-L111" target="_blank">source</a>]

### UNLOAD_UNITS

```lua
CMD.UNLOAD_UNITS = 80
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L113-L113" target="_blank">source</a>]

### UNLOAD_UNIT

```lua
CMD.UNLOAD_UNIT = 81
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L115-L115" target="_blank">source</a>]

### ONOFF

```lua
CMD.ONOFF = 85
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L117-L117" target="_blank">source</a>]

### RECLAIM

```lua
CMD.RECLAIM = 90
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L119-L119" target="_blank">source</a>]

### CLOAK

```lua
CMD.CLOAK = 95
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L121-L121" target="_blank">source</a>]

### STOCKPILE

```lua
CMD.STOCKPILE = 100
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L123-L123" target="_blank">source</a>]

### MANUALFIRE

```lua
CMD.MANUALFIRE = 105
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L125-L125" target="_blank">source</a>]

### DGUN

```lua
CMD.DGUN = 105
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L127-L127" target="_blank">source</a>]

### RESTORE

```lua
CMD.RESTORE = 110
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L129-L129" target="_blank">source</a>]

### REPEAT

```lua
CMD.REPEAT = 115
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L131-L131" target="_blank">source</a>]

### TRAJECTORY

```lua
CMD.TRAJECTORY = 120
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L133-L133" target="_blank">source</a>]

### RESURRECT

```lua
CMD.RESURRECT = 125
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L135-L135" target="_blank">source</a>]

### CAPTURE

```lua
CMD.CAPTURE = 130
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L137-L137" target="_blank">source</a>]

### AUTOREPAIRLEVEL

```lua
CMD.AUTOREPAIRLEVEL = 135
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L139-L139" target="_blank">source</a>]

### LOOPBACKATTACK

```lua
CMD.LOOPBACKATTACK = 20
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L141-L141" target="_blank">source</a>]

### IDLEMODE

```lua
CMD.IDLEMODE = 145
```

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMD.cpp#L143-L143" target="_blank">source</a>]



{% endraw %}