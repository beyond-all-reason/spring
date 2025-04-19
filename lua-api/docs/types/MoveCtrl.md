---
layout: default
title: MoveCtrl
parent: Lua API
permalink: lua-api/types/MoveCtrl
---

{% raw %}

# class MoveCtrl





Accessed via `Spring.MoveCtrl`.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaSyncedMoveCtrl.cpp#L35-L40" target="_blank">source</a>]



## methods


### MoveCtrl.IsEnabled

```lua
function MoveCtrl.IsEnabled(unitID: integer) -> isEnabled boolean?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaSyncedMoveCtrl.cpp#L140-L144" target="_blank">source</a>]


### MoveCtrl.Enable

```lua
function MoveCtrl.Enable(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaSyncedMoveCtrl.cpp#L157-L160" target="_blank">source</a>]


### MoveCtrl.Disable

```lua
function MoveCtrl.Disable(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaSyncedMoveCtrl.cpp#L173-L176" target="_blank">source</a>]


### MoveCtrl.SetTag

```lua
function MoveCtrl.SetTag(
  unitID: integer,
  tag: integer
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaSyncedMoveCtrl.cpp#L191-L195" target="_blank">source</a>]


### MoveCtrl.GetTag

```lua
function MoveCtrl.GetTag(tag: integer?)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaSyncedMoveCtrl.cpp#L208-L211" target="_blank">source</a>]


### MoveCtrl.SetProgressState

```lua
function MoveCtrl.SetProgressState(
  unitID: integer,
  state: (0|1|2|"done"|"active"|"failed")
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaSyncedMoveCtrl.cpp#L228-L238" target="_blank">source</a>]


### MoveCtrl.SetExtrapolate

```lua
function MoveCtrl.SetExtrapolate(
  unitID: integer,
  extrapolate: boolean
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaSyncedMoveCtrl.cpp#L278-L282" target="_blank">source</a>]


### MoveCtrl.SetPhysics

```lua
function MoveCtrl.SetPhysics(
  unitID: integer,
  posX: number,
  posY: number,
  posZ: number,
  velX: number,
  velY: number,
  velZ: number,
  rotX: number,
  rotY: number,
  rotZ: number
)
```
@param `posX` - Position X component.

@param `posY` - Position Y component.

@param `posZ` - Position Z component.

@param `velX` - Velocity X component.

@param `velY` - Velocity Y component.

@param `velZ` - Velocity Z component.

@param `rotX` - Rotation X component.

@param `rotY` - Rotation Y component.

@param `rotZ` - Rotation Z component.






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaSyncedMoveCtrl.cpp#L297-L309" target="_blank">source</a>]


### MoveCtrl.SetPosition

```lua
function MoveCtrl.SetPosition(
  unitID: integer,
  posX: number,
  posY: number,
  posZ: number
)
```
@param `posX` - Position X component.

@param `posY` - Position Y component.

@param `posZ` - Position Z component.






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaSyncedMoveCtrl.cpp#L328-L334" target="_blank">source</a>]


### MoveCtrl.SetVelocity

```lua
function MoveCtrl.SetVelocity(
  unitID: integer,
  velX: number,
  velY: number,
  velZ: number
)
```
@param `velX` - Velocity X component.

@param `velY` - Velocity Y component.

@param `velZ` - Velocity Z component.






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaSyncedMoveCtrl.cpp#L351-L357" target="_blank">source</a>]


### MoveCtrl.SetRelativeVelocity

```lua
function MoveCtrl.SetRelativeVelocity(
  unitID: integer,
  relVelX: number,
  relVelY: number,
  relVelZ: number
)
```
@param `relVelX` - Relative velocity X component.

@param `relVelY` - Relative velocity Y component.

@param `relVelZ` - Relative velocity Z component.






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaSyncedMoveCtrl.cpp#L374-L380" target="_blank">source</a>]


### MoveCtrl.SetRotation

```lua
function MoveCtrl.SetRotation(
  unitID: integer,
  rotX: number,
  rotY: number,
  rotZ: number
)
```
@param `rotX` - Rotation X component.

@param `rotY` - Rotation Y component.

@param `rotZ` - Rotation Z component.






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaSyncedMoveCtrl.cpp#L397-L403" target="_blank">source</a>]


### MoveCtrl.SetRotationOffset

```lua
function MoveCtrl.SetRotationOffset()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaSyncedMoveCtrl.cpp#L420-L423" target="_blank">source</a>]


### MoveCtrl.SetRotationVelocity

```lua
function MoveCtrl.SetRotationVelocity(
  unitID: integer,
  rotVelX: number,
  rotVelY: number,
  rotVelZ: number
)
```
@param `rotVelX` - Rotation velocity X component.

@param `rotVelY` - Rotation velocity Y component.

@param `rotVelZ` - Rotation velocity Z component.






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaSyncedMoveCtrl.cpp#L431-L437" target="_blank">source</a>]


### MoveCtrl.SetHeading

```lua
function MoveCtrl.SetHeading(
  unitID: integer,
  heading: Heading
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaSyncedMoveCtrl.cpp#L453-L457" target="_blank">source</a>]


### MoveCtrl.SetTrackSlope

```lua
function MoveCtrl.SetTrackSlope(
  unitID: integer,
  trackSlope: boolean
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaSyncedMoveCtrl.cpp#L474-L478" target="_blank">source</a>]


### MoveCtrl.SetTrackGround

```lua
function MoveCtrl.SetTrackGround(
  unitID: integer,
  trackGround: boolean
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaSyncedMoveCtrl.cpp#L491-L495" target="_blank">source</a>]


### MoveCtrl.SetTrackLimits

```lua
function MoveCtrl.SetTrackLimits(
  unitID: integer,
  trackLimits: boolean
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaSyncedMoveCtrl.cpp#L508-L512" target="_blank">source</a>]


### MoveCtrl.SetGroundOffset

```lua
function MoveCtrl.SetGroundOffset(
  unitID: integer,
  groundOffset: number
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaSyncedMoveCtrl.cpp#L525-L529" target="_blank">source</a>]


### MoveCtrl.SetGravity

```lua
function MoveCtrl.SetGravity(
  unitID: integer,
  gravityFactor: number
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaSyncedMoveCtrl.cpp#L542-L546" target="_blank">source</a>]


### MoveCtrl.SetDrag

```lua
function MoveCtrl.SetDrag(
  unitID: integer,
  drag: number
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaSyncedMoveCtrl.cpp#L559-L563" target="_blank">source</a>]


### MoveCtrl.SetWindFactor

```lua
function MoveCtrl.SetWindFactor(
  unitID: integer,
  windFactor: number
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaSyncedMoveCtrl.cpp#L576-L580" target="_blank">source</a>]


### MoveCtrl.SetLimits

```lua
function MoveCtrl.SetLimits(
  unitID: integer,
  minX: number,
  minY: number,
  minZ: number,
  maxX: number,
  maxY: number,
  maxZ: number
)
```
@param `minX` - Minimum position X component.

@param `minY` - Minimum position Y component.

@param `minZ` - Minimum position Z component.

@param `maxX` - Maximum position X component.

@param `maxY` - Maximum position Y component.

@param `maxZ` - Maximum position Z component.






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaSyncedMoveCtrl.cpp#L593-L602" target="_blank">source</a>]


### MoveCtrl.SetNoBlocking

```lua
function MoveCtrl.SetNoBlocking(
  unitID: integer,
  noBlocking: boolean
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaSyncedMoveCtrl.cpp#L618-L622" target="_blank">source</a>]


### MoveCtrl.SetCollideStop

```lua
function MoveCtrl.SetCollideStop(
  unitID: integer,
  collideStop: boolean
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaSyncedMoveCtrl.cpp#L641-L645" target="_blank">source</a>]


### MoveCtrl.SetLimitsStop

```lua
function MoveCtrl.SetLimitsStop(
  unitID: integer,
  limitsStop: boolean
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaSyncedMoveCtrl.cpp#L658-L662" target="_blank">source</a>]


### MoveCtrl.SetGunshipMoveTypeData

```lua
function MoveCtrl.SetGunshipMoveTypeData(
  unitID: integer,
  moveType: HoverAirMoveType
) -> numAssignedValues number
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaSyncedMoveCtrl.cpp#L795-L800" target="_blank">source</a>]


### MoveCtrl.SetGunshipMoveTypeData

```lua
function MoveCtrl.SetGunshipMoveTypeData(
  unitID: integer,
  key: (GenericMoveTypeBooleanKey|"collide"|"dontLand"|"airStrafe"|"useSmoothMesh"|"bankingAllowed"),
  value: boolean
) -> numAssignedValues number
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaSyncedMoveCtrl.cpp#L801-L813" target="_blank">source</a>]


### MoveCtrl.SetGunshipMoveTypeData

```lua
function MoveCtrl.SetGunshipMoveTypeData(
  unitID: integer,
  key: (GenericMoveTypeNumberKey|"wantedHeight"|"accRate"|"decRate"|"turnRate"|"altitudeRate"|"currentBank"|"currentPitch"...),
  value: number
) -> numAssignedValues number
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaSyncedMoveCtrl.cpp#L814-L829" target="_blank">source</a>]


### MoveCtrl.SetAirMoveTypeData

```lua
function MoveCtrl.SetAirMoveTypeData(
  unitID: integer,
  moveType: StrafeAirMoveType
) -> numAssignedValues number
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaSyncedMoveCtrl.cpp#L856-L861" target="_blank">source</a>]


### MoveCtrl.SetAirMoveTypeData

```lua
function MoveCtrl.SetAirMoveTypeData(
  unitID: integer,
  key: (GenericMoveTypeBooleanKey|"collide"|"useSmoothMesh"|"loopbackAttack"),
  value: boolean
) -> numAssignedValues number
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaSyncedMoveCtrl.cpp#L862-L872" target="_blank">source</a>]


### MoveCtrl.SetAirMoveTypeData

```lua
function MoveCtrl.SetAirMoveTypeData(
  unitID: integer,
  key: (GenericMoveTypeNumberKey|"wantedHeight"|"turnRadius"|"accRate"|"decRate"|"maxAcc"|"maxDec"|"maxBank"...),
  value: number
) -> numAssignedValues number
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaSyncedMoveCtrl.cpp#L873-L893" target="_blank">source</a>]


### MoveCtrl.SetAirMoveTypeData

```lua
function MoveCtrl.SetAirMoveTypeData(
  unitID: integer,
  key: ("maneuverBlockTime"),
  value: integer
) -> numAssignedValues number
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaSyncedMoveCtrl.cpp#L894-L901" target="_blank">source</a>]


### MoveCtrl.SetGroundMoveTypeData

```lua
function MoveCtrl.SetGroundMoveTypeData(
  unitID: integer,
  moveType: GroundMoveType
) -> numAssignedValues number
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaSyncedMoveCtrl.cpp#L924-L929" target="_blank">source</a>]


### MoveCtrl.SetGroundMoveTypeData

```lua
function MoveCtrl.SetGroundMoveTypeData(
  unitID: integer,
  key: (GenericMoveTypeBooleanKey|"atGoal"|"atEndOfPath"|"pushResistant"),
  value: boolean
) -> numAssignedValues number
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaSyncedMoveCtrl.cpp#L930-L940" target="_blank">source</a>]


### MoveCtrl.SetGroundMoveTypeData

```lua
function MoveCtrl.SetGroundMoveTypeData(
  unitID: integer,
  key: (GenericMoveTypeNumberKey|"turnRate"|"turnAccel"|"accRate"|"decRate"|"myGravity"|"maxReverseDist"|"minReverseAngle"...),
  value: number
) -> numAssignedValues number
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaSyncedMoveCtrl.cpp#L941-L957" target="_blank">source</a>]


### MoveCtrl.SetGroundMoveTypeData

```lua
function MoveCtrl.SetGroundMoveTypeData(
  unitID: integer,
  key: ("minScriptChangeHeading"),
  value: integer
) -> numAssignedValues number
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaSyncedMoveCtrl.cpp#L958-L965" target="_blank">source</a>]


### MoveCtrl.SetMoveDef

```lua
function MoveCtrl.SetMoveDef(
  unitID: integer,
  moveDef: (integer|string)
) -> success boolean
```
@param `moveDef` - Name or path type of the MoveDef.


@return `success` - `true` if the `MoveDef` was set, `false` if `unitID` or `moveDef` were invalid, or if the unit does not support a `MoveDef`.





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaSyncedMoveCtrl.cpp#L976-L981" target="_blank">source</a>]






{% endraw %}