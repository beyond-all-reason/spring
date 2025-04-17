---
layout: post
title: Units of measurement
parent: Articles
permalink: articles/units-of-measurement
author: sprunk
---

## Units of measurement

In addition to standard units of measurement such as seconds or radians, Recoil uses some in-house units that warrant a bit of explanation.

### Base

 * **frames**. These represent the discrete, fundamental unit of time for simulation purposes.
Everything in a simulation happens between two frames.
Currently, a simulation at x1 speed runs frames at a constant 30 Hz (so a frame is 0.033s).
On the other hand, it can run slower or faster depending on gamespeed (including at thousands Hz when catching up or skipping over a replay).
Therefore, do not use frames to measure time for things that should use wall-clock time (for example interface animations), i.e. use `widget:Update(dt)` over `widget:GameFrame(f)`.
The base frequency value in Hz is available to Lua as `Game.gameSpeed`.
It is currently hardcoded by the engine and not configurable by games (despite being in the `Game` table and not `Engine`).

* **elmos**. "Elmo" is the name for Recoil's arbitrary unit of distance. It is purposefully underdefined so that each game can have its own sense of scale.
For example, maybe one game is some sort of galactic war and 1 elmo represents 1 parsec, while another game is about a war between bacteria and viruses and 1 elmo is 1 μm.
Most existing content seems to assume it's 1 m or 12.5 cm, or within this order of magnitude, but there is nothing to enforce consistency.
Almost all length/distance values are given in elmos (or derived values such as elmo/s for speed), unless otherwise noted.

* **arbitrary**: many values, such as mass, are also in arbitrary units that a game could define on its own if it wanted to for world-building reasons.
Unlike the elmo these aren't even named, so they're typically referred to as just, for example, "100 mass", "100 energy", or "100 map hardness".

### Derived

* **game square**. The map is divided into a grid of squares.
The high-resolution yardmap grid is in squares, as is the heightmap (height is an interpolation between the corners of a square).
The length of the edge of a game square in elmo is available as `Game.squareSize` to Lua.
It is not configurable by games and is currently hard-coded to 8 (so a 1x1 square is 8x8 elmos).
Note that for map creation, the supplied heightmap represents corners of squares (which is why its size has to be N/8 + 1).
* **footprint square**. Footprints in unit defs are defined in squares of game squares (for historical reasons).
Regular-resolution yardmap is defined per footprint square.
The length of the edge of the footprint square in game squares is available to Lua as `Game.footprintScale` and is currently hard-coded to 2 (so a 1x1 footprint is 2x2 game squares).
* **build square**. The grid to which construction of buildings is aligned.
Similarly to footprints, it's in multiples of a regular game square.
The length of the edge of a build square in elmos is available as `Game.buildSquareSize` to Lua and is currently hard-coded to 16 (which is the same as a footprint square, meaning that except for high-resolution yardmaps you can always fit buildings next to each other tightly).
* **metalmap square**. The metal-map is divided into a grid which covers multiple game squares.
Interfaces such as `Spring.GetMetalAmount` accept the co-ordinates in this unit.
It is available to Lua as `Game.metalMapSquareSize` and its value is currently 16.
Note that for map creation, the supplied metalmap represents the insides of metalmap squares (which is why its size has to be N/16).
* **lobby map size**. This is the size of maps typically shown in lobbies and other such places.
A 1x1 map is 512x512 elmos.
This is merely a convention (though a strong one for historical reasons) and thus is not directly available to Lua (you can derive it via `Game.mapSizeX / Game.mapX` if you have a map loaded, though there isn't much point because `Game.mapX` is already about the only useful value in this unit).
* **slow update**. Some performance-heavy things only happen to units once per slow-update.
A slow-update happens once per 15 frames.
It is a bit of an implementation detail, so it's not directly exposed, though some def entries related to allowing things to run more often are usually capped at the slow-update rate.
* **TA angular unit**. A full circle is 65536 TA angular units.
Used in some unit and weapon def entries - consult defs documentation for specifics.
