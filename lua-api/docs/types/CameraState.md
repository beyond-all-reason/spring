---
layout: default
title: CameraState
parent: Lua API
permalink: lua-api/types/CameraState
---

{% raw %}

# class CameraState





Parameters for camera state.

Highly dependent on the type of the current camera controller.





## fields


### CameraState.name

```lua
CameraState.name : CameraName
```



The camera name.


### CameraState.mode

```lua
CameraState.mode : CameraMode
```



The camera mode.


### CameraState.fov

```lua
CameraState.fov : number?
```




### CameraState.px

```lua
CameraState.px : number?
```



Position X of the ground point in screen center.


### CameraState.py

```lua
CameraState.py : number?
```



Position Y of the ground point in screen center.


### CameraState.pz

```lua
CameraState.pz : number?
```



Position Z of the ground point in screen center.


### CameraState.dx

```lua
CameraState.dx : number?
```



Camera direction vector X.


### CameraState.dy

```lua
CameraState.dy : number?
```



Camera direction vector Y.


### CameraState.dz

```lua
CameraState.dz : number?
```



Camera direction vector Z.


### CameraState.rx

```lua
CameraState.rx : number?
```



Camera rotation angle on X axis. (spring)


### CameraState.ry

```lua
CameraState.ry : number?
```



Camera rotation angle on Y axis. (spring)


### CameraState.rz

```lua
CameraState.rz : number?
```



Camera rotation angle on Z axis. (spring)


### CameraState.angle

```lua
CameraState.angle : number?
```



Camera rotation angle on X axis (aka tilt/pitch). (ta)


### CameraState.flipped

```lua
CameraState.flipped : number?
```



`-1` for when south is down, `1` for when north is down. (ta)


### CameraState.dist

```lua
CameraState.dist : number?
```



Camera distance from the ground. (spring)


### CameraState.height

```lua
CameraState.height : number?
```



Camera distance from the ground. (ta)


### CameraState.oldHeight

```lua
CameraState.oldHeight : number?
```



Camera distance from the ground, cannot be changed. (rot)




{% endraw %}