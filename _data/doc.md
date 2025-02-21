# ActivateGame


```lua
function ActivateGame()
```


---

# ActivateMenu


```lua
function ActivateMenu()
```


---

# ActiveCommandChanged


```lua
function ActiveCommandChanged(cmdId?: integer, cmdType?: integer)
```


---

# ActiveUniform

## length


```lua
integer
```

The character length of `name`.

## location


```lua
GL
```

## name


```lua
string
```

## size


```lua
integer
```

## type


```lua
string
```

String name of `GL_*` constant.


---

# AddConsoleLine


```lua
function AddConsoleLine(msg: string, priority: integer)
```


---

# AllowBuilderHoldFire


```lua
function AllowBuilderHoldFire(unitID: integer, unitDefID: integer, action: number)
  -> actionAllowed: boolean
```


---

# AllowCommand


```lua
function AllowCommand(unitID: integer, unitDefID: integer, unitTeam: integer, cmdID: integer, cmdParams: number[], cmdOptions: CommandOptions, cmdTag: number, synced: boolean, fromLua: boolean)
  -> whether: boolean
```


---

# AllowDirectUnitControl


```lua
function AllowDirectUnitControl(unitID: integer, unitDefID: integer, unitTeam: integer, playerID: integer)
  -> allow: boolean
```


---

# AllowDraw


```lua
function AllowDraw()
  -> allowDraw: boolean
```


---

# AllowFeatureBuildStep


```lua
function AllowFeatureBuildStep(builderID: integer, builderTeam: integer, featureID: integer, featureDefID: integer, part: number)
  -> whether: boolean
```


---

# AllowFeatureCreation


```lua
function AllowFeatureCreation(featureDefID: integer, teamID: integer, x: number, y: number, z: number)
  -> whether: boolean
```


---

# AllowResourceLevel


```lua
function AllowResourceLevel(teamID: integer, res: string, level: number)
  -> whether: boolean
```


---

# AllowResourceTransfer


```lua
function AllowResourceTransfer(oldTeamID: integer, newTeamID: integer, res: string, amount: number)
  -> whether: boolean
```


---

# AllowStartPosition


```lua
function AllowStartPosition(playerID: integer, teamID: integer, readyState: number, clampedX: number, clampedY: number, clampedZ: number, rawX: number, rawY: number, rawZ: number)
  -> allow: boolean
```


---

# AllowUnitBuildStep


```lua
function AllowUnitBuildStep(builderID: integer, builderTeam: integer, unitID: integer, unitDefID: integer, part: number)
  -> whether: boolean
```


---

# AllowUnitCaptureStep


```lua
function AllowUnitCaptureStep(builderID: integer, builderTeam: integer, unitID: integer, unitDefID: integer, part: number)
  -> whether: boolean
```


---

# AllowUnitCloak


```lua
function AllowUnitCloak(unitID: integer, enemyID?: integer)
  -> whether: boolean
```


```lua
function AllowUnitCloak(unitID: integer, objectID?: integer, weaponNum?: number)
  -> whether: boolean
```


---

# AllowUnitCreation


```lua
function AllowUnitCreation(unitDefID: integer, builderID: integer, builderTeam: integer, x: number, y: number, z: number, facing: number)
  -> allow: boolean
  2. dropOrder: boolean
```


---

# AllowUnitKamikaze


```lua
function AllowUnitKamikaze(unitID: integer, targetID: integer)
  -> whether: boolean
```


---

# AllowUnitTransfer


```lua
function AllowUnitTransfer(unitID: integer, unitDefID: integer, oldTeam: integer, newTeam: integer, capture: boolean)
  -> whether: boolean
```


---

# AllowUnitTransport


```lua
function AllowUnitTransport(transporterID: integer, transporterUnitDefID: integer, transporterTeam: integer, transporteeID: integer, transporteeUnitDefID: integer, transporteeTeam: integer)
  -> whether: boolean
```


---

# AllowUnitTransportLoad


```lua
function AllowUnitTransportLoad(transporterID: integer, transporterUnitDefID: integer, transporterTeam: integer, transporteeID: integer, transporteeUnitDefID: integer, transporteeTeam: integer, x: number, y: number, z: number)
  -> whether: boolean
```


---

# AllowUnitTransportUnload


```lua
function AllowUnitTransportUnload(transporterID: integer, transporterUnitDefID: integer, transporterTeam: integer, transporteeID: integer, transporteeUnitDefID: integer, transporteeTeam: integer, x: number, y: number, z: number)
  -> whether: boolean
```


---

# AllowWeaponInterceptTarget


```lua
function AllowWeaponInterceptTarget(interceptorUnitID: integer, interceptorWeaponID: integer, targetProjectileID: integer)
  -> allowed: boolean
```


---

# AllowWeaponTarget


```lua
function AllowWeaponTarget(attackerID: integer, targetID: integer, attackerWeaponNum: integer, attackerWeaponDefID: integer, defPriority: number)
  -> allowed: boolean
  2. the: number
```


---

# AllowWeaponTargetCheck


```lua
function AllowWeaponTargetCheck(attackerID: integer, attackerWeaponNum: integer, attackerWeaponDefID: integer)
  -> allowCheck: boolean
  2. ignoreCheck: boolean
```


---

# AtmosphereParams

## cloudColor


```lua
rgba
```

Color quadruple (RGBA)


## fogEnd


```lua
number
```

## fogStart


```lua
number
```

## skyColor


```lua
rgba
```

Color quadruple (RGBA)


## sunColor


```lua
rgba
```

Color quadruple (RGBA)



---

# BuildOrderBlockedStatus


---

# CMD


```lua
enum CMD
```


---

# CMDTYPE


```lua
enum CMDTYPE
```


---

# COB


```lua
enum COB
```


---

# CameraMode


---

# CameraState

## angle


```lua
number?
```

Camera rotation angle on X axis (aka tilt/pitch) (ta)

## dist


```lua
number?
```

Camera distance from the ground (spring)

## dx


```lua
number?
```

Camera direction vector X

## dy


```lua
number?
```

Camera direction vector Y

## dz


```lua
number?
```

Camera direction vector Z

## flipped


```lua
number?
```

1 for when south is down, 1 for when north is down (ta)

## fov


```lua
number?
```

## height


```lua
number?
```

Camera distance from the ground (ta)

## mode


```lua
0|1|2|3|4...(+2)
```

The camera mode

## name


```lua
"dummy"|"fps"|"free"|"ov"|"rot"...(+2)
```


Highly dependent on the type of the current camera controller

## oldHeight


```lua
number?
```

Camera distance from the ground, cannot be changed (rot)

## px


```lua
number?
```

Position X of the ground point in screen center

## py


```lua
number?
```

Position Y of the ground point in screen center

## pz


```lua
number?
```

Position Z of the ground point in screen center

## rx


```lua
number?
```

Camera rotation angle on X axis (spring)

## ry


```lua
number?
```

Camera rotation angle on Y axis (spring)

## rz


```lua
number?
```

Camera rotation angle on Z axis (spring)


---

# CameraVectors

## botFrustumPlane


```lua
xyz
```

Cartesian triple (XYZ)


## forward


```lua
xyz
```

Cartesian triple (XYZ)


## lftFrustumPlane


```lua
xyz
```

Cartesian triple (XYZ)


## rgtFrustumPlane


```lua
xyz
```

Cartesian triple (XYZ)


## right


```lua
xyz
```

Cartesian triple (XYZ)


## topFrustumPlane


```lua
xyz
```

Cartesian triple (XYZ)


## up


```lua
xyz
```

Cartesian triple (XYZ)



---

# Command

## cmdID


```lua
integer
```

## options


```lua
CommandOptions?
```

Parameters for command options


## params


```lua
number[]?
```


---

# CommandDescription

## action


```lua
string?
```

## cursor


```lua
string?
```

## disabled


```lua
boolean?
```

## hidden


```lua
boolean?
```

## id


```lua
integer?
```

## name


```lua
string?
```

## onlyTexture


```lua
boolean?
```

## params


```lua
table<string, string>?
```

## queueing


```lua
boolean?
```

## showUnique


```lua
boolean?
```

## texture


```lua
string?
```

## tooltip


```lua
string?
```

## type


```lua
integer?
```


---

# CommandFallback


```lua
function CommandFallback(unitID: integer, unitDefID: integer, unitTeam: integer, cmdID: integer, cmdParams: number[], cmdOptions: CommandOptions, cmdTag: number)
  -> whether: boolean
```


---

# CommandNotify


```lua
function CommandNotify(cmdID: integer, cmdParams: table, options: CommandOptions)
  -> Returning: boolean
```


---

# CommandOptions

## alt


```lua
boolean
```

Alt key pressed

## coded


```lua
integer
```

## ctrl


```lua
boolean
```

Ctrl key pressed

## internal


```lua
boolean
```

## meta


```lua
boolean
```

Meta key pressed (space)

## right


```lua
boolean
```

Right mouse key pressed

## shift


```lua
boolean
```

Shift key pressed


---

# Configuration

## declarationFile


```lua
string
```

## declarationLine


```lua
string
```

## defaultValue


```lua
string
```

## description


```lua
string
```

## maximumValue


```lua
string
```

## minimumValue


```lua
string
```

## name


```lua
string
```

## readOnly


```lua
boolean
```

## safemodeValue


```lua
string
```

## type


```lua
string
```


---

# ConfigureLayout


```lua
function ConfigureLayout()
```


---

# CreateRBOData

## format


```lua
GL
```

## samples


```lua
number?
```

any number here will result in creation of multisampled RBO

## target


```lua
GL
```


---

# DefaultCommand


```lua
function DefaultCommand(type: string, id: integer)
```


---

# DownloadFailed


```lua
function DownloadFailed(id: number, errorID: number)
```


---

# DownloadFinished


```lua
function DownloadFinished(id: number)
```


---

# DownloadProgress


```lua
function DownloadProgress(id: number, downloaded: number, total: number)
```


---

# DownloadQueued


```lua
function DownloadQueued(id: number, name: string, type: string)
```


---

# DownloadStarted


```lua
function DownloadStarted(id: number)
```


---

# DrawFeature


```lua
function DrawFeature(featureID: integer, drawMode: number)
  -> suppressEngineDraw: boolean
```


---

# DrawFeaturesPostDeferred


```lua
function DrawFeaturesPostDeferred()
```


---

# DrawGenesis


```lua
function DrawGenesis()
```


---

# DrawGroundDeferred


```lua
function DrawGroundDeferred()
```


---

# DrawGroundPostDeferred


```lua
function DrawGroundPostDeferred()
```


---

# DrawGroundPostForward


```lua
function DrawGroundPostForward()
```


---

# DrawGroundPreDeferred


```lua
function DrawGroundPreDeferred()
```


---

# DrawGroundPreForward


```lua
function DrawGroundPreForward()
```


---

# DrawInMiniMap


```lua
function DrawInMiniMap(sx: number, sy: number)
```


---

# DrawInMinimapBackground


```lua
function DrawInMinimapBackground(sx: number, sy: number)
```


---

# DrawLoadScreen


```lua
function DrawLoadScreen()
```


---

# DrawMaterial


```lua
function DrawMaterial(uuid: number, drawMode: number)
  -> suppressEngineDraw: boolean
```


---

# DrawPreDecals


```lua
function DrawPreDecals()
```


---

# DrawProjectile


```lua
function DrawProjectile(projectileID: integer, drawMode: number)
  -> suppressEngineDraw: boolean
```


---

# DrawScreen


```lua
function DrawScreen(viewSizeX: number, viewSizeY: number)
```


---

# DrawScreenEffects


```lua
function DrawScreenEffects(viewSizeX: number, viewSizeY: number)
```


---

# DrawScreenPost


```lua
function DrawScreenPost(viewSizeX: number, viewSizeY: number)
```


---

# DrawShadowFeaturesLua


```lua
function DrawShadowFeaturesLua()
```


---

# DrawShadowPassTransparent


```lua
function DrawShadowPassTransparent()
```


---

# DrawShadowUnitsLua


```lua
function DrawShadowUnitsLua()
```


---

# DrawShield


```lua
function DrawShield(featureID: integer, weaponID: integer, drawMode: number)
  -> suppressEngineDraw: boolean
```


---

# DrawUnit


```lua
function DrawUnit(unitID: integer, drawMode: number)
  -> suppressEngineDraw: boolean
```


---

# DrawUnitsPostDeferred


```lua
function DrawUnitsPostDeferred()
```


---

# DrawWaterPost


```lua
function DrawWaterPost()
```


---

# DrawWorld


```lua
function DrawWorld()
```


---

# DrawWorldPreParticles


```lua
function DrawWorldPreParticles(drawAboveWater: boolean, drawBelowWater: boolean, drawReflection: boolean, drawRefraction: boolean)
```


---

# DrawWorldPreUnit


```lua
function DrawWorldPreUnit()
```


---

# DrawWorldReflection


```lua
function DrawWorldReflection()
```


---

# DrawWorldRefraction


```lua
function DrawWorldRefraction()
```


---

# DrawWorldShadow


```lua
function DrawWorldShadow()
```


---

# Engine


```lua
table
```


---

# Explosion


```lua
function Explosion(weaponDefID: number, px: number, py: number, pz: number, attackerID: number, projectileID: number)
  -> noGfx: boolean
```


---

# ExplosionParams

## craterAreaOfEffect


```lua
number
```

## damageAreaOfEffect


```lua
number
```

## damageGround


```lua
boolean
```

## edgeEffectiveness


```lua
number
```

## explosionSpeed


```lua
number
```

## gfxMod


```lua
number
```

## hitFeature


```lua
number
```

## hitUnit


```lua
number
```

## ignoreOwner


```lua
boolean
```

## impactOnly


```lua
boolean
```

## owner


```lua
number
```

## weaponDef


```lua
number
```


---

# Facing


---

# Fbo

## color0


```lua
attachment
```

attachment ::= luaTex or `RBO.rbo` or nil or { luaTex [, num target [, num level ] ] }


## color1


```lua
attachment
```

attachment ::= luaTex or `RBO.rbo` or nil or { luaTex [, num target [, num level ] ] }


## color15


```lua
attachment
```

attachment ::= luaTex or `RBO.rbo` or nil or { luaTex [, num target [, num level ] ] }


## color2


```lua
attachment
```

attachment ::= luaTex or `RBO.rbo` or nil or { luaTex [, num target [, num level ] ] }


## colorn


```lua
attachment
```

attachment ::= luaTex or `RBO.rbo` or nil or { luaTex [, num target [, num level ] ] }


## depth


```lua
attachment
```

attachment ::= luaTex or `RBO.rbo` or nil or { luaTex [, num target [, num level ] ] }


## drawbuffers


```lua
table
```

`{ GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT3_EXT, ..}`

## readbuffer


```lua
table
```

`GL_COLOR_ATTACHMENT0_EXT`

## stencil


```lua
attachment
```

attachment ::= luaTex or `RBO.rbo` or nil or { luaTex [, num target [, num level ] ] }



---

# FeatureCreated


```lua
function FeatureCreated(featureID: number, allyTeamID: number)
```


---

# FeatureDamaged


```lua
function FeatureDamaged(featureID: number, featureDefID: number, featureTeam: number, damage: number, weaponDefID: number, projectileID: number, attackerID: number, attackerDefID: number, attackerTeam: number)
```


---

# FeatureDestroyed


```lua
function FeatureDestroyed(featureID: number, allyTeamID: number)
```


---

# FeaturePreDamaged


```lua
function FeaturePreDamaged(featureID: integer, featureDefID: integer, featureTeam: integer, damage: number, weaponDefID: integer, projectileID: integer, attackerID: integer, attackerDefID: integer, attackerTeam: integer)
  -> newDamage: number
  2. impulseMult: number
```


---

# FeatureSupport

## NegativeGetUnitCurrentCommand


```lua
boolean
```

## hasExitOnlyYardmaps


```lua
boolean
```

## rmlUiApiVersion


```lua
integer
```


---

# FontsChanged


```lua
function FontsChanged()
```


---

# GL


```lua
enum GL
```


---

# GLBufferType


---

# Game


```lua
table
```


---

# GameFrame


```lua
function GameFrame(frame: number)
```


---

# GameFramePost


```lua
function GameFramePost(frame: number)
```


---

# GameID


```lua
function GameID(gameID: string)
```


---

# GameOver


```lua
function GameOver(winningAllyTeams: number[])
```


---

# GamePaused


```lua
function GamePaused(playerID: number, paused: boolean)
```


---

# GamePreload


```lua
function GamePreload()
```


---

# GameProgress


```lua
function GameProgress(serverFrameNum: integer)
```


---

# GameSetup


```lua
function GameSetup(state: string, ready: boolean, playerStates: table)
  -> success: boolean
  2. newReady: boolean
```


---

# GameStart


```lua
function GameStart()
```


---

# GetTooltip


```lua
function GetTooltip(x: number, y: number)
  -> tooltip: string
```


---

# GotChatMsg


```lua
function GotChatMsg(msg: string, playerID: number)
```


---

# GroupChanged


```lua
function GroupChanged(groupID: number)
```


---

# Initialize


```lua
function Initialize()
```


---

# IsAbove


```lua
function IsAbove(x: number, y: number)
  -> isAbove: boolean
```


---

# KeyBinding

## boundWith


```lua
string
```

## command


```lua
string
```

## extra


```lua
string
```


---

# KeyMapChanged


```lua
function KeyMapChanged()
```


---

# KeyModifiers

## alt


```lua
boolean
```

Alt key pressed

## ctrl


```lua
boolean
```

Ctrl key pressed

## right


```lua
boolean
```

Right mouse key pressed

## shift


```lua
boolean
```

Shift key pressed


---

# KeyPress


```lua
function KeyPress(keyCode: number, mods: KeyModifiers, isRepeat: boolean, label: boolean, utf32char: number, scanCode: number, actionList: table)
  -> halt: boolean
```


---

# KeyRelease


```lua
function KeyRelease(keyCode: number, mods: KeyModifiers, label: boolean, utf32char: number, scanCode: number, actionList: table)
  -> boolean
```


---

# LightParams

## ambientColor


```lua
{ red: number, green: number, blue: number }
```

## ambientDecayRate


```lua
{ ambientRedDecay: number, ambientGreenDecay: number, ambientBlueDecay: number }
```

## decayFunctionType


```lua
{ ambientDecayType: number, diffuseDecayType: number, specularDecayType: number }
```

Per-frame decay of `specularColor` (spread over TTL frames)

## diffuseColor


```lua
{ red: number, green: number, blue: number }
```

## diffuseDecayRate


```lua
{ diffuseRedDecay: number, diffuseGreenDecay: number, diffuseBlueDecay: number }
```

Per-frame decay of `ambientColor` (spread over TTL frames)

## direction


```lua
{ dx: number, dy: number, dz: number }
```

## fov


```lua
number
```

## ignoreLOS


```lua
boolean
```

## intensityWeight


```lua
{ ambientWeight: number, diffuseWeight: number, specularWeight: number }
```

## position


```lua
{ px: number, py: number, pz: number }
```

## priority


```lua
number
```

## radius


```lua
number
```

If value is `0.0` then the `*DecayRate` values will be interpreted as linear, otherwise exponential.

## specularColor


```lua
{ red: number, green: number, blue: number }
```

## specularDecayRate


```lua
{ specularRedDecay: number, specularGreenDecay: number, specularBlueDecay: number }
```

Per-frame decay of `diffuseColor` (spread over TTL frames)

## ttl


```lua
number
```


---

# Load


```lua
function Load(zipReader: table)
```


---

# LoadCode


```lua
function LoadCode()
```


---

# LoadProgress


```lua
function LoadProgress(message: string, replaceLastLine: boolean)
```


---

# LogLevel


---

# LuaZipFileReader


---

# LuaZipFileWriter


---

# MapDrawCmd


```lua
function MapDrawCmd(playerID: number, type: string, posX: number, posY: number, posZ: number, data4: string|number, pos2Y?: number, pos2Z?: number)
```


---

# MapRenderingParams

## splatDetailNormalDiffuseAlpha


```lua
boolean
```

## splatTexMults


```lua
rgba
```

Color quadruple (RGBA)


## splatTexScales


```lua
rgba
```

Color quadruple (RGBA)


## voidGround


```lua
boolean
```

## voidWater


```lua
boolean
```


---

# MouseMove


```lua
function MouseMove(x: number, y: number, dx: number, dy: number, button: number)
```


---

# MousePress


```lua
function MousePress(x: number, y: number, button: number)
  -> becomeMouseOwner: boolean
```


---

# MouseRelease


```lua
function MouseRelease(x: number, y: number, button: number)
  -> becomeMouseOwner: boolean
```


---

# MouseWheel


```lua
function MouseWheel(up: boolean, value: number)
```


---

# MoveCtrlNotify


```lua
function MoveCtrlNotify(unitID: integer, unitDefID: integer, unitTeam: integer, data: number)
  -> whether: boolean
```


---

# PieceInfo

## children


```lua
string[]
```

names

## empty


```lua
boolean
```

## max


```lua
[number, number, number]
```

(x,y,z)

## min


```lua
[number, number, number]
```

(x,y,z)

## name


```lua
string
```

## offset


```lua
[number, number, number]
```

(x,y,z)

## parent


```lua
string
```


---

# Plane

## d


```lua
number
```

## normalVecX


```lua
number
```

## normalVecY


```lua
number
```

## normalVecZ


```lua
number
```


---

# Platform


```lua
table
```


---

# PlayerAdded


```lua
function PlayerAdded(playerID: number)
```


---

# PlayerChanged


```lua
function PlayerChanged(playerID: number)
```


---

# PlayerRemoved


```lua
function PlayerRemoved(playerID: number, reason: string)
```


---

# ProjectileCreated


```lua
function ProjectileCreated(proID: number, proOwnerID: number, weaponDefID: number)
```


---

# ProjectileDestroyed


```lua
function ProjectileDestroyed(proID: number, ownerID: number, proWeaponDefID: number)
```


---

# ProjectileParams

## cegTag


```lua
string
```

## endAlpha


```lua
number
```

## error


```lua
xyz
```

Cartesian triple (XYZ)


## gravity


```lua
number
```

## maxRange


```lua
number
```

## model


```lua
string
```

## owner


```lua
integer
```

## pos


```lua
xyz
```

Cartesian triple (XYZ)


## speed


```lua
xyz
```

Cartesian triple (XYZ)


## spread


```lua
xyz
```

Cartesian triple (XYZ)


## startAlpha


```lua
number
```

