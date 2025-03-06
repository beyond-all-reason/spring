---
layout: default
title: Spring
parent: Lua API
permalink: lua-api/globals/Spring
---

# global Spring


---

## methods
---

### Spring.AddGrass
---
```lua
function Spring.AddGrass(
  x: number,
  z: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L4285-L4290" target="_blank">source</a>]


### Spring.AddHeightMap
---
```lua
function Spring.AddHeightMap(
  x: number,
  z: number,
  height: number
) -> newHeight integer?
```





Can only be called in `Spring.SetHeightMapFunc`

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L5832-L5839" target="_blank">source</a>]


### Spring.AddLightTrackingTarget
---
```lua
function Spring.AddLightTrackingTarget()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L1800-L1802" target="_blank">source</a>]


### Spring.AddMapLight
---
```lua
function Spring.AddMapLight(lightParams: LightParams) -> lightHandle integer
```





requires MaxDynamicMapLights > 0

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L1704-L1711" target="_blank">source</a>]


### Spring.AddModelLight
---
```lua
function Spring.AddModelLight(lightParams: LightParams) -> lightHandle number
```





requires MaxDynamicMapLights > 0

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L1730-L1737" target="_blank">source</a>]


### Spring.AddObjectDecal
---
```lua
function Spring.AddObjectDecal(unitID: integer) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L4244-L4249" target="_blank">source</a>]


### Spring.AddOriginalHeightMap
---
```lua
function Spring.AddOriginalHeightMap(
  x: number,
  y: number,
  height: number
) ->  nil
```





Can only be called in `Spring.SetOriginalHeightMapFunc`

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L6108-L6118" target="_blank">source</a>]


### Spring.AddSmoothMesh
---
```lua
function Spring.AddSmoothMesh(
  x: number,
  z: number,
  height: number
) -> The number?
```

@return `The` - new height, or `nil` if coordinates are invalid.





Can only be called in `Spring.SetSmoothMeshFunc`.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L6363-L6370" target="_blank">source</a>]


### Spring.AddTeamResource
---
```lua
function Spring.AddTeamResource(
  teamID: integer,
  type: ResourceName,
  amount: number
) ->  nil
```





Adds metal or energy resources to the specified team.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L1034-L1041" target="_blank">source</a>]


### Spring.AddUnitDamage
---
```lua
function Spring.AddUnitDamage(
  unitID: integer,
  damage: number,
  paralyze: number?,
  attackerID: integer?,
  weaponID: integer?,
  impulseX: number?,
  impulseY: number?,
  impulseZ: number?
) ->  nil
```
@param `paralyze` - (Default: 0) equals to the paralyzetime in the WeaponDef.

@param `attackerID` - (Default: -1)

@param `weaponID` - (Default: -1)






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L4039-L4051" target="_blank">source</a>]


### Spring.AddUnitExperience
---
```lua
function Spring.AddUnitExperience(
  unitID: integer,
  deltaExperience: number
) ->  nil
```
@param `deltaExperience` - Can be negative to subtract, but the unit will never have negative total afterwards






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L2559-L2566" target="_blank">source</a>]


### Spring.AddUnitIcon
---
```lua
function Spring.AddUnitIcon(
  iconName: string,
  texFile: string,
  size: number?,
  dist: number?,
  radAdjust: number?
) -> added boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L2396-L2407" target="_blank">source</a>]


