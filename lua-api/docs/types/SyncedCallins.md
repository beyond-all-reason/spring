---
layout: default
title: SyncedCallins
parent: Lua API
permalink: lua-api/types/SyncedCallins
---

# class SyncedCallins





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaHandleSynced.cpp#L547-L550" target="_blank">source</a>]

Functions called by the Engine (Synced).

---

## methods
---

### SyncedCallins.AllowBuilderHoldFire
---
```lua
function SyncedCallins.AllowBuilderHoldFire(
  unitID: integer,
  unitDefID: integer,
  action: number
) -> actionAllowed boolean
```
@param `action` - one of following:

-1 Build
CMD.REPAIR Repair
CMD.RECLAIM Reclaim
CMD.RESTORE Restore
CMD.RESURRECT Resurrect
CMD.CAPTURE Capture






Called when a construction unit wants to "use his nano beams".

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaHandleSynced.cpp#L1234-L1250" target="_blank">source</a>]


### SyncedCallins.AllowCommand
---
```lua
function SyncedCallins.AllowCommand(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  cmdID: integer,
  cmdParams: number[],
  cmdOptions: CommandOptions,
  cmdTag: number,
  synced: boolean,
  fromLua: boolean
) -> whether boolean
```

@return `whether` - it should be let into the queue.





Called when the command is given, before the unit's queue is altered.

The queue remains untouched when a command is blocked, whether it would be queued or replace the queue.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaHandleSynced.cpp#L601-L617" target="_blank">source</a>]


### SyncedCallins.AllowDirectUnitControl
---
```lua
function SyncedCallins.AllowDirectUnitControl(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  playerID: integer
) -> allow boolean
```





Determines if this unit can be controlled directly in FPS view.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaHandleSynced.cpp#L1199-L1207" target="_blank">source</a>]


### SyncedCallins.AllowFeatureBuildStep
---
```lua
function SyncedCallins.AllowFeatureBuildStep(
  builderID: integer,
  builderTeam: integer,
  featureID: integer,
  featureDefID: integer,
  part: number
) -> whether boolean
```

@return `whether` - or not the change is permitted





Called just before a feature changes its build percentage.

Note that this is also called for resurrecting features, and for refilling features with resources before resurrection.
On reclaim the part values are negative, and on refill and resurrect they are positive.
Part is the percentage the feature be built or reclaimed per frame.
Eg. for a 30 workertime builder, that's a build power of 1 per frame.
For a 50 buildtime feature reclaimed by this builder, part will be 100/-50(/1) = -2%, or -0.02 numerically.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaHandleSynced.cpp#L1086-L1103" target="_blank">source</a>]


### SyncedCallins.AllowFeatureCreation
---
```lua
function SyncedCallins.AllowFeatureCreation(
  featureDefID: integer,
  teamID: integer,
  x: number,
  y: number,
  z: number
) -> whether boolean
```

@return `whether` - or not the creation is permitted





Called just before feature is created.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaHandleSynced.cpp#L1049-L1058" target="_blank">source</a>]


### SyncedCallins.AllowResourceLevel
---
```lua
function SyncedCallins.AllowResourceLevel(
  teamID: integer,
  res: string,
  level: number
) -> whether boolean
```

@return `whether` - or not the sharing level is permitted





Called when a team sets the sharing level of a resource.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaHandleSynced.cpp#L1131-L1138" target="_blank">source</a>]


### SyncedCallins.AllowResourceTransfer
---
```lua
function SyncedCallins.AllowResourceTransfer(
  oldTeamID: integer,
  newTeamID: integer,
  res: string,
  amount: number
) -> whether boolean
```

@return `whether` - or not the transfer is permitted.





Called just before resources are transferred between players.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaHandleSynced.cpp#L1164-L1172" target="_blank">source</a>]


### SyncedCallins.AllowStartPosition
---
```lua
function SyncedCallins.AllowStartPosition(
  playerID: integer,
  teamID: integer,
  readyState: number,
  clampedX: number,
  clampedY: number,
  clampedZ: number,
  rawX: number,
  rawY: number,
  rawZ: number
) -> allow boolean
```





Whether a start position should be allowed

clamped{X,Y,Z} are the coordinates clamped into start-boxes, raw is where player tried to place their marker.

The readyState can be any one of:

0 - player picked a position,
1 - player clicked ready,
2 - player pressed ready OR the game was force-started (player did not click ready, but is now forcibly readied) or
3 - the player failed to load.
The default 'failed to choose' start-position is the north-west point of their startbox, or (0,0,0) if they do not have a startbox.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaHandleSynced.cpp#L1276-L1301" target="_blank">source</a>]


