---
layout: default
title: CMDTYPE
parent: Lua API
permalink: lua-api/types/CMDTYPE
---

# enum CMDTYPE
---



Note, the CMDTYPE[] table is bidirectional. That means: CMDTYPE[CMDTYPE.ICON] := "CMDTYPE_ICON"

[<a href="https://github.com/beyond-all-reason/spring/blob/f30aea20ae92a148009e573d844c20fa0bb31671/rts/Lua/LuaConstCMDTYPE.cpp#L17-L36" target="_blank">source</a>]


```cpp
enum CMDTYPE {
    COMBO_BOX = number, 
    CUSTOM = number, 
    ICON = number, 
    ICON_AREA = number, 
    ICON_BUILDING = number, 
    ICON_FRONT = number, 
    ICON_MAP = number, 
    ICON_MODE = number, 
    ICON_UNIT = number, 
    ICON_UNIT_FEATURE_OR_AREA = number, 
    ICON_UNIT_OR_AREA = number, 
    ICON_UNIT_OR_MAP = number, 
    ICON_UNIT_OR_RECTANGLE = number, 
    NEXT = number, 
    NUMBER = number, 
    PREV = number, 
    
}
```