## team


```lua
integer
```

## tracking


```lua
number
```

## ttl


```lua
number
```


---

# RBO

## format


```lua
GL
```

## samples


```lua
integer
```

will return globalRendering->msaaLevel for multisampled RBO or 0 otherwise

## target


```lua
GL
```

## valid


```lua
boolean
```

## xsize


```lua
integer
```

## ysize


```lua
integer
```


---

# RecvFromSynced


```lua
function RecvFromSynced(arg1: any, arg2: any, argn: any)
```


---

# RecvLuaMsg


```lua
function RecvLuaMsg(msg: string, playerID: number)
```


---

# RecvSkirmishAIMessage


```lua
function RecvSkirmishAIMessage(aiTeam: integer, dataStr: string)
```


---

# RenderUnitDestroyed


```lua
function RenderUnitDestroyed(unitID: integer, unitDefID: integer, unitTeam: integer)
```


---

# ResourceCost

## energy


```lua
number
```

## metal


```lua
number
```


---

# ResourceName


---

# ResourceUsage


---

# Roster

## allyTeamID


```lua
integer
```

## cpuUsage


```lua
number
```

in order to find the progress, use: cpuUsage&0x1 if it's PC or BO, cpuUsage& 0xFE to get path res, (cpuUsage>>8)*1000 for the progress

## name


```lua
string
```

## pingTime


```lua
number
```

if -1, the player is pathfinding

## playerID


```lua
integer
```

## spectator


```lua
boolean
```

## teamID


```lua
integer
```


---

# RulesParams


---

# SFX


```lua
enum SFX
```


---

# Save


```lua
function Save(zip: table)
```


---

# SetWMCaption


```lua
function SetWMCaption(title: string, titleShort?: string)
  -> nil
```


---

# ShaderParams

## definitions


```lua
string?
```

string of shader #defines"

## fragment


```lua
string?
```


The "Geometry" or Geometry-shader can create new vertices and vertice-stripes
from points.

## geoInputType


```lua
integer?
```

inType

## geoOutputType


```lua
integer?
```

outType

## geoOutputVerts


```lua
integer?
```

maxVerts

## geometry


```lua
string?
```


The "TES" or Tesselation Evaluation Shader takes the abstract patch generated
by the tessellation primitive generation stage, as well as the actual vertex
data for the entire patch, and generates a particular vertex from it. Each
TES invocation generates a single vertex. It can also take per-patch data
provided by the Tessellation Control Shader.

## tcs


```lua
string?
```


The "Vertex" or vertex-shader is your GLSL-Code as string, its written in a
C-Dialect.  This shader is busy deforming the geometry of a unit but it can
not create new polygons. Use it for waves, wobbling surfaces etc.

## tes


```lua
string?
```


The "TCS" or Tesselation Control Shader controls how much tessellation a
particular patch gets; it also defines the size of a patch, thus allowing it
to augment data. It can also filter vertex data taken from the vertex shader.
The main purpose of the TCS is to feed the tessellation levels to the
Tessellation primitive generator stage, as well as to feed patch data (as its
output values) to the Tessellation Evaluation Shader stage.

## uniform


```lua
UniformParam<number>?
```


The "Fragment" or Fragment-shader (sometimes called pixel-Shader) is post
processing the already rendered picture (for example drawing stars on the
sky).

Remember textures are not always 2 dimensional pictures. They can contain
information about the depth, or the third value marks areas and the strength
at which these are processed.

## uniformFloat


```lua
UniformParam<number>?
```

## uniformInt


```lua
UniformParam<integer>?
```

## uniformMatrix


```lua
UniformParam<number>?
```

## vertex


```lua
string?
```


---

# ShieldPreDamaged


```lua
function ShieldPreDamaged(projectileID: integer, projectileOwnerID: integer, shieldWeaponNum: integer, shieldCarrierID: integer, bounceProjectile: boolean, beamEmitterWeaponNum: integer, beamEmitterUnitID: integer, startX: number, startY: number, startZ: number, hitX: number, hitY: number, hitZ: number)
  -> if: boolean
```


---

# Shutdown


```lua
function Shutdown()
  -> nil
```


---

# SideSpec

## caseName


```lua
string
```

## sideName


```lua
string
```


Used when returning arrays of side specifications, is itself an array with
positional values as below:

## startUnit


```lua
string
```


---

# SoundChannel


---

# SoundDeviceSpec

## name


```lua
string
```


Contains data about a sound device.


---

# Spring.AddGrass


```lua
function Spring.AddGrass(x: number, z: number)
  -> nil
```


---

# Spring.AddHeightMap


```lua
function Spring.AddHeightMap(x: number, z: number, height: number)
  -> newHeight: integer?
```


---

# Spring.AddLightTrackingTarget


```lua
function Spring.AddLightTrackingTarget()
```


---

# Spring.AddMapLight


```lua
function Spring.AddMapLight(lightParams: LightParams)
  -> lightHandle: integer
```


---

# Spring.AddModelLight


```lua
function Spring.AddModelLight(lightParams: LightParams)
  -> lightHandle: number
```


---

# Spring.AddObjectDecal


```lua
function Spring.AddObjectDecal(unitID: integer)
  -> nil
```


---

# Spring.AddOriginalHeightMap


```lua
function Spring.AddOriginalHeightMap(x: number, y: number, height: number)
  -> nil
```


---

# Spring.AddSmoothMesh


```lua
function Spring.AddSmoothMesh(x: number, z: number, height: number)
  -> The: number?
```


---

# Spring.AddTeamResource


```lua
function Spring.AddTeamResource(teamID: integer, type: "e"|"energy"|"m"|"metal", amount: number)
  -> nil
```


---

# Spring.AddUnitDamage


```lua
function Spring.AddUnitDamage(unitID: integer, damage: number, paralyze?: number, attackerID?: integer, weaponID?: integer, impulseX?: number, impulseY?: number, impulseZ?: number)
  -> nil
```


---

# Spring.AddUnitExperience


```lua
function Spring.AddUnitExperience(unitID: integer, deltaExperience: number)
  -> nil
```


---

# Spring.AddUnitIcon


```lua
function Spring.AddUnitIcon(iconName: string, texFile: string, size?: number, dist?: number, radAdjust?: number)
  -> added: boolean
```


---

# Spring.AddUnitImpulse


```lua
function Spring.AddUnitImpulse(unitID: integer, x: number, y: number, z: number, decayRate?: number)
  -> nil
```


---

# Spring.AddUnitResource


```lua
function Spring.AddUnitResource(unitID: integer, resource: string, amount: number)
  -> nil
```


---

# Spring.AddUnitSeismicPing


```lua
function Spring.AddUnitSeismicPing(unitID: integer, pindSize: number)
  -> nil
```


---

# Spring.AddWorldIcon


```lua
function Spring.AddWorldIcon(cmdID: integer, posX: number, posY: number, posZ: number)
  -> nil
```


---

# Spring.AddWorldText


```lua
function Spring.AddWorldText(text: string, posX: number, posY: number, posZ: number)
  -> nil
```


---

# Spring.AddWorldUnit


```lua
function Spring.AddWorldUnit(unitDefID: integer, posX: number, posY: number, posZ: number, teamID: integer, facing: number)
  -> nil
```


---

# Spring.AdjustHeightMap


```lua
function Spring.AdjustHeightMap(x1: number, y1: number, x2_height: number, y2?: number, height?: number)
  -> nil
```


---

# Spring.AdjustOriginalHeightMap


```lua
function Spring.AdjustOriginalHeightMap(x1: number, y1: number, x2_height: number, y2?: number, height?: number)
  -> nil
```


---

# Spring.AdjustSmoothMesh


```lua
function Spring.AdjustSmoothMesh(x1: number, z1: number, x2?: number, z2?: number, height: number)
  -> nil
```


---

# Spring.AreHelperAIsEnabled


```lua
function Spring.AreHelperAIsEnabled()
  -> enabled: boolean
```


---

# Spring.ArePlayersAllied


```lua
function Spring.ArePlayersAllied(playerID1: number, playerID2: number)
  -> boolean|nil
```


---

# Spring.AreTeamsAllied


```lua
function Spring.AreTeamsAllied(teamID1: number, teamID2: number)
  -> boolean|nil
```


---

# Spring.AssignMouseCursor


```lua
function Spring.AssignMouseCursor(cmdName: string, iconFileName: string, overwrite?: boolean, hotSpotTopLeft?: boolean)
  -> assigned: boolean?
```


---

# Spring.AssignPlayerToTeam


```lua
function Spring.AssignPlayerToTeam(playerID: integer, teamID: integer)
  -> nil
```


---

# Spring.BuggerOff


```lua
function Spring.BuggerOff(x: number, y: number, z?: number, radius: number, teamID: integer, spherical?: boolean, forced?: boolean, excludeUnitID?: integer, excludeUnitDefIDs?: number[])
  -> nil
```


---

# Spring.CallCOBScript


```lua
function Spring.CallCOBScript(unitID: integer, funcName?: string|integer, retArgs: integer, ...any)
  -> ...number
```


---

# Spring.ClearFeaturesPreviousDrawFlag


```lua
function Spring.ClearFeaturesPreviousDrawFlag()
  -> nil
```


---

# Spring.ClearUnitGoal


```lua
function Spring.ClearUnitGoal(unitID: integer)
  -> nil
```


---

# Spring.ClearUnitsPreviousDrawFlag


```lua
function Spring.ClearUnitsPreviousDrawFlag()
  -> nil
```


---

# Spring.ClearWatchDogTimer


```lua
function Spring.ClearWatchDogTimer(threadName?: string)
  -> nil
```


---

# Spring.ClosestBuildPos


```lua
function Spring.ClosestBuildPos(teamID: integer, unitDefID: integer, posX: number, posY: number, posZ: number, searchRadius: number, minDistance: number, buildFacing: number)
  -> buildPosX: number
  2. buildPosY: number
  3. buildPosZ: number
```


---

# Spring.CreateDir


```lua
function Spring.CreateDir(path: string)
  -> dirCreated: boolean?
```


---

# Spring.CreateFeature


```lua
function Spring.CreateFeature(featureDef: string|number, x: number, y: number, z: number, heading?: number, AllyTeamID?: integer, featureID?: integer)
  -> featureID: number
```


---

# Spring.CreateGroundDecal


```lua
function Spring.CreateGroundDecal()
  -> decalID: number|nil
```


---

# Spring.CreateUnit


```lua
function Spring.CreateUnit(unitDefName: string|number, x: number, y: number, z: number, facing: "e"|"east"|"n"|"north"|"s"...(+7), teamID: integer, build?: boolean, flattenGround?: boolean, unitID?: integer, builderID?: integer)
  -> unitID: number|nil
```


---

# Spring.DeleteProjectile


```lua
function Spring.DeleteProjectile(projectileID: integer)
  -> nil
```


---

# Spring.DeselectUnit


```lua
function Spring.DeselectUnit(unitID: integer)
  -> nil
```


---

# Spring.DeselectUnitArray


```lua
function Spring.DeselectUnitArray(unitIDs: table<any, integer>)
  -> nil
```


---

# Spring.DeselectUnitMap


```lua
function Spring.DeselectUnitMap(unitMap: table<integer, any>)
  -> nil
```


---

# Spring.DestroyFeature


```lua
function Spring.DestroyFeature(featureDefID: integer)
  -> nil
```


---

# Spring.DestroyGroundDecal


```lua
function Spring.DestroyGroundDecal(decalID: integer)
  -> delSuccess: boolean
```


---

# Spring.DestroyUnit


```lua
function Spring.DestroyUnit(unitID: integer, selfd?: boolean, reclaimed?: boolean, attackerID?: integer, cleanupImmediately?: boolean)
  -> nil
```


---

# Spring.DiffTimers


```lua
function Spring.DiffTimers(endTimer: integer, startTimer: integer, returnMs?: boolean, fromMicroSecs?: boolean)
  -> timeAmount: number
```


---

# Spring.DrawUnitCommands


```lua
function Spring.DrawUnitCommands(unitID: integer)
  -> nil
```


```lua
function Spring.DrawUnitCommands(units: table, tableOrArray?: boolean)
  -> nil
```


---

# Spring.Echo


```lua
function Spring.Echo(arg: any, ...any)
  -> nil
```


---

# Spring.EditUnitCmdDesc


```lua
function Spring.EditUnitCmdDesc(unitID: integer, cmdDescID: integer, cmdArray: CommandDescription)
```


---

# Spring.ExtractModArchiveFile


```lua
function Spring.ExtractModArchiveFile(modfile: string)
  -> extracted: boolean
```


---

# Spring.FindUnitCmdDesc


```lua
function Spring.FindUnitCmdDesc(unitID: integer)
```


---

# Spring.FixedAllies


```lua
function Spring.FixedAllies()
  -> enabled: boolean|nil
```


---

# Spring.ForceLayoutUpdate


```lua
function Spring.ForceLayoutUpdate()
  -> nil
```


---

# Spring.ForceTesselationUpdate


```lua
function Spring.ForceTesselationUpdate(normal?: boolean, shadow?: boolean)
  -> updated: boolean
```


---

# Spring.ForceUnitCollisionUpdate


```lua
function Spring.ForceUnitCollisionUpdate(unitID: integer)
  -> nil
```


---

# Spring.FreeUnitIcon


```lua
function Spring.FreeUnitIcon(iconName: string)
  -> freed: boolean?
```


---

# Spring.GameOver


```lua
function Spring.GameOver(allyTeamID1?: number, allyTeamID2?: number, allyTeamIDn?: number)
  -> nil
```


---

# Spring.GarbageCollectCtrl


```lua
function Spring.GarbageCollectCtrl(itersPerBatch?: integer, numStepsPerIter?: integer, minStepsPerIter?: integer, maxStepsPerIter?: integer, minLoopRunTime?: number, maxLoopRunTime?: number, baseRunTimeMult?: number, baseMemLoadMult?: number)
  -> nil
```


---

# Spring.GetAIInfo


```lua
function Spring.GetAIInfo(teamID: integer)
  -> skirmishAIID: number
  2. name: string
  3. hostingPlayerID: number
  4. shortName: string
  5. version: string
  6. options: table<string, string>
```


---

# Spring.GetActionHotKeys


```lua
function Spring.GetActionHotKeys(actionName: string)
  -> hotkeys: string[]?
```


---

# Spring.GetActiveCmdDesc


```lua
function Spring.GetActiveCmdDesc(cmdIndex: integer)
  -> CommandDescription?
```


---

# Spring.GetActiveCmdDescs


```lua
function Spring.GetActiveCmdDescs()
  -> cmdDescs: CommandDescription[]
```


---

# Spring.GetActiveCommand


```lua
function Spring.GetActiveCommand()
  -> cmdIndex: number?
  2. cmdID: number?
  3. cmdType: number?
  4. cmdName: string|nil
```


---

# Spring.GetActivePage


```lua
function Spring.GetActivePage()
  -> activePage: number
  2. maxPage: number
```


---

# Spring.GetAllFeatures


```lua
function Spring.GetAllFeatures()
```


---

# Spring.GetAllGroundDecals


```lua
function Spring.GetAllGroundDecals()
  -> decalIDs: number[]
```


---

# Spring.GetAllUnits


```lua
function Spring.GetAllUnits()
  -> unitIDs: number[]
```


---

# Spring.GetAllyTeamInfo


```lua
function Spring.GetAllyTeamInfo(allyTeamID: integer)
  -> table<string, string>|nil
```


---

# Spring.GetAllyTeamList


```lua
function Spring.GetAllyTeamList()
  -> list: number[]
```


---

# Spring.GetAllyTeamStartBox


```lua
function Spring.GetAllyTeamStartBox(allyID: integer)
  -> xMin: number?
  2. zMin: number?
  3. xMax: number?
  4. zMax: number?
```


---

# Spring.GetBoxSelectionByEngine


```lua
function Spring.GetBoxSelectionByEngine()
  -> when: boolean
```


---

# Spring.GetBuildFacing


```lua
function Spring.GetBuildFacing()
  -> buildFacing: "e"|"east"|"n"|"north"|"s"...(+7)
```


---

# Spring.GetBuildSpacing


```lua
function Spring.GetBuildSpacing()
  -> buildSpacing: number
```


---

# Spring.GetCEGID


```lua
function Spring.GetCEGID()
```


---

# Spring.GetCOBScriptID


```lua
function Spring.GetCOBScriptID(unitID: integer, funcName: string)
  -> funcID: integer?
```


---

# Spring.GetCameraDirection


```lua
function Spring.GetCameraDirection()
  -> dirX: number
  2. dirY: number
  3. dirZ: number
```


---

# Spring.GetCameraFOV


```lua
function Spring.GetCameraFOV()
  -> vFOV: number
  2. hFOV: number
```


---

# Spring.GetCameraNames


```lua
function Spring.GetCameraNames()
  -> Table: table<string, number>
```


---

# Spring.GetCameraPosition


```lua
function Spring.GetCameraPosition()
  -> posX: number
  2. posY: number
  3. posZ: number
```


---

# Spring.GetCameraRotation


```lua
function Spring.GetCameraRotation()
  -> rotX: number
  2. rotY: number
  3. rotZ: number
```


---

# Spring.GetCameraState


```lua
function Spring.GetCameraState(useReturns: false)
  -> cameraState: CameraState
```


```lua
function Spring.GetCameraState(useReturns?: true)
  -> name: "dummy"|"fps"|"free"|"ov"|"rot"...(+2)
  2. Fields: any
```


---

# Spring.GetCameraVectors


```lua
function Spring.GetCameraVectors()
  -> CameraVectors
```


---

# Spring.GetClipboard


```lua
function Spring.GetClipboard()
  -> text: string
```


---

# Spring.GetCmdDescIndex


```lua
function Spring.GetCmdDescIndex(cmdID: integer)
  -> cmdDescIndex: integer?
```


---

# Spring.GetCommandQueue


```lua
function Spring.GetCommandQueue(unitID: integer, count: integer)
  -> commands: Command[]
```


```lua
function Spring.GetCommandQueue(unitID: integer, count: 0)
  -> The: integer
```


---

# Spring.GetConfigFloat


```lua
function Spring.GetConfigFloat(name: string, default?: number)
  -> configFloat: number?
```


---

# Spring.GetConfigInt


```lua
function Spring.GetConfigInt(name: string, default?: number)
  -> configInt: number?
```


---

# Spring.GetConfigParams


```lua
function Spring.GetConfigParams()
  -> Configuration[]
```


---

# Spring.GetConfigString


```lua
function Spring.GetConfigString(name: string, default?: string)
  -> configString: number?
```


---

# Spring.GetConsoleBuffer


```lua
function Spring.GetConsoleBuffer(maxLines: number)
  -> buffer: { text: string, priority: integer }[]
```


---

# Spring.GetCurrentTooltip


```lua
function Spring.GetCurrentTooltip()
  -> tooltip: string
```


---

# Spring.GetDecalQuadPos


```lua
function Spring.GetDecalQuadPos(decalID: integer)
  -> posTL.x: number?
  2. posTL.z: number
  3. posTR.x: number
  4. posTR.z: number
  5. posBR.x: number
  6. posBR.z: number
  7. posBL.x: number
  8. posBL.z: number
```


---

# Spring.GetDecalTextures


```lua
function Spring.GetDecalTextures(isMainTex?: boolean)
  -> textureNames: string[]
```


---

# Spring.GetDefaultCommand


```lua
function Spring.GetDefaultCommand()
  -> cmdIndex: number?
  2. cmdID: number?
  3. cmdType: number?
  4. cmdName: string|nil
```


---

# Spring.GetDrawFrame


```lua
function Spring.GetDrawFrame()
  -> low_16bit: number
  2. high_16bit: number
```


---

# Spring.GetDrawSeconds


```lua
function Spring.GetDrawSeconds()
  -> Time: integer
```


---

# Spring.GetDrawSelectionInfo


```lua
function Spring.GetDrawSelectionInfo()
  -> boolean
```


---

# Spring.GetDualViewGeometry


```lua
function Spring.GetDualViewGeometry()
  -> dualViewSizeX: number
  2. dualViewSizeY: number
  3. dualViewPosX: number
  4. dualViewPosY: number
```


---

# Spring.GetFPS


```lua
function Spring.GetFPS()
  -> fps: number
```


---

# Spring.GetFacingFromHeading


```lua
function Spring.GetFacingFromHeading(heading: number)
  -> facing: number
```


---

# Spring.GetFactoryBuggerOff


```lua
function Spring.GetFactoryBuggerOff(unitID: integer)
```


---

# Spring.GetFactoryCommands


```lua
function Spring.GetFactoryCommands(unitID: integer, count: number)
  -> commands: number|Command[]
```


---

# Spring.GetFactoryCounts


```lua
function Spring.GetFactoryCounts(unitID: integer, count?: integer, addCmds?: boolean)
  -> counts: table<number, number>?
```


---

# Spring.GetFeatureAllyTeam


```lua
function Spring.GetFeatureAllyTeam(featureID: integer)
  -> number?
```


---

# Spring.GetFeatureAlwaysUpdateMatrix


```lua
function Spring.GetFeatureAlwaysUpdateMatrix(featureID: integer)
  -> nil: boolean?
```


---

# Spring.GetFeatureBlocking


```lua
function Spring.GetFeatureBlocking(featureID: integer)
  -> isBlocking: boolean|nil
  2. isSolidObjectCollidable: boolean
  3. isProjectileCollidable: boolean
  4. isRaySegmentCollidable: boolean
  5. crushable: boolean
  6. blockEnemyPushing: boolean
  7. blockHeightChanges: boolean
```


---

# Spring.GetFeatureCollisionVolumeData


```lua
function Spring.GetFeatureCollisionVolumeData(featureID: integer)
```


---

# Spring.GetFeatureDefID


```lua
function Spring.GetFeatureDefID(featureID: integer)
  -> number?
```


---

# Spring.GetFeatureDirection


```lua
function Spring.GetFeatureDirection(featureID: integer)
  -> dirX: number?
  2. dirY: number?
  3. dirZ: number?
```


---

# Spring.GetFeatureDrawFlag


```lua
function Spring.GetFeatureDrawFlag(featureID: integer)
  -> nil: number?
```


---

# Spring.GetFeatureEngineDrawMask


```lua
function Spring.GetFeatureEngineDrawMask(featureID: integer)
  -> nil: boolean?
```


---

# Spring.GetFeatureHeading


```lua
function Spring.GetFeatureHeading(featureID: integer)
```


---

# Spring.GetFeatureHealth


