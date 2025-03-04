---
layout: default
title: losAccess
parent: Lua API
permalink: lua-api/types/losAccess
---

# class losAccess





Parameters for los access


If one condition is fulfilled all beneath it are too (e.g. if an unit is in
LOS it can read params with `inradar=true` even if the param has
`inlos=false`) All GameRulesParam are public, TeamRulesParams can just be
`private`,`allied` and/or `public` You can read RulesParams from any Lua
environments! With those losAccess policies you can limit their access.

All GameRulesParam are public, TeamRulesParams can just be `private`,`allied` and/or `public`
You can read RulesParams from any Lua environments! With those losAccess policies you can limit their access.

[<a href="https://github.com/beyond-all-reason/spring/blob/f30aea20ae92a148009e573d844c20fa0bb31671/rts/Lua/LuaSyncedCtrl.cpp#L1328-L1348" target="_blank">source</a>]

---



## fields
---

### losAccess.allied
---
```lua
losAccess.allied : boolean?
```



readable by ally + ingame allied


### losAccess.inlos
---
```lua
losAccess.inlos : boolean?
```



readable if the unit is in LOS


### losAccess.inradar
---
```lua
losAccess.inradar : boolean?
```



readable if the unit is in AirLOS


### losAccess.private
---
```lua
losAccess.private : boolean?
```



only readable by the ally (default)


### losAccess.public
---
```lua
losAccess.public : boolean?
```



readable by all


