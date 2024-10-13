---
layout: post
title: Running changelog
parent: Changelogs
permalink: changelogs/running-changelog
author: sprunk
---

This is the changelog **since version 2590**.

# Caveats
These are the entries which may require special attention when migrating:
* (none so far)

# Features

### Lua
* add `Spring.AllocateTable(arraySlots, hashSlots) -> table`. Returns a table with more space allocated.
Use as a microoptimisation when you have a big table which you are going to populate with a known number of elements, for example `#UnitDefs`.
* `pairs()` now looks at the `__pairs` metamethod in tables, same as in Lua 5.2.

## Fixes
* fix draw position for asymmetric models, they no longer disappear when not appropriate.