```lua
function Spring.GetFeatureHealth(featureID: integer)
  -> health: number?
  2. defHealth: number
  3. resurrectProgress: number
```


---

# Spring.GetFeatureHeight


```lua
function Spring.GetFeatureHeight(featureID: integer)
  -> number?
```


---

# Spring.GetFeatureLastAttackedPiece


```lua
function Spring.GetFeatureLastAttackedPiece(featureID: integer)
```


---

# Spring.GetFeatureLuaDraw


```lua
function Spring.GetFeatureLuaDraw(featureID: integer)
  -> nil: boolean?
```


---

# Spring.GetFeatureMass


```lua
function Spring.GetFeatureMass(featureID: integer)
  -> number?
```


---

# Spring.GetFeatureNoDraw


```lua
function Spring.GetFeatureNoDraw(featureID: integer)
  -> nil: boolean?
```


---

# Spring.GetFeatureNoSelect


```lua
function Spring.GetFeatureNoSelect(featureID: integer)
  -> boolean|nil
```


---

# Spring.GetFeaturePieceCollisionVolumeData


```lua
function Spring.GetFeaturePieceCollisionVolumeData(featureID: integer)
```


---

# Spring.GetFeaturePieceDirection


```lua
function Spring.GetFeaturePieceDirection(featureID: integer, pieceIndex: integer)
  -> dirX: number|nil
  2. dirY: number
  3. dirZ: number
```


---

# Spring.GetFeaturePieceInfo


```lua
function Spring.GetFeaturePieceInfo(featureID: integer, pieceIndex: integer)
  -> pieceInfo: PieceInfo?
```


---

# Spring.GetFeaturePieceList


```lua
function Spring.GetFeaturePieceList(featureID: integer)
  -> pieceNames: string[]
```


---

# Spring.GetFeaturePieceMap


```lua
function Spring.GetFeaturePieceMap(featureID: integer)
  -> pieceInfos: table<string, number>
```


---

# Spring.GetFeaturePieceMatrix


```lua
function Spring.GetFeaturePieceMatrix(featureID: integer)
  -> m11: number|nil
  2. m12: number
  3. m13: number
  4. m14: number
  5. m21: number
  6. m22: number
  7. m23: number
  8. m24: number
  9. m31: number
 10. m32: number
 11. m33: number
 12. m34: number
 13. m41: number
 14. m42: number
 15. m43: number
 16. m44: number
```


---

# Spring.GetFeaturePiecePosDir


```lua
function Spring.GetFeaturePiecePosDir(featureID: integer, pieceIndex: integer)
  -> posX: number|nil
  2. posY: number
  3. posZ: number
  4. dirX: number
  5. dirY: number
  6. dirZ: number
```


---

# Spring.GetFeaturePiecePosition


```lua
function Spring.GetFeaturePiecePosition(featureID: integer, pieceIndex: integer)
  -> posX: number|nil
  2. posY: number
  3. posZ: number
```


---

# Spring.GetFeaturePosition


```lua
function Spring.GetFeaturePosition(featureID: integer)
```


---

# Spring.GetFeatureRadius


```lua
function Spring.GetFeatureRadius(featureID: integer)
  -> number?
```


---

# Spring.GetFeatureResources


```lua
function Spring.GetFeatureResources(featureID: integer)
  -> metal: number?
  2. defMetal: number
  3. energy: number
  4. defEnergy: number
  5. reclaimLeft: number
  6. reclaimTime: number
```


---

# Spring.GetFeatureResurrect


```lua
function Spring.GetFeatureResurrect(featureID: integer)
```


---

# Spring.GetFeatureRootPiece


```lua
function Spring.GetFeatureRootPiece(featureID: integer)
  -> index: number
```


---

# Spring.GetFeatureRotation


```lua
function Spring.GetFeatureRotation(featureID: integer)
  -> pitch: number?
  2. yaw: number?
  3. roll: number?
```


---

# Spring.GetFeatureRulesParam


```lua
function Spring.GetFeatureRulesParam(featureID: integer, ruleRef: string|number)
  -> value: string|number|nil
```


---

# Spring.GetFeatureRulesParams


```lua
function Spring.GetFeatureRulesParams(featureID: integer)
  -> rulesParams: RulesParams
```


---

# Spring.GetFeatureSelectionVolumeData


```lua
function Spring.GetFeatureSelectionVolumeData(featureID: integer)
  -> scaleX: number?
  2. scaleY: number
  3. scaleZ: number
  4. offsetX: number
  5. offsetY: number
  6. offsetZ: number
  7. volumeType: number
  8. useContHitTest: number
  9. getPrimaryAxis: number
 10. ignoreHits: boolean
```


---

# Spring.GetFeatureSeparation


```lua
function Spring.GetFeatureSeparation(featureID1: number, featureID2: number, direction?: boolean)
  -> number?
```


---

# Spring.GetFeatureTeam


```lua
function Spring.GetFeatureTeam(featureID: integer)
  -> number?
```


---

# Spring.GetFeatureTransformMatrix


```lua
function Spring.GetFeatureTransformMatrix(featureID: integer)
  -> m11: number?
  2. m12: number
  3. m13: number
  4. m14: number
  5. m21: number
  6. m22: number
  7. m23: number
  8. m24: number
  9. m31: number
 10. m32: number
 11. m33: number
 12. m34: number
 13. m41: number
 14. m42: number
 15. m43: number
 16. m44: number
```


---

# Spring.GetFeatureVelocity


```lua
function Spring.GetFeatureVelocity(featureID: integer)
```


---

# Spring.GetFeaturesInCylinder


```lua
function Spring.GetFeaturesInCylinder(x: number, z: number, radius: number, allegiance?: number)
  -> featureIDs: number[]
```


---

# Spring.GetFeaturesInRectangle


```lua
function Spring.GetFeaturesInRectangle(xmin: number, zmin: number, xmax: number, zmax: number)
  -> featureIDs: number[]
```


---

# Spring.GetFeaturesInScreenRectangle


```lua
function Spring.GetFeaturesInScreenRectangle(left: number, top: number, right: number, bottom: number)
  -> featureIDs: number[]|nil
```


---

# Spring.GetFeaturesInSphere


```lua
function Spring.GetFeaturesInSphere(x: number, y: number, z: number, radius: number)
  -> featureIDs: number[]
```


---

# Spring.GetFrameTimeOffset


```lua
function Spring.GetFrameTimeOffset()
  -> offset: number?
```


---

# Spring.GetFrameTimer


```lua
function Spring.GetFrameTimer(lastFrameTime?: boolean)
  -> integer
```


---

# Spring.GetFullBuildQueue


```lua
function Spring.GetFullBuildQueue(unitID: integer)
  -> buildqueue: table<number, number>|nil
```


---

# Spring.GetGaiaTeamID


```lua
function Spring.GetGaiaTeamID()
  -> teamID: number
```


---

# Spring.GetGameFrame


```lua
function Spring.GetGameFrame()
  -> t1: number
  2. t2: number
```


---

# Spring.GetGameName


```lua
function Spring.GetGameName()
  -> name: string
```


---

# Spring.GetGameRulesParam


```lua
function Spring.GetGameRulesParam(ruleRef: string|number)
  -> number?
```


---

# Spring.GetGameRulesParams


```lua
function Spring.GetGameRulesParams()
  -> rulesParams: RulesParams
```


---

# Spring.GetGameSeconds


```lua
function Spring.GetGameSeconds()
  -> seconds: number
```


---

# Spring.GetGameSpeed


```lua
function Spring.GetGameSpeed()
  -> wantedSpeedFactor: number
  2. speedFactor: number
  3. paused: boolean
```


---

# Spring.GetGameState


```lua
function Spring.GetGameState(maxLatency?: number)
  -> doneLoading: boolean
  2. isSavedGame: boolean
  3. isClientPaused: boolean
  4. isSimLagging: boolean
```


---

# Spring.GetGatherMode


```lua
function Spring.GetGatherMode()
  -> gatherMode: number
```


---

# Spring.GetGlobalLos


```lua
function Spring.GetGlobalLos(teamID?: integer)
  -> enabled: boolean
```


---

# Spring.GetGrass


```lua
function Spring.GetGrass(x: number, z: number)
  -> number
```


---

# Spring.GetGroundBlocked


```lua
function Spring.GetGroundBlocked()
```


---

# Spring.GetGroundDecalAlpha


```lua
function Spring.GetGroundDecalAlpha(decalID: integer)
  -> alpha: number?
  2. alphaFalloff: number
```


---

# Spring.GetGroundDecalCreationFrame


```lua
function Spring.GetGroundDecalCreationFrame(decalID: integer)
  -> creationFrameMin: number?
  2. creationFrameMax: number
```


---

# Spring.GetGroundDecalMiddlePos


```lua
function Spring.GetGroundDecalMiddlePos(decalID: integer)
  -> posX: number?
  2. posZ: number
```


---

# Spring.GetGroundDecalMisc


```lua
function Spring.GetGroundDecalMisc(decalID: integer)
  -> dotElimExp: number?
  2. refHeight: number
  3. minHeight: number
  4. maxHeight: number
  5. forceHeightMode: number
```


---

# Spring.GetGroundDecalNormal


```lua
function Spring.GetGroundDecalNormal(decalID: integer)
  -> normal.x: number?
  2. normal.y: number
  3. normal.z: number
```


---

# Spring.GetGroundDecalOwner


```lua
function Spring.GetGroundDecalOwner(decalID: integer)
  -> unitID: number?
```


---

# Spring.GetGroundDecalRotation


```lua
function Spring.GetGroundDecalRotation(decalID: integer)
  -> rotation: number?
```


---

# Spring.GetGroundDecalSizeAndHeight


```lua
function Spring.GetGroundDecalSizeAndHeight(decalID: integer)
  -> sizeX: number?
  2. sizeY: number
  3. projCubeHeight: number
```


---

# Spring.GetGroundDecalTexture


```lua
function Spring.GetGroundDecalTexture(decalID: integer, isMainTex?: boolean)
  -> texture: string|nil
```


---

# Spring.GetGroundDecalTint


```lua
function Spring.GetGroundDecalTint(decalID: integer)
  -> tintR: number?
  2. tintG: number
  3. tintB: number
  4. tintA: number
```


---

# Spring.GetGroundDecalType


```lua
function Spring.GetGroundDecalType(decalID: integer)
  -> type: string|nil
```


---

# Spring.GetGroundExtremes


```lua
function Spring.GetGroundExtremes()
  -> initMinHeight: number
  2. initMaxHeight: number
  3. currMinHeight: number
  4. currMaxHeight: number
```


---

# Spring.GetGroundHeight


```lua
function Spring.GetGroundHeight(x: number, z: number)
  -> number
```


---

# Spring.GetGroundInfo


```lua
function Spring.GetGroundInfo(x: number, z: number)
  -> ix: number
  2. iz: number
  3. terrainTypeIndex: number
  4. name: string
  5. metalExtraction: number
  6. hardness: number
  7. tankSpeed: number
  8. kbotSpeed: number
  9. hoverSpeed: number
 10. shipSpeed: number
 11. receiveTracks: boolean
```


---

# Spring.GetGroundNormal


```lua
function Spring.GetGroundNormal(x: number, z: number, smoothed?: boolean)
  -> normalX: number
  2. normalY: number
  3. normalZ: number
  4. slope: number
```


---

# Spring.GetGroundOrigHeight


```lua
function Spring.GetGroundOrigHeight(x: number, z: number)
  -> number
```


---

# Spring.GetGroupList


```lua
function Spring.GetGroupList()
  -> where: table<number, number>|nil
```


---

# Spring.GetGroupUnits


```lua
function Spring.GetGroupUnits(groupID: integer)
  -> unitIDs: number[]|nil
```


---

# Spring.GetGroupUnitsCount


```lua
function Spring.GetGroupUnitsCount(groupID: integer)
  -> groupSize: number?
```


---

# Spring.GetGroupUnitsCounts


```lua
function Spring.GetGroupUnitsCounts(groupID: integer)
  -> where: table<number, number>|nil
```


---

# Spring.GetGroupUnitsSorted


```lua
function Spring.GetGroupUnitsSorted(groupID: integer)
  -> where: table<number, number[]>|nil
```


---

# Spring.GetHeadingFromFacing


```lua
function Spring.GetHeadingFromFacing(facing: number)
  -> heading: number
```


---

# Spring.GetHeadingFromVector


```lua
function Spring.GetHeadingFromVector(x: number, z: number)
  -> heading: number
```


---

# Spring.GetInvertQueueKey


```lua
function Spring.GetInvertQueueKey()
  -> queueKey: number?
```


---

# Spring.GetKeyBindings


```lua
function Spring.GetKeyBindings(keySet1?: string, keySet2?: string)
  -> KeyBinding[]
```


---

# Spring.GetKeyCode


```lua
function Spring.GetKeyCode(keySym: string)
  -> keyCode: number
```


---

# Spring.GetKeyFromScanSymbol


```lua
function Spring.GetKeyFromScanSymbol(scanSymbol: string)
  -> keyName: string
```


---

# Spring.GetKeyState


```lua
function Spring.GetKeyState(keyCode: number)
  -> pressed: boolean
```


---

# Spring.GetKeySymbol


```lua
function Spring.GetKeySymbol(keyCode: number)
  -> keyCodeName: string
  2. keyCodeDefaultName: string
```


---

# Spring.GetLastMessagePositions


```lua
function Spring.GetLastMessagePositions()
  -> message: xyz[]
```


---

# Spring.GetLastUpdateSeconds


```lua
function Spring.GetLastUpdateSeconds()
  -> lastUpdateSeconds: number?
```


---

# Spring.GetLocalAllyTeamID


```lua
function Spring.GetLocalAllyTeamID()
  -> allyTeamID: number
```


---

# Spring.GetLocalPlayerID


```lua
function Spring.GetLocalPlayerID()
  -> playerID: number
```


---

# Spring.GetLocalTeamID


```lua
function Spring.GetLocalTeamID()
  -> teamID: number
```


---

# Spring.GetLogSections


```lua
function Spring.GetLogSections()
  -> sections: table<string, number>
```


---

# Spring.GetLosViewColors


```lua
function Spring.GetLosViewColors()
  -> always: rgb
  2. LOS: rgb
  3. radar: rgb
  4. jam: rgb
  5. radar2: rgb
```


---

# Spring.GetLuaMemUsage


```lua
function Spring.GetLuaMemUsage()
  -> luaHandleAllocedMem: number
  2. luaHandleNumAllocs: number
  3. luaGlobalAllocedMem: number
  4. luaGlobalNumAllocs: number
  5. luaUnsyncedGlobalAllocedMem: number
  6. luaUnsyncedGlobalNumAllocs: number
  7. luaSyncedGlobalAllocedMem: number
  8. luaSyncedGlobalNumAllocs: number
```


---

# Spring.GetMapDrawMode


```lua
function Spring.GetMapDrawMode()
  -> "height"|"los"|"metal"|"normal"|"pathTraversability"
```


---

# Spring.GetMapOption


```lua
function Spring.GetMapOption(mapOption: string)
  -> value: string
```


---

# Spring.GetMapOptions


```lua
function Spring.GetMapOptions()
  -> mapOptions: table<string, string>
```


---

# Spring.GetMapSquareTexture


```lua
function Spring.GetMapSquareTexture(texSquareX: number, texSquareY: number, lodMin: number, luaTexName: string, lodMax?: number)
  -> success: boolean?
```


---

# Spring.GetMapStartPositions


```lua
function Spring.GetMapStartPositions()
  -> array: xyz[]
```


---

# Spring.GetMenuName


```lua
function Spring.GetMenuName()
  -> name: string
```


---

# Spring.GetMiniMapDualScreen


```lua
function Spring.GetMiniMapDualScreen()
  -> position: string|false
```


---

# Spring.GetMiniMapGeometry


```lua
function Spring.GetMiniMapGeometry()
  -> minimapPosX: number
  2. minimapPosY: number
  3. minimapSizeX: number
  4. minimapSizeY: number
  5. minimized: boolean
  6. maximized: boolean
```


---

# Spring.GetMiniMapRotation


```lua
function Spring.GetMiniMapRotation()
  -> amount: number
```


---

# Spring.GetModKeyState


```lua
function Spring.GetModKeyState()
  -> alt: boolean
  2. ctrl: boolean
  3. meta: boolean
  4. shift: boolean
```


---

# Spring.GetModOption


```lua
function Spring.GetModOption(modOption: string)
  -> value: string
```


---

# Spring.GetModOptions


```lua
function Spring.GetModOptions()
  -> modOptions: table<string, string>
```


---

# Spring.GetModelPieceList


```lua
function Spring.GetModelPieceList(modelName: string)
  -> pieceNames: string[]|nil
```


---

# Spring.GetModelPieceMap


```lua
function Spring.GetModelPieceMap(modelName: string)
  -> pieceInfos: table<string, number>|nil
```


---

# Spring.GetModelRootPiece


```lua
function Spring.GetModelRootPiece(modelName: string)
  -> index: number
```


---

# Spring.GetMouseCursor


```lua
function Spring.GetMouseCursor()
  -> cursorName: string
  2. cursorScale: number
```


---

# Spring.GetMouseStartPosition


```lua
function Spring.GetMouseStartPosition(button: number)
  -> x: number
  2. y: number
  3. camPosX: number
  4. camPosY: number
  5. camPosZ: number
  6. dirX: number
  7. dirY: number
  8. dirZ: number
```


---

# Spring.GetMouseState


```lua
function Spring.GetMouseState()
  -> x: number
  2. y: number
  3. lmbPressed: number
  4. mmbPressed: number
  5. rmbPressed: number
  6. offscreen: boolean
  7. mmbScroll: boolean
```


---

# Spring.GetNanoProjectileParams


```lua
function Spring.GetNanoProjectileParams()
  -> rotVal: number
  2. rotVel: number
  3. rotAcc: number
  4. rotValRng: number
  5. rotVelRng: number
  6. rotAccRng: number
```


---

# Spring.GetNumDisplays


```lua
function Spring.GetNumDisplays()
  -> numDisplays: number
```


---

# Spring.GetPieceProjectileParams


```lua
function Spring.GetPieceProjectileParams(projectileID: integer)
  -> explosionFlags: number?
  2. spinAngle: number
  3. spinSpeed: number
  4. spinVectorX: number
  5. spinVectorY: number
  6. spinVectorZ: number
```


---

# Spring.GetPixelDir


```lua
function Spring.GetPixelDir(x: number, y: number)
  -> dirX: number
  2. dirY: number
  3. dirZ: number
```


---

# Spring.GetPlayerControlledUnit


```lua
function Spring.GetPlayerControlledUnit(playerID: integer)
  -> number?
```


---

# Spring.GetPlayerInfo


```lua
function Spring.GetPlayerInfo(playerID: integer, getPlayerOpts?: boolean)
  -> name: string
  2. active: boolean
  3. spectator: boolean
  4. teamID: number
  5. allyTeamID: number
  6. pingTime: number
  7. cpuUsage: number
  8. country: string
  9. rank: number
 10. hasSkirmishAIsInTeam: boolean
 11. playerOpts: { [string]: string }
 12. desynced: boolean
```


---

# Spring.GetPlayerList


```lua
function Spring.GetPlayerList(teamID?: integer, active?: boolean)
  -> list: number[]?
```


---

# Spring.GetPlayerRoster


```lua
function Spring.GetPlayerRoster(sortType?: number, showPathingPlayers?: boolean)
  -> playerTable: Roster[]?
```


---

# Spring.GetPlayerRulesParam


```lua
function Spring.GetPlayerRulesParam(playerID: integer, ruleRef: string|number)
  -> value: string|number|nil
```


---

# Spring.GetPlayerRulesParams


```lua
function Spring.GetPlayerRulesParams(playerID: integer)
  -> rulesParams: RulesParams
```


---

# Spring.GetPlayerStatistics


```lua
function Spring.GetPlayerStatistics(playerID: integer)
  -> mousePixels: number?
  2. mouseClicks: number
  3. keyPresses: number
  4. numCommands: number
  5. unitCommands: number
```


---

# Spring.GetPlayerTraffic


```lua
function Spring.GetPlayerTraffic(playerID: integer, packetID?: integer)
  -> traffic: number
```


---

# Spring.GetPositionLosState


```lua
function Spring.GetPositionLosState(posX: number, posY: number, posZ: number, allyTeamID?: integer)
  -> inLosOrRadar: boolean
  2. inLos: boolean
  3. inRadar: boolean
  4. inJammer: boolean
```


---

# Spring.GetPressedKeys


```lua
function Spring.GetPressedKeys()
  -> where: table<string|number, true>
```


---

# Spring.GetPressedScans


```lua
function Spring.GetPressedScans()
  -> where: table<string|number, true>
```


---

# Spring.GetProfilerRecordNames


```lua
function Spring.GetProfilerRecordNames()
  -> profilerNames: string[]
```


---

# Spring.GetProfilerTimeRecord


```lua
function Spring.GetProfilerTimeRecord(profilerName: string, frameData?: boolean)
  -> total: number
  2. current: number
  3. max_dt: number
  4. time_pct: number
  5. peak_pct: number
  6. frameData: table<number, number>?
```


---

# Spring.GetProjectileAllyTeamID


```lua
function Spring.GetProjectileAllyTeamID(projectileID: integer)
  -> number?
```


---

# Spring.GetProjectileDamages


```lua
function Spring.GetProjectileDamages(projectileID: integer, tag: string)
  -> number?
```


---

# Spring.GetProjectileDefID


```lua
function Spring.GetProjectileDefID(projectileID: integer)
  -> number?
```


---

# Spring.GetProjectileDirection


```lua
function Spring.GetProjectileDirection(projectileID: integer)
  -> dirX: number?
  2. dirY: number
  3. dirZ: number
```


---

# Spring.GetProjectileGravity


```lua
function Spring.GetProjectileGravity(projectileID: integer)
  -> number?
```


---

# Spring.GetProjectileIsIntercepted


```lua
function Spring.GetProjectileIsIntercepted(projectileID: integer)
  -> boolean|nil
```


---

# Spring.GetProjectileOwnerID


```lua
function Spring.GetProjectileOwnerID(projectileID: integer)
  -> number?
```


---

# Spring.GetProjectilePosition


```lua
function Spring.GetProjectilePosition(projectileID: integer)
  -> posX: number?
  2. posY: number
  3. posZ: number
```


---

# Spring.GetProjectileTarget