### Spring.AddUnitImpulse
---
```lua
function Spring.AddUnitImpulse(
  unitID: integer,
  x: number,
  y: number,
  z: number,
  decayRate: number?
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L4091-L4099" target="_blank">source</a>]


### Spring.AddUnitResource
---
```lua
function Spring.AddUnitResource(
  unitID: integer,
  resource: string,
  amount: number
) ->  nil
```
@param `resource` - "m" | "e"






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L4136-L4142" target="_blank">source</a>]


### Spring.AddUnitSeismicPing
---
```lua
function Spring.AddUnitSeismicPing(
  unitID: integer,
  pindSize: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L4116-L4121" target="_blank">source</a>]


### Spring.AddWorldIcon
---
```lua
function Spring.AddWorldIcon(
  cmdID: integer,
  posX: number,
  posY: number,
  posZ: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L974-L982" target="_blank">source</a>]


### Spring.AddWorldText
---
```lua
function Spring.AddWorldText(
  text: string,
  posX: number,
  posY: number,
  posZ: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L994-L1002" target="_blank">source</a>]


### Spring.AddWorldUnit
---
```lua
function Spring.AddWorldUnit(
  unitDefID: integer,
  posX: number,
  posY: number,
  posZ: number,
  teamID: integer,
  facing: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L1014-L1024" target="_blank">source</a>]


### Spring.AdjustHeightMap
---
```lua
function Spring.AdjustHeightMap(
  x1: number,
  y1: number,
  x2_height: number,
  y2: number?,
  height: number?
) ->  nil
```
@param `x2_height` - if y2 and height are nil then this parameter is the height






Add a certain height to a point or rectangle area on the world

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L5752-L5761" target="_blank">source</a>]


### Spring.AdjustOriginalHeightMap
---
```lua
function Spring.AdjustOriginalHeightMap(
  x1: number,
  y1: number,
  x2_height: number,
  y2: number?,
  height: number?
) ->  nil
```
@param `x2_height` - if y2 and height are nil then this parameter is the height






Add height to a point or rectangle area to the original map height cache

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L6030-L6039" target="_blank">source</a>]


### Spring.AdjustSmoothMesh
---
```lua
function Spring.AdjustSmoothMesh(
  x1: number,
  z1: number,
  x2: number?,
  z2: number?,
  height: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L6295-L6303" target="_blank">source</a>]


### Spring.AreHelperAIsEnabled
---
```lua
function Spring.AreHelperAIsEnabled() -> enabled boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L839-L844" target="_blank">source</a>]


### Spring.ArePlayersAllied
---
```lua
function Spring.ArePlayersAllied(
  playerID1: number,
  playerID2: number
) ->  (nil|boolean)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L2360-L2366" target="_blank">source</a>]


### Spring.AreTeamsAllied
---
```lua
function Spring.AreTeamsAllied(
  teamID1: number,
  teamID2: number
) ->  (nil|boolean)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L2340-L2346" target="_blank">source</a>]


### Spring.AssignMouseCursor
---
```lua
function Spring.AssignMouseCursor(
  cmdName: string,
  iconFileName: string,
  overwrite: boolean?,
  hotSpotTopLeft: boolean?
) -> assigned boolean?
```
@param `iconFileName` - not the full filename, instead it is like this:
Wanted filename: Anims/cursorattack_0.bmp
=> iconFileName: cursorattack

@param `overwrite` - (Default: true)

@param `hotSpotTopLeft` - (Default: false)






Changes/creates the cursor of a single CursorCmd.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L2850-L2861" target="_blank">source</a>]


### Spring.AssignPlayerToTeam
---
```lua
function Spring.AssignPlayerToTeam(
  playerID: integer,
  teamID: integer
) ->  nil
```





Assigns a player to a team.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L878-L884" target="_blank">source</a>]


### Spring.BuggerOff
---
```lua
function Spring.BuggerOff(
  x: number,
  y: number,
  z: number?,
  radius: number,
  teamID: integer,
  spherical: boolean?,
  forced: boolean?,
  excludeUnitID: integer?,
  excludeUnitDefIDs: number[]?
) ->  nil
```
@param `z` - uses ground height when unspecified

@param `spherical` - (Default: true)

@param `forced` - (Default: true)






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L3997-L4010" target="_blank">source</a>]


### Spring.CallCOBScript
---
```lua
function Spring.CallCOBScript(
  unitID: integer,
  funcName: (integer|string)?,
  retArgs: integer,
  ...: any
) ->  number...
```
@param `funcName` - Function ID or name.

@param `retArgs` - Number of values to return.

@param `...` - Arguments






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L1551-L1558" target="_blank">source</a>]


### Spring.ClearFeaturesPreviousDrawFlag
---
```lua
function Spring.ClearFeaturesPreviousDrawFlag() ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L2157-L2161" target="_blank">source</a>]


### Spring.ClearUnitGoal
---
```lua
function Spring.ClearUnitGoal(unitID: integer) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L3778-L3782" target="_blank">source</a>]


### Spring.ClearUnitsPreviousDrawFlag
---
```lua
function Spring.ClearUnitsPreviousDrawFlag() ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L2146-L2150" target="_blank">source</a>]


### Spring.ClearWatchDogTimer
---
```lua
function Spring.ClearWatchDogTimer(threadName: string?) ->  nil
```
@param `threadName` - (Default: main)






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L5219-L5222" target="_blank">source</a>]


### Spring.ClosestBuildPos
---
```lua
function Spring.ClosestBuildPos(
  teamID: integer,
  unitDefID: integer,
  posX: number,
  posY: number,
  posZ: number,
  searchRadius: number,
  minDistance: number,
  buildFacing: number
)
 -> buildPosX number
 -> buildPosY number
 -> buildPosZ number

```
@param `buildFacing` - one of SOUTH = 0, EAST = 1, NORTH = 2, WEST  = 3






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L7846-L7860" target="_blank">source</a>]


### Spring.CreateDir
---
```lua
function Spring.CreateDir(path: string) -> dirCreated boolean?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L2639-L2644" target="_blank">source</a>]


### Spring.CreateFeature
---
```lua
function Spring.CreateFeature(
  featureDef: (string|number),
  x: number,
  y: number,
  z: number,
  heading: number?,
  AllyTeamID: integer?,
  featureID: integer?
) -> featureID number
```
@param `featureDef` - name or id






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L4321-L4331" target="_blank">source</a>]


### Spring.CreateGroundDecal
---
```lua
function Spring.CreateGroundDecal() -> decalID (nil|number)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L4692-L4696" target="_blank">source</a>]


### Spring.CreateUnit
---
```lua
function Spring.CreateUnit(
  unitDefName: (string|number),
  x: number,
  y: number,
  z: number,
  facing: Facing,
  teamID: integer,
  build: boolean?,
  flattenGround: boolean?,
  unitID: integer?,
  builderID: integer?
) -> unitID (number|nil)
```
@param `unitDefName` - or unitDefID

@param `build` - (Default: false) the unit is created in "being built" state with buildProgress = 0

@param `flattenGround` - (Default: true) the unit flattens ground, if it normally does so

@param `unitID` - requests specific unitID


@return `unitID` - meaning unit was created





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L1651-L1668" target="_blank">source</a>]


### Spring.DeleteProjectile
---
```lua
function Spring.DeleteProjectile(projectileID: integer) ->  nil
```





Silently removes projectiles (no explosion).

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L6815-L6820" target="_blank">source</a>]


### Spring.DeselectUnit
---
```lua
function Spring.DeselectUnit(unitID: integer) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L1471-L1476" target="_blank">source</a>]


### Spring.DeselectUnitArray
---
```lua
function Spring.DeselectUnitArray(unitIDs: table<any,integer>) ->  nil
```
@param `unitIDs` - Table with unit IDs as value.






Deselects multiple units.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L1513-L1518" target="_blank">source</a>]


### Spring.DeselectUnitMap
---
```lua
function Spring.DeselectUnitMap(unitMap: table<integer,any>) ->  nil
```
@param `unitMap` - Table with unit IDs as keys.






Deselects multiple units.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L1524-L1529" target="_blank">source</a>]


### Spring.DestroyFeature
---
```lua
function Spring.DestroyFeature(featureDefID: integer) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L4409-L4413" target="_blank">source</a>]


### Spring.DestroyGroundDecal
---
```lua
function Spring.DestroyGroundDecal(decalID: integer) -> delSuccess boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L4708-L4713" target="_blank">source</a>]


### Spring.DestroyUnit
---
```lua
function Spring.DestroyUnit(
  unitID: integer,
  selfd: boolean?,
  reclaimed: boolean?,
  attackerID: integer?,
  cleanupImmediately: boolean?
) ->  nil
```
@param `selfd` - (Default: false) makes the unit act like it self-destructed.

@param `reclaimed` - (Default: false) don't show any DeathSequences, don't leave a wreckage. This does not give back the resources to the team!

@param `cleanupImmediately` - (Default: false) stronger version of reclaimed, removes the unit unconditionally and makes its ID available for immediate reuse (otherwise it takes a few frames)






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L1753-L1762" target="_blank">source</a>]


### Spring.DiffTimers
---
```lua
function Spring.DiffTimers(
  endTimer: integer,
  startTimer: integer,
  returnMs: boolean?,
  fromMicroSecs: boolean?
) -> timeAmount number
```
@param `returnMs` - (Default: false) whether to return `timeAmount` in milliseconds as opposed to seconds

@param `fromMicroSecs` - (Default: false) whether timers are in microseconds instead of milliseconds






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L738-L746" target="_blank">source</a>]


### Spring.DrawUnitCommands
---
```lua
function Spring.DrawUnitCommands(unitID: integer) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L1047-L1052" target="_blank">source</a>]


### Spring.Echo
---
```lua
function Spring.Echo(
  arg: any,
  ...: any
) ->  nil
```





Useful for debugging.

Prints values in the spring chat console.
Hint: the default print() writes to STDOUT.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L471-L481" target="_blank">source</a>]


### Spring.EditUnitCmdDesc
---
```lua
function Spring.EditUnitCmdDesc(
  unitID: integer,
  cmdDescID: integer,
  cmdArray: CommandDescription
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L7289-L7294" target="_blank">source</a>]


### Spring.ExtractModArchiveFile
---
```lua
function Spring.ExtractModArchiveFile(modfile: string) -> extracted boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L2575-L2580" target="_blank">source</a>]


### Spring.FindUnitCmdDesc
---
```lua
function Spring.FindUnitCmdDesc(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L6563-L6567" target="_blank">source</a>]


### Spring.FixedAllies
---
```lua
function Spring.FixedAllies() -> enabled (boolean|nil)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L852-L857" target="_blank">source</a>]


### Spring.ForceLayoutUpdate
---
```lua
function Spring.ForceLayoutUpdate() ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L2781-L2783" target="_blank">source</a>]


### Spring.ForceTesselationUpdate
---
```lua
function Spring.ForceTesselationUpdate(
  normal: boolean?,
  shadow: boolean?
) -> updated boolean
```
@param `normal` - (Default: true)

@param `shadow` - (Default: false)






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L4112-L4118" target="_blank">source</a>]


### Spring.ForceUnitCollisionUpdate
---
```lua
function Spring.ForceUnitCollisionUpdate(unitID: integer) ->  nil
```





Prevent collision checks from working on outdated data

There's a rare edge case that requires units to be in specific positions
and being shot by specific weapons but which can result in shots ghosting
through the unit. This is because the unit's collision volume is stale.
The `movement.unitQuadPositionUpdateRate` modrule controls this behaviour
and can guarantee 100% correctness if set to 1, but the default value is 3
and large-scale games generally don't want to set it so low. This function
lets you guarantee success for important weapons regardless of how high
the normal update rate is set.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L6643-L6657" target="_blank">source</a>]


### Spring.FreeUnitIcon
---
```lua
function Spring.FreeUnitIcon(iconName: string) -> freed boolean?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L2426-L2433" target="_blank">source</a>]


### Spring.GameOver
---
```lua
function Spring.GameOver(
  allyTeamID1: number?,
  allyTeamID2: number?,
  allyTeamIDn: number?
) ->  nil
```





Will declare game over.

A list of winning allyteams can be passed, if undecided (like when dropped from the host) it should be empty (no winner), in the case of a draw with multiple winners, all should be listed.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L961-L971" target="_blank">source</a>]


### Spring.GarbageCollectCtrl
---
```lua
function Spring.GarbageCollectCtrl(
  itersPerBatch: integer?,
  numStepsPerIter: integer?,
  minStepsPerIter: integer?,
  maxStepsPerIter: integer?,
  minLoopRunTime: number?,
  maxLoopRunTime: number?,
  baseRunTimeMult: number?,
  baseMemLoadMult: number?
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L4199-L4210" target="_blank">source</a>]


### Spring.GetAIInfo
---
```lua
function Spring.GetAIInfo(teamID: integer)
 -> skirmishAIID number
 -> name string
 -> hostingPlayerID number
 -> shortName string
 -> version string
 -> options table<string,string>

```

@return `shortName` - when synced "SYNCED_NOSHORTNAME", otherwise the AI shortname or "UNKNOWN"

@return `version` - when synced "SYNCED_NOVERSION", otherwise the AI version or "UNKNOWN"





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L2255-L2265" target="_blank">source</a>]


### Spring.GetActionHotKeys
---
```lua
function Spring.GetActionHotKeys(actionName: string) -> hotkeys string[]?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L3960-L3965" target="_blank">source</a>]


### Spring.GetActiveCmdDesc
---
```lua
function Spring.GetActiveCmdDesc(cmdIndex: integer) ->  CommandDescription?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L3382-L3387" target="_blank">source</a>]


### Spring.GetActiveCmdDescs
---
```lua
function Spring.GetActiveCmdDescs() -> cmdDescs CommandDescription[]
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L3353-L3357" target="_blank">source</a>]


### Spring.GetActiveCommand
---
```lua
function Spring.GetActiveCommand()
 -> cmdIndex number?
 -> cmdID number?
 -> cmdType number?
 -> cmdName (nil|string)

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L3295-L3302" target="_blank">source</a>]


### Spring.GetActivePage
---
```lua
function Spring.GetActivePage()
 -> activePage number
 -> maxPage number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L3488-L3493" target="_blank">source</a>]


### Spring.GetAllFeatures
---
```lua
function Spring.GetAllFeatures()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L6603-L6606" target="_blank">source</a>]


### Spring.GetAllGroundDecals
---
```lua
function Spring.GetAllGroundDecals() -> decalIDs number[]
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L4459-L4464" target="_blank">source</a>]


### Spring.GetAllUnits
---
```lua
function Spring.GetAllUnits() -> unitIDs number[]
```





Get a list of all unitIDs

Note that when called from a widget, this also returns units that are only
radar blips.

For units that are radar blips, you may want to check if they are in los,
as GetUnitDefID() will still return true if they have previously been seen.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L2396-L2409" target="_blank">source</a>]


### Spring.GetAllyTeamInfo
---
```lua
function Spring.GetAllyTeamInfo(allyTeamID: integer) ->  (nil|table<string,string>)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L2314-L2319" target="_blank">source</a>]


### Spring.GetAllyTeamList
---
```lua
function Spring.GetAllyTeamList() -> list number[]
```

@return `list` - of allyTeamIDs





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L1612-L1616" target="_blank">source</a>]


### Spring.GetAllyTeamStartBox
---
```lua
function Spring.GetAllyTeamStartBox(allyID: integer)
 -> xMin number?
 -> zMin number?
 -> xMax number?
 -> zMax number?

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L1526-L1536" target="_blank">source</a>]


### Spring.GetBoxSelectionByEngine
---
```lua
function Spring.GetBoxSelectionByEngine() -> when boolean
```

@return `when` - true engine won't select units inside selection box when released





Get if selection box is handled by engine

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L2431-L2439" target="_blank">source</a>]


### Spring.GetBuildFacing
---
```lua
function Spring.GetBuildFacing() -> buildFacing Facing
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L3441-L3445" target="_blank">source</a>]


### Spring.GetBuildSpacing
---
```lua
function Spring.GetBuildSpacing() -> buildSpacing number
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L3456-L3460" target="_blank">source</a>]


### Spring.GetCEGID
---
```lua
function Spring.GetCEGID()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L5872-L5875" target="_blank">source</a>]


### Spring.GetCOBScriptID
---
```lua
function Spring.GetCOBScriptID(
  unitID: integer,
  funcName: string
) -> funcID integer?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L1612-L1617" target="_blank">source</a>]


### Spring.GetCameraDirection
---
```lua
function Spring.GetCameraDirection()
 -> dirX number
 -> dirY number
 -> dirZ number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L2753-L2758" target="_blank">source</a>]


### Spring.GetCameraFOV
---
```lua
function Spring.GetCameraFOV()
 -> vFOV number
 -> hFOV number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L2781-L2785" target="_blank">source</a>]


### Spring.GetCameraNames
---
```lua
function Spring.GetCameraNames() -> Table table<string,number>
```

@return `Table` - where where keys are names and values are indices.





Get available cameras.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L2642-L2647" target="_blank">source</a>]


### Spring.GetCameraPosition
---
```lua
function Spring.GetCameraPosition()
 -> posX number
 -> posY number
 -> posZ number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L2739-L2744" target="_blank">source</a>]


### Spring.GetCameraRotation
---
```lua
function Spring.GetCameraRotation()
 -> rotX number
 -> rotY number
 -> rotZ number

```

@return `rotX` - in radians

@return `rotY` - in radians

@return `rotZ` - in radians





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L2767-L2772" target="_blank">source</a>]


### Spring.GetCameraState
---
```lua
function Spring.GetCameraState(useReturns: unknown) -> cameraState CameraState
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L2663-L2667" target="_blank">source</a>]


### Spring.GetCameraVectors
---
```lua
function Spring.GetCameraVectors() ->  CameraVectors
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L2804-L2807" target="_blank">source</a>]


### Spring.GetClipboard
---
```lua
function Spring.GetClipboard() -> text string
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L3596-L3600" target="_blank">source</a>]


### Spring.GetCmdDescIndex
---
```lua
function Spring.GetCmdDescIndex(cmdID: integer) -> cmdDescIndex integer?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L3406-L3411" target="_blank">source</a>]


### Spring.GetCommandQueue
---
```lua
function Spring.GetCommandQueue(
  unitID: integer,
  count: integer
) -> commands Command[]
```
@param `count` - Number of commands to return, `-1` returns all commands, `0` returns command count.






Get the commands for a unit.

Same as `Spring.GetUnitCommands`

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L6395-L6405" target="_blank">source</a>]


### Spring.GetConfigFloat
---
```lua
function Spring.GetConfigFloat(
  name: string,
  default: number?
) -> configFloat number?
```
@param `default` - (Default: `0`)






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L4389-L4395" target="_blank">source</a>]


### Spring.GetConfigInt
---
```lua
function Spring.GetConfigInt(
  name: string,
  default: number?
) -> configInt number?
```
@param `default` - (Default: `0`)






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L4368-L4374" target="_blank">source</a>]


### Spring.GetConfigParams
---
```lua
function Spring.GetConfigParams() ->  Configuration[]
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L4294-L4298" target="_blank">source</a>]


### Spring.GetConfigString
---
```lua
function Spring.GetConfigString(
  name: string,
  default: string?
) -> configString number?
```
@param `default` - (Default: `""`)






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L4410-L4416" target="_blank">source</a>]


### Spring.GetConsoleBuffer
---
```lua
function Spring.GetConsoleBuffer(maxLines: number) -> buffer { priority: integer,text: string }[]
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L3653-L3657" target="_blank">source</a>]


### Spring.GetCurrentTooltip
---
```lua
function Spring.GetCurrentTooltip() -> tooltip string
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L3689-L3692" target="_blank">source</a>]


### Spring.GetDecalQuadPos
---
```lua
function Spring.GetDecalQuadPos(decalID: integer)
 -> posTL.x number?
 -> posTL.z number
 -> posTR.x number
 -> posTR.z number
 -> posBR.x number
 -> posBR.z number
 -> posBL.x number
 -> posBL.z number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L4514-L4526" target="_blank">source</a>]


### Spring.GetDecalTextures
---
```lua
function Spring.GetDecalTextures(isMainTex: boolean?) -> textureNames string[]
```
@param `isMainTex` - (Default: true) If false, it gets the texture for normals/glow maps


@return `textureNames` - All textures on the atlas and available for use in SetGroundDecalTexture





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L4605-L4610" target="_blank">source</a>]


### Spring.GetDefaultCommand
---
```lua
function Spring.GetDefaultCommand()
 -> cmdIndex number?
 -> cmdID number?
 -> cmdType number?
 -> cmdName (nil|string)

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L3324-L3331" target="_blank">source</a>]


### Spring.GetDrawFrame
---
```lua
function Spring.GetDrawFrame()
 -> low_16bit number
 -> high_16bit number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L1101-L1107" target="_blank">source</a>]


### Spring.GetDrawSeconds
---
```lua
function Spring.GetDrawSeconds() -> Time integer
```

@return `Time` - in seconds





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L3089-L3093" target="_blank">source</a>]


### Spring.GetDrawSelectionInfo
---
```lua
function Spring.GetDrawSelectionInfo() ->  boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L1057-L1061" target="_blank">source</a>]


### Spring.GetDualViewGeometry
---
```lua
function Spring.GetDualViewGeometry()
 -> dualViewSizeX number
 -> dualViewSizeY number
 -> dualViewPosX number
 -> dualViewPosY number

```

@return `dualViewSizeX` - in px

@return `dualViewSizeY` - in px

@return `dualViewPosX` - offset from leftmost screen left border in px

@return `dualViewPosY` - offset from bottommost screen bottom border in px





Get dual view geometry (minimap when enabled)

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L828-L836" target="_blank">source</a>]


### Spring.GetFPS
---
```lua
function Spring.GetFPS() -> fps number
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L3239-L3243" target="_blank">source</a>]


### Spring.GetFacingFromHeading
---
```lua
function Spring.GetFacingFromHeading(heading: number) -> facing number
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L1394-L1398" target="_blank">source</a>]


### Spring.GetFactoryBuggerOff
---
```lua
function Spring.GetFactoryBuggerOff(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L6279-L6283" target="_blank">source</a>]


### Spring.GetFactoryCommands
---
```lua
function Spring.GetFactoryCommands(
  unitID: integer,
  count: number
) -> commands (number|Command[])
```
@param `count` - when 0 returns the number of commands in the units queue, when -1 returns all commands, number of commands to return otherwise






Get the number or list of commands for a factory

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L6222-L6229" target="_blank">source</a>]


### Spring.GetFactoryCounts
---
```lua
function Spring.GetFactoryCounts(
  unitID: integer,
  count: integer?,
  addCmds: boolean?
) -> counts table<number,number>?
```
@param `count` - (Default: -1) Number of commands to retrieve, `-1` for all.

@param `addCmds` - (Default: false) Retrieve commands other than buildunit


@return `counts` - Build queue count by `unitDefID` or `-cmdID`, or `nil` if unit is not found.





Gets the build queue of a factory

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L6359-L6367" target="_blank">source</a>]


### Spring.GetFeatureAllyTeam
---
```lua
function Spring.GetFeatureAllyTeam(featureID: integer) ->  number?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L6669-L6674" target="_blank">source</a>]


### Spring.GetFeatureAlwaysUpdateMatrix
---
```lua
function Spring.GetFeatureAlwaysUpdateMatrix(featureID: integer) -> nil boolean?
```

@return `nil` - when featureID cannot be parsed





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L1417-L1422" target="_blank">source</a>]


### Spring.GetFeatureBlocking
---
```lua
function Spring.GetFeatureBlocking(featureID: integer)
 -> isBlocking (nil|boolean)
 -> isSolidObjectCollidable boolean
 -> isProjectileCollidable boolean
 -> isRaySegmentCollidable boolean
 -> crushable boolean
 -> blockEnemyPushing boolean
 -> blockHeightChanges boolean

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L6907-L6918" target="_blank">source</a>]


### Spring.GetFeatureCollisionVolumeData
---
```lua
function Spring.GetFeatureCollisionVolumeData(featureID: integer)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L6976-L6980" target="_blank">source</a>]


### Spring.GetFeatureDefID
---
```lua
function Spring.GetFeatureDefID(featureID: integer) ->  number?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L6631-L6636" target="_blank">source</a>]


### Spring.GetFeatureDirection
---
```lua
function Spring.GetFeatureDirection(featureID: integer)
 -> frontDirX number
 -> frontDirY number
 -> frontDirZ number
 -> rightDirX number
 -> rightDirY number
 -> rightDirZ number
 -> upDirX number
 -> upDirY number
 -> upDirZ number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L6812-L6825" target="_blank">source</a>]


### Spring.GetFeatureDrawFlag
---
```lua
function Spring.GetFeatureDrawFlag(featureID: integer) -> nil number?
```

@return `nil` - when featureID cannot be parsed





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L1434-L1439" target="_blank">source</a>]


### Spring.GetFeatureEngineDrawMask
---
```lua
function Spring.GetFeatureEngineDrawMask(featureID: integer) -> nil boolean?
```

@return `nil` - when featureID cannot be parsed





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L1406-L1411" target="_blank">source</a>]


### Spring.GetFeatureHeading
---
```lua
function Spring.GetFeatureHeading(featureID: integer)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L6864-L6868" target="_blank">source</a>]


### Spring.GetFeatureHealth
---
```lua
function Spring.GetFeatureHealth(featureID: integer)
 -> health number?
 -> defHealth number
 -> resurrectProgress number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L6686-L6693" target="_blank">source</a>]


### Spring.GetFeatureHeight
---
```lua
function Spring.GetFeatureHeight(featureID: integer) ->  number?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L6707-L6712" target="_blank">source</a>]


### Spring.GetFeatureLastAttackedPiece
---
```lua
function Spring.GetFeatureLastAttackedPiece(featureID: integer)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L6966-L6970" target="_blank">source</a>]


### Spring.GetFeatureLuaDraw
---
```lua
function Spring.GetFeatureLuaDraw(featureID: integer) -> nil boolean?
```

@return `nil` - when featureID cannot be parsed





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L1384-L1389" target="_blank">source</a>]


### Spring.GetFeatureMass
---
```lua
function Spring.GetFeatureMass(featureID: integer) ->  number?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L6740-L6745" target="_blank">source</a>]


### Spring.GetFeatureNoDraw
---
```lua
function Spring.GetFeatureNoDraw(featureID: integer) -> nil boolean?
```

@return `nil` - when featureID cannot be parsed





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L1395-L1400" target="_blank">source</a>]


### Spring.GetFeatureNoSelect
---
```lua
function Spring.GetFeatureNoSelect(featureID: integer) ->  (nil|boolean)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L6925-L6930" target="_blank">source</a>]


### Spring.GetFeaturePieceCollisionVolumeData
---
```lua
function Spring.GetFeaturePieceCollisionVolumeData(featureID: integer)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L6991-L6995" target="_blank">source</a>]


### Spring.GetFeaturePieceDirection
---
```lua
function Spring.GetFeaturePieceDirection(
  featureID: integer,
  pieceIndex: integer
)
 -> dirX (number|nil)
 -> dirY number
 -> dirZ number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L8688-L8696" target="_blank">source</a>]


### Spring.GetFeaturePieceInfo
---
```lua
function Spring.GetFeaturePieceInfo(
  featureID: integer,
  pieceIndex: integer
) -> pieceInfo PieceInfo?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L8645-L8651" target="_blank">source</a>]


### Spring.GetFeaturePieceList
---
```lua
function Spring.GetFeaturePieceList(featureID: integer) -> pieceNames string[]
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L8634-L8639" target="_blank">source</a>]


### Spring.GetFeaturePieceMap
---
```lua
function Spring.GetFeaturePieceMap(featureID: integer) -> pieceInfos table<string,number>
```

@return `pieceInfos` - where keys are piece names and values are indices





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L8623-L8628" target="_blank">source</a>]


### Spring.GetFeaturePieceMatrix
---
```lua
function Spring.GetFeaturePieceMatrix(featureID: integer)
 -> m11 (number|nil)
 -> m12 number
 -> m13 number
 -> m14 number
 -> m21 number
 -> m22 number
 -> m23 number
 -> m24 number
 -> m31 number
 -> m32 number
 -> m33 number
 -> m34 number
 -> m41 number
 -> m42 number
 -> m43 number
 -> m44 number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L8702-L8722" target="_blank">source</a>]


### Spring.GetFeaturePiecePosDir
---
```lua
function Spring.GetFeaturePiecePosDir(
  featureID: integer,
  pieceIndex: integer
)
 -> posX (number|nil)
 -> posY number
 -> posZ number
 -> dirX number
 -> dirY number
 -> dirZ number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L8657-L8668" target="_blank">source</a>]


### Spring.GetFeaturePiecePosition
---
```lua
function Spring.GetFeaturePiecePosition(
  featureID: integer,
  pieceIndex: integer
)
 -> posX (number|nil)
 -> posY number
 -> posZ number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L8674-L8682" target="_blank">source</a>]


### Spring.GetFeaturePosition
---
```lua
function Spring.GetFeaturePosition(featureID: integer)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L6751-L6755" target="_blank">source</a>]


### Spring.GetFeatureRadius
---
```lua
function Spring.GetFeatureRadius(featureID: integer) ->  number?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L6724-L6729" target="_blank">source</a>]


### Spring.GetFeatureResources
---
```lua
function Spring.GetFeatureResources(featureID: integer)
 -> metal number?
 -> defMetal number
 -> energy number
 -> defEnergy number
 -> reclaimLeft number
 -> reclaimTime number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L6880-L6890" target="_blank">source</a>]


### Spring.GetFeatureResurrect
---
```lua
function Spring.GetFeatureResurrect(featureID: integer)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L6943-L6947" target="_blank">source</a>]


### Spring.GetFeatureRootPiece
---
```lua
function Spring.GetFeatureRootPiece(featureID: integer) -> index number
```

@return `index` - of the root piece





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L8613-L8618" target="_blank">source</a>]


### Spring.GetFeatureRotation
---
```lua
function Spring.GetFeatureRotation(featureID: integer)
 -> pitch number
 -> yaw number
 -> roll number

```

@return `pitch` - Rotation in X axis

@return `yaw` - Rotation in Y axis

@return `roll` - Rotation in Z axis





Note: PYR order

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L6794-L6802" target="_blank">source</a>]


### Spring.GetFeatureRulesParam
---
```lua
function Spring.GetFeatureRulesParam(
  featureID: integer,
  ruleRef: (number|string)
) -> value (nil|number|string)
```
@param `ruleRef` - the rule index or name






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L1224-L1232" target="_blank">source</a>]


### Spring.GetFeatureRulesParams
---
```lua
function Spring.GetFeatureRulesParams(featureID: integer) -> rulesParams RulesParams
```

@return `rulesParams` - map with rules names as key and values as values





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L1095-L1102" target="_blank">source</a>]


### Spring.GetFeatureSelectionVolumeData
---
```lua
function Spring.GetFeatureSelectionVolumeData(featureID: integer)
 -> scaleX number?
 -> scaleY number
 -> scaleZ number
 -> offsetX number
 -> offsetY number
 -> offsetZ number
 -> volumeType number
 -> useContHitTest number
 -> getPrimaryAxis number
 -> ignoreHits boolean

```

@return `scaleX` - nil when unitID cannot be parsed





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L1451-L1465" target="_blank">source</a>]


### Spring.GetFeatureSeparation
---
```lua
function Spring.GetFeatureSeparation(
  featureID1: number,
  featureID2: number,
  direction: boolean?
) ->  number?
```
@param `direction` - (Default: false) to subtract from, default featureID1 - featureID2






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L6762-L6769" target="_blank">source</a>]


### Spring.GetFeatureTeam
---
```lua
function Spring.GetFeatureTeam(featureID: integer) ->  number?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L6648-L6653" target="_blank">source</a>]


### Spring.GetFeatureTransformMatrix
---
```lua
function Spring.GetFeatureTransformMatrix(featureID: integer)
 -> m11 number?
 -> m12 number
 -> m13 number
 -> m14 number
 -> m21 number
 -> m22 number
 -> m23 number
 -> m24 number
 -> m31 number
 -> m32 number
 -> m33 number
 -> m34 number
 -> m41 number
 -> m42 number
 -> m43 number
 -> m44 number

```

@return `m11` - nil when featureID cannot be parsed





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L1524-L1544" target="_blank">source</a>]


### Spring.GetFeatureVelocity
---
```lua
function Spring.GetFeatureVelocity(featureID: integer)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L6853-L6857" target="_blank">source</a>]


### Spring.GetFeaturesInCylinder
---
```lua
function Spring.GetFeaturesInCylinder(
  x: number,
  z: number,
  radius: number,
  allegiance: number?
) -> featureIDs number[]
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L3564-L3572" target="_blank">source</a>]


### Spring.GetFeaturesInRectangle
---
```lua
function Spring.GetFeaturesInRectangle(
  xmin: number,
  zmin: number,
  xmax: number,
  zmax: number
) -> featureIDs number[]
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L3513-L3521" target="_blank">source</a>]


### Spring.GetFeaturesInScreenRectangle
---
```lua
function Spring.GetFeaturesInScreenRectangle(
  left: number,
  top: number,
  right: number,
  bottom: number
) -> featureIDs (nil|number[])
```





Get features inside a rectangle area on the map

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L2267-L2275" target="_blank">source</a>]


### Spring.GetFeaturesInSphere
---
```lua
function Spring.GetFeaturesInSphere(
  x: number,
  y: number,
  z: number,
  radius: number
) -> featureIDs number[]
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L3539-L3547" target="_blank">source</a>]


### Spring.GetFrameTimeOffset
---
```lua
function Spring.GetFrameTimeOffset() -> offset number?
```

@return `offset` - of the current draw frame from the last sim frame, expressed in fractions of a frame





Ideally, when running 30hz sim, and 60hz rendering, the draw frames should
have and offset of either 0.0 frames, or 0.5 frames.

When draw frames are not integer multiples of sim frames, some interpolation
happens, and this timeoffset shows how far along it is.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L1116-L1127" target="_blank">source</a>]


### Spring.GetFrameTimer
---
```lua
function Spring.GetFrameTimer(lastFrameTime: boolean?) ->  integer
```
@param `lastFrameTime` - (Default: false) whether to use last frame time instead of last frame start






Get a timer for the start of the frame

This should give better results for camera interpolations

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L718-L726" target="_blank">source</a>]


### Spring.GetFullBuildQueue
---
```lua
function Spring.GetFullBuildQueue(unitID: integer) -> buildqueue (nil|table<number,number>)
```

@return `buildqueue` - indexed by unitDefID with count values





Returns the build queue

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L6497-L6502" target="_blank">source</a>]


### Spring.GetGaiaTeamID
---
```lua
function Spring.GetGaiaTeamID() -> teamID number
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L1510-L1515" target="_blank">source</a>]


### Spring.GetGameFrame
---
```lua
function Spring.GetGameFrame()
 -> t1 number
 -> t2 number

```

@return `t1` - frameNum % dayFrames

@return `t2` - frameNum / dayFrames





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L887-L893" target="_blank">source</a>]


### Spring.GetGameName
---
```lua
function Spring.GetGameName() -> name string
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L499-L504" target="_blank">source</a>]


### Spring.GetGameRulesParam
---
```lua
function Spring.GetGameRulesParam(ruleRef: (number|string)) ->  number?
```
@param `ruleRef` - the rule index or name


@return  - |string value





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L1131-L1138" target="_blank">source</a>]


### Spring.GetGameRulesParams
---
```lua
function Spring.GetGameRulesParams() -> rulesParams RulesParams
```

@return `rulesParams` - map with rules names as key and values as values





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L969-L974" target="_blank">source</a>]


### Spring.GetGameSeconds
---
```lua
function Spring.GetGameSeconds() -> seconds number
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L905-L910" target="_blank">source</a>]


### Spring.GetGameSpeed
---
```lua
function Spring.GetGameSpeed()
 -> wantedSpeedFactor number
 -> speedFactor number
 -> paused boolean

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L3253-L3259" target="_blank">source</a>]


### Spring.GetGameState
---
```lua
function Spring.GetGameState(maxLatency: number?)
 -> doneLoading boolean
 -> isSavedGame boolean
 -> isClientPaused boolean
 -> isSimLagging boolean

```
@param `maxLatency` - (Default: 500) used for `isSimLagging` return parameter






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L3268-L3276" target="_blank">source</a>]


### Spring.GetGatherMode
---
```lua
function Spring.GetGatherMode() -> gatherMode number
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L3471-L3475" target="_blank">source</a>]


### Spring.GetGlobalLos
---
```lua
function Spring.GetGlobalLos(teamID: integer?) -> enabled boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L820-L827" target="_blank">source</a>]


### Spring.GetGrass
---
```lua
function Spring.GetGrass(
  x: number,
  z: number
) ->  number
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L7666-L7672" target="_blank">source</a>]


### Spring.GetGroundBlocked
---
```lua
function Spring.GetGroundBlocked()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L7579-L7582" target="_blank">source</a>]


### Spring.GetGroundDecalAlpha
---
```lua
function Spring.GetGroundDecalAlpha(decalID: integer)
 -> alpha number?
 -> alphaFalloff number

```

@return `alpha` - Between 0 and 1

@return `alphaFalloff` - Between 0 and 1, per second





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L4641-L4647" target="_blank">source</a>]


### Spring.GetGroundDecalCreationFrame
---
```lua
function Spring.GetGroundDecalCreationFrame(decalID: integer)
 -> creationFrameMin number?
 -> creationFrameMax number

```





Min can be not equal to max for "gradient" style decals, e.g. unit tracks

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L4739-L4748" target="_blank">source</a>]


### Spring.GetGroundDecalMiddlePos
---
```lua
function Spring.GetGroundDecalMiddlePos(decalID: integer)
 -> posX number?
 -> posZ number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L4493-L4499" target="_blank">source</a>]


### Spring.GetGroundDecalMisc
---
```lua
function Spring.GetGroundDecalMisc(decalID: integer)
 -> dotElimExp number?
 -> refHeight number
 -> minHeight number
 -> maxHeight number
 -> forceHeightMode number

```





Returns less important parameters of a ground decal

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L4713-L4723" target="_blank">source</a>]


### Spring.GetGroundDecalNormal
---
```lua
function Spring.GetGroundDecalNormal(decalID: integer)
 -> normal.x number?
 -> normal.y number
 -> normal.z number

```





If all three equal 0, the decal follows the normals of ground at midpoint

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L4661-L4671" target="_blank">source</a>]


### Spring.GetGroundDecalOwner
---
```lua
function Spring.GetGroundDecalOwner(decalID: integer) -> unitID number?
```

@return `unitID` - |number featureID(+MAX_UNITS)





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L4763-L4768" target="_blank">source</a>]


### Spring.GetGroundDecalRotation
---
```lua
function Spring.GetGroundDecalRotation(decalID: integer) -> rotation number?
```

@return `rotation` - in radians





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L4571-L4576" target="_blank">source</a>]


### Spring.GetGroundDecalSizeAndHeight
---
```lua
function Spring.GetGroundDecalSizeAndHeight(decalID: integer)
 -> sizeX number?
 -> sizeY number
 -> projCubeHeight number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L4547-L4554" target="_blank">source</a>]


### Spring.GetGroundDecalTexture
---
```lua
function Spring.GetGroundDecalTexture(
  decalID: integer,
  isMainTex: boolean?
) -> texture (nil|string)
```
@param `isMainTex` - (Default: true) If false, it gets the normals/glow map






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L4590-L4596" target="_blank">source</a>]


### Spring.GetGroundDecalTint
---
```lua
function Spring.GetGroundDecalTint(decalID: integer)
 -> tintR number?
 -> tintG number
 -> tintB number
 -> tintA number

```





Gets the tint of the ground decal.
A color of (0.5, 0.5, 0.5, 0.5) is effectively no tint

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L4686-L4696" target="_blank">source</a>]


### Spring.GetGroundDecalType
---
```lua
function Spring.GetGroundDecalType(decalID: integer) -> type (nil|string)
```

@return `type` - "explosion"|"plate"|"lua"|"track"|"unknown"





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L4784-L4789" target="_blank">source</a>]


### Spring.GetGroundExtremes
---
```lua
function Spring.GetGroundExtremes()
 -> initMinHeight number
 -> initMaxHeight number
 -> currMinHeight number
 -> currMaxHeight number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L7624-L7631" target="_blank">source</a>]


### Spring.GetGroundHeight
---
```lua
function Spring.GetGroundHeight(
  x: number,
  z: number
) ->  number
```





Get ground height

On sea, this returns the negative depth of the seafloor

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L7409-L7417" target="_blank">source</a>]


### Spring.GetGroundInfo
---
```lua
function Spring.GetGroundInfo(
  x: number,
  z: number
)
 -> ix number
 -> iz number
 -> terrainTypeIndex number
 -> name string
 -> metalExtraction number
 -> hardness number
 -> tankSpeed number
 -> kbotSpeed number
 -> hoverSpeed number
 -> shipSpeed number
 -> receiveTracks boolean

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L7508-L7524" target="_blank">source</a>]


### Spring.GetGroundNormal
---
```lua
function Spring.GetGroundNormal(
  x: number,
  z: number,
  smoothed: boolean?
)
 -> normalX number
 -> normalY number
 -> normalZ number
 -> slope number

```
@param `smoothed` - (Default: false) raw or smoothed center normal






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L7478-L7488" target="_blank">source</a>]


### Spring.GetGroundOrigHeight
---
```lua
function Spring.GetGroundOrigHeight(
  x: number,
  z: number
) ->  number
```





Get ground height as it was at game start

Returns the original height before the ground got deformed

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L7460-L7468" target="_blank">source</a>]


### Spring.GetGroupList
---
```lua
function Spring.GetGroupList() -> where (nil|table<number,number>)
```

@return `where` - keys are groupIDs and values are counts





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L3986-L3990" target="_blank">source</a>]


### Spring.GetGroupUnits
---
```lua
function Spring.GetGroupUnits(groupID: integer) -> unitIDs (nil|number[])
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L4061-L4066" target="_blank">source</a>]


### Spring.GetGroupUnitsCount
---
```lua
function Spring.GetGroupUnitsCount(groupID: integer) -> groupSize number?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L4112-L4117" target="_blank">source</a>]


### Spring.GetGroupUnitsCounts
---
```lua
function Spring.GetGroupUnitsCounts(groupID: integer) -> where (nil|table<number,number>)
```

@return `where` - keys are unitDefIDs and values are counts





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L4095-L4100" target="_blank">source</a>]


### Spring.GetGroupUnitsSorted
---
```lua
function Spring.GetGroupUnitsSorted(groupID: integer) -> where (nil|table<number,number[]>)
```

@return `where` - keys are unitDefIDs and values are unitIDs





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L4078-L4083" target="_blank">source</a>]


### Spring.GetHeadingFromFacing
---
```lua
function Spring.GetHeadingFromFacing(facing: number) -> heading number
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L1405-L1409" target="_blank">source</a>]


### Spring.GetHeadingFromVector
---
```lua
function Spring.GetHeadingFromVector(
  x: number,
  z: number
) -> heading number
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L1357-L1365" target="_blank">source</a>]


### Spring.GetInvertQueueKey
---
```lua
function Spring.GetInvertQueueKey() -> queueKey number?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L3835-L3839" target="_blank">source</a>]


### Spring.GetKeyBindings
---
```lua
function Spring.GetKeyBindings(
  keySet1: string?,
  keySet2: string?
) ->  KeyBinding[]
```
@param `keySet1` - filters keybindings bound to this keyset

@param `keySet2` - OR bound to this keyset






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L3907-L3912" target="_blank">source</a>]


### Spring.GetKeyCode
---
```lua
function Spring.GetKeyCode(keySym: string) -> keyCode number
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L3850-L3855" target="_blank">source</a>]


### Spring.GetKeyFromScanSymbol
---
```lua
function Spring.GetKeyFromScanSymbol(scanSymbol: string) -> keyName string
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L3706-L3710" target="_blank">source</a>]


### Spring.GetKeyState
---
```lua
function Spring.GetKeyState(keyCode: number) -> pressed boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L3741-L3746" target="_blank">source</a>]


### Spring.GetKeySymbol
---
```lua
function Spring.GetKeySymbol(keyCode: number)
 -> keyCodeName string
 -> keyCodeDefaultName string

```

@return `keyCodeDefaultName` - name when there are not aliases





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L3863-L3869" target="_blank">source</a>]


### Spring.GetLastMessagePositions
---
```lua
function Spring.GetLastMessagePositions() -> message xyz[]
```

@return `message` - positions





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L3630-L3634" target="_blank">source</a>]


### Spring.GetLastUpdateSeconds
---
```lua
function Spring.GetLastUpdateSeconds() -> lastUpdateSeconds number?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L1146-L1151" target="_blank">source</a>]


### Spring.GetLocalAllyTeamID
---
```lua
function Spring.GetLocalAllyTeamID() -> allyTeamID number
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L2347-L2351" target="_blank">source</a>]


### Spring.GetLocalPlayerID
---
```lua
function Spring.GetLocalPlayerID() -> playerID number
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L2323-L2327" target="_blank">source</a>]


### Spring.GetLocalTeamID
---
```lua
function Spring.GetLocalTeamID() -> teamID number
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L2335-L2339" target="_blank">source</a>]


### Spring.GetLogSections
---
```lua
function Spring.GetLogSections() -> sections table<string,number>
```

@return `sections` - where keys are names and loglevel are values. E.g. `{ "KeyBindings" = LOG.INFO, "Font" = LOG.INFO, "Sound" = LOG.WARNING, ... }`





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L4431-L4435" target="_blank">source</a>]


### Spring.GetLosViewColors
---
```lua
function Spring.GetLosViewColors()
 -> always rgb
 -> LOS rgb
 -> radar rgb
 -> jam rgb
 -> radar2 rgb

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L2590-L2597" target="_blank">source</a>]


### Spring.GetLuaMemUsage
---
```lua
function Spring.GetLuaMemUsage()
 -> luaHandleAllocedMem number
 -> luaHandleNumAllocs number
 -> luaGlobalAllocedMem number
 -> luaGlobalNumAllocs number
 -> luaUnsyncedGlobalAllocedMem number
 -> luaUnsyncedGlobalNumAllocs number
 -> luaSyncedGlobalAllocedMem number
 -> luaSyncedGlobalNumAllocs number

```

@return `luaHandleAllocedMem` - in kilobytes

@return `luaHandleNumAllocs` - divided by 1000

@return `luaGlobalAllocedMem` - in kilobytes

@return `luaGlobalNumAllocs` - divided by 1000

@return `luaUnsyncedGlobalAllocedMem` - in kilobytes

@return `luaUnsyncedGlobalNumAllocs` - divided by 1000

@return `luaSyncedGlobalAllocedMem` - in kilobytes

@return `luaSyncedGlobalNumAllocs` - divided by 1000





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L589-L601" target="_blank">source</a>]


### Spring.GetMapDrawMode
---
```lua
function Spring.GetMapDrawMode() ->  ("normal"|"height"|"metal"|"pathTraversability"|"los")
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L2504-L2508" target="_blank">source</a>]


### Spring.GetMapOption
---
```lua
function Spring.GetMapOption(mapOption: string) -> value string
```

@return `value` - Value of `modOption`.





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L1300-L1307" target="_blank">source</a>]


### Spring.GetMapOptions
---
```lua
function Spring.GetMapOptions() -> mapOptions table<string,string>
```

@return `mapOptions` - Table with options names as keys and values as values.





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L1312-L1317" target="_blank">source</a>]


### Spring.GetMapSquareTexture
---
```lua
function Spring.GetMapSquareTexture(
  texSquareX: number,
  texSquareY: number,
  lodMin: number,
  luaTexName: string,
  lodMax: number?
) -> success boolean?
```
@param `lodMax` - (Default: lodMin)






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L2533-L2542" target="_blank">source</a>]


### Spring.GetMapStartPositions
---
```lua
function Spring.GetMapStartPositions() -> array float3[]
```

@return `array` - of positions indexed by teamID





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L1586-L1590" target="_blank">source</a>]


### Spring.GetMenuName
---
```lua
function Spring.GetMenuName() -> name string
```

@return `name` - name .. version from Modinfo.lua. E.g. "Spring: 1944 test-5640-ac2d15b".





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L511-L516" target="_blank">source</a>]


### Spring.GetMiniMapDualScreen
---
```lua
function Spring.GetMiniMapDualScreen() -> position string
```

@return `position` - "left"|"right" when dual screen is enabled, false when not





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L1000-L1004" target="_blank">source</a>]


### Spring.GetMiniMapGeometry
---
```lua
function Spring.GetMiniMapGeometry()
 -> minimapPosX number
 -> minimapPosY number
 -> minimapSizeX number
 -> minimapSizeY number
 -> minimized boolean
 -> maximized boolean

```

@return `minimapPosX` - in px

@return `minimapPosY` - in px

@return `minimapSizeX` - in px

@return `minimapSizeY` - in px





Get minimap geometry

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L957-L967" target="_blank">source</a>]


### Spring.GetMiniMapRotation
---
```lua
function Spring.GetMiniMapRotation() -> amount number
```

@return `amount` - in radians





Get minimap rotation

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L984-L988" target="_blank">source</a>]


### Spring.GetModKeyState
---
```lua
function Spring.GetModKeyState()
 -> alt boolean
 -> ctrl boolean
 -> meta boolean
 -> shift boolean

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L3755-L3762" target="_blank">source</a>]


### Spring.GetModOption
---
```lua
function Spring.GetModOption(modOption: string) -> value string
```

@return `value` - Value of `modOption`.





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L1324-L1331" target="_blank">source</a>]


### Spring.GetModOptions
---
```lua
function Spring.GetModOptions() -> modOptions table<string,string>
```

@return `modOptions` - Table with options names as keys and values as values.





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L1338-L1343" target="_blank">source</a>]


### Spring.GetModelPieceList
---
```lua
function Spring.GetModelPieceList(modelName: string) -> pieceNames (nil|string[])
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L8488-L8493" target="_blank">source</a>]


### Spring.GetModelPieceMap
---
```lua
function Spring.GetModelPieceMap(modelName: string) -> pieceInfos (nil|table<string,number>)
```

@return `pieceInfos` - where keys are piece names and values are indices





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L8477-L8482" target="_blank">source</a>]


### Spring.GetModelRootPiece
---
```lua
function Spring.GetModelRootPiece(modelName: string) -> index number
```

@return `index` - of the root piece





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L8467-L8472" target="_blank">source</a>]


### Spring.GetMouseCursor
---
```lua
function Spring.GetMouseCursor()
 -> cursorName string
 -> cursorScale number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L3539-L3544" target="_blank">source</a>]


### Spring.GetMouseStartPosition
---
```lua
function Spring.GetMouseStartPosition(button: number)
 -> x number
 -> y number
 -> camPosX number
 -> camPosY number
 -> camPosZ number
 -> dirX number
 -> dirY number
 -> dirZ number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L3555-L3567" target="_blank">source</a>]


### Spring.GetMouseState
---
```lua
function Spring.GetMouseState()
 -> x number
 -> y number
 -> lmbPressed number
 -> mmbPressed number
 -> rmbPressed number
 -> offscreen boolean
 -> mmbScroll boolean

```

@return `lmbPressed` - left mouse button pressed

@return `mmbPressed` - middle mouse button pressed

@return `rmbPressed` - right mouse button pressed





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L3511-L3521" target="_blank">source</a>]


### Spring.GetNanoProjectileParams
---
```lua
function Spring.GetNanoProjectileParams()
 -> rotVal number
 -> rotVel number
 -> rotAcc number
 -> rotValRng number
 -> rotVelRng number
 -> rotAccRng number

```

@return `rotVal` - in degrees

@return `rotVel` - in degrees

@return `rotAcc` - in degrees

@return `rotValRng` - in degrees

@return `rotVelRng` - in degrees

@return `rotAccRng` - in degrees





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L2618-L2627" target="_blank">source</a>]


### Spring.GetNumDisplays
---
```lua
function Spring.GetNumDisplays() -> numDisplays number
```

@return `numDisplays` - as returned by `SDL_GetNumVideoDisplays`





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L796-L801" target="_blank">source</a>]


### Spring.GetPieceProjectileParams
---
```lua
function Spring.GetPieceProjectileParams(projectileID: integer)
 -> explosionFlags number?
 -> spinAngle number
 -> spinSpeed number
 -> spinVectorX number
 -> spinVectorY number
 -> spinVectorZ number

```

@return `explosionFlags` - encoded bitwise with SHATTER = 1, EXPLODE = 2, EXPLODE_ON_HIT = 2, FALL = 4, SMOKE = 8, FIRE = 16, NONE = 32, NO_CEG_TRAIL = 64, NO_HEATCLOUD = 128





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L7085-L7095" target="_blank">source</a>]


### Spring.GetPixelDir
---
```lua
function Spring.GetPixelDir(
  x: number,
  y: number
)
 -> dirX number
 -> dirY number
 -> dirZ number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L2980-L2988" target="_blank">source</a>]


### Spring.GetPlayerControlledUnit
---
```lua
function Spring.GetPlayerControlledUnit(playerID: integer) ->  number?
```





Returns unit controlled by player on FPS mode

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L2219-L2224" target="_blank">source</a>]


### Spring.GetPlayerInfo
---
```lua
function Spring.GetPlayerInfo(
  playerID: integer,
  getPlayerOpts: boolean?
)
 -> name string
 -> active boolean
 -> spectator boolean
 -> teamID number
 -> allyTeamID number
 -> pingTime number
 -> cpuUsage number
 -> country string
 -> rank number
 -> hasSkirmishAIsInTeam boolean
 -> playerOpts { , [string]: string }
 -> desynced boolean

```
@param `getPlayerOpts` - (Default: true) whether to return custom player options


@return `playerOpts` - when playerOpts is true





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L2154-L2171" target="_blank">source</a>]


### Spring.GetPlayerList
---
```lua
function Spring.GetPlayerList(
  teamID: integer?,
  active: boolean?
) -> list number[]?
```
@param `teamID` - (Default: -1) to filter by when >= 0

@param `active` - (Default: false) whether to filter only active teams


@return `list` - of playerIDs





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L1673-L1679" target="_blank">source</a>]


### Spring.GetPlayerRoster
---
```lua
function Spring.GetPlayerRoster(
  sortType: number?,
  showPathingPlayers: boolean?
) -> playerTable Roster[]?
```
@param `sortType` - return unsorted if unspecified. Disabled = 0, Allies = 1, TeamID = 2, PlayerName = 3, PlayerCPU = 4, PlayerPing = 5

@param `showPathingPlayers` - (Default: false)






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L4150-L4156" target="_blank">source</a>]


### Spring.GetPlayerRulesParam
---
```lua
function Spring.GetPlayerRulesParam(
  playerID: integer,
  ruleRef: (number|string)
) -> value (nil|number|string)
```
@param `ruleRef` - the rule index or name






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L1174-L1182" target="_blank">source</a>]


### Spring.GetPlayerRulesParams
---
```lua
function Spring.GetPlayerRulesParams(playerID: integer) -> rulesParams RulesParams
```

@return `rulesParams` - map with rules names as key and values as values





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L1008-L1015" target="_blank">source</a>]


### Spring.GetPlayerStatistics
---
```lua
function Spring.GetPlayerStatistics(playerID: integer)
 -> mousePixels number?
 -> mouseClicks number
 -> keyPresses number
 -> numCommands number
 -> unitCommands number

```

@return `mousePixels` - nil when invalid playerID





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L4238-L4247" target="_blank">source</a>]


### Spring.GetPlayerTraffic
---
```lua
function Spring.GetPlayerTraffic(
  playerID: integer,
  packetID: integer?
) -> traffic number
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L4186-L4192" target="_blank">source</a>]


### Spring.GetPositionLosState
---
```lua
function Spring.GetPositionLosState(
  posX: number,
  posY: number,
  posZ: number,
  allyTeamID: integer?
)
 -> inLosOrRadar boolean
 -> inLos boolean
 -> inRadar boolean
 -> inJammer boolean

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L7914-L7925" target="_blank">source</a>]


### Spring.GetPressedKeys
---
```lua
function Spring.GetPressedKeys() -> where table<(number|string),unknown>
```

@return `where` - keys are keyCodes or key names





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L3773-L3777" target="_blank">source</a>]


### Spring.GetPressedScans
---
```lua
function Spring.GetPressedScans() -> where table<(number|string),unknown>
```

@return `where` - keys are scanCodes or scan names





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L3804-L3808" target="_blank">source</a>]


### Spring.GetProfilerRecordNames
---
```lua
function Spring.GetProfilerRecordNames() -> profilerNames string[]
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L567-L572" target="_blank">source</a>]


### Spring.GetProfilerTimeRecord
---
```lua
function Spring.GetProfilerTimeRecord(
  profilerName: string,
  frameData: boolean?
)
 -> total number
 -> current number
 -> max_dt number
 -> time_pct number
 -> peak_pct number
 -> frameData table<number,number>?

```
@param `frameData` - (Default: false)


@return `total` - in ms

@return `current` - in ms

@return `frameData` - Table where key is the frame index and value is duration.





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L530-L543" target="_blank">source</a>]


### Spring.GetProjectileAllyTeamID
---
```lua
function Spring.GetProjectileAllyTeamID(projectileID: integer) ->  number?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L7250-L7255" target="_blank">source</a>]


### Spring.GetProjectileDamages
---
```lua
function Spring.GetProjectileDamages(
  projectileID: integer,
  tag: string
) ->  number?
```
@param `tag` - one of:
"paralyzeDamageTime"
"impulseFactor"
"impulseBoost"
"craterMult"
"craterBoost"
"dynDamageExp"
"dynDamageMin"
"dynDamageRange"
"dynDamageInverted"
"craterAreaOfEffect"
"damageAreaOfEffect"
"edgeEffectiveness"
"explosionSpeed"
- or -
an armor type index to get the damage against it.






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L7320-L7341" target="_blank">source</a>]


### Spring.GetProjectileDefID
---
```lua
function Spring.GetProjectileDefID(projectileID: integer) ->  number?
```





Using this to get a weaponDefID is HIGHLY preferred to indexing WeaponDefNames via GetProjectileName

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L7291-L7299" target="_blank">source</a>]


### Spring.GetProjectileDirection
---
```lua
function Spring.GetProjectileDirection(projectileID: integer)
 -> dirX number?
 -> dirY number
 -> dirZ number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L7030-L7037" target="_blank">source</a>]


### Spring.GetProjectileGravity
---
```lua
function Spring.GetProjectileGravity(projectileID: integer) ->  number?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L7066-L7071" target="_blank">source</a>]


### Spring.GetProjectileIsIntercepted
---
```lua
function Spring.GetProjectileIsIntercepted(projectileID: integer) ->  (nil|boolean)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L7167-L7172" target="_blank">source</a>]


### Spring.GetProjectileOwnerID
---
```lua
function Spring.GetProjectileOwnerID(projectileID: integer) ->  number?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L7207-L7212" target="_blank">source</a>]


### Spring.GetProjectilePosition
---
```lua
function Spring.GetProjectilePosition(projectileID: integer)
 -> posX number?
 -> posY number
 -> posZ number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L7009-L7016" target="_blank">source</a>]


### Spring.GetProjectileTarget
---
```lua
function Spring.GetProjectileTarget(projectileID: integer)
 -> targetTypeInt number?
 -> target (number|float3)

```

@return `targetTypeInt` - where
string.byte('g') := GROUND
string.byte('u') := UNIT
string.byte('f') := FEATURE
string.byte('p') := PROJECTILE

@return `target` - targetID or targetPos when targetTypeInt == string.byte('g')





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L7115-L7125" target="_blank">source</a>]


### Spring.GetProjectileTeamID
---
```lua
function Spring.GetProjectileTeamID(projectileID: integer) ->  number?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L7229-L7234" target="_blank">source</a>]


### Spring.GetProjectileTimeToLive
---
```lua
function Spring.GetProjectileTimeToLive(projectileID: integer) ->  number?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L7187-L7192" target="_blank">source</a>]


### Spring.GetProjectileType
---
```lua
function Spring.GetProjectileType(projectileID: integer)
 -> weapon (nil|boolean)
 -> piece boolean

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L7271-L7277" target="_blank">source</a>]


### Spring.GetProjectileVelocity
---
```lua
function Spring.GetProjectileVelocity(projectileID: integer)
 -> velX number?
 -> velY number
 -> velZ number
 -> velW number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L7051-L7059" target="_blank">source</a>]


### Spring.GetProjectilesInRectangle
---
```lua
function Spring.GetProjectilesInRectangle(
  xmin: number,
  zmin: number,
  xmax: number,
  zmax: number,
  excludeWeaponProjectiles: boolean?,
  excludePieceProjectiles: boolean?
) -> projectileIDs number[]
```
@param `excludeWeaponProjectiles` - (Default: false)

@param `excludePieceProjectiles` - (Default: false)






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L3588-L3598" target="_blank">source</a>]


### Spring.GetRadarErrorParams
---
```lua
function Spring.GetRadarErrorParams(allyTeamID: integer)
 -> radarErrorSize number?
 -> baseRadarErrorSize number
 -> baseRadarErrorMult number

```

@return `radarErrorSize` - actual radar error size (when allyTeamID is allied to current team) or base radar error size





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L8883-L8892" target="_blank">source</a>]


### Spring.GetRealBuildQueue
---
```lua
function Spring.GetRealBuildQueue(unitID: integer) -> buildqueue (nil|table<number,number>)
```

@return `buildqueue` - indexed by unitDefID with count values





Returns the build queue cleaned of things the unit can't build itself

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L6509-L6514" target="_blank">source</a>]


### Spring.GetRenderFeatures
---
```lua
function Spring.GetRenderFeatures()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L2128-L2131" target="_blank">source</a>]


### Spring.GetRenderFeaturesDrawFlagChanged
---
```lua
function Spring.GetRenderFeaturesDrawFlagChanged()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L2137-L2140" target="_blank">source</a>]


### Spring.GetRenderUnits
---
```lua
function Spring.GetRenderUnits()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L2110-L2113" target="_blank">source</a>]


### Spring.GetRenderUnitsDrawFlagChanged
---
```lua
function Spring.GetRenderUnitsDrawFlagChanged()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L2119-L2122" target="_blank">source</a>]


### Spring.GetReplayLength
---
```lua
function Spring.GetReplayLength() -> timeInSeconds number?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L479-L484" target="_blank">source</a>]


### Spring.GetScanSymbol
---
```lua
function Spring.GetScanSymbol(scanCode: number)
 -> scanCodeName string
 -> scanCodeDefaultName string

```

@return `scanCodeDefaultName` - name when there are not aliases





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L3879-L3885" target="_blank">source</a>]


### Spring.GetScreenGeometry
---
```lua
function Spring.GetScreenGeometry(
  displayIndex: number?,
  queryUsable: boolean?
)
 -> screenSizeX number
 -> screenSizeY number
 -> screenPosX number
 -> screenPosY number
 -> windowBorderTop number
 -> windowBorderLeft number
 -> windowBorderBottom number
 -> windowBorderRight number
 -> screenUsableSizeX number?
 -> screenUsableSizeY number?
 -> screenUsablePosX number?
 -> screenUsablePosY number?

```
@param `displayIndex` - (Default: -1)

@param `queryUsable` - (Default: false)


@return `screenSizeX` - in px

@return `screenSizeY` - in px

@return `screenPosX` - in px

@return `screenPosY` - in px

@return `windowBorderTop` - in px

@return `windowBorderLeft` - in px

@return `windowBorderBottom` - in px

@return `windowBorderRight` - in px

@return `screenUsableSizeX` - in px

@return `screenUsableSizeY` - in px

@return `screenUsablePosX` - in px

@return `screenUsablePosY` - in px





Get screen geometry

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L900-L919" target="_blank">source</a>]


### Spring.GetSelectedGroup
---
```lua
function Spring.GetSelectedGroup() -> groupID number
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L4014-L4018" target="_blank">source</a>]


### Spring.GetSelectedUnits
---
```lua
function Spring.GetSelectedUnits() -> unitIDs number[]
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L2378-L2382" target="_blank">source</a>]


### Spring.GetSelectedUnitsCount
---
```lua
function Spring.GetSelectedUnitsCount() -> selectedUnitsCount number
```





Returns the amount of selected units

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L2420-L2424" target="_blank">source</a>]


### Spring.GetSelectedUnitsCounts
---
```lua
function Spring.GetSelectedUnitsCounts()
 -> unitsCounts table<number,number>
 -> the integer

```

@return `unitsCounts` - where keys are unitDefIDs and values are counts

@return `the` - number of unitDefIDs





Get an aggregate count of selected units per unitDefID

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L2404-L2410" target="_blank">source</a>]


### Spring.GetSelectedUnitsSorted
---
```lua
function Spring.GetSelectedUnitsSorted()
 -> where table<number,number[]>
 -> the integer

```

@return `where` - keys are unitDefIDs and values are unitIDs

@return `the` - number of unitDefIDs





Get selected units aggregated by unitDefID

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L2389-L2394" target="_blank">source</a>]


### Spring.GetSelectionBox
---
```lua
function Spring.GetSelectionBox()
 -> left number?
 -> top number?
 -> right number?
 -> bottom number?

```





Get vertices from currently active selection box

Returns nil when selection box is inactive

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L1023-L1035" target="_blank">source</a>]


### Spring.GetSideData
---
```lua
function Spring.GetSideData(sideName: string)
 -> startUnit (nil|string)
 -> caseSensitiveSideName string

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L1437-L1445" target="_blank">source</a>]


### Spring.GetSmoothMeshHeight
---
```lua
function Spring.GetSmoothMeshHeight(
  x: number,
  z: number
) -> height number
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L7682-L7688" target="_blank">source</a>]


### Spring.GetSoundDevices
---
```lua
function Spring.GetSoundDevices() -> devices SoundDeviceSpec[]
```

@return `devices` - Sound devices.





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L3115-L3119" target="_blank">source</a>]


### Spring.GetSoundEffectParams
---
```lua
function Spring.GetSoundEffectParams()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L3149-L3152" target="_blank">source</a>]


### Spring.GetSoundStreamTime
---
```lua
function Spring.GetSoundStreamTime()
 -> playTime number
 -> time number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L3135-L3140" target="_blank">source</a>]


### Spring.GetSpectatingState
---
```lua
function Spring.GetSpectatingState()
 -> spectating boolean
 -> spectatingFullView boolean
 -> spectatingFullSelect boolean

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L2359-L2365" target="_blank">source</a>]


### Spring.GetSyncedGCInfo
---
```lua
function Spring.GetSyncedGCInfo(collectGC: boolean?) -> GC number?
```
@param `collectGC` - (Default: false) collect before returning metric


@return `GC` - values are expressed in Kbytes: #bytes/2^10





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L4826-L4831" target="_blank">source</a>]


### Spring.GetTeamAllyTeamID
---
```lua
function Spring.GetTeamAllyTeamID(teamID: integer) -> allyTeamID integer?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L1778-L1783" target="_blank">source</a>]


### Spring.GetTeamColor
---
```lua
function Spring.GetTeamColor(teamID: integer)
 -> r number?
 -> g number?
 -> b number?
 -> a number?

```

@return `r` - factor from 0 to 1

@return `g` - factor from 0 to 1

@return `b` - factor from 0 to 1

@return `a` - factor from 0 to 1





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L3035-L3043" target="_blank">source</a>]


### Spring.GetTeamDamageStats
---
```lua
function Spring.GetTeamDamageStats(teamID: integer)
 -> damageDealt number
 -> damageReceived number

```





Gets team damage dealt/received totals

Returns a team's damage stats. Note that all damage is counted,
including self-inflicted and unconfirmed out-of-sight.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L1942-L1952" target="_blank">source</a>]


### Spring.GetTeamInfo
---
```lua
function Spring.GetTeamInfo(
  teamID: integer,
  getTeamKeys: boolean?
)
 -> teamID number?
 -> leader number
 -> isDead number
 -> hasAI number
 -> side string
 -> allyTeam number
 -> incomeMultiplier number
 -> customTeamKeys table<string,string>

```
@param `getTeamKeys` - (Default: true) whether to return the customTeamKeys table


@return `customTeamKeys` - when getTeamKeys is true, otherwise nil





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L1727-L1740" target="_blank">source</a>]


### Spring.GetTeamList
---
```lua
function Spring.GetTeamList(allyTeamID: integer?) -> list number[]?
```
@param `allyTeamID` - (Default: -1) to filter teams belonging to when >= 0


@return `list` - of teamIDs





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L1632-L1637" target="_blank">source</a>]


### Spring.GetTeamLuaAI
---
```lua
function Spring.GetTeamLuaAI(teamID: integer) ->  string
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L2096-L2101" target="_blank">source</a>]


### Spring.GetTeamMaxUnits
---
```lua
function Spring.GetTeamMaxUnits(teamID: integer)
 -> maxUnits number
 -> currentUnits number?

```





Returns a team's unit cap.

Also returns the current unit count for readable teams as the 2nd value.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L2129-L2137" target="_blank">source</a>]


### Spring.GetTeamOrigColor
---
```lua
function Spring.GetTeamOrigColor(teamID: integer)
 -> r number?
 -> g number?
 -> b number?
 -> a number?

```

@return `r` - factor from 0 to 1

@return `g` - factor from 0 to 1

@return `b` - factor from 0 to 1

@return `a` - factor from 0 to 1





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L3062-L3070" target="_blank">source</a>]


### Spring.GetTeamResourceStats
---
```lua
function Spring.GetTeamResourceStats(
  teamID: integer,
  resource: ResourceName
)
 -> used number?
 -> produced number
 -> excessed number
 -> received number
 -> sent number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L1893-L1903" target="_blank">source</a>]


### Spring.GetTeamResources
---
```lua
function Spring.GetTeamResources(
  teamID: integer,
  resource: ResourceName
)
 -> currentLevel number?
 -> storage number
 -> pull number
 -> income number
 -> expense number
 -> share number
 -> sent number
 -> received number
 -> excess number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L1799-L1813" target="_blank">source</a>]


### Spring.GetTeamRulesParam
---
```lua
function Spring.GetTeamRulesParam(
  teamID: integer,
  ruleRef: (number|string)
) -> value (nil|number|string)
```
@param `ruleRef` - the rule index or name






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L1146-L1154" target="_blank">source</a>]


### Spring.GetTeamRulesParams
---
```lua
function Spring.GetTeamRulesParams(teamID: integer) -> rulesParams RulesParams
```

@return `rulesParams` - map with rules names as key and values as values





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L982-L989" target="_blank">source</a>]


### Spring.GetTeamStartPosition
---
```lua
function Spring.GetTeamStartPosition(teamID: integer)
 -> x number?
 -> y number?
 -> x number?

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L1558-L1567" target="_blank">source</a>]


### Spring.GetTeamStatsHistory
---
```lua
function Spring.GetTeamStatsHistory(teamID: integer) -> historyCount integer?
```

@return `historyCount` - The number of history entries, or `nil` if unable to resolve team.





Get the number of history entries.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L1997-L2002" target="_blank">source</a>]


### Spring.GetTeamUnitCount
---
```lua
function Spring.GetTeamUnitCount(teamID: integer) -> count number?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L2850-L2855" target="_blank">source</a>]


### Spring.GetTeamUnitDefCount
---
```lua
function Spring.GetTeamUnitDefCount(
  teamID: integer,
  unitDefID: integer
) -> count number?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L2789-L2795" target="_blank">source</a>]


### Spring.GetTeamUnitStats
---
```lua
function Spring.GetTeamUnitStats(teamID: integer)
 -> killed number?
 -> died number
 -> capturedBy number
 -> capturedFrom number
 -> received number
 -> sent number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L1858-L1868" target="_blank">source</a>]


### Spring.GetTeamUnits
---
```lua
function Spring.GetTeamUnits(teamID: integer) -> unitIDs number[]?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L2434-L2439" target="_blank">source</a>]


### Spring.GetTeamUnitsByDefs
---
```lua
function Spring.GetTeamUnitsByDefs(
  teamID: integer,
  unitDefIDs: (number|number[])
) -> unitIDs number[]?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L2713-L2719" target="_blank">source</a>]


### Spring.GetTeamUnitsCounts
---
```lua
function Spring.GetTeamUnitsCounts(teamID: integer) -> countByUnit table<number,number>?
```

@return `countByUnit` - A table where keys are unitDefIDs and values are counts.





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L2630-L2635" target="_blank">source</a>]


### Spring.GetTeamUnitsSorted
---
```lua
function Spring.GetTeamUnitsSorted(teamID: integer) -> unitsByDef table<integer,integer>
```

@return `unitsByDef` - A table where keys are unitDefIDs and values are unitIDs





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L2534-L2539" target="_blank">source</a>]


### Spring.GetTerrainTypeData
---
```lua
function Spring.GetTerrainTypeData(terrainTypeInfo: number)
 -> index number
 -> name string
 -> hardness number
 -> tankSpeed number
 -> kbotSpeed number
 -> hoverSpeed number
 -> shipSpeed number
 -> receiveTracks boolean

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L7642-L7654" target="_blank">source</a>]


### Spring.GetTidal
---
```lua
function Spring.GetTidal() -> tidalStrength number
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L924-L929" target="_blank">source</a>]


### Spring.GetTimer
---
```lua
function Spring.GetTimer() ->  integer
```





Get a timer with millisecond resolution

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L694-L698" target="_blank">source</a>]


### Spring.GetTimerMicros
---
```lua
function Spring.GetTimerMicros() ->  integer
```





Get a timer with microsecond resolution

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L706-L710" target="_blank">source</a>]


### Spring.GetUnitAllyTeam
---
```lua
function Spring.GetUnitAllyTeam(unitID: integer) ->  number?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L4063-L4068" target="_blank">source</a>]


### Spring.GetUnitAlwaysUpdateMatrix
---
```lua
function Spring.GetUnitAlwaysUpdateMatrix(unitID: integer) -> nil boolean?
```

@return `nil` - when unitID cannot be parsed





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L1253-L1258" target="_blank">source</a>]


### Spring.GetUnitArmored
---
```lua
function Spring.GetUnitArmored(unitID: integer)
 -> armored (nil|boolean)
 -> armorMultiple number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L3791-L3797" target="_blank">source</a>]


### Spring.GetUnitArrayCentroid
---
```lua
function Spring.GetUnitArrayCentroid(units: table)
 -> centerX number
 -> centerY number
 -> centerZ number

```
@param `units` - { unitID, unitID, ... }






Returns the centroid of an array of units

Returns nil for an empty array

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L3386-L3395" target="_blank">source</a>]


### Spring.GetUnitBasePosition
---
```lua
function Spring.GetUnitBasePosition(unitID: integer)
 -> posX number?
 -> posY number
 -> posZ number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L4411-L4418" target="_blank">source</a>]


### Spring.GetUnitBlocking
---
```lua
function Spring.GetUnitBlocking(unitID: integer)
 -> isBlocking (nil|boolean)
 -> isSolidObjectCollidable boolean
 -> isProjectileCollidable boolean
 -> isRaySegmentCollidable boolean
 -> crushable boolean
 -> blockEnemyPushing boolean
 -> blockHeightChanges boolean

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L5883-L5894" target="_blank">source</a>]


### Spring.GetUnitBuildFacing
---
```lua
function Spring.GetUnitBuildFacing(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L4539-L4543" target="_blank">source</a>]


### Spring.GetUnitBuildParams
---
```lua
function Spring.GetUnitBuildParams(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L4791-L4795" target="_blank">source</a>]


### Spring.GetUnitBuildeeRadius
---
```lua
function Spring.GetUnitBuildeeRadius(unitID: integer) ->  number?
```





Gets the unit's radius for when targeted by build, repair, reclaim-type commands.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L4362-L4368" target="_blank">source</a>]


### Spring.GetUnitCmdDescs
---
```lua
function Spring.GetUnitCmdDescs(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L6524-L6528" target="_blank">source</a>]


### Spring.GetUnitCollisionVolumeData
---
```lua
function Spring.GetUnitCollisionVolumeData(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L5740-L5744" target="_blank">source</a>]


### Spring.GetUnitCommands
---
```lua
function Spring.GetUnitCommands(
  unitID: integer,
  count: integer
) -> commands Command[]
```
@param `count` - Number of commands to return, `-1` returns all commands, `0` returns command count.






Get the commands for a unit.

Same as `Spring.GetCommandQueue`

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L6168-L6178" target="_blank">source</a>]


### Spring.GetUnitCostTable
---
```lua
function Spring.GetUnitCostTable(unitID: integer)
 -> cost ResourceCost?
 -> buildTime number?

```

@return `cost` - The cost of the unit, or `nil` if invalid.

@return `buildTime` - The build time the unit, or `nil` if invalid.





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L4267-L4272" target="_blank">source</a>]


### Spring.GetUnitCosts
---
```lua
function Spring.GetUnitCosts(unitID: integer)
 -> buildTime number?
 -> metalCost number
 -> energyCost number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L4242-L4248" target="_blank">source</a>]


### Spring.GetUnitCurrentBuildPower
---
```lua
function Spring.GetUnitCurrentBuildPower(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L4733-L4737" target="_blank">source</a>]


### Spring.GetUnitCurrentCommand
---
```lua
function Spring.GetUnitCurrentCommand(
  unitID: integer,
  cmdIndex: integer
)
```
@param `unitID` - Unit id.

@param `cmdIndex` - Command index to get. If negative will count from the end of the queue,
for example -1 will be the last command.






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L6126-L6133" target="_blank">source</a>]


### Spring.GetUnitDefDimensions
---
```lua
function Spring.GetUnitDefDimensions(unitDefID: integer)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L5838-L5842" target="_blank">source</a>]


### Spring.GetUnitDefID
---
```lua
function Spring.GetUnitDefID(unitID: integer) ->  number?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L4021-L4026" target="_blank">source</a>]


### Spring.GetUnitDirection
---
```lua
function Spring.GetUnitDirection(unitID: integer)
 -> frontDirX number
 -> frontDirY number
 -> frontDirZ number
 -> rightDirX number
 -> rightDirY number
 -> rightDirZ number
 -> upDirX number
 -> upDirY number
 -> upDirZ number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L4468-L4481" target="_blank">source</a>]


### Spring.GetUnitDrawFlag
---
```lua
function Spring.GetUnitDrawFlag(unitID: integer) -> nil number?
```

@return `nil` - when unitID cannot be parsed





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L1270-L1275" target="_blank">source</a>]


### Spring.GetUnitEffectiveBuildRange
---
```lua
function Spring.GetUnitEffectiveBuildRange(
  unitID: integer,
  buildeeDefID: integer
) -> effectiveBuildRange number
```
@param `buildeeDefID` - or nil


@return `effectiveBuildRange` - counted to the center of prospective buildee; buildRange if buildee nil





Useful for setting move goals manually.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L4677-L4684" target="_blank">source</a>]


### Spring.GetUnitEngineDrawMask
---
```lua
function Spring.GetUnitEngineDrawMask(unitID: integer) -> nil boolean?
```

@return `nil` - when unitID cannot be parsed





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L1242-L1247" target="_blank">source</a>]


### Spring.GetUnitEstimatedPath
---
```lua
function Spring.GetUnitEstimatedPath(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L5690-L5694" target="_blank">source</a>]


### Spring.GetUnitExperience
---
```lua
function Spring.GetUnitExperience(unitID: integer)
 -> xp number
 -> limXp number

```

@return `xp` - [0.0; +∞)

@return `limXp` - [0.0; 1.0) as experience approaches infinity





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L4310-L4316" target="_blank">source</a>]


### Spring.GetUnitFeatureSeparation
---
```lua
function Spring.GetUnitFeatureSeparation(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L5803-L5807" target="_blank">source</a>]


### Spring.GetUnitFlanking
---
```lua
function Spring.GetUnitFlanking(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L4990-L4994" target="_blank">source</a>]


### Spring.GetUnitGroup
---
```lua
function Spring.GetUnitGroup(unitID: integer) -> groupID number?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L4026-L4031" target="_blank">source</a>]


### Spring.GetUnitHarvestStorage
---
```lua
function Spring.GetUnitHarvestStorage(unitID: integer)
 -> storedMetal number
 -> maxStoredMetal number
 -> storedEnergy number
 -> maxStoredEnergy number

```





Get a unit's carried resources

Checks resources being carried internally by the unit.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L4766-L4777" target="_blank">source</a>]


### Spring.GetUnitHeading
---
```lua
function Spring.GetUnitHeading(
  unitID: integer,
  convertToRadians: boolean?
) -> heading number
```
@param `convertToRadians` - (Default: false)






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L4505-L4511" target="_blank">source</a>]


### Spring.GetUnitHealth
---
```lua
function Spring.GetUnitHealth(unitID: integer)
 -> health number?
 -> maxHealth number
 -> paralyzeDamage number
 -> captureProgress number
 -> buildProgress number

```

@return `buildProgress` - between 0.0-1.0





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L4101-L4110" target="_blank">source</a>]


### Spring.GetUnitHeight
---
```lua
function Spring.GetUnitHeight(unitID: integer) ->  number?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L4329-L4334" target="_blank">source</a>]


### Spring.GetUnitInBuildStance
---
```lua
function Spring.GetUnitInBuildStance(unitID: integer) -> inBuildStance boolean
```





Is builder in build stance

Checks if a builder is in build stance, i.e. can create nanoframes.
Returns nil for non-builders.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L4824-L4833" target="_blank">source</a>]


### Spring.GetUnitIsActive
---
```lua
function Spring.GetUnitIsActive(unitID: integer) -> isActive boolean?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L3810-L3815" target="_blank">source</a>]


### Spring.GetUnitIsBeingBuilt
---
```lua
function Spring.GetUnitIsBeingBuilt(unitID: integer)
 -> beingBuilt boolean
 -> buildProgress number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L4184-L4190" target="_blank">source</a>]


### Spring.GetUnitIsBuilding
---
```lua
function Spring.GetUnitIsBuilding(unitID: integer) -> buildeeUnitID number
```

@return `buildeeUnitID` - or nil





Checks whether a unit is currently building another (NOT for checking if it's a structure)

Works for both mobile builders and factories.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L4555-L4563" target="_blank">source</a>]


### Spring.GetUnitIsCloaked
---
```lua
function Spring.GetUnitIsCloaked(unitID: integer) -> isCloaked boolean?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L3827-L3832" target="_blank">source</a>]


### Spring.GetUnitIsDead
---
```lua
function Spring.GetUnitIsDead(unitID: integer) ->  (nil|boolean)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L4140-L4145" target="_blank">source</a>]


### Spring.GetUnitIsStunned
---
```lua
function Spring.GetUnitIsStunned(unitID: integer)
 -> stunnedOrBuilt (nil|boolean)
 -> stunned boolean
 -> beingBuilt boolean

```

@return `stunnedOrBuilt` - unit is disabled

@return `stunned` - unit is either stunned via EMP or being transported by a non-fireplatform

@return `beingBuilt` - unit is under construction





Checks whether a unit is disabled and can't act

The first return value is a simple OR of the following ones,
any of those conditions is sufficient to disable the unit.

Note that EMP and being transported are mechanically the same and thus lumped together.
Use other callouts to differentiate them if you need to.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L4157-L4170" target="_blank">source</a>]


### Spring.GetUnitIsTransporting
---
```lua
function Spring.GetUnitIsTransporting(unitID: integer) -> transporteeArray integer[]?
```

@return `transporteeArray` - An array of unitIDs being transported by this unit, or `nil` if not a transport.





Get units being transported

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L4929-L4936" target="_blank">source</a>]


### Spring.GetUnitLastAttackedPiece
---
```lua
function Spring.GetUnitLastAttackedPiece(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L5730-L5734" target="_blank">source</a>]


### Spring.GetUnitLastAttacker
---
```lua
function Spring.GetUnitLastAttacker(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L5710-L5714" target="_blank">source</a>]


### Spring.GetUnitLosState
---
```lua
function Spring.GetUnitLosState(
  unitID: integer,
  allyTeamID: integer?,
  raw: unknown
) -> bitmask integer?
```
@param `raw` - Return a bitmask.


@return `bitmask` - A bitmask integer, or `nil` if `unitID` is invalid.

Bitmask bits:





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L8032-L8045" target="_blank">source</a>]


### Spring.GetUnitLuaDraw
---
```lua
function Spring.GetUnitLuaDraw(unitID: integer) -> draw boolean?
```

@return `draw` - nil when unitID cannot be parsed





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L1220-L1225" target="_blank">source</a>]


### Spring.GetUnitMapCentroid
---
```lua
function Spring.GetUnitMapCentroid(units: table)
 -> centerX number
 -> centerY number
 -> centerZ number

```
@param `units` - { [unitID] = true, [unitID] = true, ... }






Returns the centroid of a map of units

Returns nil for an empty map

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L3401-L3410" target="_blank">source</a>]


### Spring.GetUnitMass
---
```lua
function Spring.GetUnitMass(unitID: integer) ->  number?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L4379-L4384" target="_blank">source</a>]


### Spring.GetUnitMaxRange
---
```lua
function Spring.GetUnitMaxRange(unitID: integer) -> maxRange number
```





Get a unit's engagement range

Returns the range at which a unit will stop to engage.
By default this is the highest among the unit's weapon ranges (hence name),
but can be changed dynamically. Also note that unarmed units ignore this.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L5049-L5059" target="_blank">source</a>]


### Spring.GetUnitMetalExtraction
---
```lua
function Spring.GetUnitMetalExtraction(unitID: integer) -> metalExtraction number?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L4290-L4295" target="_blank">source</a>]


### Spring.GetUnitMoveTypeData
---
```lua
function Spring.GetUnitMoveTypeData(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L5901-L5905" target="_blank">source</a>]


### Spring.GetUnitNanoPieces
---
```lua
function Spring.GetUnitNanoPieces(unitID: integer) -> pieceArray integer[]
```





Get construction FX attachment points

Returns an array of pieces which represent construction
points. Default engine construction FX (nano spray) will
originate there.

Only works on builders and factories, returns nil (NOT empty table)
for other units.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L4850-L4863" target="_blank">source</a>]


### Spring.GetUnitNearestAlly
---
```lua
function Spring.GetUnitNearestAlly(
  unitID: integer,
  range: number?
) -> unitID number?
```
@param `range` - (Default: 1.0e9f)






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L3417-L3423" target="_blank">source</a>]


### Spring.GetUnitNearestEnemy
---
```lua
function Spring.GetUnitNearestEnemy(
  unitID: integer,
  range: number?,
  useLOS: boolean?
) -> unitID number?
```
@param `range` - (Default: 1.0e9f)

@param `useLOS` - (Default: true)






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L3442-L3449" target="_blank">source</a>]


### Spring.GetUnitNeutral
---
```lua
function Spring.GetUnitNeutral(unitID: integer) ->  (nil|boolean)
```





Checks if a unit is neutral (NOT Gaia!)

Note that a "neutral" unit can belong to any ally-team (ally, enemy, Gaia).
To check if a unit is Gaia, check its owner team.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L4080-L4089" target="_blank">source</a>]


### Spring.GetUnitNoDraw
---
```lua
function Spring.GetUnitNoDraw(unitID: integer) -> nil boolean?
```

@return `nil` - when unitID cannot be parsed





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L1231-L1236" target="_blank">source</a>]


### Spring.GetUnitNoGroup
---
```lua
function Spring.GetUnitNoGroup(unitID: integer) -> noGroup (nil|boolean)
```

@return `noGroup` - `nil` when `unitID` cannot be parsed.





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L1304-L1309" target="_blank">source</a>]


### Spring.GetUnitNoMinimap
---
```lua
function Spring.GetUnitNoMinimap(unitID: integer) -> nil boolean?
```

@return `nil` - when unitID cannot be parsed





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L1287-L1292" target="_blank">source</a>]


### Spring.GetUnitNoSelect
---
```lua
function Spring.GetUnitNoSelect(unitID: integer) -> noSelect boolean?
```

@return `noSelect` - `nil` when `unitID` cannot be parsed.





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L1321-L1326" target="_blank">source</a>]


### Spring.GetUnitPhysicalState
---
```lua
function Spring.GetUnitPhysicalState(unitID: integer) -> Unit number
```

@return `Unit` - 's PhysicalState bitmask





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L3242-L3246" target="_blank">source</a>]


### Spring.GetUnitPieceDirection
---
```lua
function Spring.GetUnitPieceDirection(
  unitID: integer,
  pieceIndex: integer
)
 -> dirX (number|nil)
 -> dirY number
 -> dirZ number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L8574-L8582" target="_blank">source</a>]


### Spring.GetUnitPieceInfo
---
```lua
function Spring.GetUnitPieceInfo(
  unitID: integer,
  pieceIndex: integer
) -> pieceInfo PieceInfo?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L8531-L8537" target="_blank">source</a>]


### Spring.GetUnitPieceList
---
```lua
function Spring.GetUnitPieceList(unitID: integer) -> pieceNames string[]
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L8520-L8525" target="_blank">source</a>]


### Spring.GetUnitPieceMap
---
```lua
function Spring.GetUnitPieceMap(unitID: integer) -> pieceInfos (nil|table<string,number>)
```

@return `pieceInfos` - where keys are piece names and values are indices





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L8509-L8514" target="_blank">source</a>]


### Spring.GetUnitPieceMatrix
---
```lua
function Spring.GetUnitPieceMatrix(unitID: integer)
 -> m11 (number|nil)
 -> m12 number
 -> m13 number
 -> m14 number
 -> m21 number
 -> m22 number
 -> m23 number
 -> m24 number
 -> m31 number
 -> m32 number
 -> m33 number
 -> m34 number
 -> m41 number
 -> m42 number
 -> m43 number
 -> m44 number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L8588-L8608" target="_blank">source</a>]


### Spring.GetUnitPiecePosDir
---
```lua
function Spring.GetUnitPiecePosDir(
  unitID: integer,
  pieceIndex: integer
)
 -> posX (number|nil)
 -> posY number
 -> posZ number
 -> dirX number
 -> dirY number
 -> dirZ number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L8543-L8554" target="_blank">source</a>]


### Spring.GetUnitPiecePosition
---
```lua
function Spring.GetUnitPiecePosition(
  unitID: integer,
  pieceIndex: integer
)
 -> posX (number|nil)
 -> posY number
 -> posZ number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L8560-L8568" target="_blank">source</a>]


### Spring.GetUnitPosErrorParams
---
```lua
function Spring.GetUnitPosErrorParams(
  unitID: integer,
  allyTeamID: integer?
)
 -> posErrorVectorX number?
 -> posErrorVectorY number
 -> posErrorVectorZ number
 -> posErrorDeltaX number
 -> posErrorDeltaY number
 -> posErrorDeltaZ number
 -> nextPosErrorUpdatebaseErrorMult number
 -> posErrorBit boolean

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L3944-L3957" target="_blank">source</a>]


### Spring.GetUnitPosition
---
```lua
function Spring.GetUnitPosition(
  unitID: integer,
  midPos: boolean?,
  aimPos: boolean?
)
 -> basePointX number?
 -> basePointY number
 -> basePointZ number
 -> midPointX number?
 -> midPointY number
 -> midPointZ number
 -> aimPointX number?
 -> aimPointY number
 -> aimPointZ number

```
@param `midPos` - (Default: false) return midpoint as well

@param `aimPos` - (Default: false) return aimpoint as well






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L4390-L4405" target="_blank">source</a>]


### Spring.GetUnitRadius
---
```lua
function Spring.GetUnitRadius(unitID: integer) ->  number?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L4346-L4351" target="_blank">source</a>]


### Spring.GetUnitResources
---
```lua
function Spring.GetUnitResources(unitID: integer)
 -> metalMake number?
 -> metalUse number
 -> energyMake number
 -> energyUse number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L4202-L4210" target="_blank">source</a>]


### Spring.GetUnitRootPiece
---
```lua
function Spring.GetUnitRootPiece(unitID: integer) -> index number
```

@return `index` - of the root piece





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L8499-L8504" target="_blank">source</a>]


### Spring.GetUnitRotation
---
```lua
function Spring.GetUnitRotation(unitID: integer)
 -> pitch number
 -> yaw number
 -> roll number

```

@return `pitch` - Rotation in X axis

@return `yaw` - Rotation in Y axis

@return `roll` - Rotation in Z axis





Note: PYR order

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L4453-L4461" target="_blank">source</a>]


### Spring.GetUnitRulesParam
---
```lua
function Spring.GetUnitRulesParam(
  unitID: integer,
  ruleRef: (number|string)
) -> value (nil|number|string)
```
@param `ruleRef` - the rule index or name






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L1205-L1213" target="_blank">source</a>]


### Spring.GetUnitRulesParams
---
```lua
function Spring.GetUnitRulesParams(unitID: integer) -> rulesParams RulesParams
```

@return `rulesParams` - map with rules names as key and values as values





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L1077-L1084" target="_blank">source</a>]


### Spring.GetUnitScriptNames
---
```lua
function Spring.GetUnitScriptNames(unitID: integer) -> where table<string,number>
```

@return `where` - keys are piece names and values are piece indices





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L8773-L8780" target="_blank">source</a>]


### Spring.GetUnitScriptPiece
---
```lua
function Spring.GetUnitScriptPiece(unitID: integer) -> pieceIndices integer[]
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L8727-L8733" target="_blank">source</a>]


### Spring.GetUnitSeismicSignature
---
```lua
function Spring.GetUnitSeismicSignature(unitID: integer) -> seismicSignature number?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L3844-L3849" target="_blank">source</a>]


### Spring.GetUnitSelectionVolumeData
---
```lua
function Spring.GetUnitSelectionVolumeData(unitID: integer)
 -> scaleX number?
 -> scaleY number
 -> scaleZ number
 -> offsetX number
 -> offsetY number
 -> offsetZ number
 -> volumeType number
 -> useContHitTest number
 -> getPrimaryAxis number
 -> ignoreHits boolean

```

@return `scaleX` - nil when unitID cannot be parsed





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L1357-L1371" target="_blank">source</a>]


### Spring.GetUnitSelfDTime
---
```lua
function Spring.GetUnitSelfDTime(unitID: integer) -> selfDTime integer?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L3860-L3865" target="_blank">source</a>]


### Spring.GetUnitSensorRadius
---
```lua
function Spring.GetUnitSensorRadius(
  unitID: integer,
  type: string
) -> radius number?
```
@param `type` - one of los, airLos, radar, sonar, seismic, radarJammer, sonarJammer






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L3901-L3907" target="_blank">source</a>]


### Spring.GetUnitSeparation
---
```lua
function Spring.GetUnitSeparation(
  unitID1: number,
  unitID2: number,
  direction: boolean?,
  subtractRadii: boolean?
) ->  number?
```
@param `direction` - (Default: false) to subtract from, default unitID1 - unitID2

@param `subtractRadii` - (Default: false) whether units radii should be subtracted from the total






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L5761-L5769" target="_blank">source</a>]


### Spring.GetUnitShieldState
---
```lua
function Spring.GetUnitShieldState(
  unitID: integer,
  weaponNum: number?
)
 -> isEnabled number
 -> currentPower number

```
@param `weaponNum` - Optional if the unit has just one shield


@return `isEnabled` - Warning, number not boolean. 0 or 1





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L4958-L4965" target="_blank">source</a>]


### Spring.GetUnitStates
---
```lua
function Spring.GetUnitStates(unitID: integer) ->  UnitState
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L3698-L3703" target="_blank">source</a>]


### Spring.GetUnitStockpile
---
```lua
function Spring.GetUnitStockpile(unitID: integer)
 -> numStockpiled integer?
 -> numStockpileQued integer?
 -> buildPercent number?

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L3877-L3884" target="_blank">source</a>]


### Spring.GetUnitStorage
---
```lua
function Spring.GetUnitStorage(unitID: integer)
 -> Unit number
 -> Unit number

```

@return `Unit` - 's metal storage

@return `Unit` - 's energy storage





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L4224-L4229" target="_blank">source</a>]


### Spring.GetUnitTeam
---
```lua
function Spring.GetUnitTeam(unitID: integer) ->  number?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L4046-L4051" target="_blank">source</a>]


### Spring.GetUnitTooltip
---
```lua
function Spring.GetUnitTooltip(unitID: integer) ->  (nil|string)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L3981-L3986" target="_blank">source</a>]


### Spring.GetUnitTransformMatrix
---
```lua
function Spring.GetUnitTransformMatrix(unitID: integer)
 -> m11 number?
 -> m12 number
 -> m13 number
 -> m14 number
 -> m21 number
 -> m22 number
 -> m23 number
 -> m24 number
 -> m31 number
 -> m32 number
 -> m33 number
 -> m34 number
 -> m41 number
 -> m42 number
 -> m43 number
 -> m44 number

```

@return `m11` - nil when unitID cannot be parsed





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L1500-L1520" target="_blank">source</a>]


### Spring.GetUnitTransporter
---
```lua
function Spring.GetUnitTransporter(unitID: integer) -> transportUnitID (number|nil)
```





Get the transport carrying the unit

Returns the unit ID of the transport, if any.
Returns nil if the unit is not being transported.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L4905-L4914" target="_blank">source</a>]


### Spring.GetUnitVectors
---
```lua
function Spring.GetUnitVectors(unitID: integer)
 -> front float3?
 -> up float3
 -> right float3

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L4425-L4432" target="_blank">source</a>]


### Spring.GetUnitVelocity
---
```lua
function Spring.GetUnitVelocity(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L4528-L4532" target="_blank">source</a>]


### Spring.GetUnitViewPosition
---
```lua
function Spring.GetUnitViewPosition(
  unitID: integer,
  midPos: boolean?
)
 -> x number?
 -> y number
 -> z number

```
@param `midPos` - (Default: false)


@return `x` - nil when unitID cannot be parsed





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L1682-L1690" target="_blank">source</a>]


### Spring.GetUnitWeaponCanFire
---
```lua
function Spring.GetUnitWeaponCanFire(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L5550-L5554" target="_blank">source</a>]


### Spring.GetUnitWeaponDamages
---
```lua
function Spring.GetUnitWeaponDamages(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L5284-L5288" target="_blank">source</a>]


### Spring.GetUnitWeaponHaveFreeLineOfFire
---
```lua
function Spring.GetUnitWeaponHaveFreeLineOfFire(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L5484-L5488" target="_blank">source</a>]


### Spring.GetUnitWeaponState
---
```lua
function Spring.GetUnitWeaponState(
  unitID: integer,
  weaponNum: number,
  stateName: string
) -> stateValue number
```





Check the state of a unit's weapon

Available states to poll:
"reloadFrame" (frame on which the weapon will be ready to fire),
"reloadSpeed" (reload time in seconds),
"range" (in elmos),
"autoTargetRangeBoost" (predictive aiming range buffer, in elmos),
"projectileSpeed" (in elmos/frame),
"reloadTimeXP" (reload time after XP bonus, in seconds),
"reaimTime" (frames between AimWeapon calls),
"burst" (shots in a burst),
"burstRate" (delay between shots in a burst, in seconds),
"projectiles" (projectiles per shot),
"salvoLeft" (shots remaining in ongoing burst),
"nextSalvo" (simframe of the next shot in an ongoing burst),
"accuracy" (INaccuracy after XP bonus),
"sprayAngle" (spray angle after XP bonus),
"targetMoveError" (extra inaccuracy against moving targets, after XP bonus)
"avoidFlags" (bitmask for targeting avoidance),
"collisionFlags" (bitmask for collisions).

The state "salvoError" is an exception and returns a table: {x, y, z},
which represents the inaccuracy error of the ongoing burst.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L5078-L5108" target="_blank">source</a>]


### Spring.GetUnitWeaponTarget
---
```lua
function Spring.GetUnitWeaponTarget(
  unitID: integer,
  weaponNum: integer
)
 -> TargetType 0
 -> isUserTarget boolean

```

@return `TargetType` - none





Checks a weapon's target

Note that this doesn't need to reflect the unit's Attack orders or such, and
that weapons can aim individually unless slaved.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L5583-L5594" target="_blank">source</a>]


### Spring.GetUnitWeaponTestRange
---
```lua
function Spring.GetUnitWeaponTestRange(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L5446-L5450" target="_blank">source</a>]


### Spring.GetUnitWeaponTestTarget
---
```lua
function Spring.GetUnitWeaponTestTarget(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L5408-L5412" target="_blank">source</a>]


### Spring.GetUnitWeaponTryTarget
---
```lua
function Spring.GetUnitWeaponTryTarget(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L5363-L5367" target="_blank">source</a>]


### Spring.GetUnitWeaponVectors
---
```lua
function Spring.GetUnitWeaponVectors(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L5323-L5327" target="_blank">source</a>]


### Spring.GetUnitWorkerTask
---
```lua
function Spring.GetUnitWorkerTask(unitID: integer)
 -> cmdID number
 -> targetID number

```

@return `cmdID` - of the relevant command

@return `targetID` - if applicable (all except RESTORE)





Checks a builder's current task

Checks what a builder is currently doing. This is not the same as `Spring.GetUnitCurrentCommand`,
because you can have a command at the front of the queue and not be doing it (for example because
the target is still too far away), and on the other hand you can also be doing a task despite not
having it in front of the queue (for example you're Guarding another builder who does). Also, it
resolves the Repair command into either actual repair, or construction assist (in which case it
returns the appropriate "build" command). Only build-related commands are returned (no Move or any
custom commands).

The possible commands returned are repair, reclaim, resurrect, capture, restore,
and build commands (negative buildee unitDefID).

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L4643-L4661" target="_blank">source</a>]


### Spring.GetUnitsInBox
---
```lua
function Spring.GetUnitsInBox(
  xmin: number,
  ymin: number,
  zmin: number,
  xmax: number,
  ymax: number,
  zmax: number,
  allegiance: number?
) -> unitIDs number[]
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L3029-L3040" target="_blank">source</a>]


### Spring.GetUnitsInCylinder
---
```lua
function Spring.GetUnitsInCylinder(
  x: number,
  z: number,
  radius: number
) -> unitIDs number[]
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L3099-L3106" target="_blank">source</a>]


### Spring.GetUnitsInPlanes
---
```lua
function Spring.GetUnitsInPlanes(
  planes: Plane[],
  allegiance: integer?
) -> unitIDs integer[]
```





Plane normals point towards accepted space, so the acceptance criteria for each plane is:

```
radius     = unit radius
px, py, pz = unit position
[(nx * px) + (ny * py) + (nz * pz) + (d - radius)]  <=  0
```

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L3252-L3267" target="_blank">source</a>]


### Spring.GetUnitsInRectangle
---
```lua
function Spring.GetUnitsInRectangle(
  xmin: number,
  zmin: number,
  xmax: number,
  zmax: number,
  allegiance: number?
) -> unitIDs number[]
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L2963-L2972" target="_blank">source</a>]


### Spring.GetUnitsInScreenRectangle
---
```lua
function Spring.GetUnitsInScreenRectangle(
  left: number,
  top: number,
  right: number,
  bottom: number,
  allegiance: number?
) -> unitIDs (nil|number[])
```
@param `allegiance` - (Default: -1) teamID when > 0, when < 0 one of AllUnits = -1, MyUnits = -2, AllyUnits = -3, EnemyUnits = -4






Get units inside a rectangle area on the map

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L2168-L2177" target="_blank">source</a>]


### Spring.GetUnitsInSphere
---
```lua
function Spring.GetUnitsInSphere(
  x: number,
  y: number,
  z: number,
  radius: number
) -> unitIDs number[]
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L3161-L3169" target="_blank">source</a>]


### Spring.GetVectorFromHeading
---
```lua
function Spring.GetVectorFromHeading(heading: number)
 -> x number
 -> z number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L1376-L1384" target="_blank">source</a>]


### Spring.GetVidMemUsage
---
```lua
function Spring.GetVidMemUsage()
 -> usedMem number
 -> availableMem number

```

@return `usedMem` - in MB

@return `availableMem` - in MB





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L640-L646" target="_blank">source</a>]


### Spring.GetVideoCapturingMode
---
```lua
function Spring.GetVideoCapturingMode() -> allowRecord boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L1159-L1164" target="_blank">source</a>]


### Spring.GetViewGeometry
---
```lua
function Spring.GetViewGeometry()
 -> viewSizeX number
 -> viewSizeY number
 -> viewPosX number
 -> viewPosY number

```

@return `viewSizeX` - in px

@return `viewSizeY` - in px

@return `viewPosX` - offset from leftmost screen left border in px

@return `viewPosY` - offset from bottommost screen bottom border in px





Get main view geometry (map and game rendering)

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L809-L817" target="_blank">source</a>]


### Spring.GetVisibleFeatures
---
```lua
function Spring.GetVisibleFeatures(
  teamID: integer?,
  radius: number?,
  icons: boolean?,
  geos: boolean?
) -> featureIDs (nil|number[])
```
@param `teamID` - (Default: -1)

@param `radius` - (Default: 30)

@param `icons` - (Default: true)

@param `geos` - (Default: true)






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L1875-L1883" target="_blank">source</a>]


### Spring.GetVisibleProjectiles
---
```lua
function Spring.GetVisibleProjectiles(
  allyTeamID: integer?,
  addSyncedProjectiles: boolean?,
  addWeaponProjectiles: boolean?,
  addPieceProjectiles: boolean?
) -> projectileIDs (nil|number[])
```
@param `allyTeamID` - (Default: -1)

@param `addSyncedProjectiles` - (Default: true)

@param `addWeaponProjectiles` - (Default: true)

@param `addPieceProjectiles` - (Default: true)






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L1956-L1964" target="_blank">source</a>]


### Spring.GetVisibleUnits
---
```lua
function Spring.GetVisibleUnits(
  teamID: integer?,
  radius: number?,
  icons: boolean?
) -> unitIDs (nil|number[])
```
@param `teamID` - (Default: -1)

@param `radius` - (Default: 30)

@param `icons` - (Default: true)






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L1777-L1784" target="_blank">source</a>]


### Spring.GetWaterLevel
---
```lua
function Spring.GetWaterLevel(
  x: number,
  z: number
) -> waterLevel number
```





Get water level in a specific position

Water is currently a flat plane, so this returns the same value regardless of XZ.
However water may become more dynamic at some point so by using this you are future-proof.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L7441-L7450" target="_blank">source</a>]


### Spring.GetWaterMode
---
```lua
function Spring.GetWaterMode()
 -> waterRendererID number
 -> waterRendererName string

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L2488-L2494" target="_blank">source</a>]


### Spring.GetWaterPlaneLevel
---
```lua
function Spring.GetWaterPlaneLevel() -> waterPlaneLevel number
```





Get water plane height

Water may at some point become shaped (rivers etc) but for now it is always a flat plane.
Use this function instead of GetWaterLevel to denote you are relying on that assumption.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L7426-L7434" target="_blank">source</a>]


### Spring.GetWind
---
```lua
function Spring.GetWind() -> windStrength number
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L937-L942" target="_blank">source</a>]


### Spring.GetWindowDisplayMode
---
```lua
function Spring.GetWindowDisplayMode()
 -> width number
 -> height number
 -> bits number
 -> refresh number

```

@return `width` - in px

@return `height` - in px

@return `bits` - per pixel

@return `refresh` - rate in Hz





Get main window display mode

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L877-L884" target="_blank">source</a>]


### Spring.GetWindowGeometry
---
```lua
function Spring.GetWindowGeometry()
 -> winSizeX number
 -> winSizeY number
 -> winPosX number
 -> winPosY number
 -> windowBorderTop number
 -> windowBorderLeft number
 -> windowBorderBottom number
 -> windowBorderRight number

```

@return `winSizeX` - in px

@return `winSizeY` - in px

@return `winPosX` - in px

@return `winPosY` - in px

@return `windowBorderTop` - in px

@return `windowBorderLeft` - in px

@return `windowBorderBottom` - in px

@return `windowBorderRight` - in px





Get main window geometry

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L847-L859" target="_blank">source</a>]


### Spring.GiveOrder
---
```lua
function Spring.GiveOrder(
  cmdID: integer,
  params: table,
  options: cmdOpts
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L3317-L3324" target="_blank">source</a>]


### Spring.GiveOrderArrayToUnit
---
```lua
function Spring.GiveOrderArrayToUnit(
  unitID: integer,
  cmdArray: Command[]
) -> ordersGiven boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L5530-L5536" target="_blank">source</a>]


### Spring.GiveOrderArrayToUnitArray
---
```lua
function Spring.GiveOrderArrayToUnitArray(
  unitArray: number[],
  commands: Command[]
) ->  nil
```
@param `unitArray` - containing unitIDs






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L5613-L5618" target="_blank">source</a>]


### Spring.GiveOrderArrayToUnitMap
---
```lua
function Spring.GiveOrderArrayToUnitMap(
  unitMap: { , [number]: any },
  commands: Command[]
) -> unitsOrdered number
```
@param `unitMap` - table with unitIDs as keys






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L5571-L5576" target="_blank">source</a>]


### Spring.GiveOrderToUnit
---
```lua
function Spring.GiveOrderToUnit(
  unitID: integer,
  cmdID: integer,
  params: number[]?,
  options: CommandOptions?
) -> unitOrdered boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L5407-L5414" target="_blank">source</a>]


### Spring.GiveOrderToUnitArray
---
```lua
function Spring.GiveOrderToUnitArray(
  unitIDs: number[],
  cmdID: integer,
  params: number[]?,
  options: CommandOptions?
) -> unitsOrdered number
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L5485-L5493" target="_blank">source</a>]


### Spring.GiveOrderToUnitMap
---
```lua
function Spring.GiveOrderToUnitMap(
  unitMap: table<number,table>,
  cmdID: integer,
  params: number[]?,
  options: CommandOptions?
) -> unitsOrdered number
```
@param `unitMap` - table with unitIDs as keys






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L5443-L5450" target="_blank">source</a>]


### Spring.HaveAdvShading
---
```lua
function Spring.HaveAdvShading()
 -> useAdvShading boolean
 -> groundUseAdvShading boolean

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L2474-L2479" target="_blank">source</a>]


### Spring.HaveShadows
---
```lua
function Spring.HaveShadows() -> shadowsLoaded boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L2462-L2466" target="_blank">source</a>]


### Spring.InsertUnitCmdDesc
---
```lua
function Spring.InsertUnitCmdDesc(
  unitID: integer,
  cmdDescID: integer?,
  cmdArray: CommandDescription
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L7321-L7326" target="_blank">source</a>]


### Spring.IsAABBInView
---
```lua
function Spring.IsAABBInView(
  minX: number,
  minY: number,
  minZ: number,
  maxX: number,
  maxY: number,
  maxZ: number
) -> inView boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L1632-L1642" target="_blank">source</a>]


### Spring.IsAboveMiniMap
---
```lua
function Spring.IsAboveMiniMap(
  x: number,
  y: number
) -> isAbove boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L1069-L1077" target="_blank">source</a>]


### Spring.IsCheatingEnabled
---
```lua
function Spring.IsCheatingEnabled() -> enabled boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L753-L758" target="_blank">source</a>]


### Spring.IsDevLuaEnabled
---
```lua
function Spring.IsDevLuaEnabled() -> enabled boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L781-L786" target="_blank">source</a>]


### Spring.IsEditDefsEnabled
---
```lua
function Spring.IsEditDefsEnabled() -> enabled boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L794-L799" target="_blank">source</a>]


### Spring.IsGUIHidden
---
```lua
function Spring.IsGUIHidden() ->  boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L2450-L2454" target="_blank">source</a>]


### Spring.IsGameOver
---
```lua
function Spring.IsGameOver() -> isGameOver boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L865-L870" target="_blank">source</a>]


### Spring.IsGodModeEnabled
---
```lua
function Spring.IsGodModeEnabled() -> enabled boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L766-L771" target="_blank">source</a>]


### Spring.IsNoCostEnabled
---
```lua
function Spring.IsNoCostEnabled() -> enabled boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L807-L812" target="_blank">source</a>]


### Spring.IsPosInAirLos
---
```lua
function Spring.IsPosInAirLos(
  posX: number,
  posY: number,
  posZ: number,
  allyTeamID: integer?
) ->  boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L8006-L8014" target="_blank">source</a>]


### Spring.IsPosInLos
---
```lua
function Spring.IsPosInLos(
  posX: number,
  posY: number,
  posZ: number,
  allyTeamID: integer?
) ->  boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L7954-L7962" target="_blank">source</a>]


### Spring.IsPosInMap
---
```lua
function Spring.IsPosInMap(
  x: number,
  z: number
)
 -> inPlayArea boolean
 -> inMap boolean

```

@return `inPlayArea` - whether the position is in the active play area

@return `inMap` - whether the position is in the full map area (currently this is the same as above)





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L7368-L7375" target="_blank">source</a>]


### Spring.IsPosInRadar
---
```lua
function Spring.IsPosInRadar(
  posX: number,
  posY: number,
  posZ: number,
  allyTeamID: integer?
) ->  boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L7980-L7988" target="_blank">source</a>]


### Spring.IsReplay
---
```lua
function Spring.IsReplay() -> isReplay boolean?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L466-L471" target="_blank">source</a>]


### Spring.IsSphereInView
---
```lua
function Spring.IsSphereInView(
  posX: number,
  posY: number,
  posZ: number,
  radius: number?
) -> inView boolean
```
@param `radius` - (Default: 0)






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L1661-L1669" target="_blank">source</a>]


### Spring.IsUnitAllied
---
```lua
function Spring.IsUnitAllied(unitID: integer) -> isAllied boolean?
```

@return `isAllied` - nil with unitID cannot be parsed





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L1178-L1183" target="_blank">source</a>]


### Spring.IsUnitIcon
---
```lua
function Spring.IsUnitIcon(unitID: integer) -> isUnitIcon boolean?
```

@return `isUnitIcon` - nil when unitID cannot be parsed





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L1614-L1619" target="_blank">source</a>]


### Spring.IsUnitInAirLos
---
```lua
function Spring.IsUnitInAirLos(
  unitID: integer,
  allyTeamID: integer
) -> inAirLos boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L8120-L8126" target="_blank">source</a>]


### Spring.IsUnitInJammer
---
```lua
function Spring.IsUnitInJammer(
  unitID: integer,
  allyTeamID: integer
) -> inJammer boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L8168-L8174" target="_blank">source</a>]


### Spring.IsUnitInLos
---
```lua
function Spring.IsUnitInLos(
  unitID: integer,
  allyTeamID: integer
) -> inLos boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L8096-L8102" target="_blank">source</a>]


### Spring.IsUnitInRadar
---
```lua
function Spring.IsUnitInRadar(
  unitID: integer,
  allyTeamID: integer
) -> inRadar boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L8144-L8150" target="_blank">source</a>]


### Spring.IsUnitInView
---
```lua
function Spring.IsUnitInView(unitID: integer) -> inView boolean?
```

@return `inView` - nil when unitID cannot be parsed





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L1554-L1559" target="_blank">source</a>]


### Spring.IsUnitSelected
---
```lua
function Spring.IsUnitSelected(unitID: integer) -> isSelected boolean?
```

@return `isSelected` - nil when unitID cannot be parsed





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L1202-L1207" target="_blank">source</a>]


### Spring.IsUnitVisible
---
```lua
function Spring.IsUnitVisible(
  unitID: integer,
  radius: number?,
  checkIcon: boolean
) -> isVisible boolean?
```
@param `radius` - unitRadius when not specified


@return `isVisible` - nil when unitID cannot be parsed





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L1572-L1579" target="_blank">source</a>]


### Spring.IsUserWriting
---
```lua
function Spring.IsUserWriting() ->  boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L3612-L3616" target="_blank">source</a>]


### Spring.KillTeam
---
```lua
function Spring.KillTeam(teamID: integer) ->  nil
```





Will declare a team to be dead (no further orders can be assigned to such teams units).

Gaia team cannot be killed.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L930-L938" target="_blank">source</a>]


### Spring.LevelHeightMap
---
```lua
function Spring.LevelHeightMap(
  x1: number,
  z1: number,
  x2_height: number,
  z2: number?,
  height: number?
) ->  nil
```
@param `x2_height` - if y2 and height are nil then this parameter is the height






Set a certain height to a point or rectangle area on the world

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L5722-L5731" target="_blank">source</a>]


### Spring.LevelOriginalHeightMap
---
```lua
function Spring.LevelOriginalHeightMap(
  x1: number,
  y1: number,
  x2_height: number,
  y2: number?,
  height: number?
) ->  nil
```
@param `x2_height` - if y2 and height are nil then this parameter is the height






Set a height to a point or rectangle area to the original map height cache

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L6001-L6010" target="_blank">source</a>]


### Spring.LevelSmoothMesh
---
```lua
function Spring.LevelSmoothMesh(
  x1: number,
  z1: number,
  x2: number?,
  z2: number?,
  height: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L6269-L6277" target="_blank">source</a>]


### Spring.LoadCmdColorsConfig
---
```lua
function Spring.LoadCmdColorsConfig(config: string) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L2756-L2759" target="_blank">source</a>]


### Spring.LoadCtrlPanelConfig
---
```lua
function Spring.LoadCtrlPanelConfig(config: string) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L2767-L2770" target="_blank">source</a>]


### Spring.LoadModelTextures
---
```lua
function Spring.LoadModelTextures(modelName: string) -> success boolean?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L4656-L4660" target="_blank">source</a>]


### Spring.LoadSoundDef
---
```lua
function Spring.LoadSoundDef(soundfile: string) -> success boolean
```





Loads a SoundDefs file, the format is the same as in `gamedata/sounds.lua`.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L680-L685" target="_blank">source</a>]


### Spring.Log
---
```lua
function Spring.Log(
  section: string,
  logLevel: LogLevel?,
  ...: string
)
```
@param `logLevel` - (Default: "notice")

@param `...` - messages






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L499-L504" target="_blank">source</a>]


### Spring.MarkerAddLine
---
```lua
function Spring.MarkerAddLine(
  x1: number,
  y1: number,
  z1: number,
  x2: number,
  y2: number,
  z2: number,
  localOnly: boolean?,
  playerId: number?
) ->  nil
```
@param `localOnly` - (Default: false)






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L3811-L3821" target="_blank">source</a>]


### Spring.MarkerAddPoint
---
```lua
function Spring.MarkerAddPoint(
  x: number,
  y: number,
  z: number,
  text: string?,
  localOnly: boolean?
) ->  nil
```
@param `text` - (Default: "")






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L3782-L3789" target="_blank">source</a>]


### Spring.MarkerErasePosition
---
```lua
function Spring.MarkerErasePosition(
  x: number,
  y: number,
  z: number,
  unused: nil,
  localOnly: boolean?,
  playerId: number?,
  alwaysErase: boolean?
) ->  nil
```
@param `unused` - This argument is ignored.

@param `localOnly` - (Default: false) do not issue a network message, erase only for the current player

@param `playerId` - when not specified it uses the issuer playerId

@param `alwaysErase` - (Default: false) erase any marker when `localOnly` and current player is spectating. Allows spectators to erase players markers locally






Issue an erase command for markers on the map.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L3845-L3857" target="_blank">source</a>]


### Spring.PauseDollyCamera
---
```lua
function Spring.PauseDollyCamera(fraction: number) ->  nil
```
@param `fraction` - Fraction of the total runtime to pause at, 0 to 1 inclusive. A null value pauses at current percent






Pause Dolly Camera

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L1269-L1274" target="_blank">source</a>]


### Spring.PauseSoundStream
---
```lua
function Spring.PauseSoundStream() ->  nil
```





Pause any SoundStream currently running.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L839-L843" target="_blank">source</a>]


### Spring.Ping
---
```lua
function Spring.Ping(pingTag: number) ->  nil
```





Send a ping request to the server

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L452-L459" target="_blank">source</a>]


### Spring.PlaySoundFile
---
```lua
function Spring.PlaySoundFile(
  soundfile: string,
  volume: number?,
  posx: number?,
  posy: number?,
  posz: number?,
  speedx: number?,
  speedy: number?,
  speedz: number?,
  channel: SoundChannel?
) -> playSound boolean
```
@param `volume` - (Default: 1.0)

@param `channel` - (Default: `0|"general"`)






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L712-L723" target="_blank">source</a>]


### Spring.PlaySoundStream
---
```lua
function Spring.PlaySoundStream(
  oggfile: string,
  volume: number?,
  enqueue: boolean?
) -> success boolean
```
@param `volume` - (Default: 1.0)






Allows to play an Ogg Vorbis (.OGG) and mp3 compressed sound file.

Multiple sound streams may be played at once.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L803-L814" target="_blank">source</a>]


### Spring.Pos2BuildPos
---
```lua
function Spring.Pos2BuildPos(
  unitDefID: integer,
  posX: number,
  posY: number,
  posZ: number,
  buildFacing: number?
)
 -> buildPosX number
 -> buildPosY number
 -> buildPosZ number

```
@param `buildFacing` - (Default: 0) one of SOUTH = 0, EAST = 1, NORTH = 2, WEST  = 3






Snaps a position to the building grid

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L7817-L7828" target="_blank">source</a>]


### Spring.PreloadFeatureDefModel
---
```lua
function Spring.PreloadFeatureDefModel(featureDefID: integer) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L4624-L4628" target="_blank">source</a>]


### Spring.PreloadSoundItem
---
```lua
function Spring.PreloadSoundItem(name: string) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L4640-L4644" target="_blank">source</a>]


### Spring.PreloadUnitDefModel
---
```lua
function Spring.PreloadUnitDefModel(unitDefID: integer) ->  nil
```





Allow the engine to load the unit's model (and texture) in a background thread.
Wreckages and buildOptions of a unit are automatically preloaded.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L4605-L4612" target="_blank">source</a>]


### Spring.Quit
---
```lua
function Spring.Quit() ->  nil
```





Closes the application

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L3191-L3195" target="_blank">source</a>]


### Spring.RebuildSmoothMesh
---
```lua
function Spring.RebuildSmoothMesh() ->  nil
```





Heightmap changes normally take up to 25s to propagate to the smooth mesh.
Use to force a mapwide update immediately.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L6254-L6261" target="_blank">source</a>]


### Spring.Reload
---
```lua
function Spring.Reload(startScript: string) ->  nil
```
@param `startScript` - the CONTENT of the script.txt spring should use to start.






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L5125-L5128" target="_blank">source</a>]


### Spring.RemoveGrass
---
```lua
function Spring.RemoveGrass(
  x: number,
  z: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L4300-L4305" target="_blank">source</a>]


### Spring.RemoveObjectDecal
---
```lua
function Spring.RemoveObjectDecal(unitID: integer) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L4262-L4266" target="_blank">source</a>]


### Spring.RemoveUnitCmdDesc
---
```lua
function Spring.RemoveUnitCmdDesc(
  unitID: integer,
  cmdDescID: integer?
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L7361-L7365" target="_blank">source</a>]


### Spring.ReplaceMouseCursor
---
```lua
function Spring.ReplaceMouseCursor(
  oldFileName: string,
  newFileName: string,
  hotSpotTopLeft: boolean?
) -> assigned boolean?
```
@param `hotSpotTopLeft` - (Default: false)






Mass replace all occurrences of the cursor in all CursorCmds.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L2877-L2884" target="_blank">source</a>]


### Spring.Restart
---
```lua
function Spring.Restart(
  commandline_args: string,
  startScript: string
) ->  nil
```
@param `commandline_args` - commandline arguments passed to spring executable.






If this call returns, something went wrong

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L5135-L5142" target="_blank">source</a>]


### Spring.ResumeDollyCamera
---
```lua
function Spring.ResumeDollyCamera() ->  nil
```





Resume Dolly Camera

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L1284-L1288" target="_blank">source</a>]


### Spring.RevertHeightMap
---
```lua
function Spring.RevertHeightMap(
  x1: number,
  y1: number,
  x2_factor: number,
  y2: number?,
  factor: number?
) ->  nil
```
@param `x2_factor` - if y2 and factor are nil then this parameter is the factor






Restore original map height to a point or rectangle area on the world

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L5784-L5793" target="_blank">source</a>]


### Spring.RevertOriginalHeightMap
---
```lua
function Spring.RevertOriginalHeightMap(
  x1: number,
  y1: number,
  x2_factor: number,
  y2: number?,
  factor: number?
) ->  nil
```
@param `x2_factor` - if y2 and factor are nil then this parameter is the factor






Restore original map height cache to a point or rectangle area on the world

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L6061-L6070" target="_blank">source</a>]


### Spring.RevertSmoothMesh
---
```lua
function Spring.RevertSmoothMesh(
  x1: number,
  z1: number,
  x2: number?,
  z2: number?,
  origFactor: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L6320-L6329" target="_blank">source</a>]


### Spring.RunDollyCamera
---
```lua
function Spring.RunDollyCamera(runtime: number) ->  nil
```
@param `runtime` - Runtime in milliseconds.






Runs Dolly Camera

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L1254-L1259" target="_blank">source</a>]


### Spring.SDLSetTextInputRect
---
```lua
function Spring.SDLSetTextInputRect(
  x: number,
  y: number,
  width: number,
  height: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L5018-L5026" target="_blank">source</a>]


### Spring.SDLStartTextInput
---
```lua
function Spring.SDLStartTextInput() ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L5038-L5042" target="_blank">source</a>]


### Spring.SDLStopTextInput
---
```lua
function Spring.SDLStopTextInput() ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L5049-L5053" target="_blank">source</a>]


### Spring.SelectUnit
---
```lua
function Spring.SelectUnit(
  unitID: integer?,
  append: boolean?
) ->  nil
```
@param `append` - (Default: false) Append to current selection.






Selects a single unit

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L1447-L1453" target="_blank">source</a>]


### Spring.SelectUnitArray
---
```lua
function Spring.SelectUnitArray(
  unitMap: table<any,integer>,
  append: boolean?
) ->  nil
```
@param `unitMap` - Table with unit IDs as values.

@param `append` - (Default: false) append to current selection






Selects multiple units, or appends to selection. Accepts a table with unitIDs as values

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L1535-L1541" target="_blank">source</a>]


### Spring.SelectUnitMap
---
```lua
function Spring.SelectUnitMap(
  unitMap: table<integer,any>,
  append: boolean?
) ->  nil
```
@param `unitMap` - Table with unit IDs as keys.

@param `append` - (Default: false) append to current selection






Selects multiple units, or appends to selection. Accepts a table with unitIDs as keys

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L1547-L1553" target="_blank">source</a>]


### Spring.SendCommands
---
```lua
function Spring.SendCommands(commands: string[])
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L511-L514" target="_blank">source</a>]


### Spring.SendLuaGaiaMsg
---
```lua
function Spring.SendLuaGaiaMsg(message: string) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L3605-L3608" target="_blank">source</a>]


### Spring.SendLuaMenuMsg
---
```lua
function Spring.SendLuaMenuMsg(msg: string)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L3642-L3647" target="_blank">source</a>]


### Spring.SendLuaRulesMsg
---
```lua
function Spring.SendLuaRulesMsg(message: string) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L3624-L3627" target="_blank">source</a>]


### Spring.SendLuaUIMsg
---
```lua
function Spring.SendLuaUIMsg(
  message: string,
  mode: string
) ->  nil
```
@param `mode` - "s"/"specs" | "a"/"allies"






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L3580-L3584" target="_blank">source</a>]


### Spring.SendMessage
---
```lua
function Spring.SendMessage(message: string) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L602-L605" target="_blank">source</a>]


### Spring.SendMessageToAllyTeam
---
```lua
function Spring.SendMessageToAllyTeam(
  allyID: integer,
  message: string
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L660-L664" target="_blank">source</a>]


### Spring.SendMessageToPlayer
---
```lua
function Spring.SendMessageToPlayer(
  playerID: integer,
  message: string
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L632-L636" target="_blank">source</a>]


### Spring.SendMessageToSpectators
---
```lua
function Spring.SendMessageToSpectators(message: unknown) ->  nil
```
@param `message` - "`<PLAYER#>`"`` where `#` is a player ID.

This will be replaced with the player's name. e.g.
```lua
Spring.SendMessage("`<PLAYER1>` did something") -- "ProRusher did something"
```






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L613-L622" target="_blank">source</a>]


### Spring.SendMessageToTeam
---
```lua
function Spring.SendMessageToTeam(
  teamID: integer,
  message: string
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L646-L650" target="_blank">source</a>]


### Spring.SendSkirmishAIMessage
---
```lua
function Spring.SendSkirmishAIMessage(
  aiTeam: number,
  message: string
) -> ai_processed boolean?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L4150-L4154" target="_blank">source</a>]


### Spring.SetActiveCommand
---
```lua
function Spring.SetActiveCommand(
  action: string,
  actionExtra: string?
) -> commandSet boolean?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L2717-L2722" target="_blank">source</a>]


### Spring.SetAlly
---
```lua
function Spring.SetAlly(
  firstAllyTeamID: integer,
  secondAllyTeamID: integer,
  ally: boolean
) ->  nil
```





Changes the value of the (one-sided) alliance between: firstAllyTeamID -> secondAllyTeamID.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L823-L830" target="_blank">source</a>]


### Spring.SetAllyTeamStartBox
---
```lua
function Spring.SetAllyTeamStartBox(
  allyTeamID: integer,
  xMin: number,
  zMin: number,
  xMax: number,
  zMax: number
) ->  nil
```
@param `xMin` - left start box boundary (elmos)

@param `zMin` - top start box boundary (elmos)

@param `xMax` - right start box boundary (elmos)

@param `zMax` - bottom start box boundary (elmos)






Changes the start box position of an allyTeam.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L846-L855" target="_blank">source</a>]


### Spring.SetAtmosphere
---
```lua
function Spring.SetAtmosphere(params: AtmosphereParams)
```





It can be used to modify the following atmosphere parameters

Usage:
```lua
Spring.SetAtmosphere({ fogStart = 0, fogEnd = 0.5, fogColor = { 0.7, 0.2, 0.2, 1 }})
```

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L3896-L3906" target="_blank">source</a>]


### Spring.SetAutoShowMetal
---
```lua
function Spring.SetAutoShowMetal(autoShow: boolean) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L4231-L4234" target="_blank">source</a>]


### Spring.SetBoxSelectionByEngine
---
```lua
function Spring.SetBoxSelectionByEngine(state: boolean) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L2809-L2814" target="_blank">source</a>]


### Spring.SetBuildFacing
---
```lua
function Spring.SetBuildFacing(facing: number) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L3558-L3563" target="_blank">source</a>]


### Spring.SetBuildSpacing
---
```lua
function Spring.SetBuildSpacing(spacing: number) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L3543-L3548" target="_blank">source</a>]


### Spring.SetCameraOffset
---
```lua
function Spring.SetCameraOffset(
  posX: number?,
  posY: number?,
  posZ: number?,
  tiltX: number?,
  tiltY: number?,
  tiltZ: number?
) ->  nil
```
@param `posX` - (Default: 0)

@param `posY` - (Default: 0)

@param `posZ` - (Default: 0)

@param `tiltX` - (Default: 0)

@param `tiltY` - (Default: 0)

@param `tiltZ` - (Default: 0)






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L1184-L1195" target="_blank">source</a>]


### Spring.SetCameraState
---
```lua
function Spring.SetCameraState(
  camState: camState,
  transitionTime: number?,
  transitionTimeFactor: number?,
  transitionTimeExponent: number?
) -> set boolean
```
@param `transitionTime` - (Default: 0) in nanoseconds

@param `transitionTimeFactor` - multiplicative factor applied to this and all subsequent transition times for
this camera mode.

Defaults to "CamTimeFactor" springsetting unless set previously.

@param `transitionTimeExponent` - tween factor applied to this and all subsequent transitions for this camera
mode.

Defaults to "CamTimeExponent" springsetting unless set previously.






Sets camera state

The fields in `camState` must be consistent with the name/mode and current/new camera mode

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L1209-L1231" target="_blank">source</a>]


### Spring.SetCameraTarget
---
```lua
function Spring.SetCameraTarget(
  x: number,
  y: number,
  z: number,
  transTime: number?
) ->  nil
```





For Spring Engine XZ represents horizontal, from north west corner of map and Y vertical, from water level and rising.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L1146-L1155" target="_blank">source</a>]


### Spring.SetClipboard
---
```lua
function Spring.SetClipboard(text: string) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L5240-L5243" target="_blank">source</a>]


### Spring.SetConfigFloat
---
```lua
function Spring.SetConfigFloat(
  name: string,
  value: number,
  useOverla: boolean?
) ->  nil
```
@param `useOverla` - (Default: false) the value will only be set in memory, and not be restored for the next game.y






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L3090-L3097" target="_blank">source</a>]


### Spring.SetConfigInt
---
```lua
function Spring.SetConfigInt(
  name: string,
  value: number,
  useOverlay: boolean?
) ->  nil
```
@param `useOverlay` - (Default: false) the value will only be set in memory, and not be restored for the next game.






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L3062-L3069" target="_blank">source</a>]


### Spring.SetConfigString
---
```lua
function Spring.SetConfigString(
  name: string,
  value: number,
  useOverlay: boolean?
) ->  nil
```
@param `useOverlay` - (Default: false) the value will only be set in memory, and not be restored for the next game.






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L3113-L3120" target="_blank">source</a>]


### Spring.SetCustomCommandDrawData
---
```lua
function Spring.SetCustomCommandDrawData(cmdID: integer) -> assigned boolean?
```





Register your custom cmd so it gets visible in the unit's cmd queue

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L2902-L2908" target="_blank">source</a>]


### Spring.SetDollyCameraCurve
---
```lua
function Spring.SetDollyCameraCurve(
  degree: number,
  cpoints: ControlPoint[],
  knots: table
) ->  nil
```
@param `cpoints` - NURBS control point positions.






Sets Dolly Camera movement Curve

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L1326-L1333" target="_blank">source</a>]


### Spring.SetDollyCameraLookCurve
---
```lua
function Spring.SetDollyCameraLookCurve(
  degree: number,
  cpoints: ControlPoint[],
  knots: table
) ->  nil
```
@param `cpoints` - NURBS control point positions.






Sets Dolly Camera Look Curve

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L1380-L1387" target="_blank">source</a>]


### Spring.SetDollyCameraLookPosition
---
```lua
function Spring.SetDollyCameraLookPosition(
  x: number,
  y: number,
  z: number
) ->  nil
```





Sets Dolly Camera Look Position

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L1404-L1411" target="_blank">source</a>]


### Spring.SetDollyCameraLookUnit
---
```lua
function Spring.SetDollyCameraLookUnit(unitID: integer) ->  nil
```
@param `unitID` - The unit to look at.






Sets target unit for Dolly Camera to look towards

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L1424-L1429" target="_blank">source</a>]


### Spring.SetDollyCameraMode
---
```lua
function Spring.SetDollyCameraMode(mode: (1|2)) ->  nil
```
@param `mode` - `1` static position, `2` nurbs curve






Sets Dolly Camera movement mode

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L1349-L1354" target="_blank">source</a>]


### Spring.SetDollyCameraPosition
---
```lua
function Spring.SetDollyCameraPosition(
  x: number,
  y: number,
  z: number
) ->  nil
```





Sets Dolly Camera Position

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L1296-L1303" target="_blank">source</a>]


### Spring.SetDollyCameraRelativeMode
---
```lua
function Spring.SetDollyCameraRelativeMode(relativeMode: unknown) ->  nil
```
@param `relativeMode` - world, `2` look target






Sets Dolly Camera movement curve to world relative or look target relative

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L1364-L1369" target="_blank">source</a>]


### Spring.SetDrawGround
---
```lua
function Spring.SetDrawGround(drawGround: boolean) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L4264-L4267" target="_blank">source</a>]


### Spring.SetDrawGroundDeferred
---
```lua
function Spring.SetDrawGroundDeferred(
  drawGroundDeferred: boolean,
  drawGroundForward: boolean?
) ->  nil
```
@param `drawGroundForward` - allows disabling of the forward pass






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L4275-L4279" target="_blank">source</a>]


### Spring.SetDrawModelsDeferred
---
```lua
function Spring.SetDrawModelsDeferred(
  drawUnitsDeferred: boolean,
  drawFeaturesDeferred: boolean,
  drawUnitsForward: boolean?,
  drawFeaturesForward: boolean?
) ->  nil
```
@param `drawUnitsForward` - allows disabling of the respective forward passes

@param `drawFeaturesForward` - allows disabling of the respective forward passes






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L4292-L4298" target="_blank">source</a>]


### Spring.SetDrawSelectionInfo
---
```lua
function Spring.SetDrawSelectionInfo(enable: boolean) ->  nil
```





Disables the "Selected Units x" box in the GUI.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L2794-L2799" target="_blank">source</a>]


### Spring.SetDrawSky
---
```lua
function Spring.SetDrawSky(drawSky: boolean) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L4242-L4245" target="_blank">source</a>]


### Spring.SetDrawWater
---
```lua
function Spring.SetDrawWater(drawWater: boolean) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L4253-L4256" target="_blank">source</a>]


### Spring.SetExperienceGrade
---
```lua
function Spring.SetExperienceGrade(
  expGrade: number,
  ExpPowerScale: number?,
  ExpHealthScale: number?,
  ExpReloadScale: number?
) ->  nil
```





Defines how often `Callins.UnitExperience` will be called.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L7123-L7131" target="_blank">source</a>]


### Spring.SetFactoryBuggerOff
---
```lua
function Spring.SetFactoryBuggerOff(
  unitID: integer,
  buggerOff: boolean?,
  offset: number?,
  radius: number?,
  relHeading: number?,
  spherical: boolean?,
  forced: boolean?
) -> buggerOff (nil|number)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L3963-L3974" target="_blank">source</a>]


### Spring.SetFeatureAlwaysUpdateMatrix
---
```lua
function Spring.SetFeatureAlwaysUpdateMatrix(
  featureID: integer,
  alwaysUpdateMat: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L2322-L2328" target="_blank">source</a>]


### Spring.SetFeatureAlwaysVisible
---
```lua
function Spring.SetFeatureAlwaysVisible(
  featureID: integer,
  enable: boolean
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L4455-L4460" target="_blank">source</a>]


### Spring.SetFeatureBlocking
---
```lua
function Spring.SetFeatureBlocking(
  featureID: integer,
  isBlocking: boolean,
  isSolidObjectCollidable: boolean,
  isProjectileCollidable: boolean,
  isRaySegmentCollidable: boolean,
  crushable: boolean,
  blockEnemyPushing: boolean,
  blockHeightChanges: boolean
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L4797-L4808" target="_blank">source</a>]


### Spring.SetFeatureCollisionVolumeData
---
```lua
function Spring.SetFeatureCollisionVolumeData(
  featureID: integer,
  scaleX: number,
  scaleY: number,
  scaleZ: number,
  offsetX: number,
  offsetY: number,
  offsetZ: number,
  vType: number,
  tType: number,
  Axis: number
) ->  nil
```





Check `Spring.SetUnitCollisionVolumeData` for further explanation of the arguments.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L4919-L4935" target="_blank">source</a>]


### Spring.SetFeatureDirection
---
```lua
function Spring.SetFeatureDirection(
  featureID: integer,
  frontx: number,
  fronty: number,
  frontz: number
) ->  nil
```





Set feature front direction vector. The vector is normalized in
the engine.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L4733-L4746" target="_blank">source</a>]


### Spring.SetFeatureEngineDrawMask
---
```lua
function Spring.SetFeatureEngineDrawMask(
  featureID: integer,
  engineDrawMask: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L2303-L2309" target="_blank">source</a>]


### Spring.SetFeatureFade
---
```lua
function Spring.SetFeatureFade(
  featureID: integer,
  allow: boolean
) ->  nil
```





Control whether a feature will fade or not when zoomed out.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L2341-L2349" target="_blank">source</a>]


### Spring.SetFeatureHeadingAndUpDir
---
```lua
function Spring.SetFeatureHeadingAndUpDir(
  featureID: integer,
  heading: number,
  upx: number,
  upy: number,
  upz: number
) ->  nil
```





Use this call to set up feature direction in a robust way. Heading (-32768 to 32767) represents a 2D (xz plane) feature orientation if feature was completely upright, new {upx,upy,upz} direction will be used as new "up" vector, the rotation set by "heading" will remain preserved.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L4768-L4777" target="_blank">source</a>]


### Spring.SetFeatureHealth
---
```lua
function Spring.SetFeatureHealth(
  featureID: integer,
  health: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L4479-L4484" target="_blank">source</a>]


### Spring.SetFeatureMass
---
```lua
function Spring.SetFeatureMass(
  featureID: integer,
  mass: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L4680-L4685" target="_blank">source</a>]


### Spring.SetFeatureMaxHealth
---
```lua
function Spring.SetFeatureMaxHealth(
  featureID: integer,
  maxHealth: number
) ->  nil
```
@param `maxHealth` - minimum 0.1






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L4497-L4503" target="_blank">source</a>]


### Spring.SetFeatureMidAndAimPos
---
```lua
function Spring.SetFeatureMidAndAimPos(
  featureID: integer,
  mpX: number,
  mpY: number,
  mpZ: number,
  apX: number,
  apY: number,
  apZ: number,
  relative: boolean?
) -> success boolean
```





Check `Spring.SetUnitMidAndAimPos` for further explanation of the arguments.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L4833-L4847" target="_blank">source</a>]


### Spring.SetFeatureMoveCtrl
---
```lua
function Spring.SetFeatureMoveCtrl(
  featureID: integer,
  enable: boolean?,
  arg1: number?,
  arg2: number?,
  argn: number?
) ->  nil
```





Use this callout to control feature movement. The arg* arguments are parsed as follows and all optional:

If enable is true:
[, velVector(x,y,z)  * initial velocity for feature
[, accVector(x,y,z)  * acceleration added every frame]]

If enable is false:
[, velocityMask(x,y,z)  * dimensions in which velocity is allowed to build when not using MoveCtrl
[, impulseMask(x,y,z)  * dimensions in which impulse is allowed to apply when not using MoveCtrl
[, movementMask(x,y,z)  * dimensions in which feature is allowed to move when not using MoveCtrl]]]

It is necessary to unlock feature movement on x,z axis before changing feature physics.

For example use `Spring.SetFeatureMoveCtrl(featureID,false,1,1,1,1,1,1,1,1,1)` to unlock all movement prior to making `Spring.SetFeatureVelocity` calls.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L4600-L4626" target="_blank">source</a>]


### Spring.SetFeatureNoDraw
---
```lua
function Spring.SetFeatureNoDraw(
  featureID: integer,
  noDraw: boolean
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L2282-L2290" target="_blank">source</a>]


### Spring.SetFeatureNoSelect
---
```lua
function Spring.SetFeatureNoSelect(
  featureID: integer,
  noSelect: boolean
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L4815-L4820" target="_blank">source</a>]


### Spring.SetFeaturePhysics
---
```lua
function Spring.SetFeaturePhysics(
  featureID: integer,
  posX: number,
  posY: number,
  posZ: number,
  velX: number,
  velY: number,
  velZ: number,
  rotX: number,
  rotY: number,
  rotZ: number,
  dragX: number,
  dragY: number,
  dragZ: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L4657-L4673" target="_blank">source</a>]


### Spring.SetFeaturePieceCollisionVolumeData
---
```lua
function Spring.SetFeaturePieceCollisionVolumeData(
  featureID: integer,
  pieceIndex: number,
  enable: boolean,
  scaleX: number,
  scaleY: number,
  scaleZ: number,
  offsetX: number,
  offsetY: number,
  offsetZ: number,
  Axis: number,
  volumeType: number,
  primaryAxis: number?
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L4942-L4957" target="_blank">source</a>]


### Spring.SetFeaturePieceVisible
---
```lua
function Spring.SetFeaturePieceVisible(
  featureID: integer,
  pieceIndex: number,
  visible: boolean
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L4963-L4970" target="_blank">source</a>]


### Spring.SetFeaturePosition
---
```lua
function Spring.SetFeaturePosition(
  featureID: integer,
  x: number,
  y: number,
  z: number,
  snapToGround: boolean?
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L4692-L4700" target="_blank">source</a>]


### Spring.SetFeatureRadiusAndHeight
---
```lua
function Spring.SetFeatureRadiusAndHeight(
  featureID: integer,
  radius: number,
  height: number
) -> success boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L4884-L4890" target="_blank">source</a>]


### Spring.SetFeatureReclaim
---
```lua
function Spring.SetFeatureReclaim(
  featureID: integer,
  reclaimLeft: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L4517-L4522" target="_blank">source</a>]


### Spring.SetFeatureResources
---
```lua
function Spring.SetFeatureResources(
  featureID: integer,
  metal: number,
  energy: number,
  reclaimTime: number?,
  reclaimLeft: number?,
  featureDefMetal: number?,
  featureDefEnergy: number?
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L4534-L4544" target="_blank">source</a>]


### Spring.SetFeatureResurrect
---
```lua
function Spring.SetFeatureResurrect(
  featureID: integer,
  unitDef: (string|integer),
  facing: Facing?,
  progress: number?
) ->  nil
```
@param `unitDef` - Can be a number id or a string name, this allows cancelling resurrection by passing `-1`.

@param `facing` - (Default: `"south"`)

@param `progress` - Set the level of progress.






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L4563-L4571" target="_blank">source</a>]


### Spring.SetFeatureRotation
---
```lua
function Spring.SetFeatureRotation(
  featureID: integer,
  pitch: number,
  yaw: number,
  roll: number
) ->  nil
```
@param `pitch` - Rotation in X axis

@param `yaw` - Rotation in Y axis

@param `roll` - Rotation in Z axis






Note: PYR order

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L4718-L4726" target="_blank">source</a>]


### Spring.SetFeatureRulesParam
---
```lua
function Spring.SetFeatureRulesParam(
  featureID: integer,
  paramName: string,
  paramValue: (number|string)?,
  losAccess: losAccess?
) ->  nil
```
@param `paramValue` - numeric paramValues in quotes will be converted to number.






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L1487-L1494" target="_blank">source</a>]


### Spring.SetFeatureSelectionVolumeData
---
```lua
function Spring.SetFeatureSelectionVolumeData(
  featureID: integer,
  scaleX: number,
  scaleY: number,
  scaleZ: number,
  offsetX: number,
  offsetY: number,
  offsetZ: number,
  vType: number,
  tType: number,
  Axis: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L2362-L2377" target="_blank">source</a>]


### Spring.SetFeatureUseAirLos
---
```lua
function Spring.SetFeatureUseAirLos(
  featureID: integer,
  useAirLos: boolean
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L4466-L4472" target="_blank">source</a>]


### Spring.SetFeatureVelocity
---
```lua
function Spring.SetFeatureVelocity(
  featureID: integer,
  velX: number,
  velY: number,
  velZ: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L4783-L4790" target="_blank">source</a>]


### Spring.SetGameRulesParam
---
```lua
function Spring.SetGameRulesParam(
  paramName: string,
  paramValue: (number|string)?,
  losAccess: losAccess?
) ->  nil
```
@param `paramValue` - numeric paramValues in quotes will be converted to number.






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L1411-L1417" target="_blank">source</a>]


### Spring.SetGlobalLos
---
```lua
function Spring.SetGlobalLos(
  allyTeamID: integer,
  globallos: boolean
) ->  nil
```





Changes access to global line of sight for a team and its allies.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L906-L912" target="_blank">source</a>]


### Spring.SetGroundDecalAlpha
---
```lua
function Spring.SetGroundDecalAlpha(
  decalID: integer,
  alpha: number?,
  alphaFalloff: number?
) -> decalSet boolean
```
@param `alpha` - (Default: currAlpha) Between 0 and 1

@param `alphaFalloff` - (Default: currAlphaFalloff) Between 0 and 1, per second






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L4868-L4875" target="_blank">source</a>]


### Spring.SetGroundDecalCreationFrame
---
```lua
function Spring.SetGroundDecalCreationFrame(
  decalID: integer,
  creationFrameMin: number?,
  creationFrameMax: number?
) -> decalSet boolean
```
@param `creationFrameMin` - (Default: currCreationFrameMin)

@param `creationFrameMax` - (Default: currCreationFrameMax)






Use separate min and max for "gradient" style decals such as tank tracks

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L4985-L4995" target="_blank">source</a>]


### Spring.SetGroundDecalMisc
---
```lua
function Spring.SetGroundDecalMisc(
  decalID: integer,
  dotElimExp: number?,
  refHeight: number?,
  minHeight: number?,
  maxHeight: number?,
  forceHeightMode: number?
) -> decalSet boolean
```
@param `dotElimExp` - (Default: curValue) pow(max(dot(decalProjVector, SurfaceNormal), 0.0), dotElimExp), used to reduce decal artifacts on surfaces non-collinear with the projection vector

@param `refHeight` - (Default: curValue)

@param `minHeight` - (Default: curValue)

@param `maxHeight` - (Default: curValue)

@param `forceHeightMode` - (Default: curValue) in case forceHeightMode==1.0 ==> force relative height: midPoint.y = refHeight + clamp(midPoint.y - refHeight, minHeight); forceHeightMode==2.0 ==> force absolute height: midPoint.y = midPoint.y, clamp(midPoint.y, minHeight, maxHeight); other forceHeightMode values do not enforce the height of the center position






Sets varios secondary parameters of a decal

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L4955-L4966" target="_blank">source</a>]


### Spring.SetGroundDecalNormal
---
```lua
function Spring.SetGroundDecalNormal(
  decalID: integer,
  normalX: number?,
  normalY: number?,
  normalZ: number?
) -> decalSet boolean
```
@param `normalX` - (Default: 0)

@param `normalY` - (Default: 0)

@param `normalZ` - (Default: 0)






Sets projection cube normal to orient in 3D space.
In case the normal (0,0,0) then normal is picked from the terrain

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L4891-L4901" target="_blank">source</a>]


### Spring.SetGroundDecalPosAndDims
---
```lua
function Spring.SetGroundDecalPosAndDims(
  decalID: integer,
  midPosX: number?,
  midPosZ: number?,
  sizeX: number?,
  sizeZ: number?,
  projCubeHeight: number?
) -> decalSet boolean
```
@param `midPosX` - (Default: currMidPosX)

@param `midPosZ` - (Default: currMidPosZ)

@param `sizeX` - (Default: currSizeX)

@param `sizeZ` - (Default: currSizeZ)

@param `projCubeHeight` - (Default: calculateProjCubeHeight)






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L4721-L4731" target="_blank">source</a>]


### Spring.SetGroundDecalQuadPosAndHeight
---
```lua
function Spring.SetGroundDecalQuadPosAndHeight(
  decalID: integer,
  posTL: xz?,
  posTR: xz?,
  posBR: xz?,
  posBL: xz?,
  projCubeHeight: number?
) -> decalSet boolean
```
@param `posTL` - (Default: currPosTL)

@param `posTR` - (Default: currPosTR)

@param `posBR` - (Default: currPosBR)

@param `posBL` - (Default: currPosBL)

@param `projCubeHeight` - (Default: calculateProjCubeHeight)






Use for non-rectangular decals

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L4770-L4783" target="_blank">source</a>]


### Spring.SetGroundDecalRotation
---
```lua
function Spring.SetGroundDecalRotation(
  decalID: integer,
  rot: number?
) -> decalSet boolean
```
@param `rot` - (Default: random) in radians






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L4806-L4812" target="_blank">source</a>]


### Spring.SetGroundDecalTexture
---
```lua
function Spring.SetGroundDecalTexture(
  decalID: integer,
  textureName: string,
  isMainTex: boolean?
) -> decalSet (nil|boolean)
```
@param `textureName` - The texture has to be on the atlas which seems to mean it's defined as an explosion, unit tracks, or building plate decal on some unit already (no arbitrary textures)

@param `isMainTex` - (Default: true) If false, it sets the normals/glow map






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L4828-L4835" target="_blank">source</a>]


### Spring.SetGroundDecalTextureParams
---
```lua
function Spring.SetGroundDecalTextureParams(
  decalID: integer,
  texWrapDistance: number?,
  texTraveledDistance: number?
) -> decalSet (nil|boolean)
```
@param `texWrapDistance` - (Default: currTexWrapDistance) if non-zero sets the mode to repeat the texture along the left-right direction of the decal every texWrapFactor elmos

@param `texTraveledDistance` - (Default: currTexTraveledDistance) shifts the texture repetition defined by texWrapFactor so the texture of a next line in the continuous multiline can start where the previous finished. For that it should collect all elmo lengths of the previously set multiline segments.






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L4844-L4851" target="_blank">source</a>]


### Spring.SetGroundDecalTint
---
```lua
function Spring.SetGroundDecalTint(
  decalID: integer,
  tintColR: number?,
  tintColG: number?,
  tintColB: number?,
  tintColA: number?
) -> decalSet boolean
```
@param `tintColR` - (Default: curTintColR)

@param `tintColG` - (Default: curTintColG)

@param `tintColB` - (Default: curTintColB)

@param `tintColA` - (Default: curTintColA)






Sets the tint of the ground decal. Color = 2 * textureColor * tintColor
Respectively a color of (0.5, 0.5, 0.5, 0.5) is effectively no tint

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L4923-L4934" target="_blank">source</a>]


### Spring.SetHeightMap
---
```lua
function Spring.SetHeightMap(
  x: number,
  z: number,
  height: number,
  terraform: number?
) -> absHeightDiff integer?
```
@param `terraform` - (Default: 1) Scaling factor.


@return `absHeightDiff` - If `0`, nothing will be changed (the terraform starts), if `1` the terraform will be finished.





Can only be called in `Spring.SetHeightMapFunc`. The terraform argument is

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L5877-L5889" target="_blank">source</a>]


### Spring.SetHeightMapFunc
---
```lua
function Spring.SetHeightMapFunc(
  luaFunction: function,
  arg: number,
  ...: number
) -> absTotalHeightMapAmountChanged integer?
```





Example code:

```lua
function Spring.SetHeightMapFunc(function()
  for z=0,Game.mapSizeZ, Game.squareSize do
    for x=0,Game.mapSizeX, Game.squareSize do
      Spring.SetHeightMap( x, z, 200 + 20 * math.cos((x + z) / 90) )
    end
  end
end)
```

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L5936-L5955" target="_blank">source</a>]


### Spring.SetLastMessagePosition
---
```lua
function Spring.SetLastMessagePosition(
  x: number,
  y: number,
  z: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L3759-L3764" target="_blank">source</a>]


### Spring.SetLogSectionFilterLevel
---
```lua
function Spring.SetLogSectionFilterLevel(
  sectionName: string,
  logLevel: (string|number)?
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L4184-L4188" target="_blank">source</a>]


### Spring.SetLosViewColors
---
```lua
function Spring.SetLosViewColors(
  always: rgb,
  LOS: rgb,
  radar: rgb,
  jam: rgb,
  radar2: rgb
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L2982-L2989" target="_blank">source</a>]


### Spring.SetMapLightTrackingState
---
```lua
function Spring.SetMapLightTrackingState(
  lightHandle: number,
  unitOrProjectileID: integer,
  enableTracking: boolean,
  unitOrProjectile: boolean
) -> success boolean
```





Set a map-illuminating light to start/stop tracking the position of a moving object (unit or projectile)

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L1857-L1866" target="_blank">source</a>]


### Spring.SetMapRenderingParams
---
```lua
function Spring.SetMapRenderingParams(params: MapRenderingParams) ->  nil
```





Allows to change map rendering params at runtime.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L4046-L4051" target="_blank">source</a>]


### Spring.SetMapShader
---
```lua
function Spring.SetMapShader(
  standardShaderID: integer,
  deferredShaderID: integer
) ->  nil
```





The ID's must refer to valid programs returned by `gl.CreateShader`.
Passing in a value of 0 will cause the respective shader to revert back to its engine default.
Custom map shaders that declare a uniform ivec2 named "texSquare" can sample from the default diffuse texture(s), which are always bound to TU 0.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L1935-L1944" target="_blank">source</a>]


### Spring.SetMapShadingTexture
---
```lua
function Spring.SetMapShadingTexture(
  texType: string,
  texName: string
) -> success boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L2061-L2066" target="_blank">source</a>]


### Spring.SetMapSquareTerrainType
---
```lua
function Spring.SetMapSquareTerrainType(
  x: number,
  z: number,
  newType: number
) -> oldType integer?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L6490-L6496" target="_blank">source</a>]


### Spring.SetMapSquareTexture
---
```lua
function Spring.SetMapSquareTexture(
  texSqrX: number,
  texSqrY: number,
  luaTexName: string
) -> success boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L1964-L1969" target="_blank">source</a>]


### Spring.SetMetalAmount
---
```lua
function Spring.SetMetalAmount(
  x: integer,
  z: integer,
  metalAmount: number
) ->  nil
```
@param `x` - in worldspace/16.

@param `z` - in worldspace/16.

@param `metalAmount` - must be between 0 and 255*maxMetal (with maxMetal from the .smd or mapinfo.lua).






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaMetalMap.cpp#L52-L58" target="_blank">source</a>]


### Spring.SetModelLightTrackingState
---
```lua
function Spring.SetModelLightTrackingState(
  lightHandle: number,
  unitOrProjectileID: integer,
  enableTracking: boolean,
  unitOrProjectile: boolean
) -> success boolean
```





Set a model-illuminating light to start/stop tracking the position of a moving object (unit or projectile)

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L1893-L1902" target="_blank">source</a>]


### Spring.SetMouseCursor
---
```lua
function Spring.SetMouseCursor(
  cursorName: string,
  cursorScale: number?
) ->  nil
```
@param `cursorScale` - (Default: 1.0)






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L2961-L2965" target="_blank">source</a>]


### Spring.SetNanoProjectileParams
---
```lua
function Spring.SetNanoProjectileParams(
  rotVal: number?,
  rotVel: number?,
  rotAcc: number?,
  rotValRng: number?,
  rotVelRng: number?,
  rotAccRng: number?
) ->  nil
```
@param `rotVal` - (Default: 0) in degrees

@param `rotVel` - (Default: 0) in degrees

@param `rotAcc` - (Default: 0) in degrees

@param `rotValRng` - (Default: 0) in degrees

@param `rotVelRng` - (Default: 0) in degrees

@param `rotAccRng` - (Default: 0) in degrees






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L3028-L3038" target="_blank">source</a>]


### Spring.SetNoPause
---
```lua
function Spring.SetNoPause(noPause: boolean) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L7105-L7109" target="_blank">source</a>]


### Spring.SetOriginalHeightMap
---
```lua
function Spring.SetOriginalHeightMap(
  x: number,
  y: number,
  height: number,
  factor: number?
) ->  nil
```





Can only be called in `Spring.SetOriginalHeightMapFunc`

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L6150-L6161" target="_blank">source</a>]


### Spring.SetOriginalHeightMapFunc
---
```lua
function Spring.SetOriginalHeightMapFunc(heightMapFunc: function) ->  nil
```





Cannot recurse on itself

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L6202-L6210" target="_blank">source</a>]


### Spring.SetPieceProjectileParams
---
```lua
function Spring.SetPieceProjectileParams(
  projectileID: integer,
  explosionFlags: number?,
  spinAngle: number?,
  spinSpeed: number?,
  spinVectorX: number?,
  spinVectorY: number?,
  spinVectorZ: number?
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L5317-L5327" target="_blank">source</a>]


### Spring.SetPlayerRulesParam
---
```lua
function Spring.SetPlayerRulesParam(
  playerID: integer,
  paramName: string,
  paramValue: (number|string)?,
  losAccess: losAccess?
) ->  nil
```
@param `paramValue` - numeric paramValues in quotes will be converted to number.






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L1443-L1450" target="_blank">source</a>]


### Spring.SetProjectileAlwaysVisible
---
```lua
function Spring.SetProjectileAlwaysVisible(
  projectileID: integer,
  alwaysVisible: boolean
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L4999-L5004" target="_blank">source</a>]


### Spring.SetProjectileCEG
---
```lua
function Spring.SetProjectileCEG(
  projectileID: integer,
  ceg_name: string
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L5349-L5354" target="_blank">source</a>]


### Spring.SetProjectileCollision
---
```lua
function Spring.SetProjectileCollision(projectileID: integer) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L5080-L5084" target="_blank">source</a>]


### Spring.SetProjectileDamages
---
```lua
function Spring.SetProjectileDamages(
  unitID: integer,
  weaponNum: number,
  key: string,
  value: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L5227-L5234" target="_blank">source</a>]


### Spring.SetProjectileGravity
---
```lua
function Spring.SetProjectileGravity(
  projectileID: integer,
  grav: number?
) ->  nil
```
@param `grav` - (Default: 0)






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L5294-L5299" target="_blank">source</a>]


### Spring.SetProjectileIgnoreTrackingError
---
```lua
function Spring.SetProjectileIgnoreTrackingError(
  projectileID: integer,
  ignore: boolean
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L5264-L5269" target="_blank">source</a>]


### Spring.SetProjectileIsIntercepted
---
```lua
function Spring.SetProjectileIsIntercepted(projectileID: integer) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L5207-L5211" target="_blank">source</a>]


### Spring.SetProjectileMoveControl
---
```lua
function Spring.SetProjectileMoveControl(
  projectileID: integer,
  enable: boolean
) ->  nil
```





Disables engine movecontrol, so lua can fully control the physics.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L5024-L5030" target="_blank">source</a>]


### Spring.SetProjectilePosition
---
```lua
function Spring.SetProjectilePosition(
  projectileID: integer,
  posX: number?,
  posY: number?,
  posZ: number?
) ->  nil
```
@param `posX` - (Default: 0)

@param `posY` - (Default: 0)

@param `posZ` - (Default: 0)






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L5044-L5051" target="_blank">source</a>]


### Spring.SetProjectileTarget
---
```lua
function Spring.SetProjectileTarget(
  projectileID: integer,
  arg1: number?,
  arg2: number?,
  posZ: number?
) -> validTarget boolean?
```
@param `arg1` - (Default: 0) targetID or posX

@param `arg2` - (Default: 0) targetType or posY

@param `posZ` - (Default: 0)






targetTypeStr can be one of:
'u' - unit
'f' - feature
'p' - projectile
while targetTypeInt is one of:
string.byte('g') := GROUND
string.byte('u') := UNIT
string.byte('f') := FEATURE
string.byte('p') := PROJECTILE

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L5096-L5114" target="_blank">source</a>]


### Spring.SetProjectileTimeToLive
---
```lua
function Spring.SetProjectileTimeToLive(
  projectileID: integer,
  ttl: number
) ->  nil
```
@param `ttl` - Remaining time to live in frames






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L5185-L5190" target="_blank">source</a>]


### Spring.SetProjectileUseAirLos
---
```lua
function Spring.SetProjectileUseAirLos(
  projectileID: integer,
  useAirLos: boolean
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L5011-L5017" target="_blank">source</a>]


### Spring.SetProjectileVelocity
---
```lua
function Spring.SetProjectileVelocity(
  projectileID: integer,
  velX: number?,
  velY: number?,
  velZ: number?
) ->  nil
```
@param `velX` - (Default: 0)

@param `velY` - (Default: 0)

@param `velZ` - (Default: 0)






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L5066-L5074" target="_blank">source</a>]


### Spring.SetRadarErrorParams
---
```lua
function Spring.SetRadarErrorParams(
  allyTeamID: integer,
  allyteamErrorSize: number,
  baseErrorSize: number?,
  baseErrorMult: number?
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L7154-L7162" target="_blank">source</a>]


### Spring.SetShareLevel
---
```lua
function Spring.SetShareLevel(
  resource: string,
  shareLevel: number
) ->  nil
```
@param `resource` - metal | energy






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L3663-L3670" target="_blank">source</a>]


### Spring.SetSkyBoxTexture
---
```lua
function Spring.SetSkyBoxTexture(texName: string) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L2082-L2085" target="_blank">source</a>]


### Spring.SetSmoothMesh
---
```lua
function Spring.SetSmoothMesh(
  x: number,
  z: number,
  height: number,
  terraform: number?
) -> The number?
```
@param `terraform` - (Default: 1)


@return `The` - absolute height difference, or `nil` if coordinates are invalid.





Can only be called in `Spring.SetSmoothMeshFunc`.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L6401-L6409" target="_blank">source</a>]


### Spring.SetSmoothMeshFunc
---
```lua
function Spring.SetSmoothMeshFunc(
  luaFunction: function,
  arg: any?,
  ...: any?
) -> absTotalHeightMapAmountChanged number?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L6449-L6455" target="_blank">source</a>]


### Spring.SetSoundEffectParams
---
```lua
function Spring.SetSoundEffectParams()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L864-L865" target="_blank">source</a>]


### Spring.SetSoundStreamVolume
---
```lua
function Spring.SetSoundStreamVolume(volume: number) ->  nil
```





Set volume for SoundStream

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L851-L856" target="_blank">source</a>]


### Spring.SetSquareBuildingMask
---
```lua
function Spring.SetSquareBuildingMask(
  x: number,
  z: number,
  mask: number
) -> See nil
```

@return `See` - also buildingMask unitdef tag.





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L6567-L6575" target="_blank">source</a>]


### Spring.SetSunDirection
---
```lua
function Spring.SetSunDirection(
  dirX: number,
  dirY: number,
  dirZ: number,
  intensity: number?
) ->  nil
```
@param `intensity` - (Default: `1.0`)






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L3970-L3978" target="_blank">source</a>]


### Spring.SetSunLighting
---
```lua
function Spring.SetSunLighting(params: { groundDiffuseColor: rgb,groundAmbientColor: rgb })
```





Modify sun lighting parameters.

```lua
Spring.SetSunLighting({groundAmbientColor = {1, 0.1, 1}, groundDiffuseColor = {1, 0.1, 1} })
```

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L3988-L3997" target="_blank">source</a>]


### Spring.SetTeamColor
---
```lua
function Spring.SetTeamColor(
  teamID: integer,
  r: number,
  g: number,
  b: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L2823-L2831" target="_blank">source</a>]


### Spring.SetTeamResource
---
```lua
function Spring.SetTeamResource(
  teamID: integer,
  resource: (ResourceName|StorageName),
  amount: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L1166-L1172" target="_blank">source</a>]


### Spring.SetTeamRulesParam
---
```lua
function Spring.SetTeamRulesParam(
  teamID: integer,
  paramName: string,
  paramValue: (number|string)?,
  losAccess: losAccess?
) ->  nil
```
@param `paramValue` - numeric paramValues in quotes will be converted to number.






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L1425-L1432" target="_blank">source</a>]


### Spring.SetTeamShareLevel
---
```lua
function Spring.SetTeamShareLevel(
  teamID: integer,
  type: ResourceName,
  amount: number
) ->  nil
```





Changes the resource amount for a team beyond which resources aren't stored but transferred to other allied teams if possible.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L1216-L1223" target="_blank">source</a>]


### Spring.SetTerrainTypeData
---
```lua
function Spring.SetTerrainTypeData(
  typeIndex: number,
  speedTanks: number?,
  speedKBOts: number?,
  speedHovers: number?,
  speedShips: number?
) ->  boolean?
```
@param `speedTanks` - (Default: nil)

@param `speedKBOts` - (Default: nil)

@param `speedHovers` - (Default: nil)

@param `speedShips` - (Default: nil)


@return  - true





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L6520-L6528" target="_blank">source</a>]


### Spring.SetTidal
---
```lua
function Spring.SetTidal(strength: number) ->  nil
```





Set tidal strength

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L1008-L1013" target="_blank">source</a>]


### Spring.SetUnitAlwaysUpdateMatrix
---
```lua
function Spring.SetUnitAlwaysUpdateMatrix(
  unitID: integer,
  alwaysUpdateMatrix: boolean
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L2142-L2148" target="_blank">source</a>]


### Spring.SetUnitAlwaysVisible
---
```lua
function Spring.SetUnitAlwaysVisible(
  unitID: integer,
  alwaysVisible: boolean
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L2830-L2835" target="_blank">source</a>]


### Spring.SetUnitArmored
---
```lua
function Spring.SetUnitArmored(
  unitID: integer,
  armored: boolean?,
  armorMultiple: number?
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L2580-L2586" target="_blank">source</a>]


### Spring.SetUnitBlocking
---
```lua
function Spring.SetUnitBlocking(
  unitID: integer,
  isblocking: boolean,
  isSolidObjectCollidable: boolean,
  isProjectileCollidable: boolean,
  isRaySegmentCollidable: boolean,
  crushable: boolean,
  blockEnemyPushing: boolean,
  blockHeightChanges: boolean
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L3048-L3059" target="_blank">source</a>]


### Spring.SetUnitBuildParams
---
```lua
function Spring.SetUnitBuildParams(
  unitID: integer,
  paramName: string,
  value: (number|boolean)
) ->  nil
```
@param `paramName` - one of `buildRange`|`buildDistance`|`buildRange3D`

@param `value` - boolean when `paramName` is `buildRange3D`, otherwise number.






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L2903-L2910" target="_blank">source</a>]


### Spring.SetUnitBuildSpeed
---
```lua
function Spring.SetUnitBuildSpeed(
  builderID: integer,
  buildSpeed: number,
  repairSpeed: number?,
  reclaimSpeed: number?,
  captureSpeed: number?,
  terraformSpeed: number?
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L2937-L2946" target="_blank">source</a>]


### Spring.SetUnitBuildeeRadius
---
```lua
function Spring.SetUnitBuildeeRadius(
  unitID: integer,
  build: number
) ->  nil
```
@param `build` - radius for when targeted by build, repair, reclaim-type commands.






Sets the unit's radius for when targeted by build, repair, reclaim-type commands.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L3449-L3455" target="_blank">source</a>]


### Spring.SetUnitCloak
---
```lua
function Spring.SetUnitCloak(
  unitID: integer,
  cloak: (boolean|number),
  cloakArg: (boolean|number)
) ->  nil
```





If the 2nd argument is a number, the value works like this:
1:=normal cloak
2:=for free cloak (cost no E)
3:=for free + no decloaking (except the unit is stunned)
4:=ultimate cloak (no ecost, no decloaking, no stunned decloak)

The decloak distance is only changed:


### Spring.SetUnitCollisionVolumeData
---
```lua
function Spring.SetUnitCollisionVolumeData(
  unitID: integer,
  scaleX: number,
  scaleY: number,
  scaleZ: number,
  offsetX: number,
  offsetY: number,
  offsetZ: number,
  vType: number,
  tType: number,
  Axis: number
) -> enum nil
```

@return `enum` - COLVOL_TYPES {
COLVOL_TYPE_DISABLED = -1,
COLVOL_TYPE_ELLIPSOID = 0,
COLVOL_TYPE_CYLINDER,
COLVOL_TYPE_BOX,
COLVOL_TYPE_SPHERE,
COLVOL_NUM_TYPES       // number of non-disabled collision volume types
};
enum COLVOL_TESTS {
COLVOL_TEST_DISC = 0,
COLVOL_TEST_CONT = 1,
COLVOL_NUM_TESTS = 2   // number of tests
};
enum COLVOL_AXES {
COLVOL_AXIS_X   = 0,
COLVOL_AXIS_Y   = 1,
COLVOL_AXIS_Z   = 2,
COLVOL_NUM_AXES = 3    // number of collision volume axes
};





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L3546-L3579" target="_blank">source</a>]


### Spring.SetUnitCosts
---
```lua
function Spring.SetUnitCosts(
  unitID: integer,
  where: table<number,number>
) ->  nil
```
@param `where` - keys and values are, respectively and in this order: buildTime=amount, metalCost=amount, energyCost=amount






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L1843-L1848" target="_blank">source</a>]


### Spring.SetUnitCrashing
---
```lua
function Spring.SetUnitCrashing(
  unitID: integer,
  crashing: boolean
) -> success boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L3066-L3071" target="_blank">source</a>]


### Spring.SetUnitDefIcon
---
```lua
function Spring.SetUnitDefIcon(
  unitDefID: integer,
  iconName: string
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L2483-L2491" target="_blank">source</a>]


### Spring.SetUnitDefImage
---
```lua
function Spring.SetUnitDefImage(
  unitDefID: integer,
  image: string
) ->  nil
```
@param `image` - luaTexture|texFile






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L2523-L2531" target="_blank">source</a>]


### Spring.SetUnitDirection
---
```lua
function Spring.SetUnitDirection(
  unitID: integer,
  frontx: number,
  fronty: number,
  frontz: number
) ->  nil
```





Set unit front direction vector. The vector is normalized in
the engine.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L3899-L3912" target="_blank">source</a>]


### Spring.SetUnitEngineDrawMask
---
```lua
function Spring.SetUnitEngineDrawMask(
  unitID: integer,
  drawMask: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L2123-L2129" target="_blank">source</a>]


### Spring.SetUnitExperience
---
```lua
function Spring.SetUnitExperience(
  unitID: integer,
  experience: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L2540-L2547" target="_blank">source</a>]


### Spring.SetUnitFlanking
---
```lua
function Spring.SetUnitFlanking(
  unitID: integer,
  type: string,
  arg1: number,
  y: number?,
  z: number?
) ->  nil
```
@param `type` - "dir"|"minDamage"|"maxDamage"|"moveFactor"|"mode"

@param `arg1` - x|minDamage|maxDamage|moveFactor|mode

@param `y` - only when type is "dir"

@param `z` - only when type is "dir"






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L3176-L3184" target="_blank">source</a>]


### Spring.SetUnitGroup
---
```lua
function Spring.SetUnitGroup(
  unitID: integer,
  groupID: number
) ->  nil
```
@param `groupID` - the group number to be assigned, or -1 for deassignment






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L3207-L3213" target="_blank">source</a>]


### Spring.SetUnitHarvestStorage
---
```lua
function Spring.SetUnitHarvestStorage(
  unitID: integer,
  metal: number
) ->  nil
```





See also harvestStorage UnitDef tag.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L2882-L2888" target="_blank">source</a>]


### Spring.SetUnitHeadingAndUpDir
---
```lua
function Spring.SetUnitHeadingAndUpDir(
  unitID: integer,
  heading: number,
  upx: number,
  upy: number,
  upz: number
) ->  nil
```





Use this call to set up unit direction in a robust way. Heading (-32768 to 32767) represents a 2D (xz plane) unit orientation if unit was completely upright, new {upx,upy,upz} direction will be used as new "up" vector, the rotation set by "heading" will remain preserved.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L3934-L3943" target="_blank">source</a>]


### Spring.SetUnitHealth
---
```lua
function Spring.SetUnitHealth(
  unitID: integer,
  health: (number|SetUnitHealthAmounts)
) ->  nil
```
@param `health` - If a number, sets the units health
to that value. Pass a table to update health, capture progress, paralyze
damage, and build progress.






Note, if your game's custom shading framework doesn't support reverting into nanoframes
then reverting into nanoframes via the "build" tag will fail to render properly.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L2088-L2101" target="_blank">source</a>]


### Spring.SetUnitIconDraw
---
```lua
function Spring.SetUnitIconDraw(
  unitID: integer,
  drawIcon: boolean
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L2464-L2470" target="_blank">source</a>]


### Spring.SetUnitLandGoal
---
```lua
function Spring.SetUnitLandGoal(
  unitID: integer,
  goalX: number,
  goalY: number,
  goalZ: number,
  goalRadius: number?
) ->  nil
```





Used in conjunction with Spring.UnitAttach et al. to re-implement old airbase & fuel system in Lua.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L3748-L3757" target="_blank">source</a>]


### Spring.SetUnitLeaveTracks
---
```lua
function Spring.SetUnitLeaveTracks(
  unitID: integer,
  unitLeaveTracks: boolean
) ->  nil
```
@param `unitLeaveTracks` - whether unit leaves tracks on movement






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L2230-L2236" target="_blank">source</a>]


### Spring.SetUnitLoadingTransport
---
```lua
function Spring.SetUnitLoadingTransport(
  passengerID: integer,
  transportID: integer
) ->  nil
```





Disables collisions between the two units to allow colvol intersection during the approach.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L6763-L6769" target="_blank">source</a>]


### Spring.SetUnitLosMask
---
```lua
function Spring.SetUnitLosMask(
  unitID: integer,
  allyTeam: number,
  losTypes: (number|table)
) ->  nil
```





The 3rd argument is either the bit-and combination of the following numbers:

LOS_INLOS = 1
LOS_INRADAR = 2
LOS_PREVLOS = 4
LOS_CONTRADAR = 8

or a table of the following form:

losTypes = {
[los = boolean,]
[radar = boolean,]
[prevLos = boolean,]
[contRadar = boolean]
}

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L2654-L2677" target="_blank">source</a>]


### Spring.SetUnitLosState
---
```lua
function Spring.SetUnitLosState(
  unitID: integer,
  allyTeam: number,
  los: (number|table)
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L2702-L2708" target="_blank">source</a>]


### Spring.SetUnitMass
---
```lua
function Spring.SetUnitMass(
  unitID: integer,
  mass: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L3817-L3822" target="_blank">source</a>]


### Spring.SetUnitMaxHealth
---
```lua
function Spring.SetUnitMaxHealth(
  unitID: integer,
  maxHealth: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L2153-L2158" target="_blank">source</a>]


### Spring.SetUnitMaxRange
---
```lua
function Spring.SetUnitMaxRange(
  unitID: integer,
  maxRange: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L2522-L2527" target="_blank">source</a>]


### Spring.SetUnitMetalExtraction
---
```lua
function Spring.SetUnitMetalExtraction(
  unitID: integer,
  depth: number,
  range: number?
) ->  nil
```
@param `depth` - corresponds to metal extraction rate

@param `range` - similar to "extractsMetal" in unitDefs.






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L2855-L2861" target="_blank">source</a>]


### Spring.SetUnitMidAndAimPos
---
```lua
function Spring.SetUnitMidAndAimPos(
  unitID: integer,
  mpX: number,
  mpY: number,
  mpZ: number,
  apX: number,
  apY: number,
  apZ: number,
  relative: boolean?
) -> success boolean
```
@param `mpX` - new middle positionX of unit

@param `mpY` - new middle positionY of unit

@param `mpZ` - new middle positionZ of unit

@param `apX` - new positionX that enemies aim at on this unit

@param `apY` - new positionY that enemies aim at on this unit

@param `apZ` - new positionZ that enemies aim at on this unit

@param `relative` - (Default: false) are the new coordinates relative to world (false) or unit (true) coordinates? Also, note that apy is inverted!






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L3365-L3376" target="_blank">source</a>]


### Spring.SetUnitMoveGoal
---
```lua
function Spring.SetUnitMoveGoal(
  unitID: integer,
  goalX: number,
  goalY: number,
  goalZ: number,
  goalRadius: number?,
  moveSpeed: number?,
  moveRaw: boolean?
) ->  nil
```





Used by default commands to get in build-, attackrange etc.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L3711-L3722" target="_blank">source</a>]


### Spring.SetUnitNanoPieces
---
```lua
function Spring.SetUnitNanoPieces(
  builderID: integer,
  pieces: table
) ->  nil
```





This saves a lot of engine calls, by replacing: function script.QueryNanoPiece() return currentpiece end
Use it!

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L2988-L2998" target="_blank">source</a>]


### Spring.SetUnitNeutral
---
```lua
function Spring.SetUnitNeutral(
  unitID: integer,
  neutral: boolean
) -> setNeutral (nil|boolean)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L3265-L3272" target="_blank">source</a>]


### Spring.SetUnitNoDraw
---
```lua
function Spring.SetUnitNoDraw(
  unitID: integer,
  noDraw: boolean
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L2104-L2110" target="_blank">source</a>]


### Spring.SetUnitNoGroup
---
```lua
function Spring.SetUnitNoGroup(
  unitID: integer,
  unitNoGroup: boolean
)
```
@param `unitNoGroup` - Whether unit can be added to selection groups






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L2180-L2185" target="_blank">source</a>]


### Spring.SetUnitNoMinimap
---
```lua
function Spring.SetUnitNoMinimap(
  unitID: integer,
  unitNoMinimap: boolean
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L2161-L2167" target="_blank">source</a>]


### Spring.SetUnitNoSelect
---
```lua
function Spring.SetUnitNoSelect(
  unitID: integer,
  unitNoSelect: boolean
) ->  nil
```
@param `unitNoSelect` - whether unit can be selected or not






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L2202-L2208" target="_blank">source</a>]


### Spring.SetUnitPhysicalStateBit
---
```lua
function Spring.SetUnitPhysicalStateBit(
  unitID: integer,
  Physical
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L3223-L3228" target="_blank">source</a>]


### Spring.SetUnitPhysics
---
```lua
function Spring.SetUnitPhysics(
  unitID: integer,
  posX: number,
  posY: number,
  posZ: number,
  velX: number,
  velY: number,
  velZ: number,
  rotX: number,
  rotY: number,
  rotZ: number,
  dragX: number,
  dragY: number,
  dragZ: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L3795-L3811" target="_blank">source</a>]


### Spring.SetUnitPieceCollisionVolumeData
---
```lua
function Spring.SetUnitPieceCollisionVolumeData(
  unitID: integer,
  pieceIndex: number,
  enable: boolean,
  scaleX: number,
  scaleY: number,
  scaleZ: number,
  offsetX: number,
  offsetY: number,
  offsetZ: number,
  volumeType: number?,
  primaryAxis: number?
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L3586-L3600" target="_blank">source</a>]


### Spring.SetUnitPieceMatrix
---
```lua
function Spring.SetUnitPieceMatrix(
  unitID: integer,
  pieceNum: number,
  matrix: number[]
) ->  nil
```
@param `matrix` - an array of 16 floats






Sets the local (i.e. parent-relative) matrix of the given piece.

If any of the first three elements are non-zero, and also blocks all script animations from modifying it until {0, 0, 0} is passed.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L3510-L3520" target="_blank">source</a>]


### Spring.SetUnitPieceParent
---
```lua
function Spring.SetUnitPieceParent(
  unitID: integer,
  AlteredPiece: number,
  ParentPiece: number
) ->  nil
```





Changes the pieces hierarchy of a unit by attaching a piece to a new parent.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L3469-L3476" target="_blank">source</a>]


### Spring.SetUnitPieceVisible
---
```lua
function Spring.SetUnitPieceVisible(
  unitID: integer,
  pieceIndex: number,
  visible: boolean
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L3607-L3614" target="_blank">source</a>]


### Spring.SetUnitPosErrorParams
---
```lua
function Spring.SetUnitPosErrorParams(
  unitID: integer,
  posErrorVectorX: number,
  posErrorVectorY: number,
  posErrorVectorZ: number,
  posErrorDeltaX: number,
  posErrorDeltaY: number,
  posErrorDeltaZ: number,
  nextPosErrorUpdate: number?
) ->  nil
```





Sets a unit's radar wobble

Controls how much a unit's radar dot will wobble. Note that setting
this above the allyTeam's default wobble may result in the edgemost
dot positions failing to register in ray traces, i.e. things like
native "is under cursor" checks and some Lua interfaces.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L3670-L3687" target="_blank">source</a>]


### Spring.SetUnitPosition
---
```lua
function Spring.SetUnitPosition(
  unitID: integer,
  x: number,
  z: number,
  floating: boolean?
) ->  nil
```
@param `floating` - (Default: false) If true, over water the position is on surface. If false, on seafloor.






Set unit position (2D)

Sets a unit's position in 2D, at terrain height.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L3829-L3839" target="_blank">source</a>]


### Spring.SetUnitRadiusAndHeight
---
```lua
function Spring.SetUnitRadiusAndHeight(
  unitID: integer,
  radius: number,
  height: number
) -> success boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L3413-L3419" target="_blank">source</a>]


### Spring.SetUnitResourcing
---
```lua
function Spring.SetUnitResourcing(
  unitID: integer,
  res: string,
  amount: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L1972-L1978" target="_blank">source</a>]


### Spring.SetUnitRotation
---
```lua
function Spring.SetUnitRotation(
  unitID: integer,
  pitch: number,
  yaw: number,
  roll: number
) ->  nil
```
@param `pitch` - Rotation in X axis

@param `yaw` - Rotation in Y axis

@param `roll` - Rotation in Z axis






Note: PYR order

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L3884-L3892" target="_blank">source</a>]


### Spring.SetUnitRulesParam
---
```lua
function Spring.SetUnitRulesParam(
  unitID: integer,
  paramName: string,
  paramValue: (number|string)?,
  losAccess: losAccess?
) ->  nil
```
@param `paramValue` - numeric paramValues in quotes will be converted to number.






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L1466-L1474" target="_blank">source</a>]


### Spring.SetUnitSeismicSignature
---
```lua
function Spring.SetUnitSeismicSignature(
  unitID: integer,
  seismicSignature: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L2814-L2819" target="_blank">source</a>]


### Spring.SetUnitSelectionVolumeData
---
```lua
function Spring.SetUnitSelectionVolumeData(
  unitID: integer,
  featureID: integer,
  scaleX: number,
  scaleY: number,
  scaleZ: number,
  offsetX: number,
  offsetY: number,
  offsetZ: number,
  vType: number,
  tType: number,
  Axis: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L2249-L2264" target="_blank">source</a>]


### Spring.SetUnitSensorRadius
---
```lua
function Spring.SetUnitSensorRadius(
  unitID: integer,
  type: ("los"|"airLos"|"radar"|"sonar"|"seismic"|"radarJammer"|"sonarJammer"),
  radius: number
) -> New number?
```

@return `New` - radius, or `nil` if unit is invalid.





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L3621-L3627" target="_blank">source</a>]


### Spring.SetUnitShieldRechargeDelay
---
```lua
function Spring.SetUnitShieldRechargeDelay(
  unitID: integer,
  weaponID: integer?,
  rechargeTime: number?
) ->  nil
```
@param `weaponID` - (optional if the unit only has one shield)

@param `rechargeTime` - (in seconds; emulates a regular hit if nil)






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L3139-L3145" target="_blank">source</a>]


### Spring.SetUnitShieldState
---
```lua
function Spring.SetUnitShieldState(
  unitID: integer,
  weaponID: integer?,
  enabled: boolean?,
  power: number?
) ->  nil
```
@param `weaponID` - (Default: -1)






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L3101-L3108" target="_blank">source</a>]


### Spring.SetUnitSonarStealth
---
```lua
function Spring.SetUnitSonarStealth(
  unitID: integer,
  sonarStealth: boolean
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L2797-L2802" target="_blank">source</a>]


### Spring.SetUnitStealth
---
```lua
function Spring.SetUnitStealth(
  unitID: integer,
  stealth: boolean
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L2779-L2784" target="_blank">source</a>]


### Spring.SetUnitStockpile
---
```lua
function Spring.SetUnitStockpile(
  unitID: integer,
  stockpile: number?,
  buildPercent: number?
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L2172-L2178" target="_blank">source</a>]


### Spring.SetUnitStorage
---
```lua
function Spring.SetUnitStorage(
  unitID: number,
  res: string,
  amount: number
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L2018-L2023" target="_blank">source</a>]


### Spring.SetUnitTarget
---
```lua
function Spring.SetUnitTarget(
  unitID: integer,
  enemyUnitID: integer?,
  dgun: boolean?,
  userTarget: boolean?,
  weaponNum: number?
) -> success boolean
```
@param `enemyUnitID` - when nil drops the units current target.

@param `dgun` - (Default: false)

@param `userTarget` - (Default: false)

@param `weaponNum` - (Default: -1)






Defines a unit's target.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L3285-L3294" target="_blank">source</a>]


### Spring.SetUnitTooltip
---
```lua
function Spring.SetUnitTooltip(
  unitID: integer,
  tooltip: string
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L2057-L2062" target="_blank">source</a>]


### Spring.SetUnitUseAirLos
---
```lua
function Spring.SetUnitUseAirLos(
  unitID: integer,
  useAirLos: boolean
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L2842-L2848" target="_blank">source</a>]


### Spring.SetUnitUseWeapons
---
```lua
function Spring.SetUnitUseWeapons(
  unitID: integer,
  forceUseWeapons: number?,
  allowUseWeapons: number?
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L2307-L2314" target="_blank">source</a>]


### Spring.SetUnitVelocity
---
```lua
function Spring.SetUnitVelocity(
  unitID: integer,
  velX: number,
  velY: number,
  velZ: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L3949-L3956" target="_blank">source</a>]


### Spring.SetUnitWeaponDamages
---
```lua
function Spring.SetUnitWeaponDamages(
  unitID: integer,
  weaponNum: (number|"selfDestruct"|"explode"),
  damages: WeaponDamages
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L2462-L2468" target="_blank">source</a>]


### Spring.SetUnitWeaponState
---
```lua
function Spring.SetUnitWeaponState(
  unitID: integer,
  weaponNum: number,
  states: WeaponState
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L2327-L2333" target="_blank">source</a>]


### Spring.SetVideoCapturingMode
---
```lua
function Spring.SetVideoCapturingMode(allowCaptureMode: boolean) ->  nil
```





This doesn't actually record the game in any way, it just regulates the framerate and interpolations.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L4316-L4321" target="_blank">source</a>]


### Spring.SetVideoCapturingTimeOffset
---
```lua
function Spring.SetVideoCapturingTimeOffset(timeOffset: boolean) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L4329-L4332" target="_blank">source</a>]


### Spring.SetWMIcon
---
```lua
function Spring.SetWMIcon(iconFileName: string) ->  nil
```





Sets the icon for the process which is seen in the OS task-bar and other places (default: spring-logo).

Note: has to be 24bit or 32bit.
Note: on windows, it has to be 32x32 pixels in size (recommended for cross-platform)
Note: *.bmp images have to be in BGR format (default for m$ ones).
Note: *.ico images are not supported.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L5171-L5182" target="_blank">source</a>]


### Spring.SetWaterParams
---
```lua
function Spring.SetWaterParams(waterParams: WaterParams) ->  nil
```





Does not need cheating enabled.

Allows to change water params (mostly `BumpWater` ones) at runtime. You may
want to set `BumpWaterUseUniforms` in your `springrc` to 1, then you don't even
need to restart `BumpWater` via `/water 4`.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L4384-L4394" target="_blank">source</a>]


### Spring.SetWind
---
```lua
function Spring.SetWind(
  minStrength: number,
  maxStrength: number
) ->  nil
```





Set wind strength

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L1021-L1027" target="_blank">source</a>]


### Spring.SetWindowGeometry
---
```lua
function Spring.SetWindowGeometry(
  displayIndex: number,
  winRelPosX: number,
  winRelPosY: number,
  winSizeX: number,
  winSizeY: number,
  fullScreen: boolean,
  borderless: boolean
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L5066-L5077" target="_blank">source</a>]


### Spring.SetWindowMaximized
---
```lua
function Spring.SetWindowMaximized() -> maximized boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L5107-L5111" target="_blank">source</a>]


### Spring.SetWindowMinimized
---
```lua
function Spring.SetWindowMinimized() -> minimized boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L5096-L5100" target="_blank">source</a>]


### Spring.ShareResources
---
```lua
function Spring.ShareResources(
  teamID: integer,
  units: string
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L3694-L3701" target="_blank">source</a>]


### Spring.ShareTeamResource
---
```lua
function Spring.ShareTeamResource(
  teamID_src: integer,
  teamID_recv: integer,
  type: ResourceName,
  amount: number
) ->  nil
```





Transfers resources between two teams.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L1253-L1261" target="_blank">source</a>]


### Spring.SolveNURBSCurve
---
```lua
function Spring.SolveNURBSCurve(groupID: integer) -> unitIDs number[]?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L4853-L4858" target="_blank">source</a>]


### Spring.SpawnCEG
---
```lua
function Spring.SpawnCEG(
  cegname: string,
  posX: number?,
  posY: number?,
  posZ: number?,
  dirX: number?,
  dirY: number?,
  dirZ: number?,
  radius: number?,
  damage: number?
)
 -> success boolean?
 -> cegID number

```
@param `posX` - (Default: 0)

@param `posY` - (Default: 0)

@param `posZ` - (Default: 0)

@param `dirX` - (Default: 0)

@param `dirY` - (Default: 0)

@param `dirZ` - (Default: 0)

@param `radius` - (Default: 0)

@param `damage` - (Default: 0)






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L7033-L7046" target="_blank">source</a>]


### Spring.SpawnExplosion
---
```lua
function Spring.SpawnExplosion(
  posX: number?,
  posY: number?,
  posZ: number?,
  dirX: number?,
  dirY: number?,
  dirZ: number?,
  explosionParams: ExplosionParams
) ->  nil
```
@param `posX` - (Default: 0)

@param `posY` - (Default: 0)

@param `posZ` - (Default: 0)

@param `dirX` - (Default: 0)

@param `dirY` - (Default: 0)

@param `dirZ` - (Default: 0)






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L6961-L6971" target="_blank">source</a>]


### Spring.SpawnProjectile
---
```lua
function Spring.SpawnProjectile(
  weaponDefID: integer,
  projectileParams: ProjectileParams
) -> projectileID integer?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L6793-L6799" target="_blank">source</a>]


### Spring.SpawnSFX
---
```lua
function Spring.SpawnSFX(
  unitID: integer?,
  sfxID: integer?,
  posX: number?,
  posY: number?,
  posZ: number?,
  dirX: number?,
  dirY: number?,
  dirZ: number?,
  radius: number?,
  damage: number?,
  absolute: boolean?
) -> success boolean?
```
@param `unitID` - (Default: 0)

@param `sfxID` - (Default: 0)

@param `posX` - (Default: 0)

@param `posY` - (Default: 0)

@param `posZ` - (Default: 0)

@param `dirX` - (Default: 0)

@param `dirY` - (Default: 0)

@param `dirZ` - (Default: 0)

@param `radius` - (Default: 0)

@param `damage` - (Default: 0)






Equal to the UnitScript versions of EmitSFX, but takes position and direction arguments (in either unit- or piece-space) instead of a piece index.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L7065-L7080" target="_blank">source</a>]


### Spring.Start
---
```lua
function Spring.Start(
  commandline_args: string,
  startScript: string
) ->  nil
```
@param `commandline_args` - commandline arguments passed to spring executable.

@param `startScript` - the CONTENT of the script.txt spring should use to start (if empty, no start-script is added, you can still point spring to your custom script.txt when you add the file-path to commandline_args.






Launches a new Spring instance without terminating the existing one.

If this call returns, something went wrong

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L5150-L5159" target="_blank">source</a>]


### Spring.StopSoundStream
---
```lua
function Spring.StopSoundStream() ->  nil
```





Terminates any SoundStream currently running.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L827-L831" target="_blank">source</a>]


### Spring.TestBuildOrder
---
```lua
function Spring.TestBuildOrder(
  unitDefID: integer,
  x: number,
  y: number,
  z: number,
  facing: Facing
)
 -> blocking BuildOrderBlockedStatus
 -> featureID integer?

```

@return `featureID` - A reclaimable feature in the way.





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L7767-L7776" target="_blank">source</a>]


### Spring.TestMoveOrder
---
```lua
function Spring.TestMoveOrder(
  unitDefID: integer,
  pos: float3,
  dir: float3?,
  testTerrain: boolean?,
  testObjects: boolean?,
  centerOnly: boolean?
) ->  boolean
```
@param `dir` - (Default: `{ x: 0, y: 0, z: 0 }`)

@param `testTerrain` - (Default: true)

@param `testObjects` - (Default: true)

@param `centerOnly` - (Default: false)






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L7706-L7716" target="_blank">source</a>]


### Spring.TraceRayGroundBetweenPositions
---
```lua
function Spring.TraceRayGroundBetweenPositions(
  startX: number,
  startY: number,
  startZ: number,
  endX: number,
  endY: number,
  endZ: number,
  testWater: boolean?
)
 -> rayLength number
 -> posX number
 -> posY number
 -> posZ number

```
@param `testWater` - (Default: `true`)






Checks for a ground collision between two positions

Checks if there is surface (ground, optionally water) between two positions
and returns the distance to the closest hit and its position, if any.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L8845-L8863" target="_blank">source</a>]


### Spring.TraceRayGroundInDirection
---
```lua
function Spring.TraceRayGroundInDirection(
  posX: number,
  posY: number,
  posZ: number,
  dirX: number,
  dirY: number,
  dirZ: number,
  testWater: boolean?
)
 -> rayLength number
 -> posX number
 -> posY number
 -> posZ number

```
@param `testWater` - (Default: `true`)






Checks for a ground collision in given direction

Checks if there is surface (ground, optionally water) towards a vector
and returns the distance to the closest hit and its position, if any.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L8816-L8834" target="_blank">source</a>]


### Spring.TraceScreenRay
---
```lua
function Spring.TraceScreenRay(
  screenX: number,
  screenY: number,
  onlyCoords: boolean?,
  useMinimap: boolean?,
  includeSky: boolean?,
  ignoreWater: boolean?,
  heightOffset: number?
)
 -> description (nil|string)
 -> unitID (nil|number|string|xyz)
 -> featureID (nil|number|string)
 -> coords (nil|xyz)

```
@param `screenX` - position on x axis in mouse coordinates (origin on left border of view)

@param `screenY` - position on y axis in mouse coordinates (origin on top border of view)

@param `onlyCoords` - (Default: false) return only description (1st return value) and coordinates (2nd return value)

@param `useMinimap` - (Default: false) if position arguments are contained by minimap, use the minimap corresponding world position

@param `includeSky` - (Default: false)

@param `ignoreWater` - (Default: false)

@param `heightOffset` - (Default: 0)


@return `description` - of traced position

@return `unitID` - or feature, position triple when onlyCoords=true

@return `featureID` - or ground





Get information about a ray traced from screen to world position

Extended to allow a custom plane, parameters are (0, 1, 0, D=0) where D is the offset D can be specified in the third argument (if all the bools are false) or in the seventh (as shown).

Intersection coordinates are returned in t[4],t[5],t[6] when the ray goes offmap and includeSky is true), or when no unit or feature is hit (or onlyCoords is true).

This will only work for units & objects with the default collision sphere. Per Piece collision and custom collision objects are not supported.

The unit must be selectable, to appear to a screen trace ray.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L2854-L2877" target="_blank">source</a>]


### Spring.TransferFeature
---
```lua
function Spring.TransferFeature(
  featureDefID: integer,
  teamID: integer
) ->  nil
```





Feature Control

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L4432-L4438" target="_blank">source</a>]


### Spring.TransferUnit
---
```lua
function Spring.TransferUnit(
  unitID: integer,
  newTeamID: integer,
  given: boolean?
) ->  nil
```
@param `given` - (Default: true) if false, the unit is captured.






[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L1798-L1804" target="_blank">source</a>]


### Spring.UnitAttach
---
```lua
function Spring.UnitAttach(
  transporterID: integer,
  passengerID: integer,
  pieceNum: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L6670-L6677" target="_blank">source</a>]


### Spring.UnitDetach
---
```lua
function Spring.UnitDetach(passengerID: integer) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L6709-L6713" target="_blank">source</a>]


### Spring.UnitDetachFromAir
---
```lua
function Spring.UnitDetachFromAir(passengerID: integer) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L6731-L6735" target="_blank">source</a>]


### Spring.UnitFinishCommand
---
```lua
function Spring.UnitFinishCommand(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L5388-L5391" target="_blank">source</a>]


### Spring.UnitIconGetDraw
---
```lua
function Spring.UnitIconGetDraw(unitID: integer) -> drawIcon boolean?
```

@return `drawIcon` - `true` if icon is being drawn, `nil` when unitID is invalid, otherwise `false`.





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L1339-L1345" target="_blank">source</a>]


### Spring.UnitIconSetDraw
---
```lua
function Spring.UnitIconSetDraw(
  unitID: integer,
  drawIcon: boolean
) ->  nil
```





Use Spring.SetUnitIconDraw instead.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L2444-L2452" target="_blank">source</a>]


### Spring.UnitWeaponFire
---
```lua
function Spring.UnitWeaponFire(
  unitID: integer,
  weaponID: integer
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L6599-L6604" target="_blank">source</a>]


### Spring.UnitWeaponHoldFire
---
```lua
function Spring.UnitWeaponHoldFire(
  unitID: integer,
  weaponID: integer
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L6622-L6627" target="_blank">source</a>]


### Spring.UpdateMapLight
---
```lua
function Spring.UpdateMapLight(
  lightHandle: number,
  lightParams: LightParams
) -> success boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L1756-L1762" target="_blank">source</a>]


### Spring.UpdateModelLight
---
```lua
function Spring.UpdateModelLight(
  lightHandle: number,
  lightParams: LightParams
) -> success boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L1778-L1784" target="_blank">source</a>]


### Spring.UseTeamResource
---
```lua
function Spring.UseTeamResource(
  teamID: integer,
  type: ResourceName,
  amount: number
) -> hadEnough boolean
```
@param `type` - Resource type.

@param `amount` - Amount of resource to use.


@return `hadEnough` - True if enough of the resource type was available and was consumed, otherwise false.





Consumes metal or energy resources of the specified team.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L1070-L1079" target="_blank">source</a>]


### Spring.UseUnitResource
---
```lua
function Spring.UseUnitResource(
  unitID: integer,
  resource: ResourceName,
  amount: number
) -> okay boolean?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedCtrl.cpp#L4166-L4172" target="_blank">source</a>]


### Spring.ValidFeatureID
---
```lua
function Spring.ValidFeatureID(featureID: integer) ->  boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L6590-L6595" target="_blank">source</a>]


### Spring.ValidUnitID
---
```lua
function Spring.ValidUnitID(unitID: integer) ->  boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaSyncedRead.cpp#L3671-L3676" target="_blank">source</a>]


### Spring.WarpMouse
---
```lua
function Spring.WarpMouse(
  x: number,
  y: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L2947-L2951" target="_blank">source</a>]


### Spring.WorldToScreenCoords
---
```lua
function Spring.WorldToScreenCoords(
  x: number,
  y: number,
  z: number
)
 -> viewPortX number
 -> viewPortY number
 -> viewPortZ number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedRead.cpp#L2831-L2840" target="_blank">source</a>]


### Spring.Yield
---
```lua
function Spring.Yield() -> when boolean
```

@return `when` - true caller should continue calling `Spring.Yield` during the widgets/gadgets load, when false it shouldn't call it any longer.





Relinquish control of the game loading thread and OpenGL context back to the UI (LuaIntro).

Should be called after each widget/unsynced gadget is loaded in widget/gadget handler. Use it to draw screen updates and process windows events.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaUnsyncedCtrl.cpp#L5251-L5265" target="_blank">source</a>]




