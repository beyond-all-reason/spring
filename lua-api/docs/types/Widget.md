---
layout: default
title: Widget
parent: Lua API
permalink: lua-api/types/Widget
---

# class Widget


- supers: Callins, UnsyncedCallins




Widgets cannot control game logic and receive only unsynced callins.

**Attention:** To prevent complaints from Lua Language Server, e.g.

> ```md
> Duplicate field `CommandNotify` (duplicate-set-field)
> ```

Add this line at the top of your widget script:

```lua
local widget = widget ---@type Widget
```