```lua
function Spring.GetProjectileTarget(projectileID: integer)
  -> targetTypeInt: number?
  2. target: number|xyz
```


---

# Spring.GetProjectileTeamID


```lua
function Spring.GetProjectileTeamID(projectileID: integer)
  -> number?
```


---

# Spring.GetProjectileTimeToLive


```lua
function Spring.GetProjectileTimeToLive(projectileID: integer)
  -> number?
```


---

# Spring.GetProjectileType


```lua
function Spring.GetProjectileType(projectileID: integer)
  -> weapon: boolean|nil
  2. piece: boolean
```


---

# Spring.GetProjectileVelocity


```lua
function Spring.GetProjectileVelocity(projectileID: integer)
  -> velX: number?
  2. velY: number
  3. velZ: number
  4. velW: number
```


---

# Spring.GetProjectilesInRectangle


```lua
function Spring.GetProjectilesInRectangle(xmin: number, zmin: number, xmax: number, zmax: number, excludeWeaponProjectiles?: boolean, excludePieceProjectiles?: boolean)
  -> projectileIDs: number[]
```


---

# Spring.GetRadarErrorParams


```lua
function Spring.GetRadarErrorParams(allyTeamID: integer)
  -> radarErrorSize: number?
  2. baseRadarErrorSize: number
  3. baseRadarErrorMult: number
```


---

# Spring.GetRealBuildQueue


```lua
function Spring.GetRealBuildQueue(unitID: integer)
  -> buildqueue: table<number, number>|nil
```


---

# Spring.GetRenderFeatures


```lua
function Spring.GetRenderFeatures()
```


---

# Spring.GetRenderFeaturesDrawFlagChanged


```lua
function Spring.GetRenderFeaturesDrawFlagChanged()
```


---

# Spring.GetRenderUnits


```lua
function Spring.GetRenderUnits()
```


---

# Spring.GetRenderUnitsDrawFlagChanged


```lua
function Spring.GetRenderUnitsDrawFlagChanged()
```


---

# Spring.GetReplayLength


```lua
function Spring.GetReplayLength()
  -> timeInSeconds: number?
```


---

# Spring.GetScanSymbol


```lua
function Spring.GetScanSymbol(scanCode: number)
  -> scanCodeName: string
  2. scanCodeDefaultName: string
```


---

# Spring.GetScreenGeometry


```lua
function Spring.GetScreenGeometry(displayIndex?: number, queryUsable?: boolean)
  -> screenSizeX: number
  2. screenSizeY: number
  3. screenPosX: number
  4. screenPosY: number
  5. windowBorderTop: number
  6. windowBorderLeft: number
  7. windowBorderBottom: number
  8. windowBorderRight: number
  9. screenUsableSizeX: number?
 10. screenUsableSizeY: number?
 11. screenUsablePosX: number?
 12. screenUsablePosY: number?
```


---

# Spring.GetSelectedGroup


```lua
function Spring.GetSelectedGroup()
  -> groupID: number
```


---

# Spring.GetSelectedUnits


```lua
function Spring.GetSelectedUnits()
  -> unitIDs: number[]
```


---

# Spring.GetSelectedUnitsCount


```lua
function Spring.GetSelectedUnitsCount()
  -> selectedUnitsCount: number
```


---

# Spring.GetSelectedUnitsCounts


```lua
function Spring.GetSelectedUnitsCounts()
  -> unitsCounts: table<number, number>
  2. the: integer
```


---

# Spring.GetSelectedUnitsSorted


```lua
function Spring.GetSelectedUnitsSorted()
  -> where: table<number, number[]>
  2. the: integer
```


---

# Spring.GetSelectionBox


```lua
function Spring.GetSelectionBox()
  -> left: number?
  2. top: number?
  3. right: number?
  4. bottom: number?
```


---

# Spring.GetSideData


```lua
function Spring.GetSideData(sideName: string)
  -> startUnit: string|nil
  2. caseSensitiveSideName: string
```


```lua
function Spring.GetSideData(sideID: integer)
  -> sideName: string|nil
  2. startUnit: string
  3. caseSensitiveSideName: string
```


```lua
function Spring.GetSideData()
  -> sideArray: SideSpec[]
```


---

# Spring.GetSmoothMeshHeight


```lua
function Spring.GetSmoothMeshHeight(x: number, z: number)
  -> height: number
```


---

# Spring.GetSoundDevices


```lua
function Spring.GetSoundDevices()
  -> devices: SoundDeviceSpec[]
```


---

# Spring.GetSoundEffectParams


```lua
function Spring.GetSoundEffectParams()
```


---

# Spring.GetSoundStreamTime


```lua
function Spring.GetSoundStreamTime()
  -> playTime: number
  2. time: number
```


---

# Spring.GetSpectatingState


```lua
function Spring.GetSpectatingState()
  -> spectating: boolean
  2. spectatingFullView: boolean
  3. spectatingFullSelect: boolean
```


---

# Spring.GetSyncedGCInfo


```lua
function Spring.GetSyncedGCInfo(collectGC?: boolean)
  -> GC: number?
```


---

# Spring.GetTeamAllyTeamID


```lua
function Spring.GetTeamAllyTeamID(teamID: integer)
  -> allyTeamID: integer?
```


---

# Spring.GetTeamColor


```lua
function Spring.GetTeamColor(teamID: integer)
  -> r: number?
  2. g: number?
  3. b: number?
  4. a: number?
```


---

# Spring.GetTeamDamageStats


```lua
function Spring.GetTeamDamageStats(teamID: integer)
  -> damageDealt: number
  2. damageReceived: number
```


---

# Spring.GetTeamInfo


```lua
function Spring.GetTeamInfo(teamID: integer, getTeamKeys?: boolean)
  -> teamID: number?
  2. leader: number
  3. isDead: number
  4. hasAI: number
  5. side: string
  6. allyTeam: number
  7. incomeMultiplier: number
  8. customTeamKeys: table<string, string>
```


---

# Spring.GetTeamList


```lua
function Spring.GetTeamList(allyTeamID?: integer)
  -> list: number[]?
```


---

# Spring.GetTeamLuaAI


```lua
function Spring.GetTeamLuaAI(teamID: integer)
  -> string
```


---

# Spring.GetTeamMaxUnits


```lua
function Spring.GetTeamMaxUnits(teamID: integer)
  -> maxUnits: number
  2. currentUnits: number?
```


---

# Spring.GetTeamOrigColor


```lua
function Spring.GetTeamOrigColor(teamID: integer)
  -> r: number?
  2. g: number?
  3. b: number?
  4. a: number?
```


---

# Spring.GetTeamResourceStats


```lua
function Spring.GetTeamResourceStats(teamID: integer, resource: "e"|"energy"|"m"|"metal")
  -> used: number?
  2. produced: number
  3. excessed: number
  4. received: number
  5. sent: number
```


---

# Spring.GetTeamResources


```lua
function Spring.GetTeamResources(teamID: integer, resource: "e"|"energy"|"m"|"metal")
  -> currentLevel: number?
  2. storage: number
  3. pull: number
  4. income: number
  5. expense: number
  6. share: number
  7. sent: number
  8. received: number
  9. excess: number
```


---

# Spring.GetTeamRulesParam


```lua
function Spring.GetTeamRulesParam(teamID: integer, ruleRef: string|number)
  -> value: string|number|nil
```


---

# Spring.GetTeamRulesParams


```lua
function Spring.GetTeamRulesParams(teamID: integer)
  -> rulesParams: RulesParams
```


---

# Spring.GetTeamStartPosition


```lua
function Spring.GetTeamStartPosition(teamID: integer)
  -> x: number?
  2. y: number?
  3. x: number?
```


---

# Spring.GetTeamStatsHistory


```lua
function Spring.GetTeamStatsHistory(teamID: integer)
  -> historyCount: integer?
```


```lua
function Spring.GetTeamStatsHistory(teamID: integer, startIndex: integer, endIndex?: integer)
  -> The: TeamStats[]
```


---

# Spring.GetTeamUnitCount


```lua
function Spring.GetTeamUnitCount(teamID: integer)
  -> count: number?
```


---

# Spring.GetTeamUnitDefCount


```lua
function Spring.GetTeamUnitDefCount(teamID: integer, unitDefID: integer)
  -> count: number?
```


---

# Spring.GetTeamUnitStats


```lua
function Spring.GetTeamUnitStats(teamID: integer)
  -> killed: number?
  2. died: number
  3. capturedBy: number
  4. capturedFrom: number
  5. received: number
  6. sent: number
```


---

# Spring.GetTeamUnits


```lua
function Spring.GetTeamUnits(teamID: integer)
  -> unitIDs: number[]?
```


---

# Spring.GetTeamUnitsByDefs


```lua
function Spring.GetTeamUnitsByDefs(teamID: integer, unitDefIDs: number|number[])
  -> unitIDs: number[]?
```


---

# Spring.GetTeamUnitsCounts


```lua
function Spring.GetTeamUnitsCounts(teamID: integer)
  -> countByUnit: table<number, number>?
```


---

# Spring.GetTeamUnitsSorted


```lua
function Spring.GetTeamUnitsSorted(teamID: integer)
  -> unitsByDef: table<integer, integer>
```


---

# Spring.GetTerrainTypeData


```lua
function Spring.GetTerrainTypeData(terrainTypeInfo: number)
  -> index: number
  2. name: string
  3. hardness: number
  4. tankSpeed: number
  5. kbotSpeed: number
  6. hoverSpeed: number
  7. shipSpeed: number
  8. receiveTracks: boolean
```


---

# Spring.GetTidal


```lua
function Spring.GetTidal()
  -> tidalStrength: number
```


---

# Spring.GetTimer


```lua
function Spring.GetTimer()
  -> integer
```


---

# Spring.GetTimerMicros


```lua
function Spring.GetTimerMicros()
  -> integer
```


---

# Spring.GetUnitAllyTeam


```lua
function Spring.GetUnitAllyTeam(unitID: integer)
  -> number?
```


---

# Spring.GetUnitAlwaysUpdateMatrix


```lua
function Spring.GetUnitAlwaysUpdateMatrix(unitID: integer)
  -> nil: boolean?
```


---

# Spring.GetUnitArmored


```lua
function Spring.GetUnitArmored(unitID: integer)
  -> armored: boolean|nil
  2. armorMultiple: number
```


---

# Spring.GetUnitArrayCentroid


```lua
function Spring.GetUnitArrayCentroid(units: table)
  -> centerX: number
  2. centerY: number
  3. centerZ: number
```


---

# Spring.GetUnitBasePosition


```lua
function Spring.GetUnitBasePosition(unitID: integer)
  -> posX: number?
  2. posY: number
  3. posZ: number
```


---

# Spring.GetUnitBlocking


```lua
function Spring.GetUnitBlocking(unitID: integer)
  -> isBlocking: boolean|nil
  2. isSolidObjectCollidable: boolean
  3. isProjectileCollidable: boolean
  4. isRaySegmentCollidable: boolean
  5. crushable: boolean
  6. blockEnemyPushing: boolean
  7. blockHeightChanges: boolean
```


---

# Spring.GetUnitBuildFacing


```lua
function Spring.GetUnitBuildFacing(unitID: integer)
```


---

# Spring.GetUnitBuildParams


```lua
function Spring.GetUnitBuildParams(unitID: integer)
```


---

# Spring.GetUnitBuildeeRadius


```lua
function Spring.GetUnitBuildeeRadius(unitID: integer)
  -> number?
```


---

# Spring.GetUnitCmdDescs


```lua
function Spring.GetUnitCmdDescs(unitID: integer)
```


---

# Spring.GetUnitCollisionVolumeData


```lua
function Spring.GetUnitCollisionVolumeData(unitID: integer)
```


---

# Spring.GetUnitCommands


```lua
function Spring.GetUnitCommands(unitID: integer, count: integer)
  -> commands: Command[]
```


```lua
function Spring.GetUnitCommands(unitID: integer, count: 0)
  -> The: integer
```


---

# Spring.GetUnitCostTable


```lua
function Spring.GetUnitCostTable(unitID: integer)
  -> cost: ResourceCost?
  2. buildTime: number?
```


---

# Spring.GetUnitCosts


```lua
function Spring.GetUnitCosts(unitID: integer)
  -> buildTime: number?
  2. metalCost: number
  3. energyCost: number
```


---

# Spring.GetUnitCurrentBuildPower


```lua
function Spring.GetUnitCurrentBuildPower(unitID: integer)
```


---

# Spring.GetUnitCurrentCommand


```lua
function Spring.GetUnitCurrentCommand(unitID: integer, cmdIndex: integer)
```


---

# Spring.GetUnitDefDimensions


```lua
function Spring.GetUnitDefDimensions(unitDefID: integer)
```


---

# Spring.GetUnitDefID


```lua
function Spring.GetUnitDefID(unitID: integer)
  -> number?
```


---

# Spring.GetUnitDirection


```lua
function Spring.GetUnitDirection(unitID: integer)
  -> dirX: number?
  2. dirY: number?
  3. dirZ: number?
```


---

# Spring.GetUnitDrawFlag


```lua
function Spring.GetUnitDrawFlag(unitID: integer)
  -> nil: number?
```


---

# Spring.GetUnitEffectiveBuildRange


```lua
function Spring.GetUnitEffectiveBuildRange(unitID: integer, buildeeDefID: integer)
  -> effectiveBuildRange: number
```


---

# Spring.GetUnitEngineDrawMask


```lua
function Spring.GetUnitEngineDrawMask(unitID: integer)
  -> nil: boolean?
```


---

# Spring.GetUnitEstimatedPath


```lua
function Spring.GetUnitEstimatedPath(unitID: integer)
```


---

# Spring.GetUnitExperience


```lua
function Spring.GetUnitExperience(unitID: integer)
  -> xp: number
  2. limXp: number
```


---

# Spring.GetUnitFeatureSeparation


```lua
function Spring.GetUnitFeatureSeparation(unitID: integer)
```


---

# Spring.GetUnitFlanking


```lua
function Spring.GetUnitFlanking(unitID: integer)
```


---

# Spring.GetUnitGroup


```lua
function Spring.GetUnitGroup(unitID: integer)
  -> groupID: number?
```


---

# Spring.GetUnitHarvestStorage


```lua
function Spring.GetUnitHarvestStorage(unitID: integer)
  -> storedMetal: number
  2. maxStoredMetal: number
  3. storedEnergy: number
  4. maxStoredEnergy: number
```


---

# Spring.GetUnitHeading


```lua
function Spring.GetUnitHeading(unitID: integer, convertToRadians?: boolean)
  -> heading: number
```


---

# Spring.GetUnitHealth


```lua
function Spring.GetUnitHealth(unitID: integer)
  -> health: number?
  2. maxHealth: number
  3. paralyzeDamage: number
  4. captureProgress: number
  5. buildProgress: number
```


---

# Spring.GetUnitHeight


```lua
function Spring.GetUnitHeight(unitID: integer)
  -> number?
```


---

# Spring.GetUnitInBuildStance


```lua
function Spring.GetUnitInBuildStance(unitID: integer)
  -> inBuildStance: boolean
```


---

# Spring.GetUnitIsActive


```lua
function Spring.GetUnitIsActive(unitID: integer)
  -> isActive: boolean?
```


---

# Spring.GetUnitIsBeingBuilt


```lua
function Spring.GetUnitIsBeingBuilt(unitID: integer)
  -> beingBuilt: boolean
  2. buildProgress: number
```


---

# Spring.GetUnitIsBuilding


```lua
function Spring.GetUnitIsBuilding(unitID: integer)
  -> buildeeUnitID: number
```


---

# Spring.GetUnitIsCloaked


```lua
function Spring.GetUnitIsCloaked(unitID: integer)
  -> isCloaked: boolean?
```


---

# Spring.GetUnitIsDead


```lua
function Spring.GetUnitIsDead(unitID: integer)
  -> boolean|nil
```


---

# Spring.GetUnitIsStunned


```lua
function Spring.GetUnitIsStunned(unitID: integer)
  -> stunnedOrBuilt: boolean|nil
  2. stunned: boolean
  3. beingBuilt: boolean
```


---

# Spring.GetUnitIsTransporting


```lua
function Spring.GetUnitIsTransporting(unitID: integer)
  -> transporteeArray: integer[]?
```


---

# Spring.GetUnitLastAttackedPiece


```lua
function Spring.GetUnitLastAttackedPiece(unitID: integer)
```


---

# Spring.GetUnitLastAttacker


```lua
function Spring.GetUnitLastAttacker(unitID: integer)
```


---

# Spring.GetUnitLosState


```lua
function Spring.GetUnitLosState(unitID: integer, allyTeamID?: integer, raw: true)
  -> bitmask: integer?
```


```lua
function Spring.GetUnitLosState(unitID: integer, allyTeamID?: integer, raw?: false)
  -> los: { los: boolean, radar: boolean, typed: boolean }?
```


---

# Spring.GetUnitLuaDraw


```lua
function Spring.GetUnitLuaDraw(unitID: integer)
  -> draw: boolean?
```


---

# Spring.GetUnitMapCentroid


```lua
function Spring.GetUnitMapCentroid(units: table)
  -> centerX: number
  2. centerY: number
  3. centerZ: number
```


---

# Spring.GetUnitMass


```lua
function Spring.GetUnitMass(unitID: integer)
  -> number?
```


---

# Spring.GetUnitMaxRange


```lua
function Spring.GetUnitMaxRange(unitID: integer)
  -> maxRange: number
```


---

# Spring.GetUnitMetalExtraction


```lua
function Spring.GetUnitMetalExtraction(unitID: integer)
  -> metalExtraction: number?
```


---

# Spring.GetUnitMoveTypeData


```lua
function Spring.GetUnitMoveTypeData(unitID: integer)
```


---

# Spring.GetUnitNanoPieces


```lua
function Spring.GetUnitNanoPieces(unitID: integer)
  -> pieceArray: integer[]
```


---

# Spring.GetUnitNearestAlly


```lua
function Spring.GetUnitNearestAlly(unitID: integer, range?: number)
  -> unitID: number?
```


---

# Spring.GetUnitNearestEnemy


```lua
function Spring.GetUnitNearestEnemy(unitID: integer, range?: number, useLOS?: boolean)
  -> unitID: number?
```


---

# Spring.GetUnitNeutral


```lua
function Spring.GetUnitNeutral(unitID: integer)
  -> boolean|nil
```


---

# Spring.GetUnitNoDraw


```lua
function Spring.GetUnitNoDraw(unitID: integer)
  -> nil: boolean?
```


---

# Spring.GetUnitNoGroup


```lua
function Spring.GetUnitNoGroup(unitID: integer)
  -> noGroup: bool|nil
```


---

# Spring.GetUnitNoMinimap


```lua
function Spring.GetUnitNoMinimap(unitID: integer)
  -> nil: boolean?
```


---

# Spring.GetUnitNoSelect


```lua
function Spring.GetUnitNoSelect(unitID: integer)
  -> noSelect: boolean?
```


---

# Spring.GetUnitPhysicalState


```lua
function Spring.GetUnitPhysicalState(unitID: integer)
  -> Unit: number
```


---

# Spring.GetUnitPieceDirection


```lua
function Spring.GetUnitPieceDirection(unitID: integer, pieceIndex: integer)
  -> dirX: number|nil
  2. dirY: number
  3. dirZ: number
```


---

# Spring.GetUnitPieceInfo


```lua
function Spring.GetUnitPieceInfo(unitID: integer, pieceIndex: integer)
  -> pieceInfo: PieceInfo?
```


---

# Spring.GetUnitPieceList


```lua
function Spring.GetUnitPieceList(unitID: integer)
  -> pieceNames: string[]
```


---

# Spring.GetUnitPieceMap


```lua
function Spring.GetUnitPieceMap(unitID: integer)
  -> pieceInfos: table<string, number>|nil
```


---

# Spring.GetUnitPieceMatrix


```lua
function Spring.GetUnitPieceMatrix(unitID: integer)
  -> m11: number|nil
  2. m12: number
  3. m13: number
  4. m14: number
  5. m21: number
  6. m22: number
  7. m23: number
  8. m24: number
  9. m31: number
 10. m32: number
 11. m33: number
 12. m34: number
 13. m41: number
 14. m42: number
 15. m43: number
 16. m44: number
```


---

# Spring.GetUnitPiecePosDir


```lua
function Spring.GetUnitPiecePosDir(unitID: integer, pieceIndex: integer)
  -> posX: number|nil
  2. posY: number
  3. posZ: number
  4. dirX: number
  5. dirY: number
  6. dirZ: number
```


---

# Spring.GetUnitPiecePosition


```lua
function Spring.GetUnitPiecePosition(unitID: integer, pieceIndex: integer)
  -> posX: number|nil
  2. posY: number
  3. posZ: number
```


---

# Spring.GetUnitPosErrorParams


```lua
function Spring.GetUnitPosErrorParams(unitID: integer, allyTeamID?: integer)
  -> posErrorVectorX: number?
  2. posErrorVectorY: number
  3. posErrorVectorZ: number
  4. posErrorDeltaX: number
  5. posErrorDeltaY: number
  6. posErrorDeltaZ: number
  7. nextPosErrorUpdatebaseErrorMult: number
  8. posErrorBit: boolean
```


---

# Spring.GetUnitPosition


```lua
function Spring.GetUnitPosition(unitID: integer, midPos?: boolean, aimPos?: boolean)
  -> basePointX: number?
  2. basePointY: number
  3. basePointZ: number
  4. midPointX: number?
  5. midPointY: number
  6. midPointZ: number
  7. aimPointX: number?
  8. aimPointY: number
  9. aimPointZ: number
```


---

# Spring.GetUnitRadius


```lua
function Spring.GetUnitRadius(unitID: integer)
  -> number?
```


---

# Spring.GetUnitResources


```lua
function Spring.GetUnitResources(unitID: integer)
  -> metalMake: number?
  2. metalUse: number
  3. energyMake: number
  4. energyUse: number
```


---

# Spring.GetUnitRootPiece


```lua
function Spring.GetUnitRootPiece(unitID: integer)
  -> index: number
```


---

# Spring.GetUnitRotation


```lua
function Spring.GetUnitRotation(unitID: integer)
  -> pitch: number?
  2. yaw: number?
  3. roll: number?
```


---

# Spring.GetUnitRulesParam


```lua
function Spring.GetUnitRulesParam(unitID: integer, ruleRef: string|number)
  -> value: string|number|nil
```


---

# Spring.GetUnitRulesParams


```lua
function Spring.GetUnitRulesParams(unitID: integer)
  -> rulesParams: RulesParams
```


