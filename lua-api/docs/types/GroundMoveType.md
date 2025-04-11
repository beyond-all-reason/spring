---
layout: default
title: GroundMoveType
parent: Lua API
permalink: lua-api/types/GroundMoveType
---

{% raw %}

# class GroundMoveType


- supers: GenericMoveType {
    maxSpeed: number?,
    maxWantedSpeed: number?,
    maneuverLeash: number?,
    waterline: number?,
    useWantedSpeed[0]: boolean?,
    useWantedSpeed[1]: boolean?,
}




[<a href="https://github.com/beyond-all-reason/spring/blob/625902d539f43871ceb4ecdc45fc539e91a71b55/rts/Lua/LuaSyncedMoveCtrl.cpp#L907-L922" target="_blank">source</a>]





## fields


### GroundMoveType.atGoal

```lua
GroundMoveType.atGoal : boolean?
```




### GroundMoveType.atEndOfPath

```lua
GroundMoveType.atEndOfPath : boolean?
```




### GroundMoveType.pushResistant

```lua
GroundMoveType.pushResistant : boolean?
```




### GroundMoveType.minScriptChangeHeading

```lua
GroundMoveType.minScriptChangeHeading : integer?
```




### GroundMoveType.turnRate

```lua
GroundMoveType.turnRate : number?
```




### GroundMoveType.turnAccel

```lua
GroundMoveType.turnAccel : number?
```




### GroundMoveType.accRate

```lua
GroundMoveType.accRate : number?
```




### GroundMoveType.decRate

```lua
GroundMoveType.decRate : number?
```




### GroundMoveType.myGravity

```lua
GroundMoveType.myGravity : number?
```




### GroundMoveType.maxReverseDist

```lua
GroundMoveType.maxReverseDist : number?
```




### GroundMoveType.minReverseAngle

```lua
GroundMoveType.minReverseAngle : number?
```




### GroundMoveType.maxReverseSpeed

```lua
GroundMoveType.maxReverseSpeed : number?
```




### GroundMoveType.sqSkidSpeedMult

```lua
GroundMoveType.sqSkidSpeedMult : number?
```






{% endraw %}