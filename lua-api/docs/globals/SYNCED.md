---
layout: default
title: SYNCED
parent: Lua API
permalink: lua-api/globals/SYNCED
---

{% raw %}

# global SYNCED


```lua
SYNCED : table<string,any>
```




Proxy table for reading synced global state in unsynced code.

**Generally not recommended.** Instead, listen to the same events as synced
and build the table in parallel

Unsynced code can read from the synced global table (`_G`) using the `SYNCED`
proxy table. e.g. `_G.foo` can be access from unsynced via `SYNCED.foo`.

This table makes *a copy* of the object on the other side, and only copies
numbers, strings, bools and tables (recursively but with the type
restriction), in particular this does not allow access to functions.

Note that this makes a copy on each access, so is very slow and will not
reflect changes. Cache it, but remember to refresh.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaSyncedTable.cpp#L73-L91" target="_blank">source</a>]


{% endraw %}