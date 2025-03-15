---
layout: default
title: Gadget
parent: Lua API
permalink: lua-api/types/Gadget
---

# class Gadget


- supers: Callins, UnsyncedCallins, SyncedCallins




Gadgets can control game logic and receive synced and unsynced callins.

**Attention:** Callins from `SyncedCallins` will only work on the unsynced
portion of the gadget.

**Attention:** To prevent complaints from Lua Language Server, e.g.

> ```md
> Duplicate field `CommandNotify` (duplicate-set-field)
> ```

Add this line at the top of your gadget script:

```lua
local gadget = gadget ---@type Gadget
```

---


