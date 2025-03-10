---
layout: default
title: Callins
parent: Lua API
permalink: lua-api/types/Callins
---

# class Callins





[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L73-L104" target="_blank">source</a>]

Functions called by the Engine.

To use these callins in a widget, prepend `widget:` and, for a gadget,
prepend `gadget:`. For example:

```lua
function widget:UnitCreated(unitID, unitDefID, unitTeam, builderID)
  -- ...
end
```

Some functions may differ between (synced) gadget and widgets. This is
because all information should be available to synced (game logic
controlling) gadgets, but restricted to unsynced gadget/widget. e.g.
information about an enemy unit only detected via radar and not yet in LOS.

In such cases the full (synced) param list is documented.

**Attention:** Some callins will only work on the unsynced portion of the gadget.
Due to the type-unsafe nature of lua parsing, those callins not firing up
might be hard to trace.

---

## methods
---

### Callins.ActiveCommandChanged
---
```lua
function Callins.ActiveCommandChanged(
  cmdId: integer?,
  cmdType: integer?
)
```





Called when a command is issued.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L3343-L3348" target="_blank">source</a>]


### Callins.AddConsoleLine
---
```lua
function Callins.AddConsoleLine(
  msg: string,
  priority: integer
)
```





Called when text is entered into the console (e.g. `Spring.Echo`).

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L3405-L3410" target="_blank">source</a>]


### Callins.CommandNotify
---
```lua
function Callins.CommandNotify(
  cmdID: integer,
  cmdParams: table,
  options: CommandOptions
) -> Returning boolean
```

@return `Returning` - true deletes the command and does not send it through the network.





Called when a command is issued.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L3369-L3376" target="_blank">source</a>]


### Callins.DefaultCommand
---
```lua
function Callins.DefaultCommand(
  type: string,
  id: integer
)
```
@param `type` - "unit" | "feature"

@param `id` - unitID | featureID






Used to set the default command when a unit is selected. First parameter is the type of the object pointed at (either "unit or "feature") and the second is its unitID or featureID respectively.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L2487-L2492" target="_blank">source</a>]


### Callins.DownloadFailed
---
```lua
function Callins.DownloadFailed(
  id: number,
  errorID: number
)
```





Called when a Pr-downloader download fails to complete.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L3743-L3748" target="_blank">source</a>]


### Callins.DownloadFinished
---
```lua
function Callins.DownloadFinished(id: number)
```





Called when a Pr-downloader download finishes successfully.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L3720-L3724" target="_blank">source</a>]


### Callins.DownloadProgress
---
```lua
function Callins.DownloadProgress(
  id: number,
  downloaded: number,
  total: number
)
```





Called incrementally during a Pr-downloader download.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L3768-L3774" target="_blank">source</a>]


### Callins.DownloadQueued
---
```lua
function Callins.DownloadQueued(
  id: number,
  name: string,
  type: string
)
```





Called when a Pr-downloader download is queued

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L3669-L3675" target="_blank">source</a>]


### Callins.DownloadStarted
---
```lua
function Callins.DownloadStarted(id: number)
```





Called when a Pr-downloader download is started via VFS.DownloadArchive.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L3697-L3701" target="_blank">source</a>]


### Callins.DrawFeaturesPostDeferred
---
```lua
function Callins.DrawFeaturesPostDeferred()
```





Runs at the end of the feature deferred pass to inform Lua code it should make use of the $model_gbuffer_* textures before another pass overwrites them (and to allow proper blending with e.g. cloaked objects which are drawn between these events and DrawWorld via gl.CopyToTexture). N.B. The *PostDeferred events are only sent (and only have a real purpose) if forward drawing is disabled.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L2671-L2674" target="_blank">source</a>]


### Callins.DrawGenesis
---
```lua
function Callins.DrawGenesis()
```





Use this callin to update textures, shaders, etc.

Doesn't render to screen!
Also available to LuaMenu.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L2579-L2585" target="_blank">source</a>]


### Callins.DrawGroundDeferred
---
```lua
function Callins.DrawGroundDeferred()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L2650-L2652" target="_blank">source</a>]


### Callins.DrawGroundPostDeferred
---
```lua
function Callins.DrawGroundPostDeferred()
```





This runs at the end of its respective deferred pass.

Allows proper frame compositing (with ground flashes/decals/foliage/etc, which are drawn between it and `DrawWorldPreUnit`) via `gl.CopyToTexture`.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L2655-L2660" target="_blank">source</a>]


### Callins.DrawGroundPostForward
---
```lua
function Callins.DrawGroundPostForward()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L2639-L2641" target="_blank">source</a>]


### Callins.DrawGroundPreDeferred
---
```lua
function Callins.DrawGroundPreDeferred()
```





Runs at the start of the deferred pass when a custom map shader has been assigned via `Spring.SetMapShader` (convenient for setting uniforms).

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L2644-L2647" target="_blank">source</a>]


### Callins.DrawGroundPreForward
---
```lua
function Callins.DrawGroundPreForward()
```





Runs at the start of the forward pass when a custom map shader has been assigned via `Spring.SetMapShader` (convenient for setting uniforms).

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L2633-L2636" target="_blank">source</a>]


### Callins.DrawInMiniMap
---
```lua
function Callins.DrawInMiniMap(
  sx: number,
  sy: number
)
```
@param `sx` - relative to the minimap's position and scale.

@param `sy` - relative to the minimap's position and scale.






[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L2785-L2790" target="_blank">source</a>]


### Callins.DrawInMiniMapBackground
---
```lua
function Callins.DrawInMiniMapBackground(
  sx: number,
  sy: number
)
```
@param `sx` - relative to the minimap's position and scale.

@param `sy` - relative to the minimap's position and scale.






[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L2813-L2818" target="_blank">source</a>]


### Callins.DrawPreDecals
---
```lua
function Callins.DrawPreDecals()
```





Called before decals are drawn

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L2600-L2603" target="_blank">source</a>]


### Callins.DrawScreen
---
```lua
function Callins.DrawScreen(
  viewSizeX: number,
  viewSizeY: number
)
```





Also available to LuaMenu.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L2735-L2740" target="_blank">source</a>]


### Callins.DrawScreenEffects
---
```lua
function Callins.DrawScreenEffects(
  viewSizeX: number,
  viewSizeY: number
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L2752-L2756" target="_blank">source</a>]


### Callins.DrawScreenPost
---
```lua
function Callins.DrawScreenPost(
  viewSizeX: number,
  viewSizeY: number
)
```





Similar to DrawScreenEffects, this can be used to alter the contents of a frame after it has been completely rendered (i.e. World, MiniMap, Menu, UI).

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L2768-L2773" target="_blank">source</a>]


### Callins.DrawShadowFeaturesLua
---
```lua
function Callins.DrawShadowFeaturesLua()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L2682-L2684" target="_blank">source</a>]


### Callins.DrawShadowPassTransparent
---
```lua
function Callins.DrawShadowPassTransparent()
```





Invoked after semi-transparent shadows pass is about to conclude

This callin has depth and color buffer of shadowmap bound via FBO as well as the FFP state to do "semi-transparent" shadows pass (traditionally only used to draw shadows of shadow casting semi-transparent particles). Can be used to draw nice colored shadows.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L2611-L2615" target="_blank">source</a>]


### Callins.DrawShadowUnitsLua
---
```lua
function Callins.DrawShadowUnitsLua()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L2677-L2679" target="_blank">source</a>]


### Callins.DrawUnitsPostDeferred
---
```lua
function Callins.DrawUnitsPostDeferred()
```





Runs at the end of the unit deferred pass.

Informs Lua code it should make use of the $model_gbuffer_* textures before another pass overwrites them (and to allow proper blending with e.g. cloaked objects which are drawn between these events and DrawWorld via gl.CopyToTexture). N.B. The *PostDeferred events are only sent (and only have a real purpose) if forward drawing is disabled.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L2663-L2668" target="_blank">source</a>]


### Callins.DrawWaterPost
---
```lua
function Callins.DrawWaterPost()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L2606-L2608" target="_blank">source</a>]


### Callins.DrawWorld
---
```lua
function Callins.DrawWorld()
```





Spring draws command queues, 'map stuff', and map marks.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L2588-L2591" target="_blank">source</a>]


### Callins.DrawWorldPreParticles
---
```lua
function Callins.DrawWorldPreParticles(
  drawAboveWater: boolean,
  drawBelowWater: boolean,
  drawReflection: boolean,
  drawRefraction: boolean
)
```





DrawWorldPreParticles is called multiples times per draw frame.
Each call has a different permutation of values for drawAboveWater, drawBelowWater, drawReflection, and drawRefraction.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L2687-L2696" target="_blank">source</a>]


### Callins.DrawWorldPreUnit
---
```lua
function Callins.DrawWorldPreUnit()
```





Spring draws units, features, some water types, cloaked units, and the sun.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L2594-L2597" target="_blank">source</a>]


### Callins.DrawWorldReflection
---
```lua
function Callins.DrawWorldReflection()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L2623-L2625" target="_blank">source</a>]


### Callins.DrawWorldRefraction
---
```lua
function Callins.DrawWorldRefraction()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L2628-L2630" target="_blank">source</a>]


### Callins.DrawWorldShadow
---
```lua
function Callins.DrawWorldShadow()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L2618-L2620" target="_blank">source</a>]


### Callins.Explosion
---
```lua
function Callins.Explosion(
  weaponDefID: number,
  px: number,
  py: number,
  pz: number,
  attackerID: number,
  projectileID: number
) -> noGfx boolean
```

@return `noGfx` - if then no graphical effects are drawn by the engine for this explosion.





Called when an explosion occurs.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L2168-L2179" target="_blank">source</a>]


### Callins.FeatureCreated
---
```lua
function Callins.FeatureCreated(
  featureID: number,
  allyTeamID: number
)
```





Called when a feature is created.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L1957-L1963" target="_blank">source</a>]


### Callins.FeatureDamaged
---
```lua
function Callins.FeatureDamaged(
  featureID: number,
  featureDefID: number,
  featureTeam: number,
  damage: number,
  weaponDefID: number,
  projectileID: number,
  attackerID: number,
  attackerDefID: number,
  attackerTeam: number
)
```





Called when a feature is damaged.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L2011-L2024" target="_blank">source</a>]


### Callins.FeatureDestroyed
---
```lua
function Callins.FeatureDestroyed(
  featureID: number,
  allyTeamID: number
)
```





Called when a feature is destroyed.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L1984-L1990" target="_blank">source</a>]


### Callins.FontsChanged
---
```lua
function Callins.FontsChanged()
```





Called whenever fonts are updated. Signals the game display lists
and other caches should be discarded.

Gets called before other Update and Draw callins.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L2450-L2456" target="_blank">source</a>]


### Callins.GameFrame
---
```lua
function Callins.GameFrame(frame: number)
```
@param `frame` - Starts at frame 1






Called for every game simulation frame (30 per second).

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L774-L778" target="_blank">source</a>]


### Callins.GameFramePost
---
```lua
function Callins.GameFramePost(frame: number)
```
@param `frame` - Starts at frame 1






Called at the end of every game simulation frame

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L808-L812" target="_blank">source</a>]


### Callins.GameID
---
```lua
function Callins.GameID(gameID: string)
```
@param `gameID` - encoded in hex.






Called once to deliver the gameID

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L832-L836" target="_blank">source</a>]


### Callins.GameOver
---
```lua
function Callins.GameOver(winningAllyTeams: number[])
```
@param `winningAllyTeams` - list of winning allyTeams, if empty the game result was undecided (like when dropping from an host).






Called when the game ends

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L697-L701" target="_blank">source</a>]


### Callins.GamePaused
---
```lua
function Callins.GamePaused(
  playerID: number,
  paused: boolean
)
```





Called when the game is paused.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L724-L729" target="_blank">source</a>]


### Callins.GamePreload
---
```lua
function Callins.GamePreload()
```





Called before the 0 gameframe.

Is not called when a saved game is loaded.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L651-L656" target="_blank">source</a>]


### Callins.GameProgress
---
```lua
function Callins.GameProgress(serverFrameNum: integer)
```





Called every 60 frames, calculating delta between `GameFrame` and `GameProgress`.

Can give an ETA about catching up with simulation for mid-game join players.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L2888-L2894" target="_blank">source</a>]


### Callins.GameSetup
---
```lua
function Callins.GameSetup(
  state: string,
  ready: boolean,
  playerStates: table
)
 -> success boolean
 -> newReady boolean

```





[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L3573-L3581" target="_blank">source</a>]


### Callins.GameStart
---
```lua
function Callins.GameStart()
```





Called upon the start of the game.

Is not called when a saved game is loaded.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L674-L679" target="_blank">source</a>]


### Callins.GetTooltip
---
```lua
function Callins.GetTooltip(
  x: number,
  y: number
) -> tooltip string
```





Called when `Spring.IsAbove` returns true.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L3315-L3321" target="_blank">source</a>]


### Callins.GotChatMsg
---
```lua
function Callins.GotChatMsg(
  msg: string,
  playerID: number
)
```





Called when a player issues a UI command e.g. types /foo or /luarules foo.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L556-L561" target="_blank">source</a>]


### Callins.GroupChanged
---
```lua
function Callins.GroupChanged(groupID: number)
```





Called when a unit is added to or removed from a control group.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L3428-L3432" target="_blank">source</a>]


### Callins.Initialize
---
```lua
function Callins.Initialize()
```





Called when the addon is (re)loaded.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L503-L506" target="_blank">source</a>]


### Callins.IsAbove
---
```lua
function Callins.IsAbove(
  x: number,
  y: number
) -> isAbove boolean
```





Called every `Update`.

Must return true for `Mouse*` events and `Spring.GetToolTip` to be called.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L3284-L3293" target="_blank">source</a>]


### Callins.KeyMapChanged
---
```lua
function Callins.KeyMapChanged()
```





Called when the keymap changes

Can be caused due to a change in language or keyboard

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L2932-L2937" target="_blank">source</a>]


### Callins.KeyPress
---
```lua
function Callins.KeyPress(
  keyCode: number,
  mods: KeyModifiers,
  isRepeat: boolean,
  label: boolean,
  utf32char: number,
  scanCode: number,
  actionList: table
) -> halt boolean
```
@param `isRepeat` - If you want an action to occur only once check for isRepeat == false.

@param `label` - the name of the key

@param `utf32char` - (deprecated) always 0

@param `actionList` - the list of actions for this keypress


@return `halt` - whether to halt the chain for consumers of the keypress





Called repeatedly when a key is pressed down.

Return true if you don't want other callins or the engine to also receive this keypress. A list of key codes can be seen at the SDL wiki.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L2973-L2987" target="_blank">source</a>]


### Callins.KeyRelease
---
```lua
function Callins.KeyRelease(
  keyCode: number,
  mods: KeyModifiers,
  label: boolean,
  utf32char: number,
  scanCode: number,
  actionList: table
) ->  boolean
```
@param `label` - the name of the key

@param `utf32char` - (deprecated) always 0

@param `actionList` - the list of actions for this keyrelease






Called when the key is released.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L3041-L3053" target="_blank">source</a>]


### Callins.Load
---
```lua
function Callins.Load(zipReader: table)
```





Called after `GamePreload` and before `GameStart`. See Lua_SaveLoad.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L588-L592" target="_blank">source</a>]


### Callins.LoadCode
---
```lua
function Callins.LoadCode()
```





Called when the game is (re)loaded.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L508-L511" target="_blank">source</a>]


### Callins.MapDrawCmd
---
```lua
function Callins.MapDrawCmd(
  playerID: number,
  type: string,
  posX: number,
  posY: number,
  posZ: number,
  data4: (string|number),
  pos2Y: number?,
  pos2Z: number?
)
```
@param `type` - "point" | "line" | "erase"

@param `data4` - point: label, erase: radius, line: pos2X

@param `pos2Y` - when type is line

@param `pos2Z` - when type is line






[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L3502-L3513" target="_blank">source</a>]


### Callins.MouseMove
---
```lua
function Callins.MouseMove(
  x: number,
  y: number,
  dx: number,
  dy: number,
  button: number
)
```
@param `x` - final x position

@param `y` - final y position

@param `dx` - distance travelled in x

@param `dy` - distance travelled in y






Called when the mouse is moved.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L3222-L3231" target="_blank">source</a>]


### Callins.MousePress
---
```lua
function Callins.MousePress(
  x: number,
  y: number,
  button: number
) -> becomeMouseOwner boolean
```





Called when a mouse button is pressed.

The button parameter supports up to 7 buttons. Must return true for `MouseRelease` and other functions to be called.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L3160-L3169" target="_blank">source</a>]


### Callins.MouseRelease
---
```lua
function Callins.MouseRelease(
  x: number,
  y: number,
  button: number
) -> becomeMouseOwner boolean
```





Called when a mouse button is released.

Please note that in order to have Spring call `Spring.MouseRelease`, you need to have a `Spring.MousePress` call-in in the same addon that returns true.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L3193-L3203" target="_blank">source</a>]


### Callins.MouseWheel
---
```lua
function Callins.MouseWheel(
  up: boolean,
  value: number
)
```
@param `up` - the direction

@param `value` - the amount travelled






Called when the mouse wheel is moved.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L3257-L3263" target="_blank">source</a>]


### Callins.PlayerAdded
---
```lua
function Callins.PlayerAdded(playerID: number)
```





Called whenever a new player joins the game.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L934-L938" target="_blank">source</a>]


### Callins.PlayerChanged
---
```lua
function Callins.PlayerChanged(playerID: number)
```





Called whenever a player's status changes e.g. becoming a spectator.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L910-L914" target="_blank">source</a>]


### Callins.PlayerRemoved
---
```lua
function Callins.PlayerRemoved(
  playerID: number,
  reason: string
)
```





Called whenever a player is removed from the game.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L958-L963" target="_blank">source</a>]


### Callins.ProjectileCreated
---
```lua
function Callins.ProjectileCreated(
  proID: number,
  proOwnerID: number,
  weaponDefID: number
)
```





Called when the projectile is created.

Note that weaponDefID is missing if the projectile is spawned as part of a burst, but `Spring.GetProjectileDefID` and `Spring.GetProjectileName` still work in callin scope using proID.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L2065-L2075" target="_blank">source</a>]


### Callins.ProjectileDestroyed
---
```lua
function Callins.ProjectileDestroyed(
  proID: number,
  ownerID: number,
  proWeaponDefID: number
)
```





Called when the projectile is destroyed.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L2115-L2121" target="_blank">source</a>]


### Callins.RecvLuaMsg
---
```lua
function Callins.RecvLuaMsg(
  msg: string,
  playerID: number
)
```





Receives messages from unsynced sent via `Spring.SendLuaRulesMsg` or `Spring.SendLuaUIMsg`.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L2258-L2263" target="_blank">source</a>]


### Callins.RecvSkirmishAIMessage
---
```lua
function Callins.RecvSkirmishAIMessage(
  aiTeam: integer,
  dataStr: string
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L3624-L3628" target="_blank">source</a>]


### Callins.RenderUnitDestroyed
---
```lua
function Callins.RenderUnitDestroyed(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```





Called just before a unit is invalid, after it finishes its death animation.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L1919-L1926" target="_blank">source</a>]


### Callins.Save
---
```lua
function Callins.Save(zip: table)
```
@param `zip` - a userdatum representing the savegame zip file. See Lua_SaveLoad.






Called when a chat command '/save' or '/savegame' is received.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L2339-L2343" target="_blank">source</a>]


### Callins.Shutdown
---
```lua
function Callins.Shutdown() ->  nil
```





Called when the addon or the game is shutdown.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L534-L538" target="_blank">source</a>]


### Callins.StockpileChanged
---
```lua
function Callins.StockpileChanged(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  weaponNum: integer,
  oldCount: integer,
  newCount: integer
)
```





Called when a units stockpile of weapons increases or decreases.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L2223-L2233" target="_blank">source</a>]


### Callins.SunChanged
---
```lua
function Callins.SunChanged()
```





[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L2471-L2473" target="_blank">source</a>]


### Callins.TeamChanged
---
```lua
function Callins.TeamChanged(teamID: number)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L887-L890" target="_blank">source</a>]


### Callins.TeamDied
---
```lua
function Callins.TeamDied(teamID: number)
```





Called when a team dies (see `Spring.KillTeam`).

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L863-L867" target="_blank">source</a>]


### Callins.TextEditing
---
```lua
function Callins.TextEditing(
  utf8: string,
  start: number,
  length: number
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L3130-L3137" target="_blank">source</a>]


### Callins.TextInput
---
```lua
function Callins.TextInput(utf8char: string)
```





Called whenever a key press results in text input.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L3102-L3107" target="_blank">source</a>]


### Callins.UnitArrivedAtGoal
---
```lua
function Callins.UnitArrivedAtGoal(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L1902-L1909" target="_blank">source</a>]


### Callins.UnitCloaked
---
```lua
function Callins.UnitCloaked(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```





Called when a unit cloaks.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L1746-L1753" target="_blank">source</a>]


### Callins.UnitCmdDone
---
```lua
function Callins.UnitCmdDone(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  cmdID: number,
  cmdParams: table,
  options: CommandOptions,
  cmdTag: number
)
```





Called when a unit completes a command.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L1279-L1289" target="_blank">source</a>]


### Callins.UnitCommand
---
```lua
function Callins.UnitCommand(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  cmdID: number,
  cmdParams: table,
  options: CommandOptions,
  cmdTag: number
)
```





Called after when a unit accepts a command, after `AllowCommand` returns true.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L1245-L1255" target="_blank">source</a>]


### Callins.UnitConstructionDecayed
---
```lua
function Callins.UnitConstructionDecayed(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  timeSinceLastBuild: number,
  iterationPeriod: number,
  part: number
)
```





Called when a unit being built starts decaying.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L1102-L1111" target="_blank">source</a>]


### Callins.UnitCreated
---
```lua
function Callins.UnitCreated(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  builderID: number?
)
```





Called at the moment the unit is created.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L1008-L1015" target="_blank">source</a>]


### Callins.UnitDamaged
---
```lua
function Callins.UnitDamaged(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  damage: number,
  paralyzer: number,
  weaponDefID: number,
  projectileID: number,
  attackerID: number,
  attackerDefID: number,
  attackerTeam: number
)
```





Called when a unit is damaged (after UnitPreDamaged).

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L1309-L1322" target="_blank">source</a>]


### Callins.UnitDecloaked
---
```lua
function Callins.UnitDecloaked(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```





Called when a unit decloaks.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L1761-L1768" target="_blank">source</a>]


### Callins.UnitDestroyed
---
```lua
function Callins.UnitDestroyed(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  attackerID: number,
  attackerDefID: number,
  attackerTeam: number,
  weaponDefID: number
)
```





Called when a unit is destroyed.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L1135-L1145" target="_blank">source</a>]


### Callins.UnitEnteredAir
---
```lua
function Callins.UnitEnteredAir(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L1731-L1738" target="_blank">source</a>]


### Callins.UnitEnteredLos
---
```lua
function Callins.UnitEnteredLos(
  unitID: integer,
  unitTeam: integer,
  allyTeam: integer,
  unitDefID: integer
)
```
@param `allyTeam` - who's LOS the unit entered.






Called when a unit enters LOS of an allyteam.

Its called after the unit is in LOS, so you can query that unit.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L1526-L1536" target="_blank">source</a>]


### Callins.UnitEnteredRadar
---
```lua
function Callins.UnitEnteredRadar(
  unitID: integer,
  unitTeam: integer,
  allyTeam: integer,
  unitDefID: integer
)
```





Called when a unit enters radar of an allyteam.

Also called when a unit enters LOS without any radar coverage.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L1507-L1517" target="_blank">source</a>]


### Callins.UnitEnteredUnderwater
---
```lua
function Callins.UnitEnteredUnderwater(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L1659-L1665" target="_blank">source</a>]


### Callins.UnitEnteredWater
---
```lua
function Callins.UnitEnteredWater(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L1673-L1679" target="_blank">source</a>]


### Callins.UnitExperience
---
```lua
function Callins.UnitExperience(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  experience: number,
  oldExperience: number
)
```





Called when a unit gains experience greater or equal to the minimum limit set by calling `Spring.SetExperienceGrade`.

Should be called more reliably with small values of experience grade.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L1388-L1400" target="_blank">source</a>]


### Callins.UnitFeatureCollision
---
```lua
function Callins.UnitFeatureCollision(
  colliderID: number,
  collideeID: number
)
```





Called when a unit collides with a feature.

The unit must be registered with `Script.SetWatchUnit` and the feature registered with `Script.SetWatchFeature`.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L1827-L1835" target="_blank">source</a>]


### Callins.UnitFinished
---
```lua
function Callins.UnitFinished(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```





Called at the moment the unit is completed.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L1039-L1045" target="_blank">source</a>]


### Callins.UnitFromFactory
---
```lua
function Callins.UnitFromFactory(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  factID: number,
  factDefID: number,
  userOrders: boolean
)
```





Called when a factory finishes construction of a unit.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L1053-L1062" target="_blank">source</a>]


### Callins.UnitGiven
---
```lua
function Callins.UnitGiven(
  unitID: integer,
  unitDefID: integer,
  newTeam: number,
  oldTeam: number
)
```





Called when a unit is transferred between teams. This is called after `UnitTaken` and in that moment unit is assigned to the newTeam.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L1202-L1209" target="_blank">source</a>]


### Callins.UnitHarvestStorageFull
---
```lua
function Callins.UnitHarvestStorageFull(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```





Called when a unit's harvestStorage is full (according to its unitDef's entry).

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L1424-L1430" target="_blank">source</a>]


### Callins.UnitIdle
---
```lua
function Callins.UnitIdle(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```





Called when a unit is idle (empty command queue).

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L1231-L1237" target="_blank">source</a>]


### Callins.UnitLeftAir
---
```lua
function Callins.UnitLeftAir(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L1687-L1694" target="_blank">source</a>]


### Callins.UnitLeftLos
---
```lua
function Callins.UnitLeftLos(
  unitID: integer,
  unitTeam: integer,
  allyTeam: integer,
  unitDefID: integer
)
```





Called when a unit leaves LOS of an allyteam.

For widgets, this one is called just before the unit leaves los, so you can still get the position of a unit that left los.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L1565-L1575" target="_blank">source</a>]


### Callins.UnitLeftRadar
---
```lua
function Callins.UnitLeftRadar(
  unitID: integer,
  unitTeam: integer,
  allyTeam: integer,
  unitDefID: integer
)
```





Called when a unit leaves radar of an allyteam.

Also called when a unit leaves LOS without any radar coverage.
For widgets, this is called just after a unit leaves radar coverage, so
widgets cannot get the position of units that left their radar.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L1544-L1556" target="_blank">source</a>]


### Callins.UnitLeftUnderwater
---
```lua
function Callins.UnitLeftUnderwater(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L1702-L1709" target="_blank">source</a>]


### Callins.UnitLeftWater
---
```lua
function Callins.UnitLeftWater(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L1716-L1723" target="_blank">source</a>]


### Callins.UnitLoaded
---
```lua
function Callins.UnitLoaded(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  transportID: integer,
  transportTeam: integer
)
```





Called when a unit is loaded by a transport.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L1589-L1597" target="_blank">source</a>]


### Callins.UnitMoveFailed
---
```lua
function Callins.UnitMoveFailed(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```





[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L1880-L1887" target="_blank">source</a>]


### Callins.UnitReverseBuilt
---
```lua
function Callins.UnitReverseBuilt(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```





Called when a living unit becomes a nanoframe again.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L1087-L1093" target="_blank">source</a>]


### Callins.UnitSeismicPing
---
```lua
function Callins.UnitSeismicPing(
  x: number,
  y: number,
  z: number,
  strength: number,
  allyTeam: integer,
  unitID: integer,
  unitDefID: integer
)
```





Called when a unit emits a seismic ping.

See `seismicSignature`.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L1440-L1453" target="_blank">source</a>]


### Callins.UnitStunned
---
```lua
function Callins.UnitStunned(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  stunned: boolean
)
```





Called when a unit changes its stun status.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L1357-L1364" target="_blank">source</a>]


### Callins.UnitTaken
---
```lua
function Callins.UnitTaken(
  unitID: integer,
  unitDefID: integer,
  oldTeam: number,
  newTeam: number
)
```





Called when a unit is transferred between teams. This is called before `UnitGiven` and in that moment unit is still assigned to the oldTeam.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L1173-L1180" target="_blank">source</a>]


### Callins.UnitUnitCollision
---
```lua
function Callins.UnitUnitCollision(
  colliderID: number,
  collideeID: number
)
```





Called when two units collide.

Both units must be registered with `Script.SetWatchUnit`.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L1776-L1783" target="_blank">source</a>]


### Callins.UnitUnloaded
---
```lua
function Callins.UnitUnloaded(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  transportID: integer,
  transportTeam: integer
)
```





Called when a unit is unloaded by a transport.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L1621-L1629" target="_blank">source</a>]


### Callins.UnsyncedHeightMapUpdate
---
```lua
function Callins.UnsyncedHeightMapUpdate()
 -> x1 number
 -> z1 number
 -> x2 number
 -> z2 number

```





Called when the unsynced copy of the height-map is altered.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L2365-L2372" target="_blank">source</a>]


### Callins.Update
---
```lua
function Callins.Update(dt: number)
```
@param `dt` - the time since the last update.






Called for every draw frame (including when the game is paused) and at least once per sim frame except when catching up.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L2392-L2396" target="_blank">source</a>]


### Callins.ViewResize
---
```lua
function Callins.ViewResize(
  viewSizeX: number,
  viewSizeY: number
)
```





Called whenever the window is resized.

[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L2411-L2416" target="_blank">source</a>]


### Callins.WorldTooltip
---
```lua
function Callins.WorldTooltip(
  ttType: string,
  data1: number,
  data2: number?,
  data3: number?
) -> newTooltip string
```
@param `ttType` - "unit" | "feature" | "ground" | "selection"

@param `data1` - unitID | featureID | posX

@param `data2` - posY

@param `data3` - posZ






[<a href="https://github.com/beyond-all-reason/spring/blob/95d591b7c91f26313b58187692bd4485b39cb050/rts/Lua/LuaHandle.cpp#L3450-L3457" target="_blank">source</a>]




