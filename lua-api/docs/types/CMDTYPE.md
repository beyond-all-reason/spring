---
layout: default
title: CMDTYPE
parent: Lua API
permalink: lua-api/types/CMDTYPE
---

{% raw %}

# enum CMDTYPE
---



Command type constants

Note, the `CMDTYPE[]` table is bidirectional. That means: `CMDTYPE[CMDTYPE.ICON] := "CMDTYPE_ICON"`

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMDTYPE.cpp#L11-L17" target="_blank">source</a>]


### ICON

```lua
CMDTYPE.ICON = integer
```

Expect 0 parameters in return.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMDTYPE.cpp#L22-L22" target="_blank">source</a>]

### ICON_MODE

```lua
CMDTYPE.ICON_MODE = integer
```

Expect 1 parameter in return (number selected mode).

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMDTYPE.cpp#L24-L24" target="_blank">source</a>]

### ICON_MAP

```lua
CMDTYPE.ICON_MAP = integer
```

Expect 3 parameters in return (mappos).

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMDTYPE.cpp#L26-L26" target="_blank">source</a>]

### ICON_AREA

```lua
CMDTYPE.ICON_AREA = integer
```

Expect 4 parameters in return (mappos+radius).

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMDTYPE.cpp#L28-L28" target="_blank">source</a>]

### ICON_UNIT

```lua
CMDTYPE.ICON_UNIT = integer
```

Expect 1 parameters in return (unitid).

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMDTYPE.cpp#L30-L30" target="_blank">source</a>]

### ICON_UNIT_OR_MAP

```lua
CMDTYPE.ICON_UNIT_OR_MAP = integer
```

Expect 1 parameters in return (unitid) or 3 parameters in return (mappos).

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMDTYPE.cpp#L32-L32" target="_blank">source</a>]

### ICON_FRONT

```lua
CMDTYPE.ICON_FRONT = integer
```

Expect 3 or 6 parameters in return (middle and right side of front if a front was defined).

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMDTYPE.cpp#L34-L34" target="_blank">source</a>]

### ICON_UNIT_OR_AREA

```lua
CMDTYPE.ICON_UNIT_OR_AREA = integer
```

Expect 1 parameter in return (unitid) or 4 parameters in return (mappos+radius).

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMDTYPE.cpp#L36-L36" target="_blank">source</a>]

### NEXT

```lua
CMDTYPE.NEXT = integer
```

Expect command page used with `CMD_INTERNAL`.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMDTYPE.cpp#L38-L38" target="_blank">source</a>]

### PREV

```lua
CMDTYPE.PREV = integer
```

Expect command page used with `CMD_INTERNAL`.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMDTYPE.cpp#L40-L40" target="_blank">source</a>]

### ICON_UNIT_FEATURE_OR_AREA

```lua
CMDTYPE.ICON_UNIT_FEATURE_OR_AREA = integer
```

Expect 1 parameter in return (unitid or Game.maxUnits+featureid) or 4 parameters in return (mappos+radius).

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMDTYPE.cpp#L42-L42" target="_blank">source</a>]

### ICON_BUILDING

```lua
CMDTYPE.ICON_BUILDING = integer
```

Expect 3 parameters in return (mappos).

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMDTYPE.cpp#L44-L44" target="_blank">source</a>]

### CUSTOM

```lua
CMDTYPE.CUSTOM = integer
```

Expect with `CMD_INTERNAL`.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMDTYPE.cpp#L46-L46" target="_blank">source</a>]

### ICON_UNIT_OR_RECTANGLE

```lua
CMDTYPE.ICON_UNIT_OR_RECTANGLE = integer
```

Expect 1 parameter in return (unitid) or 3 parameters in return (mappos) or 6 parameters in return (startpos+endpos).

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMDTYPE.cpp#L48-L48" target="_blank">source</a>]

### NUMBER

```lua
CMDTYPE.NUMBER = integer
```

Expect 1 parameter in return (number).

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaConstCMDTYPE.cpp#L50-L50" target="_blank">source</a>]



{% endraw %}