### SyncedCallins.AllowUnitBuildStep
---
```lua
function SyncedCallins.AllowUnitBuildStep(
  builderID: integer,
  builderTeam: integer,
  unitID: integer,
  unitDefID: integer,
  part: number
) -> whether boolean
```

@return `whether` - or not the build makes progress.





Called just before a unit progresses its build percentage.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaHandleSynced.cpp#L729-L738" target="_blank">source</a>]


### SyncedCallins.AllowUnitCaptureStep
---
```lua
function SyncedCallins.AllowUnitCaptureStep(
  builderID: integer,
  builderTeam: integer,
  unitID: integer,
  unitDefID: integer,
  part: number
) -> whether boolean
```

@return `whether` - or not the capture makes progress.





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaHandleSynced.cpp#L766-L775" target="_blank">source</a>]


### SyncedCallins.AllowUnitCloak
---
```lua
function SyncedCallins.AllowUnitCloak(
  unitID: integer,
  enemyID: integer?
) -> whether boolean
```

@return `whether` - unit is allowed to cloak





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaHandleSynced.cpp#L939-L945" target="_blank">source</a>]


### SyncedCallins.AllowUnitCreation
---
```lua
function SyncedCallins.AllowUnitCreation(
  unitDefID: integer,
  builderID: integer,
  builderTeam: integer,
  x: number,
  y: number,
  z: number,
  facing: number
)
 -> allow boolean
 -> dropOrder boolean

```





Called just before unit is created.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaHandleSynced.cpp#L645-L656" target="_blank">source</a>]


### SyncedCallins.AllowUnitKamikaze
---
```lua
function SyncedCallins.AllowUnitKamikaze(
  unitID: integer,
  targetID: integer
) -> whether boolean
```

@return `whether` - unit is allowed to selfd





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaHandleSynced.cpp#L1019-L1025" target="_blank">source</a>]


### SyncedCallins.AllowUnitTransfer
---
```lua
function SyncedCallins.AllowUnitTransfer(
  unitID: integer,
  unitDefID: integer,
  oldTeam: integer,
  newTeam: integer,
  capture: boolean
) -> whether boolean
```

@return `whether` - or not the transfer is permitted.





Called just before a unit is transferred to a different team.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaHandleSynced.cpp#L692-L701" target="_blank">source</a>]


### SyncedCallins.AllowUnitTransport
---
```lua
function SyncedCallins.AllowUnitTransport(
  transporterID: integer,
  transporterUnitDefID: integer,
  transporterTeam: integer,
  transporteeID: integer,
  transporteeUnitDefID: integer,
  transporteeTeam: integer
) -> whether boolean
```

@return `whether` - or not the transport is allowed





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaHandleSynced.cpp#L803-L813" target="_blank">source</a>]


### SyncedCallins.AllowUnitTransportLoad
---
```lua
function SyncedCallins.AllowUnitTransportLoad(
  transporterID: integer,
  transporterUnitDefID: integer,
  transporterTeam: integer,
  transporteeID: integer,
  transporteeUnitDefID: integer,
  transporteeTeam: integer,
  x: number,
  y: number,
  z: number
) -> whether boolean
```

@return `whether` - or not the transport load is allowed





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaHandleSynced.cpp#L841-L854" target="_blank">source</a>]


### SyncedCallins.AllowUnitTransportUnload
---
```lua
function SyncedCallins.AllowUnitTransportUnload(
  transporterID: integer,
  transporterUnitDefID: integer,
  transporterTeam: integer,
  transporteeID: integer,
  transporteeUnitDefID: integer,
  transporteeTeam: integer,
  x: number,
  y: number,
  z: number
) -> whether boolean
```

@return `whether` - or not the transport unload is allowed





