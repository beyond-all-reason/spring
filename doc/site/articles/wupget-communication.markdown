---
layout: post
title: Wupget communication
parent: Articles
permalink: articles/wupget-communication
author: sprunk
---

There are multiple Lua environments (see [old wiki](https://springrts.com/wiki/Lua:Environments)), and within an environment each wupget is also separated to some degree. How do these communicate?

### WG and GG

* the basic way to communicate - just tables putting globals inside, visible from wupgets in given environment.
* are just regular tables exposed by the wupget handlers by convention.
* in LuaUI it's called `WG` (widget global), elsewhere it's `GG` (gadget global)
* keep in mind that the `GG` in unsynced LuaRules, the `GG` in synced LuaRules, and the `GG` in synced LuaGaia are all separate tables despite the same name! For communication between environments you'll have to use one of the methods below.

### Rules params

* synced LuaRules can set global key/value pairs as well as attach them to units, teams, players; all other environments can then read them
* the standard way to expose new gameplay-relevant traits.
* see also the article.
* examples: "unit is on fire", "unit is slowed by 30%", "team 2 is cursed", "next night falls at 6:00", "there are 7 mex spots on the map at coordinates ..." .

### `Script.LuaXYZ` events and "proper" `_G` globals

* `_G` is the "real" global table of each environment (as opposed to `GG` and `WG` which are only "logically" global),
i.e. it is the one accessible directly by the engine to be called from the outside of that environment (in particular, it is still per-environment)
* wupget handlers also expose `wupgetHandler:RegisterGlobal('EventName', func)` which is essentially `_G.EventName = func` with some basic safety on top
* you can then do `Script.LuaXYZ.EventName(args)` to call the given function
* the two LuaRules environments can call `Script.LuaRules` to refer to themselves (but not each other!) and `Script.LuaGaia` (also only of the same syncedness), while unsynced LuaRules/LuaGaia and LuaUI can call `Script.LuaUI`
* this only works if the global in question is a function, and you won't receive any return values
* you could put "normal" vars there, but it is discouraged (use `GG`/`WG`)
* the main use case for this is calling events "native style" where you do `Script.LuaXYZ.UnitDamaged` which is then handled by the wupget handler and distributed to individual wupgets
* it is safe to call functions this way even if there is nothing on the other side.
You can use the function notation to check whether anything is linked though, e.g. for optimisation:
```lua
if Script.LuaXYZ("Bla") then
  local args = ExpensiveCalculation()
  Script.LuaXYZ.Bla(args)
end
```
* other examples of events you could add: nuclear launch detected, language changed in settings, metal spot added at runtime.

### `SendToUnsynced` and `RecvFromSynced`
* synced gadgets can pass data to unsynced gadgets by calling `SendToUnsynced(args)`
* unsynced gadgets receive it via `function gadget:RecvFromSynced(args)`
* usually the first arg is a magic value that lets the correct gadget consume the message
* in unsynced, you can do `gadgetHandler:AddSyncAction("foobar", func)` which is generally equivalent to
```lua
function gadget:RecvFromSynced(magic, args)
  if magic == "foobar" then
    func(magic, args)
  end
end
```
* laundering via unsynced LuaRules is the way to call `Script.LuaUI` style events from synced LuaRules.

### `Spring.SendLuaRulesMsg` and `gadget:RecvLuaMsg`
* LuaUI can send messages to LuaRules
* similar to `SendToUnsynced` in use
* there's also `Spring.SendLuaGaiaMsg`
* use to make UI to interact with mechanics where unit orders won't suffice

### `Spring.SendLuaUIMsg` and `widget:RecvLuaMsg`
* use these one to communicate with other players' widgets
* lets you send to everyone, only allies, or only spectators
* use for things like broadcasting cursor or selection, or UI interaction like custom markers

### `SYNCED` proxy
* synced LuaRules' global table (`_G`) can also be accessed from unsynced LuaRules
* synced `_G.foo` can be accessed from unsynced as `SYNCED.foo`
* note that this makes a copy on each access, so is very slow and will not reflect changes. Cache it, but remember to refresh
* it only copies basic types (tables, strings, numbers, bools). No functions, metatables and whatnot
* generally not recommended. Listen to the same events as synced and build the table in parallel

### With the outside, unsynced
* there's `/luaui foo` for a player to pass data to LuaUI.
* LuaUI and LuaMenu have a socket API to communicate with the outside.

### With the outside, synced
* arbitrary setup data can be passed in the startscript as modoptions and player/team/AI custom keys. This would be done by the lobby or autohost.
* players, incl. the autohost, can use `/luarules foo` commands to send arbitrary data to the synced state. Lua sees the autohost as playerID 255.
* if the autohost uses [a dedicated instance (as opposed to headless)]({{ site.baseurl }}{% link guides/headless-and-dedi.markdown %}), players can send messages back to the autohost via whispers (`/wByNum 255 foo`) for non-sensitive data.

### Others
* a game can designate some other regular table for globals. For example many games put useful functions in `Spring.Utilities` which is not actually a native part of the `Spring` table.
* two environments could include the same file to get the same data (in contents but not identity). LuaUI and LuaMenu can also read arbitrary files (outside the VFS).
