--- Functions called by the Engine
-- @module Callins
--
-- This page is future looking to unified widget/gadget (aka "addon") handler, which may yet be some way off, c.f. the changelog.
--
-- Related Sourcecode: [LuaHandle.cpp](https://github.com/beyond-all-reason/spring/blob/BAR105/rts/Lua/LuaHandle.cpp)
--
-- For now, to use these addons in a widget, prepend widget: and, for a gadget, prepend gadget:. For example,
--
--    function widget:UnitCreated(unitID, unitDefID, unitTeam, builderID)
--        ...  
--    end
--
-- Some functions may differ between (synced) gadget and widgets; those are in the [Synced - Unsynced Shared](#Synced___Unsynced_Shared) section. Essentially the reason is that all information should be available to synced (game logic controlling) gadgets, but restricted to unsynced gadget/widget (e.g. information about an enemy unit only detected via radar and not yet in LOS). In such cases the full (synced) param list is documented.
--
-- Attention: some callins will only work on the unsynced portion of the gadget. Due to the type-unsafe nature of lua parsing, those callins not firing up might be hard to trace. This document will be continuously updated to properly alert about those situations.

--- Common
-- @section common

--- Called when the addon is (re)loaded.
-- @function Initialize

--- Called when the addon or the game is shutdown.
-- @function Shutdown

--- Unsynced Only
-- @section unsynced

--- Used to set the default command when a unit is selected. First parameter is the type of the object pointed at (either "unit or "feature") and the second is its unitID or featureID respectively.
-- @function DefaultCommand
-- @string type "unit" | "feature"
-- @int id unitID | featureID

--- Called when a command is issued.
-- @function CommandNotify
-- @int cmdID
-- @tparam table cmdParams
-- @tparam cmdOpts cmdOpts
-- @treturn boolean Returning true deletes the command and does not send it through the network.

--- Called when the command descriptions changed, e.g. when selecting or deselecting a unit.
-- @function CommandsChanged
-- @return ?

--- @function WorldTooltip
-- @string ttType "unit" | "feature" | "ground" | "selection"
-- @number data1 unitID | featureID | posX
-- @number[opt] data2 posY
-- @number[opt] data3 posZ
-- @treturn string newTooltip

--- Called when the unsynced copy of the height-map is altered.
-- @function UnsyncedHeightMapUpdate

--- Called every 60 frames, calculating delta between GameFrame and GameProgress.
-- Can give an ETA about catching up with simulation for mid-game join players.
-- @function GameProgress
-- @int serverFrameNum

--- @function GameSetup
-- @string state
-- @boolean ready
-- @tparam table playerStates
-- @treturn bool success
-- @treturn bool newReady

--- @function SunChanged

--- Called when text is entered into the console (e.g. Spring.Echo).
-- @function AddConsoleLine
-- @string msg
-- @int priority

--- @function RecvSkirmishAIMessage
-- @int aiTeam
-- @string dataStr

--- Receives data sent via SendToUnsynced callout.
-- @function RecvFromSynced

---  Called when a chat command '/save' or '/savegame' is received.
-- @function Save
-- @tparam table zip a userdatum representing the savegame zip file. See Lua_SaveLoad.

--- @function LoadProgress(message, replaceLastLine)
--- 
---     Only available to LuaIntro.
--- 

--- @function GroupChanged(groupID)
--- 
---     Called when a unit is added to or removed from a control group. Currently implemented for widgets only.
--- 

--- @function ConfigureLayout(???)
--- @return ???
--- 
---     ???
--- 
--- 
--- Input

--- @function IsAbove(x, y)
--- @return boolean isAbove
--- 
---     Called every Update. Must return true for Mouse* events and GetToolTip to be called.
--- 

--- @function GetTooltip(x, y)
--- @return string tooltip
--- 
---     Called when IsAbove returns true.
--- 

--- @function KeyPress(key, mods, isRepeat)
--- @return boolean becomeOwner
--- 
---     Called repeatedly when a key is pressed down. If you want an action to occur only once check for isRepeat == false. The mods parameter is a table, with keys "alt", "ctrl", "meta" and "shift" each having a boolean value. Return true if you don't want other callins or the engine to also receive this keypress. A list of key codes can be seen at the SDL wiki.
--- 

--- @function KeyRelease(key)
--- @return bool
--- 
---     Called when the key is released.
--- 

--- @function TextInput(utf8char)
--- 
---     Called whenever a key press results in text input. Introduced in 97.0.
--- 

--- @function JoystickEvent(???)
--- 
---     ???
--- 

--- @function MousePress(x, y, button)
--- @return boolean becomeMouseOwner
--- 
---     Called when a mouse button is pressed. The button parameter supports up to 7 buttons. Must return true for MouseRelease and other functions to be called.
--- 

--- @function MouseRelease(x, y, button)
--- @return boolean becomeMouseOwner
--- 
---     Called when a mouse button is released. Please note that in order to have Spring call MouseRelease, you need to have a MousePress call-in in the same addon that returns true.
--- 

--- @function MouseWheel(up, value)
--- 
---     Called when the mouse wheel is moved. The parameters indicate the direction and amount of travel.
--- 

--- @function MouseMove(x, y, dx, dy, button)
--- 
---     Called when the mouse is moved. The dx and dy parameters indicate the distance travelled, whereas the first two indicate the final position.
--- 
--- 
--- Players

--- @function PlayerChanged(playerID)
--- 
---     Called whenever a player's status changes e.g. becoming a spectator.
--- 

--- @function PlayerAdded(playerID)
--- 
---     Called whenever a new player joins the game.
--- 

--- @function PlayerRemoved(playerID, reason)
--- 
---     Called whenever a player is removed from the game.
--- 
--- 
--- Downloads

--- @function DownloadStarted(id)
--- 
---     Called when a Pr-downloader download is started via VFS.DownloadArchive.
--- 

--- @function DownloadFinished(id)
--- 
---     Called when a Pr-downloader download finishes successfully.
--- 

--- @function DownloadFailed(id, errorID)
--- 
---     Called when a Pr-downloader download fails to complete.
--- 

--- @function DownloadProgress(id, downloaded, total)
--- 
---     Called incrementally during a Pr-downloader download.
--- 
--- 
--- Drawing

--- @function ViewResize(viewSizeX, viewSizeY)
--- 
---     Called whenever the window is resized.
--- 

--- @function Update(dt)
--- 
---     Called for every draw frame (including when the game is paused) and at least once per sim frame except when catching up. The parameter is the time since the last update.
--- 
--- 
--- Draw* Functions
--- 
--- Inside the Draw* functions, you can use the Lua OpenGL Api to draw graphics.
--- Avoid doing heavy calculations inside these callins; ideally, do the calculations elsewhere and use Draw callins only for drawing.

--- @function DrawGenesis()
--- 
---     Doesn't render to screen! Use this callin to update textures, shaders, etc. Also available to LuaMenu.
--- 

--- @function DrawWorldPreParticles()
--- 
---     ??? New in version 104.0
--- 

--- @function DrawWorldPreUnit()
--- 
---     Spring draws units, features, some water types, cloaked units, and the sun.
--- 

--- @function DrawWorld()
--- 
---     Spring draws command queues, 'map stuff', and map marks.
--- 

--- @function DrawWorldShadow()
--- 
---     ???
--- 

--- @function DrawWorldReflection()
--- 
---     ???
--- 

--- @function DrawWorldRefraction()
--- 
---     ???
--- 

--- @function DrawGroundPreForward()
--- 
---     Runs at the start of the forward pass when a custom map shader has been assigned via Spring.SetMapShader (convenient for setting uniforms).
--- 

--- @function DrawGroundPreDeferred()
--- 
---     Runs at the start of the deferred pass when a custom map shader has been assigned via Spring.SetMapShader (convenient for setting uniforms).
--- 

--- @function DrawGroundPostDeferred()
--- 
---     This runs at the end of its respective deferred pass and allows proper frame compositing (with ground flashes/decals/foliage/etc, which are drawn between it and DrawWorldPreUnit) via gl.CopyToTexture.
--- 

--- @function DrawUnitsPostDeferred()
--- 
---     Runs at the end of the unit deferred pass to inform Lua code it should make use of the $model_gbuffer_* textures before another pass overwrites them (and to allow proper blending with e.g. cloaked objects which are drawn between these events and DrawWorld via gl.CopyToTexture). N.B. The *PostDeferred events are only sent (and only have a real purpose) if forward drawing is disabled.
--- 

--- @function DrawFeaturesPostDeferred()
--- 
---     Runs at the end of the feature deferred pass to inform Lua code it should make use of the $model_gbuffer_* textures before another pass overwrites them (and to allow proper blending with e.g. cloaked objects which are drawn between these events and DrawWorld via gl.CopyToTexture). N.B. The *PostDeferred events are only sent (and only have a real purpose) if forward drawing is disabled.
--- 

--- @function DrawScreen(vsx, vsy)
--- 
---     ??? Also available to LuaMenu.
--- 

--- @function DrawScreenEffects(vsx, vsy)
--- 
---     Where vsx, vsy are screen coordinates.
--- 

--- @function DrawScreenPost(vsx, vsy)
--- 
---     New in version 104.0 Similar to DrawScreenEffects, this can be used to alter the contents of a frame after it has been completely rendered (i.e. World, MiniMap, Menu, UI).
--- 

--- @function DrawLoadScreen()
--- 
---     New in version 95.0 Only available to LuaIntro, draws custom load screens.
--- 

--- @function DrawInMinimap(sx, sy)
--- 
---     Where sx, sy are values relative to the minimap's position and scale.
--- 

--- @function DrawInMinimapBackground(sx, sy)
--- 
---     Where sx, sy are values relative to the minimap's position and scale.
--- 
--- 
--- Custom Object Rendering
--- 
--- For the following calls drawMode can be one of the following, notDrawing = 0, normalDraw = 1, shadowDraw = 2, reflectionDraw = 3, refractionDraw = 4, and finally gameDeferredDraw = 5 which was added in 102.0.

--- @function DrawUnit(unitID, drawMode)
--- @return boolean suppressEngineDraw
--- 
---     For custom rendering of units, enabled here.
--- 

--- @function DrawFeature(unitID, drawMode)
--- @return boolean suppressEngineDraw
--- 
---     For custom rendering of features, enabled here.
--- 

--- @function DrawShield(unitID, weaponID, drawMode)
--- @return boolean suppressEngineDraw
--- 
---     For custom rendering of shields.
--- 

--- @function DrawProjectile(projectileID, drawMode)
--- @return boolean suppressEngineDraw
--- 
---     For custom rendering of weapon (& other) projectiles, enabled here.
--- 
--- 

--- Unsynced Menu Only
-- @section unsynced_menu

--- @function AllowDraw()
--- @return boolean allowDraw
--- 
---     Enables Draw{Genesis,Screen,ScreenPost} callins if true is returned, otherwise they are called once every 30 seconds. Only active when a game isn't running.
--- 

--- @function ActivateMenu()
--- 
---     Called whenever LuaMenu is on with no game loaded.
--- 

--- @function ActivateGame()
--- 
---     Called whenever LuaMenu is on with a game loaded.

--- Synced - Unsynced Shared
-- @section unsynced_synced

--- @function GotChatMsg(msg, player)
--- 
---     Called when a player issues a UI command e.g. types /foo or /luarules foo.
--- 
--- 
--- Game

--- @function GameID(gameID)
--- 
---     Called once to deliver the gameID. As of 101.0+ the string is encoded in hex.
--- 

--- @function GamePaused()
--- 
---     Called when the game is paused.
--- 

--- @function GameOver(winningAllyTeams)
--- 
---     The parameter is a table list of winning allyTeams, if empty the game result was undecided (like when dropping from an host).
--- 

--- @function GameFrame(frame)
--- 
---     Called for every game simulation frame (30 per second). Starts at frame 0 in 101.0+ and 1 in previous versions.
--- 

--- @function GamePreload()
--- 
---     Called before the 0 gameframe. From 104.0 onwards, will not be called when a saved game is loaded.
--- 

--- @function GameStart()
--- 
---     Called upon the start of the game. From 104.0 onwards, will not be called when a saved game is loaded.
--- 
--- 
--- Teams

--- @function TeamChanged(teamID)
--- 
---     ???
--- 

--- @function TeamDied(teamID)
--- 
---     Called when a team dies (see Spring.KillTeam).
--- 
--- 
--- Units

--- @function UnitCreated(unitID, unitDefID, unitTeam, builderID)
--- 
---     Called at the moment the unit is created.
--- 

--- @function UnitFinished(unitID, unitDefID, unitTeam)
--- 
---     Called at the moment the unit is completed.
--- 

--- @function UnitFromFactory(unitID, unitDefID, unitTeam, factID, factDefID, userOrders)
--- 
---     Called when a factory finishes construction of a unit.
--- 

--- @function UnitReverseBuilt(unitID, unitDefID, unitTeam)
--- 
---     Called when a living unit becomes a nanoframe again.
--- 

--- @function UnitGiven(unitID, unitDefID, newTeam, oldTeam)
--- 
---     Called when a unit is transferred between teams. This is called after UnitTaken and in that moment unit is assigned to the newTeam.
--- 

--- @function UnitTaken(unitID, unitDefID, oldTeam, newTeam)
--- 
---     Called when a unit is transferred between teams. This is called before UnitGiven and in that moment unit is still assigned to the oldTeam.
--- 

--- @function UnitDamaged(unitID, unitDefID, unitTeam, damage, paralyzer, weaponDefID, projectileID, attackerID, attackerDefID, attackerTeam)
--- 
---     Called when a unit is damaged (after UnitPreDamaged).
--- 

--- @function UnitDestroyed(unitID, unitDefID, unitTeam, attackerID, attackerDefID, attackerTeam)
--- 
---     Called when a unit is destroyed.
--- 

--- @function RenderUnitDestroyed(unitID, unitDefID, unitTeam)
--- 
---     Called just before a unit is invalid, after it finishes its death animation. New in version 101.0
--- 

--- @function UnitStunned(unitID, unitDefID, unitTeam, stunned)
--- 
---     Called when a unit changes its stun status. New in version 99.0
--- 

--- @function UnitUnitCollision(colliderID, collideeID)
--- 
---     Called when two units collide. Both units must be registered with Script.SetWatchUnit.
--- 

--- @function UnitFeatureCollision(colliderID, collideeID)
--- 
---     Called when a unit collides with a feature. The unit must be registered with Script.SetWatchUnit and the feature registered with Script.SetWatchFeature.
--- 

--- @function UnitHarvestStorageFull(unitID, unitDefID, unitTeam)
--- 
---     Called when a unit's harvestStorage is full (according to its unitDef's entry).
--- 

--- @function UnitCommand(unitID, unitDefID, unitTeam, cmdID, cmdParams, cmdOpts, cmdTag)
--- 
---     Called after when a unit accepts a command, after AllowCommand returns true.
--- 

--- @function UnitCmdDone(unitID, unitDefID, unitTeam, cmdID, cmdParams, cmdOpts, cmdTag)
--- 
---     Called when a unit completes a command.
--- 

--- @function UnitLoaded(unitID, unitDefID, unitTeam, transportID, transportTeam)
--- 
---     Called when a unit is loaded by a transport.
--- 

--- @function UnitUnloaded(unitID, unitDefID, unitTeam, transportID, transportTeam)
--- 
---     Called when a unit is unloaded by a transport.
--- 

--- @function UnitExperience(unitID, unitDefID, unitTeam, experience, oldExperience)
--- 
---     Called when a unit gains experience greater or equal to the minimum limit set by calling Spring.SetExperienceGrade. Should be called more reliably with small values of experience grade in 104.0+.
--- 

--- @function UnitIdle(unitID, unitDefID, unitTeam)
--- 
---     Called when a unit is idle (empty command queue).
--- 

--- @function UnitCloaked(unitID, unitDefID, unitTeam)
--- 
---     Called when a unit cloaks.
--- 

--- @function UnitDecloaked(unitID, unitDefID, unitTeam)
--- 
---     Called when a unit decloaks.
--- 

--- @function UnitMoved(???)
--- 
---     ??? Not implemented in base handler
--- 

--- @function UnitMoveFailed(???)
--- 
---     ??? Not implemented in base handler
--- 

--- @function StockpileChanged(unitID, unitDefID, unitTeam, weaponNum, oldCount, newCount)
--- 
---     Called when a units stockpile of weapons increases or decreases. See stockpile.
--- 

--- @function UnitEnteredLos(unitID, unitTeam, allyTeam, unitDefID)
--- 
---     Called when a unit enters LOS of an allyteam. Its called after the unit is in LOS, so you can query that unit. The allyTeam is who's LOS the unit entered.
--- 

--- @function UnitLeftLos(unitID, unitTeam, allyTeam, unitDefID)
--- 
---     Called when a unit leaves LOS of an allyteam. For widgets, this one is called just before the unit leaves los, so you can still get the position of a unit that left los.
--- 

--- @function UnitEnteredRadar(unitID, unitTeam, allyTeam, unitDefID)
--- 
---     Called when a unit enters radar of an allyteam. Also called when a unit enters LOS without any radar coverage.
--- 

--- @function UnitLeftRadar(unitID, unitTeam, allyTeam, unitDefID)
--- 
---     Called when a unit leaves radar of an allyteam. Also called when a unit leaves LOS without any radar coverage. For widgets, this is called just after a unit leaves radar coverage, so widgets cannot get the position of units that left their radar.
--- 

--- @function UnitEnteredAir(???)
--- 
---     ??? Not implemented by base handler
--- 

--- @function UnitLeftAir(???)
--- 
---     ??? Not implemented by base handler
--- 

--- @function UnitEnteredWater(???)
--- 
---     ??? Not implemented by base handler
--- 

--- @function UnitLeftWater(???)
--- 
---     ??? Not implemented by base handler
--- 

--- @function UnitSeismicPing(x, y, z, strength, allyTeam, unitID, unitDefID)
--- 
---     Called when a unit emits a seismic ping. See seismicSignature.
--- 
--- 
--- Features

--- @function FeatureCreated(featureID, allyTeamID)
--- 
---     Called when a feature is created.
--- 

--- @function FeatureDamaged(featureID, featureDefID, featureTeam, damage, weaponDefID, projectileID, attackerID, attackerDefID, attackerTeam)
--- 
---     Called when a feature is damaged.
--- 

--- @function FeatureDestroyed(featureID, allyTeamID)
--- 
---     Called when a feature is destroyed.
--- 

--- @function FeatureMoved(???)
--- 
---     ???
--- 
--- 
--- Projectiles
--- 
--- The following Callins are only called for weaponDefIDs registered via Script.SetWatchWeapon.

--- @function ProjectileCreated(proID, proOwnerID, weaponDefID)
--- 
---     Called when the projectile is created. Note that weaponDefID is missing if the projectile is spawned as part of a burst, but Spring.GetProjectileDefID and Spring.GetProjectileName still work in callin scope using proID.
--- 

--- @function ProjectileDestroyed(proID)
--- 
---     Called when the projectile is destroyed.
--- 
--- 
--- Synced Only

--- @function CommandFallback(unitID, unitDefID, unitTeam, cmdID, cmdParams, cmdOptions, cmdTag)
--- @return boolean used, boolean finished
--- 
---     Called when the unit reaches an unknown command in its queue (i.e. one not handled by the engine). If no addon returns used as true the command is dropped, if an addon returns true, true the command is removed because it's done, with true, false it's kept in the queue and CommandFallback gets called again on the next slowupdate.
--- 

--- @function AllowCommand(unitID, unitDefID, unitTeam, cmdID, cmdParams, cmdOptions, cmdTag, synced)
--- @return boolean allow
--- 
---     Called when the command is given, before the unit's queue is altered. The return value is whether it should be let into the queue. The queue remains untouched when a command is blocked, whether it would be queued or replace the queue.
--- 

--- @function AllowUnitCreation(unitDefID, builderID, builderTeam, x, y, z, facing)
--- @return boolean allow
--- 
---     Called just before unit is created, the boolean return value determines whether or not the creation is permitted.
--- 

--- @function AllowUnitTransfer(unitID, unitDefID, oldTeam, newTeam, capture)
--- @return boolean allow
--- 
---     Called just before a unit is transferred to a different team, the boolean return value determines whether or not the transfer is permitted.
--- 

--- @function AllowUnitBuildStep(builderID, builderTeam, unitID, unitDefID, part)
--- @return boolean allow
--- 
---     Called just before a unit progresses its build percentage, the boolean return value determines whether or not the build makes progress.
--- 

--- @function AllowFeatureCreation(featureDefID, teamID, x, y, z)
--- @return boolean allow
--- 
---     Called just before feature is created, the boolean return value determines whether or not the creation is permitted.
--- 

--- @function AllowFeatureBuildStep(builderID, builderTeam, featureID, featureDefID, part)
--- @return boolean allow
--- 
---     Called just before a feature changes its build percentage, the boolean return value determines whether or not the change is permitted. Note that this is also called for resurrecting features, and for refilling features with resources before resurrection. On reclaim the part values are negative, and on refill and ressurect they are positive. Part is the percentage the feature be built or reclaimed per frame. Eg. for a 30 workertime builder, that's a build power of 1 per frame. For a 50 buildtime feature reclaimed by this builder, part will be 100/-50(/1) = -2%, or -0.02 numerically.
--- 

--- @function AllowResourceLevel(teamID, res, level)
--- @return boolean allow
--- 
---     Called when a team sets the sharing level of a resource, the boolean return value determines whether or not the sharing level is permitted.
--- 

--- @function AllowResourceTransfer(oldTeamID, newTeamID, res, amount)
--- @return boolean allow
--- 
---     Called just before resources are transferred between players, the boolean return value determines whether or not the transfer is permitted.
--- 

--- @function AllowStartPosition(playerID, teamID, readyState, clampedX, clampedY, clampedZ, rawX, rawY, rawZ)
--- @return boolean allow
--- 
---     clamped{X,Y,Z} are the coordinates clamped into start-boxes, raw is where player tried to place their marker. The readyState can be any one of
--- 
---     0 - player picked a position,
---     1 - player clicked ready,
---     2 - player pressed ready OR the game was force-started (player did not click ready, but is now forcibly readied) or
---     3 - the player failed to load.
---     New in version 95.0 the default 'failed to choose' start-position is the north-west point of their startbox, or (0,0,0) if they do not have a startbox.
--- 
--- NB: The order of the parameters changed with the addition of teamID in 104.0. Previouly it was: clampedX, clampedY, clampedZ, playerID, readyState, rawX, rawY, rawZ
--- 

--- @function AllowDirectUnitControl(unitID, unitDefID, unitTeam, playerID)
--- @return boolean allow
--- 
---     Determines if this unit can be controlled directly in FPS view.
--- 

--- @function AllowWeaponTargetCheck(attackerID, attackerWeaponNum, attackerWeaponDefID)
--- @return boolean allowCheck, boolean ignoreCheck
--- 
---     Determines if this weapon can automatically generate targets itself. See also commandFire weaponDef tag. The ignoreCheck return value was added in 99.0 to allow ignoring the callin i.e. running normal engine check for this weapon.
--- 

--- @function AllowWeaponTarget(attackerID, targetID, attackerWeaponNum, attackerWeaponDefID, defPriority)
--- @return boolean allowed, number newPriority
--- 
---     Controls blocking of a specific target from being considered during a weapon's periodic auto-targeting sweep. The second return value is the new priority for this target (if you don't want to change it, return defPriority). Lower priority targets are targeted first.
--- 

--- @function AllowWeaponInterceptTarget(interceptorUnitID, interceptorWeaponID, targetProjectileID)
--- @return boolean allowed
--- 
---     Controls blocking of a specific intercept target from being considered during an interceptor weapon's periodic auto-targeting sweep. Only called for weaponDefIDs registered via Script.SetWatchWeapon.
--- 

--- @function AllowBuilderHoldFire(unitID, unitDefID, action)
--- @return boolean actionAllowed
--- 
---     New in version 98.0 5a82d750 Called when a construction unit wants to "use his nano beams".
---     action is one of following:
--- 
---     -1 Build
---     CMD.REPAIR Repair
---     CMD.RECLAIM Reclaim
---     CMD.RESTORE Restore
---     CMD.RESURRECT Resurrect
---     CMD.CAPTURE Capture
--- 

--- @function Explosion(weaponDefID, px, py, pz, AttackerID, ProjectileID)
--- @return boolean noGfx
--- 
---     Called when an explosion occurs. If it returns true then no graphical effects are drawn by the engine for this explosion.
--- 

--- @function TerraformComplete(unitID, unitDefID, unitTeam, buildUnitID, buildUnitDefID, buildUnitTeam)
--- @return boolean stop
--- 
---     Called when pre-building terrain levelling terraforms are completed (c.f. levelGround). If the return value is true the current build order is terminated.
--- 

--- @function MoveCtrlNotify(unitID, unitDefID, unitTeam, data)
--- @return boolean moveCtrlComplete
--- 
---     Enable both Spring.MoveCtrl.SetCollideStop and Spring.MoveCtrl.SetTrackGround to enable this call-in, data was supposed to indicate the type of notification but currently never has a value other than 1 ("unit hit the ground"). The return value determines whether or not the unit should remain script-controlled (false) or return to engine controlled movement (true).
--- 

--- @function RecvLuaMsg(msg, playerID)
--- 
---     Receives messages from unsynced sent via Spring.SendLuaRulesMsg or Spring.SendLuaUIMsg.
--- 

--- @function Load(zip)
--- 
---     Called after GamePreload and before GameStart. See Lua_SaveLoad.
--- 
--- 
--- Damage Controllers
--- 
--- For the following callins, in addition to being a regular weapon, weaponDefID may be one of the following:
--- 
---     -1 - debris collision, also default of Spring.AddUnitDamage
---     -2 - ground collision
---     -3 - object collision
---     -4 - fire damage
---     -5 - water damage
---     -6 - kill damage
---     -7 - crush damage

--- @function UnitPreDamaged(unitID, unitDefID, unitTeam, damage, paralyzer, weaponDefID, projectileID, attackerID, attackerDefID, attackerTeam)
--- @return number newDamage, number impulseMult
--- 
---     Called before damage is applied to the unit, allows fine control over how much damage and impulse is applied.
--- 

--- @function ShieldPreDamaged(proID, shieldCarrier, boolBounceProjectile, beamEmitterWeaponNum, beamEmitterUnitID, startX, startY, startZ, hitX, hitY, hitZ)
--- @return boolean handleCollision
--- 
---     Called before any engine shield-vs-projectile logic executes. If the return value is true the gadget handles the collision event and the engine does not remove the projectile. If the weapon is a hitscan type (BeamLaser or LightningCanon) then proID is nil and beamEmitterWeaponNum and beamEmitterUnitID are populated instead. The start and hit position arguments are provided from 104.0 onwards.
--- 

--- @function FeaturePreDamaged(featureID, featureDefID, featureTeam, damage, weaponDefID, projectileID, attackerID, attackerDefID, attackerTeam)
--- @return number newDamage, number impulseMult
--- 
---     Called before damage is applied to the feature, allows fine control over how much damage and impulse is applied.

--- @section end

--- Command Options
-- @int coded
-- @bool alt
-- @bool ctrl
-- @bool shift
-- @bool right
-- @bool meta
-- @bool internal
-- @table cmdOpts