---

# Spring.GetUnitScriptNames


```lua
function Spring.GetUnitScriptNames(unitID: integer)
  -> where: table<string, number>
```


---

# Spring.GetUnitScriptPiece


```lua
function Spring.GetUnitScriptPiece(unitID: integer)
  -> pieceIndices: integer[]
```


```lua
function Spring.GetUnitScriptPiece(unitID: integer, scriptPiece: integer)
  -> pieceIndex: integer
```


---

# Spring.GetUnitSeismicSignature


```lua
function Spring.GetUnitSeismicSignature(unitID: integer)
  -> seismicSignature: number?
```


---

# Spring.GetUnitSelectionVolumeData


```lua
function Spring.GetUnitSelectionVolumeData(unitID: integer)
  -> scaleX: number?
  2. scaleY: number
  3. scaleZ: number
  4. offsetX: number
  5. offsetY: number
  6. offsetZ: number
  7. volumeType: number
  8. useContHitTest: number
  9. getPrimaryAxis: number
 10. ignoreHits: boolean
```


---

# Spring.GetUnitSelfDTime


```lua
function Spring.GetUnitSelfDTime(unitID: integer)
  -> selfDTime: integer?
```


---

# Spring.GetUnitSensorRadius


```lua
function Spring.GetUnitSensorRadius(unitID: integer, type: string)
  -> radius: number?
```


---

# Spring.GetUnitSeparation


```lua
function Spring.GetUnitSeparation(unitID1: number, unitID2: number, direction?: boolean, subtractRadii?: boolean)
  -> number?
```


---

# Spring.GetUnitShieldState


```lua
function Spring.GetUnitShieldState(unitID: integer, weaponNum?: number)
  -> isEnabled: number
  2. currentPower: number
```


---

# Spring.GetUnitStates


```lua
function Spring.GetUnitStates(unitID: integer)
  -> UnitState
```


---

# Spring.GetUnitStockpile


```lua
function Spring.GetUnitStockpile(unitID: integer)
  -> numStockpiled: integer?
  2. numStockpileQued: integer?
  3. buildPercent: number?
```


---

# Spring.GetUnitStorage


```lua
function Spring.GetUnitStorage(unitID: integer)
  -> Unit: number
  2. Unit: number
```


---

# Spring.GetUnitTeam


```lua
function Spring.GetUnitTeam(unitID: integer)
  -> number?
```


---

# Spring.GetUnitTooltip


```lua
function Spring.GetUnitTooltip(unitID: integer)
  -> string|nil
```


---

# Spring.GetUnitTransformMatrix


```lua
function Spring.GetUnitTransformMatrix(unitID: integer)
  -> m11: number?
  2. m12: number
  3. m13: number
  4. m14: number
  5. m21: number
  6. m22: number
  7. m23: number
  8. m24: number
  9. m31: number
 10. m32: number
 11. m33: number
 12. m34: number
 13. m41: number
 14. m42: number
 15. m43: number
 16. m44: number
```


---

# Spring.GetUnitTransporter


```lua
function Spring.GetUnitTransporter(unitID: integer)
  -> transportUnitID: number|nil
```


---

# Spring.GetUnitVectors


```lua
function Spring.GetUnitVectors(unitID: integer)
  -> front: xyz?
  2. up: xyz
  3. right: xyz
```


---

# Spring.GetUnitVelocity


```lua
function Spring.GetUnitVelocity(unitID: integer)
```


---

# Spring.GetUnitViewPosition


```lua
function Spring.GetUnitViewPosition(unitID: integer, midPos?: boolean)
  -> x: number?
  2. y: number
  3. z: number
```


---

# Spring.GetUnitWeaponCanFire


```lua
function Spring.GetUnitWeaponCanFire(unitID: integer)
```


---

# Spring.GetUnitWeaponDamages


```lua
function Spring.GetUnitWeaponDamages(unitID: integer)
```


---

# Spring.GetUnitWeaponHaveFreeLineOfFire


```lua
function Spring.GetUnitWeaponHaveFreeLineOfFire(unitID: integer)
```


---

# Spring.GetUnitWeaponState


```lua
function Spring.GetUnitWeaponState(unitID: integer, weaponNum: number, stateName: string)
  -> stateValue: number
```


---

# Spring.GetUnitWeaponTarget


```lua
function Spring.GetUnitWeaponTarget(unitID: integer, weaponNum: integer)
  -> TargetType: 0
  2. isUserTarget: boolean
```


```lua
function Spring.GetUnitWeaponTarget(unitID: integer, weaponNum: integer)
  -> TargetType: 1
  2. isUserTarget: boolean
  3. targetUnitID: integer
```


```lua
function Spring.GetUnitWeaponTarget(unitID: integer, weaponNum: integer)
  -> TargetType: 2
  2. isUserTarget: boolean
  3. targetPosition: xyz
```


```lua
function Spring.GetUnitWeaponTarget(unitID: integer, weaponNum: integer)
  -> TargetType: 3
  2. isUserTarget: boolean
  3. targetProjectileId: integer
```


---

# Spring.GetUnitWeaponTestRange


```lua
function Spring.GetUnitWeaponTestRange(unitID: integer)
```


---

# Spring.GetUnitWeaponTestTarget


```lua
function Spring.GetUnitWeaponTestTarget(unitID: integer)
```


---

# Spring.GetUnitWeaponTryTarget


```lua
function Spring.GetUnitWeaponTryTarget(unitID: integer)
```


---

# Spring.GetUnitWeaponVectors


```lua
function Spring.GetUnitWeaponVectors(unitID: integer)
```


---

# Spring.GetUnitWorkerTask


```lua
function Spring.GetUnitWorkerTask(unitID: integer)
  -> cmdID: number
  2. targetID: number
```


---

# Spring.GetUnitsInBox


```lua
function Spring.GetUnitsInBox(xmin: number, ymin: number, zmin: number, xmax: number, ymax: number, zmax: number, allegiance?: number)
  -> unitIDs: number[]
```


---

# Spring.GetUnitsInCylinder


```lua
function Spring.GetUnitsInCylinder(x: number, z: number, radius: number)
  -> unitIDs: number[]
```


---

# Spring.GetUnitsInPlanes


```lua
function Spring.GetUnitsInPlanes(planes: Plane[], allegiance?: integer)
  -> unitIDs: integer[]
```


---

# Spring.GetUnitsInRectangle


```lua
function Spring.GetUnitsInRectangle(xmin: number, zmin: number, xmax: number, zmax: number, allegiance?: number)
  -> unitIDs: number[]
```


---

# Spring.GetUnitsInScreenRectangle


```lua
function Spring.GetUnitsInScreenRectangle(left: number, top: number, right: number, bottom: number, allegiance?: number)
  -> unitIDs: number[]|nil
```


---

# Spring.GetUnitsInSphere


```lua
function Spring.GetUnitsInSphere(x: number, y: number, z: number, radius: number)
  -> unitIDs: number[]
```


---

# Spring.GetVectorFromHeading


```lua
function Spring.GetVectorFromHeading(heading: number)
  -> x: number
  2. z: number
```


---

# Spring.GetVidMemUsage


```lua
function Spring.GetVidMemUsage()
  -> usedMem: number
  2. availableMem: number
```


---

# Spring.GetVideoCapturingMode


```lua
function Spring.GetVideoCapturingMode()
  -> allowRecord: boolean
```


---

# Spring.GetViewGeometry


```lua
function Spring.GetViewGeometry()
  -> viewSizeX: number
  2. viewSizeY: number
  3. viewPosX: number
  4. viewPosY: number
```


---

# Spring.GetVisibleFeatures


```lua
function Spring.GetVisibleFeatures(teamID?: integer, radius?: number, icons?: boolean, geos?: boolean)
  -> featureIDs: number[]|nil
```


---

# Spring.GetVisibleProjectiles


```lua
function Spring.GetVisibleProjectiles(allyTeamID?: integer, addSyncedProjectiles?: boolean, addWeaponProjectiles?: boolean, addPieceProjectiles?: boolean)
  -> projectileIDs: number[]|nil
```


---

# Spring.GetVisibleUnits


```lua
function Spring.GetVisibleUnits(teamID?: integer, radius?: number, icons?: boolean)
  -> unitIDs: number[]|nil
```


---

# Spring.GetWaterLevel


```lua
function Spring.GetWaterLevel(x: number, z: number)
  -> waterLevel: number
```


---

# Spring.GetWaterMode


```lua
function Spring.GetWaterMode()
  -> waterRendererID: number
  2. waterRendererName: string
```


---

# Spring.GetWaterPlaneLevel


```lua
function Spring.GetWaterPlaneLevel()
  -> waterPlaneLevel: number
```


---

# Spring.GetWind


```lua
function Spring.GetWind()
  -> windStrength: number
```


---

# Spring.GetWindowDisplayMode


```lua
function Spring.GetWindowDisplayMode()
  -> width: number
  2. height: number
  3. bits: number
  4. refresh: number
```


---

# Spring.GetWindowGeometry


```lua
function Spring.GetWindowGeometry()
  -> winSizeX: number
  2. winSizeY: number
  3. winPosX: number
  4. winPosY: number
  5. windowBorderTop: number
  6. windowBorderLeft: number
  7. windowBorderBottom: number
  8. windowBorderRight: number
```


---

# Spring.GiveOrder


```lua
function Spring.GiveOrder(cmdID: integer, params: table, options: cmdOpts)
  -> true|nil
```


---

# Spring.GiveOrderArrayToUnit


```lua
function Spring.GiveOrderArrayToUnit(unitID: integer, cmdArray: Command[])
  -> ordersGiven: boolean
```


```lua
function Spring.GiveOrderArrayToUnit(unitID: integer, cmdArray: Command[])
  -> ordersGiven: boolean
```


---

# Spring.GiveOrderArrayToUnitArray


```lua
function Spring.GiveOrderArrayToUnitArray(unitArray: number[], commands: Command[])
  -> nil
```


```lua
function Spring.GiveOrderArrayToUnitArray(unitArray: number[], cmdArray: Command[], pairwise?: boolean)
  -> boolean|nil
```


---

# Spring.GiveOrderArrayToUnitMap


```lua
function Spring.GiveOrderArrayToUnitMap(unitMap: { [number]: any }, commands: Command[])
  -> unitsOrdered: number
```


```lua
function Spring.GiveOrderArrayToUnitMap(unitMap: table, cmdArray: Command[])
  -> ordersGiven: boolean
```


---

# Spring.GiveOrderToUnit


```lua
function Spring.GiveOrderToUnit(unitID: integer, cmdID: integer, params?: number[], options?: CommandOptions)
  -> unitOrdered: boolean
```


```lua
function Spring.GiveOrderToUnit(unitID: integer, cmdID: integer, params: table, options: cmdOpts)
  -> true|nil
```


---

# Spring.GiveOrderToUnitArray


```lua
function Spring.GiveOrderToUnitArray(unitIDs: number[], cmdID: integer, params?: number[], options?: CommandOptions)
  -> unitsOrdered: number
```


```lua
function Spring.GiveOrderToUnitArray(unitArray: number[], cmdID: integer, params: table, options: cmdOpts)
  -> true|nil
```


---

# Spring.GiveOrderToUnitMap


```lua
function Spring.GiveOrderToUnitMap(unitMap: table<number, table>, cmdID: integer, params?: number[], options?: CommandOptions)
  -> unitsOrdered: number
```


```lua
function Spring.GiveOrderToUnitMap(unitMap: table, cmdID: integer, params: table, options: cmdOpts)
  -> true|nil
```


---

# Spring.HaveAdvShading


```lua
function Spring.HaveAdvShading()
  -> useAdvShading: boolean
  2. groundUseAdvShading: boolean
```


---

# Spring.HaveShadows


```lua
function Spring.HaveShadows()
  -> shadowsLoaded: boolean
```


---

# Spring.InsertUnitCmdDesc


```lua
function Spring.InsertUnitCmdDesc(unitID: integer, cmdDescID?: integer, cmdArray: CommandDescription)
```


---

# Spring.IsAABBInView


```lua
function Spring.IsAABBInView(minX: number, minY: number, minZ: number, maxX: number, maxY: number, maxZ: number)
  -> inView: boolean
```


---

# Spring.IsAboveMiniMap


```lua
function Spring.IsAboveMiniMap(x: number, y: number)
  -> isAbove: boolean
```


---

# Spring.IsCheatingEnabled


```lua
function Spring.IsCheatingEnabled()
  -> enabled: boolean
```


---

# Spring.IsDevLuaEnabled


```lua
function Spring.IsDevLuaEnabled()
  -> enabled: boolean
```


---

# Spring.IsEditDefsEnabled


```lua
function Spring.IsEditDefsEnabled()
  -> enabled: boolean
```


---

# Spring.IsGUIHidden


```lua
function Spring.IsGUIHidden()
  -> boolean
```


---

# Spring.IsGameOver


```lua
function Spring.IsGameOver()
  -> isGameOver: boolean
```


---

# Spring.IsGodModeEnabled


```lua
function Spring.IsGodModeEnabled()
  -> enabled: boolean
```


---

# Spring.IsNoCostEnabled


```lua
function Spring.IsNoCostEnabled()
  -> enabled: boolean
```


---

# Spring.IsPosInAirLos


```lua
function Spring.IsPosInAirLos(posX: number, posY: number, posZ: number, allyTeamID?: integer)
  -> boolean
```


---

# Spring.IsPosInLos


```lua
function Spring.IsPosInLos(posX: number, posY: number, posZ: number, allyTeamID?: integer)
  -> boolean
```


---

# Spring.IsPosInMap


```lua
function Spring.IsPosInMap(x: number, z: number)
  -> inPlayArea: boolean
  2. inMap: boolean
```


---

# Spring.IsPosInRadar


```lua
function Spring.IsPosInRadar(posX: number, posY: number, posZ: number, allyTeamID?: integer)
  -> boolean
```


---

# Spring.IsReplay


```lua
function Spring.IsReplay()
  -> isReplay: boolean?
```


---

# Spring.IsSphereInView


```lua
function Spring.IsSphereInView(posX: number, posY: number, posZ: number, radius?: number)
  -> inView: boolean
```


---

# Spring.IsUnitAllied


```lua
function Spring.IsUnitAllied(unitID: integer)
  -> isAllied: boolean?
```


---

# Spring.IsUnitIcon


```lua
function Spring.IsUnitIcon(unitID: integer)
  -> isUnitIcon: boolean?
```


---

# Spring.IsUnitInAirLos


```lua
function Spring.IsUnitInAirLos(unitID: integer, allyTeamID: integer)
  -> inAirLos: boolean
```


---

# Spring.IsUnitInJammer


```lua
function Spring.IsUnitInJammer(unitID: integer, allyTeamID: integer)
  -> inJammer: boolean
```


---

# Spring.IsUnitInLos


```lua
function Spring.IsUnitInLos(unitID: integer, allyTeamID: integer)
  -> inLos: boolean
```


---

# Spring.IsUnitInRadar


```lua
function Spring.IsUnitInRadar(unitID: integer, allyTeamID: integer)
  -> inRadar: boolean
```


---

# Spring.IsUnitInView


```lua
function Spring.IsUnitInView(unitID: integer)
  -> inView: boolean?
```


---

# Spring.IsUnitSelected


```lua
function Spring.IsUnitSelected(unitID: integer)
  -> isSelected: boolean?
```


---

# Spring.IsUnitVisible


```lua
function Spring.IsUnitVisible(unitID: integer, radius?: number, checkIcon: boolean)
  -> isVisible: boolean?
```


---

# Spring.IsUserWriting


```lua
function Spring.IsUserWriting()
  -> boolean
```


---

# Spring.KillTeam


```lua
function Spring.KillTeam(teamID: integer)
  -> nil
```


---

# Spring.LevelHeightMap


```lua
function Spring.LevelHeightMap(x1: number, z1: number, x2_height: number, z2?: number, height?: number)
  -> nil
```


---

# Spring.LevelOriginalHeightMap


```lua
function Spring.LevelOriginalHeightMap(x1: number, y1: number, x2_height: number, y2?: number, height?: number)
  -> nil
```


---

# Spring.LevelSmoothMesh


```lua
function Spring.LevelSmoothMesh(x1: number, z1: number, x2?: number, z2?: number, height: number)
  -> nil
```


---

# Spring.LoadCmdColorsConfig


```lua
function Spring.LoadCmdColorsConfig(config: string)
  -> nil
```


---

# Spring.LoadCtrlPanelConfig


```lua
function Spring.LoadCtrlPanelConfig(config: string)
  -> nil
```


---

# Spring.LoadModelTextures


```lua
function Spring.LoadModelTextures(modelName: string)
  -> success: boolean?
```


---

# Spring.LoadSoundDef


```lua
function Spring.LoadSoundDef(soundfile: string)
  -> success: boolean
```


---

# Spring.Log


```lua
function Spring.Log(section: string, logLevel?: integer|"debug"|"deprecated"|"error"|"fatal"...(+3), ...string)
```


---

# Spring.MarkerAddLine


```lua
function Spring.MarkerAddLine(x1: number, y1: number, z1: number, x2: number, y2: number, z2: number, localOnly?: boolean, playerId?: number)
  -> nil
```


---

# Spring.MarkerAddPoint


```lua
function Spring.MarkerAddPoint(x: number, y: number, z: number, text?: string, localOnly?: boolean)
  -> nil
```


---

# Spring.MarkerErasePosition


```lua
function Spring.MarkerErasePosition(x: number, y: number, z: number, unused: nil, localOnly?: boolean, playerId?: number, alwaysErase?: boolean)
  -> nil
```


---

# Spring.PauseDollyCamera


```lua
function Spring.PauseDollyCamera(fraction: number)
  -> nil
```


---

# Spring.PauseSoundStream


```lua
function Spring.PauseSoundStream()
  -> nil
```


---

# Spring.Ping


```lua
function Spring.Ping(pingTag: number)
  -> nil
```


---

# Spring.PlaySoundFile


```lua
function Spring.PlaySoundFile(soundfile: string, volume?: number, posx?: number, posy?: number, posz?: number, speedx?: number, speedy?: number, speedz?: number, channel?: "battle"|"general"|"sfx"|"ui"|"unitreply"...(+6))
  -> playSound: boolean
```


---

# Spring.PlaySoundStream


```lua
function Spring.PlaySoundStream(oggfile: string, volume?: number, enqueue?: boolean)
  -> success: boolean
```


---

# Spring.Pos2BuildPos


```lua
function Spring.Pos2BuildPos(unitDefID: integer, posX: number, posY: number, posZ: number, buildFacing?: number)
  -> buildPosX: number
  2. buildPosY: number
  3. buildPosZ: number
```


---

# Spring.PreloadFeatureDefModel


```lua
function Spring.PreloadFeatureDefModel(featureDefID: integer)
  -> nil
```


---

# Spring.PreloadSoundItem


```lua
function Spring.PreloadSoundItem(name: string)
  -> nil
```


---

# Spring.PreloadUnitDefModel


```lua
function Spring.PreloadUnitDefModel(unitDefID: integer)
  -> nil
```


---

# Spring.Quit


```lua
function Spring.Quit()
  -> nil
```


---

# Spring.RebuildSmoothMesh


```lua
function Spring.RebuildSmoothMesh()
  -> nil
```


---

# Spring.Reload


```lua
function Spring.Reload(startScript: string)
  -> nil
```


---

# Spring.RemoveGrass


```lua
function Spring.RemoveGrass(x: number, z: number)
  -> nil
```


---

# Spring.RemoveObjectDecal


```lua
function Spring.RemoveObjectDecal(unitID: integer)
  -> nil
```


---

# Spring.RemoveUnitCmdDesc


```lua
function Spring.RemoveUnitCmdDesc(unitID: integer, cmdDescID?: integer)
```


---

# Spring.ReplaceMouseCursor


```lua
function Spring.ReplaceMouseCursor(oldFileName: string, newFileName: string, hotSpotTopLeft?: boolean)
  -> assigned: boolean?
```


---

# Spring.Restart


```lua
function Spring.Restart(commandline_args: string, startScript: string)
  -> nil
```


---

# Spring.ResumeDollyCamera


```lua
function Spring.ResumeDollyCamera()
  -> nil
```


---

# Spring.RevertHeightMap


```lua
function Spring.RevertHeightMap(x1: number, y1: number, x2_factor: number, y2?: number, factor?: number)
  -> nil
```


---

# Spring.RevertOriginalHeightMap


```lua
function Spring.RevertOriginalHeightMap(x1: number, y1: number, x2_factor: number, y2?: number, factor?: number)
  -> nil
```


---

# Spring.RevertSmoothMesh


```lua
function Spring.RevertSmoothMesh(x1: number, z1: number, x2?: number, z2?: number, origFactor: number)
  -> nil
```


---

# Spring.RunDollyCamera


```lua
function Spring.RunDollyCamera(runtime: number)
  -> nil
```


---

# Spring.SDLSetTextInputRect


```lua
function Spring.SDLSetTextInputRect(x: number, y: number, width: number, height: number)
  -> nil
```


---

# Spring.SDLStartTextInput


```lua
function Spring.SDLStartTextInput()
  -> nil
```


---

# Spring.SDLStopTextInput


```lua
function Spring.SDLStopTextInput()
  -> nil
```


---

# Spring.SelectUnit


```lua
function Spring.SelectUnit(unitID?: integer, append?: boolean)
  -> nil
```


---

# Spring.SelectUnitArray


```lua
function Spring.SelectUnitArray(unitMap: table<any, integer>, append?: boolean)
  -> nil
```


---

# Spring.SelectUnitMap


```lua
function Spring.SelectUnitMap(unitMap: table<integer, any>, append?: boolean)
  -> nil
```


---

# Spring.SendCommands


```lua
function Spring.SendCommands(commands: string[])
```


```lua
function Spring.SendCommands(command: string, ...string)
  -> nil
```


---

# Spring.SendLuaGaiaMsg


```lua
function Spring.SendLuaGaiaMsg(message: string)
  -> nil
```


---

# Spring.SendLuaMenuMsg


```lua
function Spring.SendLuaMenuMsg(msg: string)
```


---

# Spring.SendLuaRulesMsg


```lua
function Spring.SendLuaRulesMsg(message: string)
  -> nil
```


---

# Spring.SendLuaUIMsg


```lua
function Spring.SendLuaUIMsg(message: string, mode: string)
  -> nil
```


---

# Spring.SendMessage


```lua
function Spring.SendMessage(message: string)
  -> nil
```


