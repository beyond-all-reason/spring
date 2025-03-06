---
layout: default
title: SetWMCaption
parent: Lua API
permalink: lua-api/globals/SetWMCaption
---

# global SetWMCaption

---

```lua
function SetWMCaption(
  title: string,
  titleShort: string?
) ->  nil
```
@param `titleShort` - (Default: title)







Sets the window title for the process (default: "Spring <version>").

The shortTitle is displayed in the OS task-bar (default: "Spring <version>").

NOTE: shortTitle is only ever possibly used under X11 (Linux & OS X), but not with QT (KDE) and never under Windows.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L5200-L5211" target="_blank">source</a>]

