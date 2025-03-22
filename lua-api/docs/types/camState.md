---
layout: default
title: camState
parent: Lua API
permalink: lua-api/types/camState
---

# class camState





Parameters for camera state

Highly dependent on the type of the current camera controller

[<a href="https://github.com/beyond-all-reason/spring/blob/4d8d14a2d6cb762d118e3e50f41c850673f13ca9/rts/Lua/LuaUnsyncedCtrl.cpp#L1099-L1123" target="_blank">source</a>]





## fields


### camState.name

```lua
camState.name : ("ta"|"spring"|"rot"|"ov"|"free"|"fps"|"dummy")
```




### camState.mode

```lua
camState.mode : number
```



the camera mode: 0 (fps), 1 (ta), 2 (spring), 3 (rot), 4 (free), 5 (ov), 6 (dummy)


### camState.fov

```lua
camState.fov : number
```




### camState.px

```lua
camState.px : number
```



Position X of the ground point in screen center


### camState.py

```lua
camState.py : number
```



Position Y of the ground point in screen center


### camState.pz

```lua
camState.pz : number
```



Position Z of the ground point in screen center


### camState.dx

```lua
camState.dx : number
```



Camera direction vector X


### camState.dy

```lua
camState.dy : number
```



Camera direction vector Y


### camState.dz

```lua
camState.dz : number
```



Camera direction vector Z


### camState.rx

```lua
camState.rx : number
```



Camera rotation angle on X axis (spring)


### camState.ry

```lua
camState.ry : number
```



Camera rotation angle on Y axis (spring)


### camState.rz

```lua
camState.rz : number
```



Camera rotation angle on Z axis (spring)


### camState.angle

```lua
camState.angle : number
```



Camera rotation angle on X axis (aka tilt/pitch) (ta)


### camState.flipped

```lua
camState.flipped : (call)
```



for when south is down, 1 for when north is down (ta)


### camState.dist

```lua
camState.dist : number
```



Camera distance from the ground (spring)


### camState.height

```lua
camState.height : number
```



Camera distance from the ground (ta)


### camState.oldHeight

```lua
camState.oldHeight : number
```



Camera distance from the ground, cannot be changed (rot)


