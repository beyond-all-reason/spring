---
layout: default
title: Roster
parent: Lua API
permalink: lua-api/types/Roster
---

# class Roster





Roster

Contains data about a player

[<a href="https://github.com/beyond-all-reason/spring/blob/f0c829c6e793112ab93b1ec454b2e6f2a1767d4c/rts/Lua/LuaUnsyncedRead.cpp#L4135-L4147" target="_blank">source</a>]

---



## fields
---

### Roster.allyTeamID
---
```lua
Roster.allyTeamID : integer
```




### Roster.cpuUsage
---
```lua
Roster.cpuUsage : number
```



in order to find the progress, use: cpuUsage&0x1 if it's PC or BO, cpuUsage& 0xFE to get path res, (cpuUsage>>8)*1000 for the progress


### Roster.name
---
```lua
Roster.name : string
```




### Roster.pingTime
---
```lua
Roster.pingTime : number
```



if -1, the player is pathfinding


### Roster.playerID
---
```lua
Roster.playerID : integer
```




### Roster.spectator
---
```lua
Roster.spectator : boolean
```




### Roster.teamID
---
```lua
Roster.teamID : integer
```




