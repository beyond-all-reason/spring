---
layout: post
title: Wupget best practices
parent: Articles
permalink: articles/wupget-best-practices
author: sprunk
---


* `Spring.Echo` and similar debugging functions accept multiple args. When echoing variables, use `,` instead of `..` since more things can be printed standalone than natively glued together:

```diff
- Spring.Echo("foo is " .. foo) -- breaks when it is nil, table, function...
+ Spring.Echo("foo", foo) -- prints e.g. "foo, <function>"
```
* for behaviours, use customparams instead of hardcoding unit def names in gadgets.
* when hardcoding unit types for non-behaviour purposes (list of things to spawn in a mission etc.), use def names instead of numerical def IDs. Numerical ID can change between matches.
* localize `UnitDefs` (and similar) accesses if you do it in an event that happens often, and you only want a limited number of traits.
Avoid:

```lua
function wupget:UnitDamaged(unitID, unitDefID, ...) -- common event
  if UnitDefs[unitDefID].health > 123 then
```

Prefer instead:
```lua
local healths = {}
for unitDefID, unitDefID in pairs(UnitDefs) do
  healths[unitDefID] = unitDef.health
end

function wupget:UnitDamaged(unitID, unitDefID, ...)
  if healths[unitDefID] > 123 then
```
