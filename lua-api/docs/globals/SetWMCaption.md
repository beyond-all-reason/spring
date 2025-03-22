---
layout: default
title: SetWMCaption
parent: Lua API
permalink: lua-api/globals/SetWMCaption
---

# global SetWMCaption


```lua
function SetWMCaption(
  title: string,
  titleShort: string?
) ->  nil
```
@param `titleShort` - (Default: title)







Sets the window title for the process (default: "Spring <version>").

[<a href="https://github.com/beyond-all-reason/spring/blob/4d8d14a2d6cb762d118e3e50f41c850673f13ca9/rts/Lua/LuaUnsyncedCtrl.cpp#L5233-L5244" target="_blank">source</a>]

The shortTitle is displayed in the OS task-bar (default: "Spring <version>").

NOTE: shortTitle is only ever possibly used under X11 (Linux & OS X), but not with QT (KDE) and never under Windows.