---

# Spring.SendMessageToAllyTeam


```lua
function Spring.SendMessageToAllyTeam(allyID: integer, message: string)
  -> nil
```


---

# Spring.SendMessageToPlayer


```lua
function Spring.SendMessageToPlayer(playerID: integer, message: string)
  -> nil
```


---

# Spring.SendMessageToSpectators


```lua
function Spring.SendMessageToSpectators(message: string)
  -> nil
```


---

# Spring.SendMessageToTeam


```lua
function Spring.SendMessageToTeam(teamID: integer, message: string)
  -> nil
```


---

# Spring.SendSkirmishAIMessage


```lua
function Spring.SendSkirmishAIMessage(aiTeam: number, message: string)
  -> ai_processed: boolean?
```


---

# Spring.SetActiveCommand


```lua
function Spring.SetActiveCommand(action: string, actionExtra?: string)
  -> commandSet: boolean?
```


```lua
function Spring.SetActiveCommand(cmdIndex: number, button?: number, leftClick?: boolean, rightClick?: boolean, alt?: boolean, ctrl?: boolean, meta?: boolean, shift?: boolean)
  -> commandSet: boolean?
```


---

# Spring.SetAlly


```lua
function Spring.SetAlly(firstAllyTeamID: integer, secondAllyTeamID: integer, ally: boolean)
  -> nil
```


---

# Spring.SetAllyTeamStartBox


```lua
function Spring.SetAllyTeamStartBox(allyTeamID: integer, xMin: number, zMin: number, xMax: number, zMax: number)
  -> nil
```


---

# Spring.SetAtmosphere


```lua
function Spring.SetAtmosphere(params: AtmosphereParams)
```


---

# Spring.SetAutoShowMetal


```lua
function Spring.SetAutoShowMetal(autoShow: boolean)
  -> nil
```


---

# Spring.SetBoxSelectionByEngine


```lua
function Spring.SetBoxSelectionByEngine(state: boolean)
  -> nil
```


---

# Spring.SetBuildFacing


```lua
function Spring.SetBuildFacing(facing: number)
  -> nil
```


---

# Spring.SetBuildSpacing


```lua
function Spring.SetBuildSpacing(spacing: number)
  -> nil
```


---

# Spring.SetCameraOffset


```lua
function Spring.SetCameraOffset(posX?: number, posY?: number, posZ?: number, tiltX?: number, tiltY?: number, tiltZ?: number)
  -> nil
```


---

# Spring.SetCameraState


```lua
function Spring.SetCameraState(camState: camState, transitionTime?: number, transitionTimeFactor?: number, transitionTimeExponent?: number)
  -> set: boolean
```


---

# Spring.SetCameraTarget


```lua
function Spring.SetCameraTarget(x: number, y: number, z: number, transTime?: number)
  -> nil
```


---

# Spring.SetClipboard


```lua
function Spring.SetClipboard(text: string)
  -> nil
```


---

# Spring.SetConfigFloat


```lua
function Spring.SetConfigFloat(name: string, value: number, useOverla?: boolean)
  -> nil
```


---

# Spring.SetConfigInt


```lua
function Spring.SetConfigInt(name: string, value: number, useOverlay?: boolean)
  -> nil
```


---

# Spring.SetConfigString


```lua
function Spring.SetConfigString(name: string, value: number, useOverlay?: boolean)
  -> nil
```


---

# Spring.SetCustomCommandDrawData


```lua
function Spring.SetCustomCommandDrawData(cmdID: integer, string: any, number: any)
  -> assigned: boolean?
```


---

# Spring.SetDollyCameraCurve


```lua
function Spring.SetDollyCameraCurve(degree: number, cpoints: table, knots: table)
  -> nil
```


---

# Spring.SetDollyCameraLookCurve


```lua
function Spring.SetDollyCameraLookCurve(degree: number, cpoints: table, knots: table)
  -> nil
```


---

# Spring.SetDollyCameraLookPosition


```lua
function Spring.SetDollyCameraLookPosition(x: number, y: number, z: number)
  -> nil
```


---

# Spring.SetDollyCameraLookUnit


```lua
function Spring.SetDollyCameraLookUnit(unitID: integer)
  -> nil
```


---

# Spring.SetDollyCameraMode


```lua
function Spring.SetDollyCameraMode(mode: 1|2)
  -> nil
```


---

# Spring.SetDollyCameraPosition


```lua
function Spring.SetDollyCameraPosition(x: number, y: number, z: number)
  -> nil
```


---

# Spring.SetDollyCameraRelativeMode


```lua
function Spring.SetDollyCameraRelativeMode(relativeMode: number)
  -> nil
```


---

# Spring.SetDrawGround


```lua
function Spring.SetDrawGround(drawGround: boolean)
  -> nil
```


---

# Spring.SetDrawGroundDeferred


```lua
function Spring.SetDrawGroundDeferred(drawGroundDeferred: boolean, drawGroundForward?: boolean)
  -> nil
```


---

# Spring.SetDrawModelsDeferred


```lua
function Spring.SetDrawModelsDeferred(drawUnitsDeferred: boolean, drawFeaturesDeferred: boolean, drawUnitsForward?: boolean, drawFeaturesForward?: boolean)
  -> nil
```


---

# Spring.SetDrawSelectionInfo


```lua
function Spring.SetDrawSelectionInfo(enable: boolean)
  -> nil
```


---

# Spring.SetDrawSky


```lua
function Spring.SetDrawSky(drawSky: boolean)
  -> nil
```


---

# Spring.SetDrawWater


```lua
function Spring.SetDrawWater(drawWater: boolean)
  -> nil
```


---

# Spring.SetExperienceGrade


```lua
function Spring.SetExperienceGrade(expGrade: number, ExpPowerScale?: number, ExpHealthScale?: number, ExpReloadScale?: number)
  -> nil
```


---

# Spring.SetFactoryBuggerOff


```lua
function Spring.SetFactoryBuggerOff(unitID: integer, buggerOff?: boolean, offset?: number, radius?: number, relHeading?: number, spherical?: boolean, forced?: boolean)
  -> buggerOff: number|nil
```


---

# Spring.SetFeatureAlwaysUpdateMatrix


```lua
function Spring.SetFeatureAlwaysUpdateMatrix(featureID: integer, alwaysUpdateMat: number)
  -> nil
```


---

# Spring.SetFeatureAlwaysVisible


```lua
function Spring.SetFeatureAlwaysVisible(featureID: integer, enable: boolean)
  -> nil
```


---

# Spring.SetFeatureBlocking


```lua
function Spring.SetFeatureBlocking(featureID: integer, isBlocking: boolean, isSolidObjectCollidable: boolean, isProjectileCollidable: boolean, isRaySegmentCollidable: boolean, crushable: boolean, blockEnemyPushing: boolean, blockHeightChanges: boolean)
  -> nil
```


---

# Spring.SetFeatureCollisionVolumeData


```lua
function Spring.SetFeatureCollisionVolumeData(featureID: integer, scaleX: number, scaleY: number, scaleZ: number, offsetX: number, offsetY: number, offsetZ: number, vType: number, tType: number, Axis: number)
  -> nil
```


---

# Spring.SetFeatureDirection


```lua
function Spring.SetFeatureDirection(featureID: integer, dirX: number, dirY: number, dirZ: number)
  -> nil
```


---

# Spring.SetFeatureEngineDrawMask


```lua
function Spring.SetFeatureEngineDrawMask(featureID: integer, engineDrawMask: number)
  -> nil
```


---

# Spring.SetFeatureFade


```lua
function Spring.SetFeatureFade(featureID: integer, allow: boolean)
  -> nil
```


---

# Spring.SetFeatureHeadingAndUpDir


```lua
function Spring.SetFeatureHeadingAndUpDir(featureID: integer, heading: number, upx: number, upy: number, upz: number)
  -> nil
```


---

# Spring.SetFeatureHealth


```lua
function Spring.SetFeatureHealth(featureID: integer, health: number)
  -> nil
```


---

# Spring.SetFeatureMass


```lua
function Spring.SetFeatureMass(featureID: integer, mass: number)
  -> nil
```


---

# Spring.SetFeatureMaxHealth


```lua
function Spring.SetFeatureMaxHealth(featureID: integer, maxHealth: number)
  -> nil
```


---

# Spring.SetFeatureMidAndAimPos


```lua
function Spring.SetFeatureMidAndAimPos(featureID: integer, mpX: number, mpY: number, mpZ: number, apX: number, apY: number, apZ: number, relative?: boolean)
  -> success: boolean
```


---

# Spring.SetFeatureMoveCtrl


```lua
function Spring.SetFeatureMoveCtrl(featureID: integer, enable?: boolean, arg1?: number, arg2?: number, argn?: number)
  -> nil
```


---

# Spring.SetFeatureNoDraw


```lua
function Spring.SetFeatureNoDraw(featureID: integer, noDraw: boolean)
  -> nil
```


---

# Spring.SetFeatureNoSelect


```lua
function Spring.SetFeatureNoSelect(featureID: integer, noSelect: boolean)
  -> nil
```


---

# Spring.SetFeaturePhysics


```lua
function Spring.SetFeaturePhysics(featureID: integer, posX: number, posY: number, posZ: number, velX: number, velY: number, velZ: number, rotX: number, rotY: number, rotZ: number, dragX: number, dragY: number, dragZ: number)
  -> nil
```


---

# Spring.SetFeaturePieceCollisionVolumeData


```lua
function Spring.SetFeaturePieceCollisionVolumeData(featureID: integer, pieceIndex: number, enable: boolean, scaleX: number, scaleY: number, scaleZ: number, offsetX: number, offsetY: number, offsetZ: number, Axis: number, volumeType: number, primaryAxis?: number)
  -> nil
```


---

# Spring.SetFeaturePieceVisible


```lua
function Spring.SetFeaturePieceVisible(featureID: integer, pieceIndex: number, visible: boolean)
  -> nil
```


---

# Spring.SetFeaturePosition


```lua
function Spring.SetFeaturePosition(featureID: integer, x: number, y: number, z: number, snapToGround?: boolean)
  -> nil
```


---

# Spring.SetFeatureRadiusAndHeight


```lua
function Spring.SetFeatureRadiusAndHeight(featureID: integer, radius: number, height: number)
  -> success: boolean
```


---

# Spring.SetFeatureReclaim


```lua
function Spring.SetFeatureReclaim(featureID: integer, reclaimLeft: number)
  -> nil
```


---

# Spring.SetFeatureResources


```lua
function Spring.SetFeatureResources(featureID: integer, metal: number, energy: number, reclaimTime?: number, reclaimLeft?: number, featureDefMetal?: number, featureDefEnergy?: number)
  -> nil
```


---

# Spring.SetFeatureResurrect


```lua
function Spring.SetFeatureResurrect(featureID: integer, unitDef: string|integer, facing?: "e"|"east"|"n"|"north"|"s"...(+7), progress?: number)
  -> nil
```


---

# Spring.SetFeatureRotation


```lua
function Spring.SetFeatureRotation(featureID: integer, rotX: number, rotY: number, rotZ: number)
  -> nil
```


---

# Spring.SetFeatureRulesParam


```lua
function Spring.SetFeatureRulesParam(featureID: integer, paramName: string, paramValue?: string|number, losAccess?: losAccess)
  -> nil
```


---

# Spring.SetFeatureSelectionVolumeData


```lua
function Spring.SetFeatureSelectionVolumeData(featureID: integer, scaleX: number, scaleY: number, scaleZ: number, offsetX: number, offsetY: number, offsetZ: number, vType: number, tType: number, Axis: number)
  -> nil
```


---

# Spring.SetFeatureUseAirLos


```lua
function Spring.SetFeatureUseAirLos(featureID: integer, useAirLos: boolean)
  -> nil
```


---

# Spring.SetFeatureVelocity


```lua
function Spring.SetFeatureVelocity(featureID: integer, velX: number, velY: number, velZ: number)
  -> nil
```


---

# Spring.SetGameRulesParam


```lua
function Spring.SetGameRulesParam(paramName: string, paramValue?: string|number, losAccess?: losAccess)
  -> nil
```


---

# Spring.SetGlobalLos


```lua
function Spring.SetGlobalLos(allyTeamID: integer, globallos: boolean)
  -> nil
```


---

# Spring.SetGroundDecalAlpha


```lua
function Spring.SetGroundDecalAlpha(decalID: integer, alpha?: number, alphaFalloff?: number)
  -> decalSet: boolean
```


---

# Spring.SetGroundDecalCreationFrame


```lua
function Spring.SetGroundDecalCreationFrame(decalID: integer, creationFrameMin?: number, creationFrameMax?: number)
  -> decalSet: boolean
```


---

# Spring.SetGroundDecalMisc


```lua
function Spring.SetGroundDecalMisc(decalID: integer, dotElimExp?: number, refHeight?: number, minHeight?: number, maxHeight?: number, forceHeightMode?: number)
  -> decalSet: boolean
```


---

# Spring.SetGroundDecalNormal


```lua
function Spring.SetGroundDecalNormal(decalID: integer, normalX?: number, normalY?: number, normalZ?: number)
  -> decalSet: boolean
```


---

# Spring.SetGroundDecalPosAndDims


```lua
function Spring.SetGroundDecalPosAndDims(decalID: integer, midPosX?: number, midPosZ?: number, sizeX?: number, sizeZ?: number, projCubeHeight?: number)
  -> decalSet: boolean
```


---

# Spring.SetGroundDecalQuadPosAndHeight


```lua
function Spring.SetGroundDecalQuadPosAndHeight(decalID: integer, posTL?: xz, posTR?: xz, posBR?: xz, posBL?: xz, projCubeHeight?: number)
  -> decalSet: boolean
```


---

# Spring.SetGroundDecalRotation


```lua
function Spring.SetGroundDecalRotation(decalID: integer, rot?: number)
  -> decalSet: boolean
```


---

# Spring.SetGroundDecalTexture


```lua
function Spring.SetGroundDecalTexture(decalID: integer, textureName: string, isMainTex?: boolean)
  -> decalSet: boolean|nil
```


---

# Spring.SetGroundDecalTextureParams


```lua
function Spring.SetGroundDecalTextureParams(decalID: integer, texWrapDistance?: number, texTraveledDistance?: number)
  -> decalSet: boolean|nil
```


```lua
function Spring.SetGroundDecalTextureParams(decalID: integer)
  -> texWrapDistance: number?
  2. texTraveledDistance: number
```


---

# Spring.SetGroundDecalTint


```lua
function Spring.SetGroundDecalTint(decalID: integer, tintColR?: number, tintColG?: number, tintColB?: number, tintColA?: number)
  -> decalSet: boolean
```


---

# Spring.SetHeightMap


```lua
function Spring.SetHeightMap(x: number, z: number, height: number, terraform?: number)
  -> absHeightDiff: integer?
```


---

# Spring.SetHeightMapFunc


```lua
function Spring.SetHeightMapFunc(luaFunction: function, arg: number, ...number)
  -> absTotalHeightMapAmountChanged: integer?
```


---

# Spring.SetLastMessagePosition


```lua
function Spring.SetLastMessagePosition(x: number, y: number, z: number)
  -> nil
```


---

# Spring.SetLogSectionFilterLevel


```lua
function Spring.SetLogSectionFilterLevel(sectionName: string, logLevel?: string|number)
  -> nil
```


---

# Spring.SetLosViewColors


```lua
function Spring.SetLosViewColors(always: rgb, LOS: rgb, radar: rgb, jam: rgb, radar2: rgb)
  -> nil
```


---

# Spring.SetMapLightTrackingState


```lua
function Spring.SetMapLightTrackingState(lightHandle: number, unitOrProjectileID: integer, enableTracking: boolean, unitOrProjectile: boolean)
  -> success: boolean
```


---

# Spring.SetMapRenderingParams


```lua
function Spring.SetMapRenderingParams(params: MapRenderingParams)
  -> nil
```


---

# Spring.SetMapShader


```lua
function Spring.SetMapShader(standardShaderID: integer, deferredShaderID: integer)
  -> nil
```


---

# Spring.SetMapShadingTexture


```lua
function Spring.SetMapShadingTexture(texType: string, texName: string)
  -> success: boolean
```


---

# Spring.SetMapSquareTerrainType


```lua
function Spring.SetMapSquareTerrainType(x: number, z: number, newType: number)
  -> oldType: integer?
```


---

# Spring.SetMapSquareTexture


```lua
function Spring.SetMapSquareTexture(texSqrX: number, texSqrY: number, luaTexName: string)
  -> success: boolean
```


---

# Spring.SetMetalAmount


```lua
function Spring.SetMetalAmount(x: integer, z: integer, metalAmount: number)
  -> nil
```


---

# Spring.SetModelLightTrackingState


```lua
function Spring.SetModelLightTrackingState(lightHandle: number, unitOrProjectileID: integer, enableTracking: boolean, unitOrProjectile: boolean)
  -> success: boolean
```


---

# Spring.SetMouseCursor


```lua
function Spring.SetMouseCursor(cursorName: string, cursorScale?: number)
  -> nil
```


---

# Spring.SetNanoProjectileParams


```lua
function Spring.SetNanoProjectileParams(rotVal?: number, rotVel?: number, rotAcc?: number, rotValRng?: number, rotVelRng?: number, rotAccRng?: number)
  -> nil
```


---

# Spring.SetNoPause


```lua
function Spring.SetNoPause(noPause: boolean)
  -> nil
```


---

# Spring.SetOriginalHeightMap


```lua
function Spring.SetOriginalHeightMap(x: number, y: number, height: number, factor?: number)
  -> nil
```


---

# Spring.SetOriginalHeightMapFunc


```lua
function Spring.SetOriginalHeightMapFunc(heightMapFunc: function)
  -> nil
```


---

# Spring.SetPieceProjectileParams


```lua
function Spring.SetPieceProjectileParams(projectileID: integer, explosionFlags?: number, spinAngle?: number, spinSpeed?: number, spinVectorX?: number, spinVectorY?: number, spinVectorZ?: number)
  -> nil
```


---

# Spring.SetPlayerRulesParam


```lua
function Spring.SetPlayerRulesParam(playerID: integer, paramName: string, paramValue?: string|number, losAccess?: losAccess)
  -> nil
```


---

# Spring.SetProjectileAlwaysVisible


```lua
function Spring.SetProjectileAlwaysVisible(projectileID: integer, alwaysVisible: boolean)
  -> nil
```


---

# Spring.SetProjectileCEG


```lua
function Spring.SetProjectileCEG(projectileID: integer, ceg_name: string)
  -> nil
```


---

# Spring.SetProjectileCollision


```lua
function Spring.SetProjectileCollision(projectileID: integer)
  -> nil
```


---

# Spring.SetProjectileDamages


```lua
function Spring.SetProjectileDamages(unitID: integer, weaponNum: number, key: string, value: number)
  -> nil
```


---

# Spring.SetProjectileGravity


```lua
function Spring.SetProjectileGravity(projectileID: integer, grav?: number)
  -> nil
```


---

# Spring.SetProjectileIgnoreTrackingError


```lua
function Spring.SetProjectileIgnoreTrackingError(projectileID: integer, ignore: boolean)
  -> nil
```


---

# Spring.SetProjectileIsIntercepted


```lua
function Spring.SetProjectileIsIntercepted(projectileID: integer)
  -> nil
```


---

# Spring.SetProjectileMoveControl


```lua
function Spring.SetProjectileMoveControl(projectileID: integer, enable: boolean)
  -> nil
```


---

# Spring.SetProjectilePosition


```lua
function Spring.SetProjectilePosition(projectileID: integer, posX?: number, posY?: number, posZ?: number)
  -> nil
```


---

# Spring.SetProjectileTarget


```lua
function Spring.SetProjectileTarget(projectileID: integer, arg1?: number, arg2?: number, posZ?: number)
  -> validTarget: boolean?
```


---

# Spring.SetProjectileTimeToLive


```lua
function Spring.SetProjectileTimeToLive(projectileID: integer, ttl: number)
  -> nil
```


---

# Spring.SetProjectileUseAirLos


```lua
function Spring.SetProjectileUseAirLos(projectileID: integer, useAirLos: boolean)
  -> nil
```


---

# Spring.SetProjectileVelocity


```lua
function Spring.SetProjectileVelocity(projectileID: integer, velX?: number, velY?: number, velZ?: number)
  -> nil
```


---

# Spring.SetRadarErrorParams


```lua
function Spring.SetRadarErrorParams(allyTeamID: integer, allyteamErrorSize: number, baseErrorSize?: number, baseErrorMult?: number)
  -> nil
```


---

# Spring.SetShareLevel


```lua
function Spring.SetShareLevel(resource: string, shareLevel: number)
  -> nil
```


---

# Spring.SetSkyBoxTexture


```lua
function Spring.SetSkyBoxTexture(texName: string)
  -> nil
```


---

# Spring.SetSmoothMesh


```lua
function Spring.SetSmoothMesh(x: number, z: number, height: number, terraform?: number)
  -> The: number?
```


---

# Spring.SetSmoothMeshFunc


```lua
function Spring.SetSmoothMeshFunc(luaFunction: function, arg?: any, ...any)
  -> absTotalHeightMapAmountChanged: number?
```


---

# Spring.SetSoundEffectParams


```lua
function Spring.SetSoundEffectParams()
```


---

# Spring.SetSoundStreamVolume


```lua
function Spring.SetSoundStreamVolume(volume: number)
  -> nil
```


---

# Spring.SetSquareBuildingMask


```lua
function Spring.SetSquareBuildingMask(x: number, z: number, mask: number)
  -> nil
```


---

# Spring.SetSunDirection


```lua
function Spring.SetSunDirection(dirX: number, dirY: number, dirZ: number, intensity?: number)
  -> nil
```


---

# Spring.SetSunLighting


```lua
function Spring.SetSunLighting(params: { groundAmbientColor: rgb, groundDiffuseColor: rgb })
```


---

# Spring.SetTeamColor


```lua
function Spring.SetTeamColor(teamID: integer, r: number, g: number, b: number)
  -> nil
```


---

# Spring.SetTeamResource


```lua
function Spring.SetTeamResource(teamID: integer, resource: "e"|"energy"|"energyStorage"|"es"|"m"...(+3), amount: number)
  -> nil
```


---

# Spring.SetTeamRulesParam


```lua
function Spring.SetTeamRulesParam(teamID: integer, paramName: string, paramValue?: string|number, losAccess?: losAccess)
  -> nil
```


---

# Spring.SetTeamShareLevel


