---
layout: default
title: Lua scripting
parent: Articles
permalink: guides/lua-scripting
---

## Synced vs Unsynced

When developing a Recoil game, the distinction between synced and unsynced
becomes an important topic. In this article we will discuss what these terms
mean and how they are linked to Lua scripting.

### Synced

Anything that must be relayed to other players and synchronized.
This is not accessible by the user side, think of game logic: mechanics and
behavior.

There's no centralized server for source of truth for the game state, although
usually the host is deemed to be the one. The simulation runs on all clients at
the same time, every once and a while checked for synchronization between each
game states via checksum.

### Unsynced

Everything not synced is allowed from the user scope.

Most often user interface code. Even though it allows a certain level of
automation that might blur the lines at times, there's limited access to the
game state from unsynced code.

## Callouts and Callins

Callouts and Callins are Recoil terminology for, respectively, methods exposed
by the engine and methods invoked by the engine on the Lua context they pertain
to (Synced/Unsynced).

## Lua script entrypoints

Whenever a game is initialized by Recoil, it will search and execute the Lua
entrypoints for each context, these are:

| Context  | Entrypoints                                                 | Description |
|:---------|:------------------------------------------------------------|:------------|
| LuaUI    | *Unsynced* `LuaUI/main.lua`                                 |   |
| LuaRules | *Synced* `LuaRules/main.lua` *Unsynced* `LuaRules/draw.lua` |   |
| LuaGaia  | *Synced* `LuaGaia/main.lua` *Unsynced* `LuaGaia/draw.lua`   |   |
| LuaIntro | `LuaIntro/main.lua`                                         |   |
| LuaMenu  | `LuaMenu/main.lua`                                          |   |
