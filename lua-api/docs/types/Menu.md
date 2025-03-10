---
layout: default
title: Menu
parent: Lua API
permalink: lua-api/types/Menu
---

# class Menu


- supers: Callins




[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaMenu.cpp#L34-L37" target="_blank">source</a>]

---

## methods
---

### Menu.ActivateGame
---
```lua
function Menu.ActivateGame()
```





Called whenever LuaMenu is on with a game loaded.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaMenu.cpp#L393-L396" target="_blank">source</a>]


### Menu.ActivateMenu
---
```lua
function Menu.ActivateMenu()
```





Called whenever LuaMenu is on with no game loaded.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaMenu.cpp#L372-L375" target="_blank">source</a>]


### Menu.AllowDraw
---
```lua
function Menu.AllowDraw() -> allowDraw boolean
```





Enables Draw{Genesis,Screen,ScreenPost} callins if true is returned,
otherwise they are called once every 30 seconds. Only active when a game
isn't running.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaMenu.cpp#L413-L420" target="_blank">source</a>]