```lua
function Spring.SetTeamShareLevel(teamID: integer, type: "e"|"energy"|"m"|"metal", amount: number)
  -> nil
```


---

# Spring.SetTerrainTypeData


```lua
function Spring.SetTerrainTypeData(typeIndex: number, speedTanks?: number, speedKBOts?: number, speedHovers?: number, speedShips?: number)
  -> true: boolean?
```


---

# Spring.SetTidal


```lua
function Spring.SetTidal(strength: number)
  -> nil
```


---

# Spring.SetUnitAlwaysUpdateMatrix


```lua
function Spring.SetUnitAlwaysUpdateMatrix(unitID: integer, alwaysUpdateMatrix: boolean)
  -> nil
```


---

# Spring.SetUnitAlwaysVisible


```lua
function Spring.SetUnitAlwaysVisible(unitID: integer, alwaysVisible: boolean)
  -> nil
```


---

# Spring.SetUnitArmored


```lua
function Spring.SetUnitArmored(unitID: integer, armored?: boolean, armorMultiple?: number)
  -> nil
```


---

# Spring.SetUnitBlocking


```lua
function Spring.SetUnitBlocking(unitID: integer, isblocking: boolean, isSolidObjectCollidable: boolean, isProjectileCollidable: boolean, isRaySegmentCollidable: boolean, crushable: boolean, blockEnemyPushing: boolean, blockHeightChanges: boolean)
  -> nil
```


---

# Spring.SetUnitBuildParams


```lua
function Spring.SetUnitBuildParams(unitID: integer, paramName: string, value: boolean|number)
  -> nil
```


---

# Spring.SetUnitBuildSpeed


```lua
function Spring.SetUnitBuildSpeed(builderID: integer, buildSpeed: number, repairSpeed?: number, reclaimSpeed?: number, captureSpeed?: number, terraformSpeed?: number)
  -> nil
```


---

# Spring.SetUnitBuildeeRadius


```lua
function Spring.SetUnitBuildeeRadius(unitID: integer, build: number)
  -> nil
```


---

# Spring.SetUnitCloak


```lua
function Spring.SetUnitCloak(unitID: integer, cloak: boolean|number, cloakArg: boolean|number)
  -> nil
```


---

# Spring.SetUnitCollisionVolumeData


```lua
function Spring.SetUnitCollisionVolumeData(unitID: integer, scaleX: number, scaleY: number, scaleZ: number, offsetX: number, offsetY: number, offsetZ: number, vType: number, tType: number, Axis: number)
  -> nil
```


---

# Spring.SetUnitCosts


```lua
function Spring.SetUnitCosts(unitID: integer, where: table<number, number>)
  -> nil
```


---

# Spring.SetUnitCrashing


```lua
function Spring.SetUnitCrashing(unitID: integer, crashing: boolean)
  -> success: boolean
```


---

# Spring.SetUnitDefIcon


```lua
function Spring.SetUnitDefIcon(unitDefID: integer, iconName: string)
  -> nil
```


---

# Spring.SetUnitDefImage


```lua
function Spring.SetUnitDefImage(unitDefID: integer, image: string)
  -> nil
```


---

# Spring.SetUnitDirection


```lua
function Spring.SetUnitDirection(unitID: integer, x: number, y: number, z: number)
  -> nil
```


---

# Spring.SetUnitEngineDrawMask


```lua
function Spring.SetUnitEngineDrawMask(unitID: integer, drawMask: number)
  -> nil
```


---

# Spring.SetUnitExperience


```lua
function Spring.SetUnitExperience(unitID: integer, experience: number)
  -> nil
```


---

# Spring.SetUnitFlanking


```lua
function Spring.SetUnitFlanking(unitID: integer, type: string, arg1: number, y?: number, z?: number)
  -> nil
```


---

# Spring.SetUnitGroup


```lua
function Spring.SetUnitGroup(unitID: integer, groupID: number)
  -> nil
```


---

# Spring.SetUnitHarvestStorage


```lua
function Spring.SetUnitHarvestStorage(unitID: integer, metal: number)
  -> nil
```


---

# Spring.SetUnitHeadingAndUpDir


```lua
function Spring.SetUnitHeadingAndUpDir(unitID: integer, heading: number, upx: number, upy: number, upz: number)
  -> nil
```


---

# Spring.SetUnitHealth


```lua
function Spring.SetUnitHealth(unitID: integer, health: number|table<string, number>)
  -> nil
```


---

# Spring.SetUnitIconDraw


```lua
function Spring.SetUnitIconDraw(unitID: integer, drawIcon: boolean)
  -> nil
```


---

# Spring.SetUnitLandGoal


```lua
function Spring.SetUnitLandGoal(unitID: integer, goalX: number, goalY: number, goalZ: number, goalRadius?: number)
  -> nil
```


---

# Spring.SetUnitLeaveTracks


```lua
function Spring.SetUnitLeaveTracks(unitID: integer, unitLeaveTracks: boolean)
  -> nil
```


---

# Spring.SetUnitLoadingTransport


```lua
function Spring.SetUnitLoadingTransport(passengerID: integer, transportID: integer)
  -> nil
```


---

# Spring.SetUnitLosMask


```lua
function Spring.SetUnitLosMask(unitID: integer, allyTeam: number, losTypes: number|table)
  -> nil
```


---

# Spring.SetUnitLosState


```lua
function Spring.SetUnitLosState(unitID: integer, allyTeam: number, los: number|table)
  -> nil
```


---

# Spring.SetUnitMass


```lua
function Spring.SetUnitMass(unitID: integer, mass: number)
  -> nil
```


---

# Spring.SetUnitMaxHealth


```lua
function Spring.SetUnitMaxHealth(unitID: integer, maxHealth: number)
  -> nil
```


---

# Spring.SetUnitMaxRange


```lua
function Spring.SetUnitMaxRange(unitID: integer, maxRange: number)
  -> nil
```


---

# Spring.SetUnitMetalExtraction


```lua
function Spring.SetUnitMetalExtraction(unitID: integer, depth: number, range?: number)
  -> nil
```


---

# Spring.SetUnitMidAndAimPos


```lua
function Spring.SetUnitMidAndAimPos(unitID: integer, mpX: number, mpY: number, mpZ: number, apX: number, apY: number, apZ: number, relative?: boolean)
  -> success: boolean
```


---

# Spring.SetUnitMoveGoal


```lua
function Spring.SetUnitMoveGoal(unitID: integer, goalX: number, goalY: number, goalZ: number, goalRadius?: number, moveSpeed?: number, moveRaw?: boolean)
  -> nil
```


---

# Spring.SetUnitNanoPieces


```lua
function Spring.SetUnitNanoPieces(builderID: integer, pieces: table)
  -> nil
```


---

# Spring.SetUnitNeutral


```lua
function Spring.SetUnitNeutral(unitID: integer, neutral: boolean)
  -> setNeutral: boolean|nil
```


---

# Spring.SetUnitNoDraw


```lua
function Spring.SetUnitNoDraw(unitID: integer, noDraw: boolean)
  -> nil
```


---

# Spring.SetUnitNoGroup


```lua
function Spring.SetUnitNoGroup(unitID: integer, unitNoGroup: boolean)
```


---

# Spring.SetUnitNoMinimap


```lua
function Spring.SetUnitNoMinimap(unitID: integer, unitNoMinimap: boolean)
  -> nil
```


---

# Spring.SetUnitNoSelect


```lua
function Spring.SetUnitNoSelect(unitID: integer, unitNoSelect: boolean)
  -> nil
```


---

# Spring.SetUnitPhysicalStateBit


```lua
function Spring.SetUnitPhysicalStateBit(unitID: integer, Physical: number)
  -> nil
```


---

# Spring.SetUnitPhysics


```lua
function Spring.SetUnitPhysics(unitID: integer, posX: number, posY: number, posZ: number, velX: number, velY: number, velZ: number, rotX: number, rotY: number, rotZ: number, dragX: number, dragY: number, dragZ: number)
  -> nil
```


---

# Spring.SetUnitPieceCollisionVolumeData


```lua
function Spring.SetUnitPieceCollisionVolumeData(unitID: integer, pieceIndex: number, enable: boolean, scaleX: number, scaleY: number, scaleZ: number, offsetX: number, offsetY: number, offsetZ: number, volumeType?: number, primaryAxis?: number)
  -> nil
```


---

# Spring.SetUnitPieceMatrix


```lua
function Spring.SetUnitPieceMatrix(unitID: integer, pieceNum: number, matrix: number[])
  -> nil
```


---

# Spring.SetUnitPieceParent


```lua
function Spring.SetUnitPieceParent(unitID: integer, AlteredPiece: number, ParentPiece: number)
  -> nil
```


---

# Spring.SetUnitPieceVisible


```lua
function Spring.SetUnitPieceVisible(unitID: integer, pieceIndex: number, visible: boolean)
  -> nil
```


---

# Spring.SetUnitPosErrorParams


```lua
function Spring.SetUnitPosErrorParams(unitID: integer, posErrorVectorX: number, posErrorVectorY: number, posErrorVectorZ: number, posErrorDeltaX: number, posErrorDeltaY: number, posErrorDeltaZ: number, nextPosErrorUpdate?: number)
  -> nil
```


---

# Spring.SetUnitPosition


```lua
function Spring.SetUnitPosition(unitID: integer, x: number, z: number, floating?: boolean)
  -> nil
```


```lua
function Spring.SetUnitPosition(unitID: integer, x: number, y: number, z: number)
  -> nil
```


---

# Spring.SetUnitRadiusAndHeight


```lua
function Spring.SetUnitRadiusAndHeight(unitID: integer, radius: number, height: number)
  -> success: boolean
```


---

# Spring.SetUnitResourcing


```lua
function Spring.SetUnitResourcing(unitID: integer, res: string, amount: number)
  -> nil
```


```lua
function Spring.SetUnitResourcing(unitID: integer, res: table<string, number>)
  -> nil
```


---

# Spring.SetUnitRotation


```lua
function Spring.SetUnitRotation(unitID: integer, yaw: number, pitch: number, roll: number)
  -> nil
```


---

# Spring.SetUnitRulesParam


```lua
function Spring.SetUnitRulesParam(unitID: integer, paramName: string, paramValue?: string|number, losAccess?: losAccess)
  -> nil
```


---

# Spring.SetUnitSeismicSignature


```lua
function Spring.SetUnitSeismicSignature(unitID: integer, seismicSignature: number)
  -> nil
```


---

# Spring.SetUnitSelectionVolumeData


```lua
function Spring.SetUnitSelectionVolumeData(unitID: integer, featureID: integer, scaleX: number, scaleY: number, scaleZ: number, offsetX: number, offsetY: number, offsetZ: number, vType: number, tType: number, Axis: number)
  -> nil
```


---

# Spring.SetUnitSensorRadius


```lua
function Spring.SetUnitSensorRadius(unitID: integer, type: "airLos"|"los"|"radar"|"radarJammer"|"seismic"...(+2), radius: number)
  -> New: number?
```


---

# Spring.SetUnitShieldRechargeDelay


```lua
function Spring.SetUnitShieldRechargeDelay(unitID: integer, weaponID?: integer, rechargeTime?: number)
  -> nil
```


---

# Spring.SetUnitShieldState


```lua
function Spring.SetUnitShieldState(unitID: integer, weaponID?: integer, enabled?: boolean, power?: number)
  -> nil
```


---

# Spring.SetUnitSonarStealth


```lua
function Spring.SetUnitSonarStealth(unitID: integer, sonarStealth: boolean)
  -> nil
```


---

# Spring.SetUnitStealth


```lua
function Spring.SetUnitStealth(unitID: integer, stealth: boolean)
  -> nil
```


---

# Spring.SetUnitStockpile


```lua
function Spring.SetUnitStockpile(unitID: integer, stockpile?: number, buildPercent?: number)
  -> nil
```


---

# Spring.SetUnitStorage


```lua
function Spring.SetUnitStorage(unitID: number, res: string, amount: number)
```


```lua
function Spring.SetUnitStorage(unitID: number, res: table<"e"|"energy"|"m"|"metal", number>)
```


---

# Spring.SetUnitTarget


```lua
function Spring.SetUnitTarget(unitID: integer, enemyUnitID?: integer, dgun?: boolean, userTarget?: boolean, weaponNum?: number)
  -> success: boolean
```


```lua
function Spring.SetUnitTarget(unitID: integer, x?: number, y?: number, z?: number, dgun?: boolean, userTarget?: boolean, weaponNum?: number)
  -> success: boolean
```


---

# Spring.SetUnitTooltip


```lua
function Spring.SetUnitTooltip(unitID: integer, tooltip: string)
  -> nil
```


---

# Spring.SetUnitUseAirLos


```lua
function Spring.SetUnitUseAirLos(unitID: integer, useAirLos: boolean)
  -> nil
```


---

# Spring.SetUnitUseWeapons


```lua
function Spring.SetUnitUseWeapons(unitID: integer, forceUseWeapons?: number, allowUseWeapons?: number)
  -> nil
```


---

# Spring.SetUnitVelocity


```lua
function Spring.SetUnitVelocity(unitID: integer, velX: number, velY: number, velZ: number)
  -> nil
```


---

# Spring.SetUnitWeaponDamages


```lua
function Spring.SetUnitWeaponDamages(unitID: integer, weaponNum: number|"explode"|"selfDestruct", damages: WeaponDamages)
  -> nil
```


```lua
function Spring.SetUnitWeaponDamages(unitID: integer, weaponNum: number|"explode"|"selfDestruct", key: string, value: number)
  -> nil
```


---

# Spring.SetUnitWeaponState


```lua
function Spring.SetUnitWeaponState(unitID: integer, weaponNum: number, states: WeaponState)
  -> nil
```


```lua
function Spring.SetUnitWeaponState(unitID: integer, weaponNum: number, key: string, value: number)
  -> nil
```


---

# Spring.SetVideoCapturingMode


```lua
function Spring.SetVideoCapturingMode(allowCaptureMode: boolean)
  -> nil
```


---

# Spring.SetVideoCapturingTimeOffset


```lua
function Spring.SetVideoCapturingTimeOffset(timeOffset: boolean)
  -> nil
```


---

# Spring.SetWMIcon


```lua
function Spring.SetWMIcon(iconFileName: string)
  -> nil
```


---

# Spring.SetWaterParams


```lua
function Spring.SetWaterParams(waterParams: WaterParams)
  -> nil
```


---

# Spring.SetWind


```lua
function Spring.SetWind(minStrength: number, maxStrength: number)
  -> nil
```


---

# Spring.SetWindowGeometry


```lua
function Spring.SetWindowGeometry(displayIndex: number, winRelPosX: number, winRelPosY: number, winSizeX: number, winSizeY: number, fullScreen: boolean, borderless: boolean)
  -> nil
```


---

# Spring.SetWindowMaximized


```lua
function Spring.SetWindowMaximized()
  -> maximized: boolean
```


---

# Spring.SetWindowMinimized


```lua
function Spring.SetWindowMinimized()
  -> minimized: boolean
```


---

# Spring.ShareResources


```lua
function Spring.ShareResources(teamID: integer, units: string)
  -> nil
```


```lua
function Spring.ShareResources(teamID: integer, resource: string, amount: number)
  -> nil
```


---

# Spring.ShareTeamResource


```lua
function Spring.ShareTeamResource(teamID_src: integer, teamID_recv: integer, type: "e"|"energy"|"m"|"metal", amount: number)
  -> nil
```


---

# Spring.SolveNURBSCurve


```lua
function Spring.SolveNURBSCurve(groupID: integer)
  -> unitIDs: number[]?
```


---

# Spring.SpawnCEG


```lua
function Spring.SpawnCEG(cegname: string, posX?: number, posY?: number, posZ?: number, dirX?: number, dirY?: number, dirZ?: number, radius?: number, damage?: number)
  -> success: boolean?
  2. cegID: number
```


---

# Spring.SpawnExplosion


```lua
function Spring.SpawnExplosion(posX?: number, posY?: number, posZ?: number, dirX?: number, dirY?: number, dirZ?: number, explosionParams: ExplosionParams)
  -> nil
```


---

# Spring.SpawnProjectile


```lua
function Spring.SpawnProjectile(weaponDefID: integer, projectileParams: ProjectileParams)
  -> projectileID: integer?
```


---

# Spring.SpawnSFX


```lua
function Spring.SpawnSFX(unitID?: integer, sfxID?: integer, posX?: number, posY?: number, posZ?: number, dirX?: number, dirY?: number, dirZ?: number, radius?: number, damage?: number, absolute?: boolean)
  -> success: boolean?
```


---

# Spring.Start


```lua
function Spring.Start(commandline_args: string, startScript: string)
  -> nil
```


---

# Spring.StopSoundStream


```lua
function Spring.StopSoundStream()
  -> nil
```


---

# Spring.TestBuildOrder


```lua
function Spring.TestBuildOrder(unitDefID: integer, x: number, y: number, z: number, facing: "e"|"east"|"n"|"north"|"s"...(+7))
  -> blocking: 0|1|2|3
  2. featureID: integer?
```


---

# Spring.TestMoveOrder


```lua
function Spring.TestMoveOrder(unitDefID: integer, pos: xyz, dir?: xyz, testTerrain?: boolean, testObjects?: boolean, centerOnly?: boolean)
  -> boolean
```


---

# Spring.TraceRayGroundBetweenPositions


```lua
function Spring.TraceRayGroundBetweenPositions(startX: number, startY: number, startZ: number, endX: number, endY: number, endZ: number, testWater?: boolean)
  -> rayLength: number
  2. posX: number
  3. posY: number
  4. posZ: number
```


---

# Spring.TraceRayGroundInDirection


```lua
function Spring.TraceRayGroundInDirection(posX: number, posY: number, posZ: number, dirX: number, dirY: number, dirZ: number, testWater?: boolean)
  -> rayLength: number
  2. posX: number
  3. posY: number
  4. posZ: number
```


---

# Spring.TraceScreenRay


```lua
function Spring.TraceScreenRay(screenX: number, screenY: number, onlyCoords?: boolean, useMinimap?: boolean, includeSky?: boolean, ignoreWater?: boolean, heightOffset?: number)
  -> description: string|nil
  2. unitID: string|number|xyz|nil
  3. featureID: string|number|nil
  4. coords: xyz|nil
```


---

# Spring.TransferFeature


```lua
function Spring.TransferFeature(featureDefID: integer, teamID: integer)
  -> nil
```


---

# Spring.TransferUnit


```lua
function Spring.TransferUnit(unitID: integer, newTeamID: integer, given?: boolean)
  -> nil
```


---

# Spring.UnitAttach


```lua
function Spring.UnitAttach(transporterID: integer, passengerID: integer, pieceNum: number)
  -> nil
```


---

# Spring.UnitDetach


```lua
function Spring.UnitDetach(passengerID: integer)
  -> nil
```


---

# Spring.UnitDetachFromAir


```lua
function Spring.UnitDetachFromAir(passengerID: integer)
  -> nil
```


---

# Spring.UnitFinishCommand


```lua
function Spring.UnitFinishCommand(unitID: integer)
```


---

# Spring.UnitIconGetDraw


```lua
function Spring.UnitIconGetDraw(unitID: integer)
  -> drawIcon: boolean?
```


---

# Spring.UnitIconSetDraw


```lua
function Spring.UnitIconSetDraw(unitID: integer, drawIcon: boolean)
  -> nil
```


---

# Spring.UnitWeaponFire


```lua
function Spring.UnitWeaponFire(unitID: integer, weaponID: integer)
  -> nil
```


---

# Spring.UnitWeaponHoldFire


```lua
function Spring.UnitWeaponHoldFire(unitID: integer, weaponID: integer)
  -> nil
```


---

# Spring.UpdateMapLight


```lua
function Spring.UpdateMapLight(lightHandle: number, lightParams: LightParams)
  -> success: boolean
```


---

# Spring.UpdateModelLight


```lua
function Spring.UpdateModelLight(lightHandle: number, lightParams: LightParams)
  -> success: boolean
```


---

# Spring.UseTeamResource


```lua
function Spring.UseTeamResource(teamID: integer, type: "e"|"energy"|"m"|"metal", amount: number)
  -> hadEnough: boolean
```


```lua
function Spring.UseTeamResource(teamID: integer, amount: table<"e"|"energy"|"m"|"metal", number>)
  -> hadEnough: boolean
```


---

# Spring.UseUnitResource


```lua
function Spring.UseUnitResource(unitID: integer, resource: "e"|"energy"|"m"|"metal", amount: number)
  -> okay: boolean?
```


```lua
function Spring.UseUnitResource(unitID: integer, resources: table<"e"|"energy"|"m"|"metal", number>)
  -> okay: boolean?
```


---

# Spring.ValidFeatureID


```lua
function Spring.ValidFeatureID(featureID: integer)
  -> boolean
```


---

# Spring.ValidUnitID


```lua
function Spring.ValidUnitID(unitID: integer)
  -> boolean
```


---

# Spring.WarpMouse


```lua
function Spring.WarpMouse(x: number, y: number)
  -> nil
```


---

# Spring.WorldToScreenCoords


```lua
function Spring.WorldToScreenCoords(x: number, y: number, z: number)
  -> viewPortX: number
  2. viewPortY: number
  3. viewPortZ: number
```


---

# Spring.Yield


```lua
function Spring.Yield()
  -> when: boolean
```


---

# StockpileChanged


```lua
function StockpileChanged(unitID: integer, unitDefID: integer, unitTeam: integer, weaponNum: integer, oldCount: integer, newCount: integer)
```


---

# StorageName


---

# SunChanged


```lua
function SunChanged()
```


---

# TargetType


---

# TeamChanged


```lua
function TeamChanged(teamID: number)
```


---

# TeamDied


```lua
function TeamDied(teamID: number)
```


---

# TeamStats

## damageDealt


```lua
number
```

## damageReceived


```lua
number
```

## energyExcess


```lua
number
```

## energyProduced


```lua
number
```

## energyReceived


```lua
number
```

## energySent


```lua
number
```

## energyUsed


```lua
number
```

## frame


```lua
number
```

## metalExcess


```lua
number
```

## metalProduced


```lua
number
```

## metalReceived


```lua
number
```

## metalSent


```lua
number
```

## metalUsed


```lua
number
```

## time


```lua
number
```

## unitsCaptured


```lua
integer
```

## unitsDied


```lua
integer
```

## unitsOutCaptured


```lua
integer
```

## unitsProduced


```lua
integer
```

## unitsReceived