[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaHandleSynced.cpp#L891-L904" target="_blank">source</a>]


### SyncedCallins.AllowWeaponInterceptTarget
---
```lua
function SyncedCallins.AllowWeaponInterceptTarget(
  interceptorUnitID: integer,
  interceptorWeaponID: integer,
  targetProjectileID: integer
) -> allowed boolean
```





Controls blocking of a specific intercept target from being considered during an interceptor weapon's periodic auto-targeting sweep.

Only called for weaponDefIDs registered via Script.SetWatchWeapon.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaHandleSynced.cpp#L1794-L1805" target="_blank">source</a>]


### SyncedCallins.AllowWeaponTarget
---
```lua
function SyncedCallins.AllowWeaponTarget(
  attackerID: integer,
  targetID: integer,
  attackerWeaponNum: integer,
  attackerWeaponDefID: integer,
  defPriority: number
)
 -> allowed boolean
 -> the number

```

@return `the` - new priority for this target (if you don't want to change it, return defPriority). Lower priority targets are targeted first.





Controls blocking of a specific target from being considered during a weapon's periodic auto-targeting sweep.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaHandleSynced.cpp#L1731-L1741" target="_blank">source</a>]


### SyncedCallins.AllowWeaponTargetCheck
---
```lua
function SyncedCallins.AllowWeaponTargetCheck(
  attackerID: integer,
  attackerWeaponNum: integer,
  attackerWeaponDefID: integer
)
 -> allowCheck boolean
 -> ignoreCheck boolean

```





Determines if this weapon can automatically generate targets itself. See also commandFire weaponDef tag.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaHandleSynced.cpp#L1691-L1699" target="_blank">source</a>]


### SyncedCallins.CommandFallback
---
```lua
function SyncedCallins.CommandFallback(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  cmdID: integer,
  cmdParams: number[],
  cmdOptions: CommandOptions,
  cmdTag: number
) -> whether boolean
```

@return `whether` - to remove the command from the queue





Called when the unit reaches an unknown command in its queue (i.e. one not handled by the engine).

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaHandleSynced.cpp#L568-L579" target="_blank">source</a>]


### SyncedCallins.FeaturePreDamaged
---
```lua
function SyncedCallins.FeaturePreDamaged(
  featureID: integer,
  featureDefID: integer,
  featureTeam: integer,
  damage: number,
  weaponDefID: integer,
  projectileID: integer,
  attackerID: integer,
  attackerDefID: integer,
  attackerTeam: integer
)
 -> newDamage number
 -> impulseMult number

```





Called before damage is applied to the feature.

Allows fine control over how much damage and impulse is applied.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaHandleSynced.cpp#L1526-L1543" target="_blank">source</a>]


### SyncedCallins.MoveCtrlNotify
---
```lua
function SyncedCallins.MoveCtrlNotify(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  data: number
) -> whether boolean
```
@param `data` - was supposed to indicate the type of notification but currently never has a value other than 1 ("unit hit the ground").


@return `whether` - or not the unit should remain script-controlled (false) or return to engine controlled movement (true).





Enable both Spring.MoveCtrl.SetCollideStop and Spring.MoveCtrl.SetTrackGround to enable this call-in.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaHandleSynced.cpp#L1334-L1344" target="_blank">source</a>]


### SyncedCallins.ShieldPreDamaged
---
```lua
function SyncedCallins.ShieldPreDamaged(
  projectileID: integer,
  projectileOwnerID: integer,
  shieldWeaponNum: integer,
  shieldCarrierID: integer,
  bounceProjectile: boolean,
  beamEmitterWeaponNum: integer,
  beamEmitterUnitID: integer,
  startX: number,
  startY: number,
  startZ: number,
  hitX: number,
  hitY: number,
  hitZ: number
) -> if boolean
```

@return `if` - true the gadget handles the collision event and the engine does not remove the projectile





Called before any engine shield-vs-projectile logic executes.

If the weapon is a hitscan type (BeamLaser or LightningCanon) then proID is nil and beamEmitterWeaponNum and beamEmitterUnitID are populated instead.

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaHandleSynced.cpp#L1609-L1630" target="_blank">source</a>]


### SyncedCallins.TerraformComplete
---
```lua
function SyncedCallins.TerraformComplete(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  buildUnitID: integer,
  buildUnitDefID: integer,
  buildUnitTeam: integer
) -> if boolean
```

@return `if` - true the current build order is terminated





Called when pre-building terrain levelling terraforms are completed (c.f. levelGround)

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaHandleSynced.cpp#L1372-L1382" target="_blank">source</a>]


### SyncedCallins.UnitPreDamaged
---
```lua
function SyncedCallins.UnitPreDamaged(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  damage: number,
  paralyzer: boolean,
  weaponDefID: integer?,
  projectileID: integer?,
  attackerID: integer?,
  attackerDefID: integer?,
  attackerTeam: integer?
)
 -> newDamage number
 -> impulseMult number

```
@param `weaponDefID` - Synced Only

@param `projectileID` - Synced Only

@param `attackerID` - Synced Only

@param `attackerDefID` - Synced Only

@param `attackerTeam` - Synced Only






Called before damage is applied to the unit, allows fine control over how much damage and impulse is applied.

Called after every damage modification (even `HitByWeaponId`) but before the damage is applied

expects two numbers returned by lua code:
1st is stored under *newDamage if newDamage != NULL
2nd is stored under *impulseMult if impulseMult != NULL

[<a href="https://github.com/beyond-all-reason/spring/blob/18fe5d6210290a1d9c085f08d997094c79f19a78/rts/Lua/LuaHandleSynced.cpp#L1430-L1452" target="_blank">source</a>]




