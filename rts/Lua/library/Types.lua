---@meta

--------------------------------------------------------------------------------
-- Vectors
--------------------------------------------------------------------------------

---Cartesian double (XY)
---
---@class xy
---@helper
---@field [1] number x
---@field [2] number y

---@alias float2 xy

---Cartesian triple (XYZ)
---
---@class xyz
---@helper
---@field [1] number x
---@field [2] number y
---@field [3] number z

---@alias float3 xyz

---Cartesian quadruple (XYZW)
---
---@class xyzw
---@helper
---@field [1] number x
---@field [2] number y
---@field [3] number z
---@field [4] number w

---@alias float4 xyzw

--------------------------------------------------------------------------------
-- Color
--------------------------------------------------------------------------------

---Color triple (RGB)
---
---@class rgb
---@helper
---@field [1] number Red value.
---@field [2] number Green value.
---@field [3] number Blue value.

---Color quadruple (RGBA)
---
---@class rgba
---@helper
---@field [1] number Red value.
---@field [2] number Green value.
---@field [3] number Blue value.
---@field [4] number Alpha value.

---Indicator bytes representing color code operations during font rendering
---
---@class TextColorCode
---@helper
---@field Color string Indicates that the following bytes contain color code information
---@field ColorAndOutline string Indicates that the following bytes contain color code and outline information
---@field Reset string Indicates reset of the current color

--------------------------------------------------------------------------------
-- Camera
--------------------------------------------------------------------------------

---@alias CameraMode
---| 0 # fps
---| 1 # ta
---| 2 # spring
---| 3 # rot
---| 4 # free
---| 5 # ov
---| 6 # dummy

---@alias CameraName
---| "ta"
---| "spring"
---| "rot"
---| "ov"
---| "free"
---| "fps"
---| "dummy"

---Parameters for camera state.
---
---Highly dependent on the type of the current camera controller.
---
---@class CameraState
---@helper
---@field name CameraName The camera name.
---@field mode CameraMode The camera mode.
---@field fov number?
---@field px number? Position X of the ground point in screen center.
---@field py number? Position Y of the ground point in screen center.
---@field pz number? Position Z of the ground point in screen center.
---@field dx number? Camera direction vector X.
---@field dy number? Camera direction vector Y.
---@field dz number? Camera direction vector Z.
---@field rx number? Camera rotation angle on X axis. (spring)
---@field ry number? Camera rotation angle on Y axis. (spring)
---@field rz number? Camera rotation angle on Z axis. (spring)
---@field angle number? Camera rotation angle on X axis (aka tilt/pitch). (ta)
---@field flipped number? `-1` for when south is down, `1` for when north is down. (ta)
---@field dist number? Camera distance from the ground. (spring)
---@field height number? Camera distance from the ground. (ta)
---@field oldHeight number? Camera distance from the ground, cannot be changed. (rot)

--------------------------------------------------------------------------------
-- Object IDs
--------------------------------------------------------------------------------

---Identifier of a unit currently present in the simulation.
---
---IDs are drawn from a pool and are recycled, so the ID of a dead unit may
---later be handed out to a different unit.
---
---@alias UnitID integer

---Identifier of a unit definition, i.e. an index into `UnitDefs`.
---
---Valid IDs start at `1`; `0` is not a valid unit definition.
---
---@alias UnitDefID integer

---Identifier of a feature currently present in the simulation.
---
---IDs are drawn from a pool and are recycled, so the ID of a destroyed feature
---may later be handed out to a different feature.
---
---@alias FeatureID integer

---Identifier of a feature definition, i.e. an index into `FeatureDefs`.
---
---@alias FeatureDefID integer

---Identifier of a solid object, i.e. either a unit or a feature.
---
---Unit and feature IDs live in separate ranges, so which of the two is meant
---follows from context: `Spring.UnitRendering` functions take a unit ID where
---`Spring.FeatureRendering` functions take a feature ID, and callins that pass
---an object ID pass the type alongside it.
---
---@alias ObjectID UnitID|FeatureID

---Identifier of a projectile currently present in the simulation.
---
---Synced and unsynced projectiles are numbered independently.
---
---@alias ProjectileID integer

---Identifier of a weapon definition, i.e. an index into `WeaponDefs`,
---or a negated `CSolidObject::DamageType`
---
---@alias WeaponDefID integer

---Identifier of a ground decal.
---
---@alias DecalID integer

--------------------------------------------------------------------------------
-- Player and team IDs
--------------------------------------------------------------------------------

---Identifier of a team.
---
---Teams are numbered from `0`; `Spring.GetGaiaTeamID` returns the Gaia team.
---
---@alias TeamID integer

---Identifier of an allyteam.
---
---Allyteams are numbered from `0`.
---
---@alias AllyTeamID integer

---Identifier of a player.
---
---Players are numbered from `0`. Note that a player is a human client, which is
---not the same thing as a team.
---
---@alias PlayerID integer

--------------------------------------------------------------------------------
-- Unit groups
--------------------------------------------------------------------------------

---Identifier of a unit (control) group.
---
---Groups are per-team. IDs `0` to `9` are the hot-key groups that players can
---create and select directly; IDs from `10` up are "special" groups that can
---only be created programmatically. Interfaces that accept or return a group
---for a unit use `-1` to mean "no group".
---
---@alias GroupID integer

--------------------------------------------------------------------------------
-- Resources
--------------------------------------------------------------------------------

---@alias ResourceName "metal"|"energy"|"m"|"e"

---@alias StorageName "metalStorage"|"energyStorage"|"ms"|"es"

---@alias ResourceUsage table<ResourceName, number>