```lua
integer
```

## unitsSent


```lua
integer
```


---

# TerraformComplete


```lua
function TerraformComplete(unitID: integer, unitDefID: integer, unitTeam: integer, buildUnitID: integer, buildUnitDefID: integer, buildUnitTeam: integer)
  -> if: boolean
```


---

# TextEditing


```lua
function TextEditing(utf8: string, start: number, length: number)
```


---

# TextInput


```lua
function TextInput(utf8char: string)
```


---

# UniformArrayType


---

# UniformParam


---

# UnitArrivedAtGoal


```lua
function UnitArrivedAtGoal(unitID: integer, unitDefID: integer, unitTeam: integer)
```


---

# UnitCloaked


```lua
function UnitCloaked(unitID: integer, unitDefID: integer, unitTeam: integer)
```


---

# UnitCmdDone


```lua
function UnitCmdDone(unitID: integer, unitDefID: integer, unitTeam: integer, cmdID: number, cmdParams: table, options: CommandOptions, cmdTag: number)
```


---

# UnitCommand


```lua
function UnitCommand(unitID: integer, unitDefID: integer, unitTeam: integer, cmdID: number, cmdParams: table, options: CommandOptions, cmdTag: number)
```


---

# UnitConstructionDecayed


```lua
function UnitConstructionDecayed(unitID: integer, unitDefID: integer, unitTeam: integer, timeSinceLastBuild: number, iterationPeriod: number, part: number)
```


---

# UnitCreated


```lua
function UnitCreated(unitID: integer, unitDefID: integer, unitTeam: integer, builderID?: number)
```


---

# UnitDamaged


```lua
function UnitDamaged(unitID: integer, unitDefID: integer, unitTeam: integer, damage: number, paralyzer: number, weaponDefID: number, projectileID: number, attackerID: number, attackerDefID: number, attackerTeam: number)
```


---

# UnitDecloaked


```lua
function UnitDecloaked(unitID: integer, unitDefID: integer, unitTeam: integer)
```


---

# UnitDestroyed


```lua
function UnitDestroyed(unitID: integer, unitDefID: integer, unitTeam: integer, attackerID: number, attackerDefID: number, attackerTeam: number, weaponDefID: number)
```


---

# UnitEnteredAir


```lua
function UnitEnteredAir(unitID: integer, unitDefID: integer, unitTeam: integer)
```


---

# UnitEnteredLos


```lua
function UnitEnteredLos(unitID: integer, unitTeam: integer, allyTeam: integer, unitDefID: integer)
```


---

# UnitEnteredRadar


```lua
function UnitEnteredRadar(unitID: integer, unitTeam: integer, allyTeam: integer, unitDefID: integer)
```


---

# UnitEnteredUnderwater


```lua
function UnitEnteredUnderwater(unitID: integer, unitDefID: integer, unitTeam: integer)
```


---

# UnitEnteredWater


```lua
function UnitEnteredWater(unitID: integer, unitDefID: integer, unitTeam: integer)
```


---

# UnitExperience


```lua
function UnitExperience(unitID: integer, unitDefID: integer, unitTeam: integer, experience: number, oldExperience: number)
```


---

# UnitFeatureCollision


```lua
function UnitFeatureCollision(colliderID: number, collideeID: number)
```


---

# UnitFinished


```lua
function UnitFinished(unitID: integer, unitDefID: integer, unitTeam: integer)
```


---

# UnitFromFactory


```lua
function UnitFromFactory(unitID: integer, unitDefID: integer, unitTeam: integer, factID: number, factDefID: number, userOrders: boolean)
```


---

# UnitGiven


```lua
function UnitGiven(unitID: integer, unitDefID: integer, newTeam: number, oldTeam: number)
```


---

# UnitHarvestStorageFull


```lua
function UnitHarvestStorageFull(unitID: integer, unitDefID: integer, unitTeam: integer)
```


---

# UnitIdle


```lua
function UnitIdle(unitID: integer, unitDefID: integer, unitTeam: integer)
```


---

# UnitLeftAir


```lua
function UnitLeftAir(unitID: integer, unitDefID: integer, unitTeam: integer)
```


---

# UnitLeftLos


```lua
function UnitLeftLos(unitID: integer, unitTeam: integer, allyTeam: integer, unitDefID: integer)
```


---

# UnitLeftRadar


```lua
function UnitLeftRadar(unitID: integer, unitTeam: integer, allyTeam: integer, unitDefID: integer)
```


---

# UnitLeftUnderwater


```lua
function UnitLeftUnderwater(unitID: integer, unitDefID: integer, unitTeam: integer)
```


---

# UnitLeftWater


```lua
function UnitLeftWater(unitID: integer, unitDefID: integer, unitTeam: integer)
```


---

# UnitLoaded


```lua
function UnitLoaded(unitID: integer, unitDefID: integer, unitTeam: integer, transportID: integer, transportTeam: integer)
```


---

# UnitMoveFailed


```lua
function UnitMoveFailed(unitID: integer, unitDefID: integer, unitTeam: integer)
```


---

# UnitPreDamaged


```lua
function UnitPreDamaged(unitID: integer, unitDefID: integer, unitTeam: integer, damage: number, paralyzer: boolean, weaponDefID?: integer, projectileID?: integer, attackerID?: integer, attackerDefID?: integer, attackerTeam?: integer)
  -> newDamage: number
  2. impulseMult: number
```


---

# UnitReverseBuilt


```lua
function UnitReverseBuilt(unitID: integer, unitDefID: integer, unitTeam: integer)
```


---

# UnitSeismicPing


```lua
function UnitSeismicPing(x: number, y: number, z: number, strength: number, allyTeam: integer, unitID: integer, unitDefID: integer)
```


---

# UnitState

## active


```lua
boolean
```

## autoland


```lua
boolean?
```

## autorepairlevel


```lua
number?
```

## cloak


```lua
boolean
```

## firestate


```lua
number
```

## loopbackattack


```lua
boolean?
```

## movestate


```lua
number
```

## repeat


```lua
boolean
```

## trajectory


```lua
boolean
```


---

# UnitStunned


```lua
function UnitStunned(unitID: integer, unitDefID: integer, unitTeam: integer, stunned: boolean)
```


---

# UnitTaken


```lua
function UnitTaken(unitID: integer, unitDefID: integer, oldTeam: number, newTeam: number)
```


---

# UnitUnitCollision


```lua
function UnitUnitCollision(colliderID: number, collideeID: number)
```


---

# UnitUnloaded


```lua
function UnitUnloaded(unitID: integer, unitDefID: integer, unitTeam: integer, transportID: integer, transportTeam: integer)
```


---

# UnsyncedHeightMapUpdate


```lua
function UnsyncedHeightMapUpdate()
  -> x1: number
  2. z1: number
  3. x2: number
  4. z2: number
```


---

# Update


```lua
function Update(dt: number)
```


---

# VAO


```lua
table
```


---

# VAO


---

# VAO.AddFeatureDefsToSubmission


```lua
(method) VAO:AddFeatureDefsToSubmission(featureDefIDs: number|number[])
  -> submittedCount: number
```


---

# VAO.AddFeaturesToSubmission


```lua
(method) VAO:AddFeaturesToSubmission(featureIDs: number|number[])
  -> submittedCount: number
```


---

# VAO.AddUnitDefsToSubmission


```lua
(method) VAO:AddUnitDefsToSubmission(unitDefIDs: number|number[])
  -> submittedCount: number
```


---

# VAO.AddUnitsToSubmission


```lua
(method) VAO:AddUnitsToSubmission(unitIDs: number|number[])
  -> submittedCount: number
```


---

# VAO.AttachIndexBuffer


```lua
(method) VAO:AttachIndexBuffer(vbo: VBO)
  -> nil
```


---

# VAO.AttachInstanceBuffer


```lua
(method) VAO:AttachInstanceBuffer(vbo: VBO)
  -> nil
```


---

# VAO.AttachVertexBuffer


```lua
(method) VAO:AttachVertexBuffer(vbo: VBO)
  -> nil
```


---

# VAO.Delete


```lua
(method) VAO:Delete()
  -> nil
```


---

# VAO.DrawArrays


```lua
(method) VAO:DrawArrays(glEnum: number, vertexCount?: number, vertexFirst?: number, instanceCount?: number, instanceFirst?: number)
  -> nil
```


---

# VAO.DrawElements


```lua
(method) VAO:DrawElements(glEnum: number, drawCount?: number, baseIndex?: number, instanceCount?: number, baseVertex?: number, baseInstance?: number)
  -> nil
```


---

# VAO.RemoveFromSubmission


```lua
(method) VAO:RemoveFromSubmission(index: number)
  -> nil
```


---

# VAO.Submit


```lua
(method) VAO:Submit()
  -> nil
```


---

# VBO


---

# VBOAttributeDef

## id


```lua
integer
```

## name


```lua
string
```

The location in the vertex shader layout e.g.: layout (location = 0) in vec2
aPos. optional attrib, specifies location in the vertex shader. If not
specified the implementation will increment the counter starting from 0.
There can be maximum 16 attributes (so id of 15 is max).

## normalized


```lua
boolean?
```

(Defaults: `false`)

## size


```lua
integer?
```


The name for this VBO, only used for debugging.

## type


```lua
GL.BYTE|GL.FLOAT|GL.INT|GL.SHORT|GL.UNSIGNED_BYTE...(+2)
```

(Default: `GL.FLOAT`)


---

# VBODataType


---

# ViewResize


```lua
function ViewResize(viewSizeX: number, viewSizeY: number)
```


---

# WaterParams

## absorb


```lua
rgb
```

Color triple (RGB)


## ambientFactor


```lua
number
```

## baseColor


```lua
rgb
```

Color triple (RGB)


## blurBase


```lua
number
```

## blurExponent


```lua
number
```

## causticsResolution


```lua
number
```

## causticsStrength


```lua
number
```

## damage


```lua
number
```

## diffuseColor


```lua
rgb
```

Color triple (RGB)


## diffuseFactor


```lua
number
```

## foamTexture


```lua
string
```

file

## forceRendering


```lua
boolean
```

## fresnelMax


```lua
number
```

## fresnelMin


```lua
number
```

## fresnelPower


```lua
number
```

## hasWaterPlane


```lua
boolean
```

## minColor


```lua
rgb
```

Color triple (RGB)


## normalTexture


```lua
string
```

file

## numTiles


```lua
integer
```

## perlinAmplitude


```lua
number
```

## perlinLacunarity


```lua
number
```

## perlinStartFreq


```lua
number
```

## planeColor


```lua
rgb
```

Color triple (RGB)


## reflectionDistortion


```lua
number
```

## repeatX


```lua
number
```

## repeatY


```lua
number
```

## shoreWaves


```lua
boolean
```

## specularColor


```lua
rgb
```

Color triple (RGB)


## specularFactor


```lua
number
```

## specularPower


```lua
number
```

## surfaceAlpha


```lua
number
```

## surfaceColor


```lua
rgb
```

Color triple (RGB)


## texture


```lua
string
```

file

## waveFoamDistortion


```lua
number
```

## waveFoamIntensity


```lua
number
```

## waveLength


```lua
number
```

## waveOffsetFactor


```lua
number
```

## windSpeed


```lua
number
```


---

# WeaponDamages

## craterAreaOfEffect


```lua
number
```

Set to `true` if a non-zero value is passed, `false` is zero is passed.

## craterBoost


```lua
number
```

## craterMult


```lua
number
```

## damageAreaOfEffect


```lua
number
```

## dynDamageExp


```lua
number
```

## dynDamageInverted


```lua
number
```

## dynDamageMin


```lua
number
```

## dynDamageRange


```lua
number
```

## edgeEffectiveness


```lua
number
```

## explosionSpeed


```lua
number
```

## impulseBoost


```lua
number
```

## impulseFactor


```lua
number
```

## paralyzeDamageTime


```lua
integer
```


---

# WeaponState

## accuracy


```lua
number?
```

## aimReady


```lua
number?
```

## avoidFlags


```lua
integer?
```

## burst


```lua
integer?
```

## burstRate


```lua
number?
```

## collisionFlags


```lua
integer?
```

## forceAim


```lua
integer?
```

Set to `true` if a non-zero value is passed, `false` is zero is passed.

## nextSalvo


```lua
integer?
```

## projectileSpeed


```lua
number?
```

If you change the range of a weapon with dynamic damage make sure you use `SetUnitWeaponDamages` to change dynDamageRange as well.

## projectiles


```lua
integer?
```

## range


```lua
number?
```

## reaimTime


```lua
integer?
```

## reloadFrame


```lua
integer?
```

Alias for `reloadState`.

## reloadState


```lua
integer?
```

## reloadTime


```lua
number?
```

## salvoLeft


```lua
integer?
```

## sprayAngle


```lua
number?
```


---

# WorldTooltip


```lua
function WorldTooltip(ttType: string, data1: number, data2?: number, data3?: number)
  -> newTooltip: string
```


---

# attachment


---

# camState

## angle


```lua
number
```

Camera rotation angle on X axis (aka tilt/pitch) (ta)

## dist


```lua
number
```

Camera distance from the ground (spring)

## dx


```lua
number
```

Camera direction vector X

## dy


```lua
number
```

Camera direction vector Y

## dz


```lua
number
```

Camera direction vector Z

## flipped


```lua
number
```

1 for when south is down, 1 for when north is down (ta)

## fov


```lua
number
```

## height


```lua
number
```

Camera distance from the ground (ta)

## mode


```lua
number
```

the camera mode: 0 (fps), 1 (ta), 2 (spring), 3 (rot), 4 (free), 5 (ov), 6 (dummy)

## name


```lua
"dummy"|"fps"|"free"|"ov"|"rot"...(+2)
```

## oldHeight


```lua
number
```

Camera distance from the ground, cannot be changed (rot)

## px


```lua
number
```

Position X of the ground point in screen center

## py


```lua
number
```

Position Y of the ground point in screen center

## pz


```lua
number
```

Position Z of the ground point in screen center

## rx


```lua
number
```

Camera rotation angle on X axis (spring)

## ry


```lua
number
```

Camera rotation angle on Y axis (spring)

## rz


```lua
number
```

Camera rotation angle on Z axis (spring)


---

# cmdOpts

## alt


```lua
boolean
```

Alt key pressed

## ctrl


```lua
boolean
```

Ctrl key pressed

## meta


```lua
boolean
```

Meta (windows/mac/mod4) key pressed

## right


```lua
boolean
```

Right mouse key pressed

## shift


```lua
boolean
```

Shift key pressed


---

# float3


---

# gl


```lua
table
```


```lua
table
```


---

# gl.ActiveFBO


```lua
function gl.ActiveFBO(fbo: Fbo, target?: GL, identities?: boolean, lua_function?: function, arg1?: any, arg2?: any, argn?: any)
```


---

# gl.ActiveShader


```lua
function gl.ActiveShader(shaderID: integer, func: function, ...any)
```


---

# gl.AddFallbackFont


```lua
function gl.AddFallbackFont(filePath: string)
  -> success: bool
```


---

# gl.BlitFBO


```lua
function gl.BlitFBO(x0Src: number, y0Src: number, x1Src: number, y1Src: number, x0Dst: number, y0Dst: number, x1Dst: number, y1Dst: number, mask?: number, filter?: number)
```


```lua
function gl.BlitFBO(fboSrc: Fbo, x0Src: number, y0Src: number, x1Src: number, y1Src: number, fboDst: Fbo, x0Dst: number, y0Dst: number, x1Dst: number, y1Dst: number, mask?: number, filter?: number)
```


---

# gl.ClearAttachmentFBO


```lua
function gl.ClearAttachmentFBO(target?: number, attachment: string|GL, clearValue0: number, clearValue1: number, clearValue2: number, clearValue3: number)
```


---

# gl.ClearFallbackFonts


```lua
function gl.ClearFallbackFonts()
  -> nil
```


---

# gl.Color


```lua
function gl.Color(r: number, g: number, b: number, a?: number)
```


```lua
function gl.Color(rgbs: [number, number, number, number])
```


```lua
function gl.Color(rgb: [number, number, number])
```


---

# gl.CreateFBO


```lua
function gl.CreateFBO(fbo: Fbo)
```


---

# gl.CreateRBO


```lua
function gl.CreateRBO(xsize: integer, ysize: integer, data: CreateRBOData)
  -> RBO
```


---

# gl.CreateShader


```lua
function gl.CreateShader(shaderParams: ShaderParams)
  -> shaderID: integer
```


---

# gl.DeleteFBO


```lua
function gl.DeleteFBO(fbo: Fbo)
```


---

# gl.DeleteRBO


```lua
function gl.DeleteRBO(rbo: RBO)
```


---

# gl.DeleteShader


```lua
function gl.DeleteShader(shaderID: integer)
```


---

# gl.GetActiveUniforms


```lua
function gl.GetActiveUniforms(shaderID: integer)
  -> activeUniforms: ActiveUniform[]
```


---

# gl.GetEngineModelUniformDataDef


```lua
function gl.GetEngineModelUniformDataDef(index: number)
  -> glslDefinition: string
```


---

# gl.GetEngineUniformBufferDef


```lua
function gl.GetEngineUniformBufferDef(index: number)
  -> glslDefinition: string
```


---

# gl.GetShaderLog


```lua
function gl.GetShaderLog()
  -> infoLog: string
```


---

# gl.GetUniformLocation


```lua
function gl.GetUniformLocation(shaderID: integer, name: string)
  -> locationID: GL
```


---

# gl.GetVAO


```lua
function gl.GetVAO()
  -> vao: VAO?
```


---

# gl.GetVBO


```lua
function gl.GetVBO(bufferType?: GL.ARRAY_BUFFER|GL.ELEMENT_ARRAY_BUFFER|GL.SHADER_STORAGE_BUFFER|GL.UNIFORM_BUFFER, freqUpdated?: boolean)
  -> VBO: VBO?
```


---

# gl.IsValidFBO


```lua
function gl.IsValidFBO(fbo: Fbo, target?: GL)
  -> valid: boolean
  2. status: number?
```


---

# gl.ObjectLabel


```lua
function gl.ObjectLabel(objectTypeIdentifier: GLenum, objectID: GLuint, label: string)
  -> nil
```


---

# gl.PopDebugGroup


```lua
function gl.PopDebugGroup()
  -> nil
```


---

# gl.PushDebugGroup


```lua
function gl.PushDebugGroup(id: GLuint, message: string, sourceIsThirdParty: boolean)
  -> nil
```


---

# gl.RawBindFBO


```lua
function gl.RawBindFBO(fbo: nil, target?: GL, rawFboId?: integer)
  -> nil
```


```lua
function gl.RawBindFBO(fbo: Fbo, target?: GL)
  -> previouslyBoundRawFboId: number
```


---

# gl.SetGeometryShaderParameter


```lua
function gl.SetGeometryShaderParameter(shaderID: integer, param: number, number: number)
  -> nil
```


---

# gl.SetTesselationShaderParameter


```lua
function gl.SetTesselationShaderParameter(param: integer, value: integer)
  -> nil
```


---

# gl.Text


```lua
function gl.Text(text: string, x: number, y: number, size: number, options?: string)
  -> nil
```


---

# gl.Uniform


```lua
function gl.Uniform(locationID: string|GL, f1: number, f2?: number, f3?: number, f4?: number)
```


---

# gl.UniformArray


```lua
function gl.UniformArray(locationID: string|integer, type: 1|2|3, uniforms: number[])
```


---

# gl.UniformInt


```lua
function gl.UniformInt(locationID: string|integer, int1: integer, int2?: integer, int3?: integer, int4?: integer)
```


---

# gl.UniformMatrix


```lua
function gl.UniformMatrix(locationID: string|integer, matrix: "camera"|"caminv"|"camprj"|"shadows")
```


```lua
function gl.UniformMatrix(locationID: string|number, matrix: number[])
```


---

# gl.UseShader


```lua
function gl.UseShader(shaderID: integer)
  -> linked: boolean
```


---

# losAccess

## allied


```lua
boolean?
```

readable by ally + ingame allied

## inlos


```lua
boolean?
```

readable if the unit is in LOS

## inradar


```lua
boolean?
```

readable if the unit is in AirLOS

## private


```lua
boolean?
```

only readable by the ally (default)

## public


```lua
boolean?
```

readable by all


---

# math.bit_and


```lua
function math.bit_and(...integer)
  -> result: integer
```


---

# math.bit_bits


```lua
function math.bit_bits(...integer)
  -> result: integer
```


---

# math.bit_inv


```lua
function math.bit_inv(value: integer)
  -> result: integer
```


---

# math.bit_or


```lua
function math.bit_or(...integer)
  -> result: integer
```


---

# math.bit_xor


```lua
function math.bit_xor(...integer)
  -> result: integer
```


---

# math.clamp


```lua
function math.clamp(value: number, min: number, max: number)
  -> clamped: number
```


---

# math.diag


```lua
function math.diag(x: number, ...number)
  -> diagonal: number
```


---

# math.erf


```lua
function math.erf(x: number)
  -> erf: number
```


---

# math.hypot


```lua
function math.hypot(x: number, y: number)
  -> number
```


---

# math.mix


```lua
function math.mix(x: number, y: number, a: number)
  -> number
```


---

# math.normalize


```lua
function math.normalize(x: number, ...number)
  -> ...number
```


---

# math.round


```lua
function math.round(x: number, decimals: number)
  -> rounded: number
```


---

# math.sgn


```lua
function math.sgn(x: number)
  -> sign: number
```


---

# math.smoothstep


```lua
function math.smoothstep(edge0: number, edge1: number, v: number)
  -> smoothstep: number
```


---

# rgb

## b


```lua
number
```

## g


```lua
number
```

## r


```lua
number
```


---

# rgba

## a


```lua
number
```

## b


```lua
number
```

## g


```lua
number
```

## r


```lua
number
```


---

# table.new


```lua
function table.new(nArray: number, nHashed: number)
  -> table
```


---

# tracy


```lua
table
```


---

# tracy.LuaTracyPlot


```lua
function tracy.LuaTracyPlot(plotName: string, plotValue: number)
```


---

# tracy.LuaTracyPlotConfig


```lua
function tracy.LuaTracyPlotConfig(plotName: string, plotFormatType: "Memory"|"Number"|"Percentage"|nil, stepwise?: boolean, fill?: boolean, color?: integer)
```


---

# xyz

## x


```lua
number
```

## y


```lua
number
```

## z


```lua
number
```


---

# xz

## x


```lua
number
```

## y


```lua
number
```