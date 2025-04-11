---
layout: default
title: StrafeAirMoveType
parent: Lua API
permalink: lua-api/types/StrafeAirMoveType
---

{% raw %}

# class StrafeAirMoveType


- supers: GenericMoveType {
    maxSpeed: number?,
    maxWantedSpeed: number?,
    maneuverLeash: number?,
    waterline: number?,
    useWantedSpeed[0]: boolean?,
    useWantedSpeed[1]: boolean?,
}




[<a href="https://github.com/beyond-all-reason/spring/blob/625902d539f43871ceb4ecdc45fc539e91a71b55/rts/Lua/LuaSyncedMoveCtrl.cpp#L835-L854" target="_blank">source</a>]





## fields


### StrafeAirMoveType.collide

```lua
StrafeAirMoveType.collide : boolean?
```




### StrafeAirMoveType.useSmoothMesh

```lua
StrafeAirMoveType.useSmoothMesh : boolean?
```




### StrafeAirMoveType.loopbackAttack

```lua
StrafeAirMoveType.loopbackAttack : boolean?
```




### StrafeAirMoveType.maneuverBlockTime

```lua
StrafeAirMoveType.maneuverBlockTime : integer?
```




### StrafeAirMoveType.wantedHeight

```lua
StrafeAirMoveType.wantedHeight : number?
```




### StrafeAirMoveType.turnRadius

```lua
StrafeAirMoveType.turnRadius : number?
```




### StrafeAirMoveType.accRate

```lua
StrafeAirMoveType.accRate : number?
```




### StrafeAirMoveType.decRate

```lua
StrafeAirMoveType.decRate : number?
```




### StrafeAirMoveType.maxAcc

```lua
StrafeAirMoveType.maxAcc : number?
```



Synonym for `accRate`.


### StrafeAirMoveType.maxDec

```lua
StrafeAirMoveType.maxDec : number?
```



Synonym for `decRate`.


### StrafeAirMoveType.maxBank

```lua
StrafeAirMoveType.maxBank : number?
```




### StrafeAirMoveType.maxPitch

```lua
StrafeAirMoveType.maxPitch : number?
```




### StrafeAirMoveType.maxAileron

```lua
StrafeAirMoveType.maxAileron : number?
```




### StrafeAirMoveType.maxElevator

```lua
StrafeAirMoveType.maxElevator : number?
```




### StrafeAirMoveType.maxRudder

```lua
StrafeAirMoveType.maxRudder : number?
```




### StrafeAirMoveType.attackSafetyDistance

```lua
StrafeAirMoveType.attackSafetyDistance : number?
```




### StrafeAirMoveType.myGravity

```lua
StrafeAirMoveType.myGravity : number?
```






{% endraw %}