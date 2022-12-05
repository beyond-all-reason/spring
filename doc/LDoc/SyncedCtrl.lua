--- Synced stuff
-- @module SyncedCtrl

--- Game End
-- @section gameend

--- Will declare a team to be dead ( no further orders can be assigned to such teams's units ), Gaia team cannot be killed.
-- @function Spring.KillTeam
-- @number teamID
-- @treturn nil

--- Will declare game over, a list of winning allyteams can be passed, if undecided ( like when dropped from the host ) it should be empty ( no winner ), in the case of a draw with multiple winners, all should be listed.
-- @function Spring.GameOver
-- @number allyTeamID1
-- @number allyTeamID2
-- @number allyTeamIDn
-- @treturn nil

--- Rules Params
-- @section rulesparams
-- Attention: Numeric paramValues in quotes will be converted to number.

--- @table losAccess
-- If one condition is fulfilled all beneath it are too (e.g. if an unit is in LOS it can read params with `inradar=true` even if the param has `inlos=false`)
-- All GameRulesParam are public, TeamRulesParams can just be `private`,`allied` and/or `public`
-- You can read RulesParams from any Lua enviroments! With those losAccess policies you can limit their access.
--
-- @bool[opt] private only readable by the ally (default)
-- @bool[opt] allied readable by ally + ingame allied
-- @bool[opt] inlos readable if the unit is in LOS
-- @bool[opt] inradar readable if the unit is in AirLOS
-- @bool[opt] public readable by all

--- @function Spring.SetUnitRulesParam
-- @number unitID
-- @string paramName
-- @tparam ?number|string paramValue
-- @tparam[opt] losAccess losAccess
-- @treturn nil


--- @function Spring.SetFeatureRulesParam
-- @number featureID
-- @string paramName
-- @tparam ?number|string paramValue
-- @tparam[opt] losAccess losAccess
-- @treturn nil


--- @function Spring.SetTeamRulesParam
-- @number teamID
-- @string paramName
-- @tparam ?number|string paramValue
-- @tparam[opt] losAccess losAccess
-- @treturn nil

--- @function Spring.SetGameRulesParam
-- @string paramName
-- @tparam ?number|string paramValue
-- @tparam[opt] losAccess losAccess
-- @treturn nil

--- Resources
-- @section resources

--- @function Spring.AddTeamResource ( number teamID, string "metal" | "energy", number amount )
-- @treturn nil
-- 
--     Adds metal or energy resources to the specified team.
-- 

--- @function Spring.UseTeamResource ( number teamID, string "metal" | "energy", number amount | { metal = number amount, energy = number amount } )
-- @treturn ?nil|bool hadEnough
-- 
--     Consumes metal and/or energy resources of the specified team.
-- 

--- @function Spring.SetTeamResource ( number teamID, string res, number amount )
-- @treturn nil
-- 
--     Possible values for res are:
--     "m" = metal
--     "e" = energy
--     "ms" = metal storage
--     "es" = energy storage
-- 

--- @function Spring.SetTeamShareLevel ( number teamID, string "metal" | "energy", number amount )
-- @treturn nil
-- 
--     Changes the resource amount for a team beyond which resources aren't stored but transferred to other allied teams if possible.
-- 

--- @function Spring.ShareTeamResource ( number teamID_src, number teamID_recv, string "metal" | "energy", number amount )
-- @treturn nil
-- 
--     Transfers resources between two teams.

--- Teams
-- @section teams

--- @function Spring.SetAlly ( number firstAllyTeamID, number secondAllyTeamID, bool ally )
-- @treturn nil
-- 
--     Changes the value of the (one-sided) alliance between: firstAllyTeamID -> secondAllyTeamID.
-- 

--- @function Spring.AssignPlayerToTeam ( number playerID, number teamID )
-- @treturn nil
-- 
--     Assigns a player to a team.
-- 

--- @function Spring.SetGlobalLos ( number allyTeamID, bool globallos )
-- @treturn nil
-- 
--     Changes access to global line of sight for a team and its allies.

--- Unit Handling
-- @section unithandling

--- @function Spring.CreateUnit ( string "defName" | number unitDefID, number x, number y, number z, string "facing" | number facing, number teamID [, bool build = false [, bool flattenGround = true [, number builderID ]]] )
-- return: number unitID | nil (meaning unit was not created)
-- 
--     Offmap positions are clamped! Use MoveCtrl to move to such positions.
--     Possible values for facing are:
--     "south" | "s" | 0
--     "east" | "e" | 1
--     "north" | "n" | 2
--     "west" | "w" | 3
-- 
--     If build is true, the unit is created in "being built" state with buildProgress = 0
-- 

--- @function Spring.DestroyUnit ( number unitID [, bool selfd = false [, bool reclaimed = false [, number attackerID ]]] )
-- @treturn nil
-- 
--     selfd := Makes the unit act like it self-destructed.
--     reclaimed := Don't show any DeathSequences, don't leave a wreckage. This does not give back the resources to the team!
-- 

--- @function Spring.TransferUnit ( number unitID, number newTeamID [, bool given = true ] )
-- @treturn nil
-- 
--     If given=false, the unit is captured.

--- Unit Control
-- @section unitcontrol

--- @function Spring.SetUnitCosts ( number unitID, { [ buildTime = number amount ], [ metalCost = number amount ], [ energyCost = number amount ] } )
-- @treturn nil
-- 

--- @function Spring.SetUnitTooltip ( number unitID, string "tooltip" )
-- @treturn nil
-- 

--- @function Spring.SetUnitHealth ( number unitID, number health | { [ health = number health ], [ capture = number capture ], [ paralyze = number paralyze ], [ build = number build ] } )
-- @treturn nil
-- 

--- @function Spring.SetUnitMaxHealth ( number unitID, number maxHealth )
-- @treturn nil
-- 

--- @function Spring.AddUnitDamage ( number unitID, number damage [, number paralyze = 0 [, number attackerID = -1 [, number weaponID = -1 [, number impulse_x [, number impulse_y [, number impulse_z ]]]]]] )
-- @treturn nil
-- 
--     The number in the paralyze parameter equals to the paralyzetime in the WeaponDef.
-- 

--- @function Spring.SetUnitStockpile ( number unitID [, number stockpile [, number buildPercent ]] )
-- @treturn nil
-- 

--- @function Spring.SetUnitExperience ( number unitID, number experience, number buildPercent )
-- @treturn nil
-- 
-- 

--- @function Spring.SetUnitCrashing ( number unitID, bool crashing )
-- return: bool success
-- 
-- 

--- @function Spring.SetUnitTarget ( number unitID, number x | nil, number y, number z [, bool dgun = false [, bool userTarget = false [, number weaponNum = -1 ]]] )
-- return: bool success
-- 
-- or

--- @function Spring.SetUnitTarget ( number unitID, number enemyUnitID | nil [, bool dgun = false [, bool userTarget = false [, number weaponNum = -1 ]]] )
-- return: bool success
-- 
--     Defines a unit's target. Nil as 2nd argument drops the unit's current target.
-- 

--- @function Spring.SetUnitMaxRange ( number unitID, number maxRange )
-- @treturn nil
-- 

--- @function Spring.SetUnitMass ( number unitID, number mass )
-- @treturn nil
-- 
-- 

--- @function Spring.SetUnitBlocking ( number unitID, bool isblocking, bool isSolidObjectCollidable, bool isProjectileCollidable, bool isRaySegmentCollidable, bool crushable, bool blockEnemyPushing, bool blockHeightChanges )
-- @treturn nil
-- 
--     Changed parameters.
-- 

--- @function Spring.SetUnitMetalExtraction ( number unitID, number depth [, number range ] )
-- @treturn nil
-- 
--     Parameter "depth" corresponds to metal extraction rate. Range value is similar to "extractsMetal" in unitDefs.
-- 

--- @function Spring.SetUnitBuildSpeed ( number builderID, number buildSpeed [, number repairSpeed [, number reclaimSpeed[, number resurrectSpeed [, number captureSpeed [, number terraformSpeed ]]]]] )
-- @treturn nil
-- 

--- @function Spring.SetUnitNanoPieces ( number builderID, table pieces )
-- @treturn nil
-- 
--     This saves a lot of engine calls, by replacing: function script.QueryNanoPiece() return currentpiece end
--     Use it!
-- 

--- @function Spring.UnitAttach ( number transporterID, number passengerID, number pieceNum )
-- @treturn nil
-- 
-- 

--- @function Spring.UnitDetach ( number passengerID )
-- @treturn nil
-- 
-- 

--- @function Spring.UnitDetachFromAir ( number passengerID )
-- @treturn nil
-- 
-- 

--- @function Spring.SetUnitLoadingTransport ( number passengerID, number transportID )
-- @treturn nil
-- 
--     Disables collisions between the two units to allow colvol intersection during the approach.
-- 

--- @function Spring.SetUnitPieceParent ( number unitID, number AlteredPiece, number ParentPiece )
-- @treturn nil
-- 
--     Changes the pieces hierarchy of a unit by attaching a piece to a new parent.
-- 

--- @function Spring.SetUnitPieceMatrix ( number unitID, number pieceNum, table matrix )
-- @treturn nil
-- 
--     Sets the local (i.e. parent-relative) matrix of the given piece if any of the first three elements are non-zero, and also blocks all script animations from modifying it until {0, 0, 0} is passed (matrix should be an array of 16 floats, but is not otherwise sanity-checked).
-- 

--- @function Spring.SetUnitArmored ( number unitID, bool armored [, number armorMultiple ] )
-- @treturn nil
-- 
-- 

--- @function Spring.SetUnitShieldState ( number unitID [, number weaponID = -1 [, bool enabled [, number power ]]] )
-- @treturn nil
-- 

--- @function Spring.SetUnitFlanking ( number unitID, string "mode", number mode )
-- @treturn nil
-- 
-- or

--- @function Spring.SetUnitFlanking ( number unitID, string "moveFactor", number factor )
-- @treturn nil
-- 
-- or

--- @function Spring.SetUnitFlanking ( number unitID, string "minDamage", number minDamage )
-- @treturn nil
-- 
-- or

--- @function Spring.SetUnitFlanking ( number unitID, string "maxDamage", number maxDamage )
-- @treturn nil
-- 
-- or

--- @function Spring.SetUnitFlanking ( number unitID, string "dir", number x, number y, number z )
-- @treturn nil
-- 

--- @table states
-- @number reloadState
-- @number reloadFrame synonym for reloadState!
-- @number reloadTime
-- @number accuracy
-- @number sprayAngle
-- @number range if you change the range of a weapon with dynamic damage make sure you use `SetUnitWeaponDamages` to change dynDamageRange as well.
-- @number projectileSpeed
-- @number burst
-- @number burstRate
-- @number projectiles
-- @number salvoLeft
-- @number nextSalvo
-- @number aimReady (<>0.0f := true)

--- @function Spring.SetUnitWeaponState
-- @number unitID
-- @number weaponNum
-- @tparam states states
-- @treturn nil

--- @function Spring.SetUnitWeaponState
-- @number unitID
-- @number weaponNum
-- @string key
-- @number value
-- @treturn nil

--- @table damages
-- @number paralyzeDamageTime
-- @number impulseFactor
-- @number impulseBoost
-- @number craterMult
-- @number craterBoost
-- @number dynDamageExp
-- @number dynDamageMin
-- @number dynDamageRange
-- @number dynDamageInverted (<>0.0f := true)
-- @number craterAreaOfEffect
-- @number damageAreaOfEffect
-- @number edgeEffectiveness
-- @number explosionSpeed
-- @number armorType

--- @function Spring.SetUnitWeaponDamages
-- @number unitID
-- @tparam ?number|string weaponNum "selfDestruct" | "explode"
-- @tparam damages damages
-- @treturn nil

--- @function Spring.SetUnitWeaponDamages
-- @number unitID
-- @tparam ?number|string weaponNum "selfDestruct" | "explode"
-- @string key
-- @number value
-- @treturn nil

--- @function Spring.SetUnitCollisionVolumeData ( number unitID, number scaleX, number scaleY, number scaleZ, number offsetX, number offsetY, number offsetZ, number vType, number tType, number Axis )
-- @treturn nil
-- 
--  enum COLVOL_TYPES {
--      COLVOL_TYPE_DISABLED = -1,
--      COLVOL_TYPE_ELLIPSOID = 0,
--      COLVOL_TYPE_CYLINDER,
--      COLVOL_TYPE_BOX,
--      COLVOL_TYPE_SPHERE,
--      COLVOL_NUM_TYPES       // number of non-disabled collision volume types
--    };
--    enum COLVOL_TESTS {
--      COLVOL_TEST_DISC = 0,
--      COLVOL_TEST_CONT = 1,
--      COLVOL_NUM_TESTS = 2   // number of tests
--    };
--    enum COLVOL_AXES {
--      COLVOL_AXIS_X   = 0,
--      COLVOL_AXIS_Y   = 1,
--      COLVOL_AXIS_Z   = 2,
--      COLVOL_NUM_AXES = 3    // number of collision volume axes
--    };
--  
-- 

--- @function Spring.SetUnitPieceCollisionVolumeData ( number unitID, number pieceIndex, bool enable, number scaleX, number scaleY, number scaleZ, number offsetX, number offsetY, number offsetZ [, number volumeType [, number primaryAxis ]] )
-- @treturn nil
-- 

--- @function Spring.SetUnitTravel ( number unitID, number travel [, number travelPeriod ] )
-- @treturn nil
-- 

--- @function Spring.SetUnitMoveGoal ( number unitID, number goalX, number goalY, number goalZ [, number goalRadius [, number moveSpeed [, bool moveRaw ]]] )
-- @treturn nil
-- 
--     Used by default commands to get in build-, attackrange etc.
-- 

--- @function Spring.SetUnitLandGoal ( number unitID, number goalX, number goalY, number goalZ [, number goalRadius ] )
-- @treturn nil
-- 
--     Used in conjunction with Spring.UnitAttach et al. to re-implement old airbase & fuel system in Lua.
-- 

--- @function Spring.ClearUnitGoal ( number unitID )
-- @treturn nil
-- 
-- 

--- @function Spring.SetUnitPhysics ( number unitID, number posX, number posY, number posZ, number velX, number velY, number velZ, number rotX, number rotY, number rotZ, number dragX, number dragY, number dragZ )
-- @treturn nil
-- 
-- 

--- @function Spring.SetUnitPosition ( number unitID, number x, number z [, bool alwaysAboveSea ] )
-- @treturn nil
-- 

--- @function Spring.SetUnitDirection ( number unitID, number x, number y, number z )
-- @treturn nil
-- 
-- 

--- @function Spring.SetUnitVelocity ( number unitID, number velX, number velY, number velZ )
-- @treturn nil
-- 

--- @function Spring.SetUnitRotation ( number unitID, number yaw, number pitch, number roll )
-- @treturn nil
-- 

--- @function Spring.AddUnitImpulse ( number unitID, number x, number y, number z [, number decayRate ] )
-- @treturn nil
-- 

--- @function Spring.AddUnitSeismicPing ( number unitID, number pingSize )
-- @treturn nil
-- 

--- @function Spring.RemoveBuildingDecal ( number unitID )
-- @treturn nil
-- 

--- @function Spring.SetUnitMidAndAimPos ( number unitID, number mpX, number mpY, number mpZ, number apX, number apY, number apZ [, bool relative ] )
-- return: bool success
-- 
--     mpx, mpy, mpz: New middle position of unit
--     apx, apy, apz: New position that enemies aim at on this unit
--     relative: Are the new coordinates relative to world (false) or unit (true) coordinates? Also, note that apy is inverted!
-- 

--- @function Spring.SetUnitRadiusAndHeight ( number unitID, number radius, number height )
-- return: bool success
-- 
-- 

--- @function Spring.UnitWeaponFire ( number unitID, number weaponID )
-- @treturn nil
-- 

--- @function Spring.UnitWeaponHoldFire ( number unitID, number weaponID )
-- @treturn nil

--- Unit LOS
-- @section unitlos

--- @function Spring.SetUnitCloak ( number unitID, bool cloaked | number scriptCloak [, bool decloakAbs | number decloakDistance ] )
-- @treturn nil
-- 
--     If the 2nd argument is a number, the value works like this:
--     1:=normal cloak
--     2:=for free cloak (cost no E)
--     3:=for free + no decloaking (except the unit is stunned)
--     4:=ultimative cloak (no ecost, no decloaking, no stunned decloak)
-- 
--     The decloak distance is only changed:
--     - if the 3th argument is a number or a boolean.
--     - if the boolean is false it takes the default decloak distance for that unitdef,
--     - if the boolean is true it takes the absolute value of it.
-- 

--- @function Spring.SetUnitSonarStealth ( number unitID, bool sonarStealth )
-- @treturn nil
-- 

--- @function Spring.SetUnitStealth ( number unitID, bool stealth )
-- @treturn nil
-- 

--- @function Spring.SetUnitAlwaysVisible ( number unitID, bool alwaysVisible )
-- @treturn nil
-- 

--- @function Spring.SetUnitLosMask ( number unitID, number allyTeam, number los | table losTypes )
-- @treturn nil
-- 
--     The 3rd argument is either the bit-and combination of the following numbers:
--     LOS_INLOS = 1
--     LOS_INRADAR = 2
--     LOS_PREVLOS = 4
--     LOS_CONTRADAR = 8
-- 
--     or a table of the following form:
--     losTypes = {
--     [los = boolean,]
--     [radar = boolean,]
--     [prevLos = boolean,]
--     [contRadar = boolean]
--     }
-- 

--- @function Spring.SetUnitLosState ( number unitID, number allyTeam, number los | table losTypes )
-- @treturn nil
-- 
--     See above for more info on the arguments.
-- 

--- @function Spring.SetUnitSensorRadius
-- @number unitID
-- @string type "los" | "airLos" | "radar" | "sonar" | "seismic" | "radarJammer" | "sonarJammer"
-- @treturn ?nil|number newRadius

--- @function Spring.SetRadarErrorParams ( number allyTeamID, number allyteamErrorSize [, number baseErrorSize [, number baseErrorMult ]] )
-- @treturn nil
-- 
-- 

--- @function Spring.SetUnitPosErrorParams ( number unitID, number posErrorVector.x, number posErrorVector.y, number posErrorVector.z,, number posErrorDelta.x, number number posErrorDelta.y, number posErrorDelta.z [, number nextPosErrorUpdate ] )
-- @treturn nil
-- 

--- Unit Resourcing
-- @section unitresourcing

--- @function Spring.SetUnitResourcing ( number unitID, string res, number amount )
-- @treturn nil
-- 
-- or

--- @function Spring.SetUnitResourcing ( number unitID, { res = number amount, ... } )
-- @treturn nil
-- 
--     Possible values for res are: "[u|c][u|m][m|e]"
--     unconditional | conditional
--     use | make
--     metal | energy
-- 

--- @function Spring.AddUnitResource ( number unitID, string "m" | "e", number amount )
-- @treturn nil
-- 

--- @function Spring.UseUnitResource ( number unitID, string "m" | "e", number amount )
-- @treturn ?nil|bool okay
-- 
-- or

--- @function Spring.UseUnitResource ( number unitID, { [ "m" | "metal" | "e" | "energy" ] = amount, ... } )
-- @treturn ?nil|bool okay
-- 

--- @function Spring.SetUnitHarvestStorage ( number unitID, number metal )
-- @treturn nil
-- 
--     See also harvestStorage UnitDef tag.


--- Feature Handling
-- @section featurehandling

--- @function Spring.CreateFeature ( string "defName" | number featureDefID, number x, number y, number z [, number heading [, number AllyTeamID [, number featureID ]]] )
-- return: number featureID

--- @function Spring.DestroyFeature ( number featureDefID )
-- @treturn nil
-- 

--- @function Spring.TransferFeature ( number featureDefID, number teamID )
-- @treturn nil
-- 
-- Feature Control

--- @function Spring.SetFeatureHealth ( number featureID, number health )
-- @treturn nil
-- 

--- @function Spring.SetFeatureResources ( number featureID, number metal, number energy [, number reclaimTime [, number reclaimLeft ]{rbracket, {{{arg6}}}, {{{arg7}}}, {{{arg8}}}, {{{arg9}}} )
-- 
-- |arg6 = |arg7 = |arg8 = |arg9 = |return = nil |info = nil }}
-- 

--- @function Spring.SetFeatureReclaim ( number featureID, number reclaimLeft )
-- @treturn nil
-- 

--- @function Spring.SetFeatureResurrect ( number featureID, number unitDefID | string unitDefName [, number facing | string "facing" [, number progress ]] )
-- @treturn nil
-- 
--     Second param can now be a number id instead of a string name, this also allows cancelling ressurection by passing -1. The level of progress can now be set via the additional 4th param.
--     Possible values for facing are:
--     "south" | "s" | 0
--     "east" | "e" | 1
--     "north" | "n" | 2
--     "west" | "w" | 3
-- 

--- @function Spring.SetFeaturePosition ( number featureID, number x, number y, number z [, bool snapToGround ] )
-- @treturn nil
-- 

--- @function Spring.SetFeatureDirection ( number featureID, number dirX, number dirY, number dirZ )
-- @treturn nil
-- 

--- @function Spring.SetFeatureRotation ( number featureID, number rotX, number rotY, number rotZ )
-- @treturn nil
-- 
-- 

--- @function Spring.SetFeatureVelocity ( number featureID, number velX, number velY, number velZ )
-- @treturn nil
-- 
-- 

--- @function Spring.SetUnitPhysics ( number featureID, number posX, number posY, number posZ, number velX, number velY, number velZ, number rotX, number rotY, number rotZ, number dragX, number dragY, number dragZ )
-- @treturn nil
-- 
-- 

--- @function Spring.SetFeatureMoveCtrl ( number featureID [, bool enable [, number* args ]] )
-- @treturn nil
-- 
--     Use this callout to control feature movement. The number* arguments are parsed as follows and all optional:
-- 
--     If enable is true:
--     [, velVector(x,y,z) -- initial velocity for feature
--     [, accVector(x,y,z) -- acceleration added every frame]]
-- 
--     If enable is false:
--     [, velocityMask(x,y,z) -- dimensions in which velocity is allowed to build when not using MoveCtrl
--     [, impulseMask(x,y,z) -- dimensions in which impulse is allowed to apply when not using MoveCtrl
--     [, movementMask(x,y,z) -- dimensions in which feature is allowed to move when not using MoveCtrl]]]
-- 
--     It is necessary to unlock feature movement on x,z axis before changing feature physics. For example use `Spring.SetFeatureMoveCtrl(featureID,false,1,1,1,1,1,1,1,1,1)` to unlock all movement prior to making `SetFeatureVelocity` calls.
-- 

--- @function Spring.SetFeatureNoSelect ( number featureID, bool noSelect )
-- @treturn nil
-- 

--- @function Spring.SetFeatureAlwaysVisible ( number featureID, bool enable )
-- @treturn nil
-- 

--- @function Spring.SetFeatureCollisionVolumeData ( number featureID, number scaleX, number scaleY, number scaleZ, number offsetX, number offsetY, number offsetZ, number vType, number tType, number Axis )
-- @treturn nil
--     Check Spring.SetUnitCollisionVolumeData for further explanation of the arguments.
-- 

--- @function Spring.SetFeaturePieceCollisionVolumeData ( number featureID, number pieceIndex, bool enable, number scaleX, number scaleY, number scaleZ, number offsetX, number offsetY, number offsetZ, number Axis, number volumeType [, number primaryAxis ] )
-- @treturn nil

--- @function Spring.SetFeatureMidAndAimPos ( number featureID, number mpX, number mpY, number mpZ, number apX, number apY, number apZ [, bool relative )
-- return: bool success
-- 
--     Check Spring.SetUnitMidAndAimPos for further explanation of the arguments.
-- 

--- @function Spring.SetFeatureRadiusAndHeight ( number featureID, number radius, number height )
-- return: bool success

--- @function Spring.SetFeatureMass ( number featureID, number mass )
-- @treturn nil

--- @function Spring.SetFeatureBlocking ( number featureID, boolean isBlocking, boolean isSolidObjectCollidable, boolean isProjectileCollidable, boolean isRaySegmentCollidable, boolean crushable, boolean blockEnemyPushing, boolean blockHeightChanges )
-- @treturn nil

--- Lua to COB
-- @section luatocob

--- @function Spring.CallCOBScript ( number unitID, number funcID | string funcName, number retArgs, COBArg1, COBArg2, ... )
-- @treturn ?nil|number returnValue
-- @treturn ?nil|number returnArg1
-- @treturn ?nil|number returnArg2
-- @treturn ?nil|number returnArgn
-- 

--- @function Spring.GetCOBScriptID ( number unitID, string funcName )
-- @treturn ?nil|number funcID
-- 

--- @function Spring.GetUnitCOBValue ( number unitID [, bool splitData=false ], number COBValue [, number param1 [, number param2 [, number param3 [, number param4 ]]]] )
-- return: number result | number result1, number result2
-- 
--     Note: Don't use Spring.[Get|Set]UnitCOBValue in LUS just because you are familar with it since bos/cob, use the LuaSpringAPI instead!
--     You can find the possible values for `COBValue` in Lua_ConstCOB. Also see Custom_Variables.
-- 

--- @function Spring.SetUnitCOBValue ( number unitID, number COBValue, number param1 [, number param2 ] )
-- @treturn nil
-- 
--     Note: Don't use Spring.[Get|Set]UnitCOBValue in LUS just because you are familar with it since bos/cob, use the LuaSpringAPI instead!
--     You can find the possible values for `COBValue` in Lua_ConstCOB. Also see Custom_Variables.
-- 

--- Give Order
-- @section giveorder
-- Options can also be a bitmask; e.g. 0 instead of an empty table (can avoid performance hit on table creation)
-- See `Constants.CMD` for relevant constants.


--- @function Spring.GiveOrderToUnit ( number unitID, number cmdID, params = { number, etc...}, options = {"alt", "ctrl", "shift", "right"} )
-- @treturn nil
-- 

--- @function Spring.GiveOrderToUnitMap ( unitMap = { [unitID] = example, etc... }, number cmdID, params = { number, etc...}, options = {"alt", "ctrl", "shift", "right"} )
-- @treturn nil
-- 

--- @function Spring.GiveOrderToUnitArray ( unitArray = { [1] = unitID, etc... }, number cmdID, params = { number, etc...}, options = {"alt", "ctrl", "shift", "right"} )
-- @treturn nil
-- 

--- @function Spring.GiveOrderArrayToUnitMap ( unitMap = { [number unitID] = example, etc... }, orderArray = { { number cmdID, params = { number, etc...}, options = {"alt", "ctrl", "shift", "right"} } } )
-- @treturn nil
-- 

--- @function Spring.GiveOrderArrayToUnitArray ( unitArray = { [1] = number unitID, etc... }, orderArray = { { number cmdID, params = { number, etc...}, options = {"alt", "ctrl", "shift", "right"} } } )
-- @treturn nil

--- Grass
-- @section grass

--- @function Spring.AddGrass
-- @number x
-- @number z
-- @treturn nil


--- @function Spring.RemoveGrass
-- @number x
-- @number z
-- @treturn nil

--- Heightmap
-- @section heightmap
-- Note that x & z coords are in worldspace (Game.mapSizeX/Z), still the heightmap resolution is Game.squareSize.

--- @function Spring.LevelHeightMap ( number x1, number z1 [, number x2, number z2 ], number height )
-- @treturn nil
-- 

--- @function Spring.AdjustHeightMap ( number x1, number z1 [, number x2, number z2 ], number height )
-- @treturn nil
-- 
--     (heightmap[x][z] += height;)
-- 

--- @function Spring.RevertHeightMap ( number x1, number z1 [, number x2, number z2 ], number origFactor )
-- @treturn nil
-- 

--- @function Spring.SetHeightMapFunc ( lua_function [, arg1 [, arg2 [, ... ]]] )
-- @treturn ?nil|number absTotalHeightMapAmountChanged
-- 
-- Example code:

--- @function Spring.SetHeightMapFunc(function()
-- 	for z=0,Game.mapSizeZ, Game.squareSize do
-- 		for x=0,Game.mapSizeX, Game.squareSize do
-- 			Spring.SetHeightMap( x, z, 200 + 20 * math.cos((x + z) / 90) )
-- 		end
-- 	end
-- end)
-- 

--- @function Spring.AddHeightMap ( number x, number z, number height )
-- @treturn ?nil|number newHeight
-- 
--     Can only be called in SetHeightMapFunc()
-- 

--- @function Spring.SetHeightMap ( number x, number z, number height [, number terraform = 1 ] )
-- @treturn ?nil|number absHeightDiff
-- 
--     Can only be called in SetHeightMapFunc(). The terraform argument is a scaling factor:
-- 
-- If =0 nothing will be changed (the terraform starts) and if =1 the terraform will be finished.
-- 

--- @function Spring.LevelSmoothMesh ( number x1, number z1 [, number x2, number z2 ], number height )
-- @treturn nil
-- 

--- @function Spring.AdjustSmoothMesh ( number x1, number z1 [, number x2, number z2 ], number height )
-- @treturn nil
-- 

--- @function Spring.RevertSmoothMesh ( number x1, number z1 [, number x2, number z2 ], number origFactor )
-- @treturn nil
-- 

--- @function Spring.SetSmoothMeshFunc ( lua_function [, arg1 [, arg2 [, ... ]]] )
-- @treturn ?nil|number absTotalHeightMapAmountChanged
-- 

--- @function Spring.AddSmoothMesh ( number x, number z, number height )
-- @treturn ?nil|number newHeight
-- 
--     Can only be called in SetSmoothMeshFunc().
-- 

--- @function Spring.SetSmoothMesh ( number x, number z, number height [, number terraform = 1 ] )
-- @treturn ?nil|number absHeightDiff
-- 
--     Can only be called in SetSmoothMeshFunc().

--- TerrainTypes
-- @section terraintypes

--- @function Spring.SetMapSquareTerrainType ( number x, number z, number newType )
-- @treturn ?nil|number oldType
-- 

--- @function Spring.SetTerrainTypeData ( number typeIndex [, number speedTanks = nil [, number speedKBOts = nil [, number speedHovers = nil [, number speedShips = nil ]]]] )
-- @treturn ?nil|bool true
-- 

--- @function Spring.SetSquareBuildingMask ( number x, number z, number mask )
-- @treturn nil
-- 
-- 
-- See also buildingMask unitdef tag.
-- MetalAmount

--- @function Spring.SetMetalAmount ( number x, number z, number metalAmount )
-- @treturn nil
-- 
--     x & z coords are in worldspace/16. metalAmount must be between 0 and 255*maxMetal (with maxMetal from the .smd or mapinfo.lua).

--- Command Descriptions
-- @section commanddescriptions
-- Doesn't work in unsynced code!

--- @function Spring.EditUnitCmdDesc ( number unitID, number cmdDescID, table cmdArray )
-- @number unitID
-- @number cmdDescID
-- @tparam table cmdArray structure of cmdArray:
--     {
--       [ id          = int ],
--       [ type        = int ],
--       [ name        = string ],
--       [ action      = string ],
--       [ tooltip     = string ],
--       [ texture     = string ],
--       [ cursor      = string ],
--       [ queueing    = boolean ],
--       [ hidden      = boolean ],
--       [ disabled    = boolean ],
--       [ showUnique  = boolean ],
--       [ onlyTexture = boolean ],
--       [ params      = { string = string, ... } ]
--     }
-- @treturn nil

--- @function Spring.InsertUnitCmdDesc ( number unitID [, number cmdDescID ], table cmdArray )
-- @treturn nil
-- 

--- @function Spring.RemoveUnitCmdDesc ( number unitID [, number cmdDescID ] )
-- @treturn nil

--- Other
-- @section other

--- @function Spring.SetNoPause
-- @bool noPause
-- @treturn nil

--- An ugly global switch that can be used to block units turning into features when they finish being built (like *A DT's do, which have the isFeature tag) thread
-- @function Spring.SetUnitToFeature
-- @bool tofeature
-- @treturn nil

--- Defines how often `Callins.UnitExperience` will be called.
-- @function Spring.SetExperienceGrade
-- @number expGrade
-- @number[opt] ExpPowerScale
-- @number[opt] ExpHealthScale
-- @number[opt] ExpReloadScale
-- @treturn nil
-- 

--- Please note the explosion defaults to 1 damage regardless of what it's defined in the weaponDef.
-- The weapondefID is only used for visuals and for passing into callins like UnitDamaged.
-- @table explosionParams
-- @number weaponDef
-- @number owner
-- @number hitUnit
-- @number hitFeature
-- @number craterAreaOfEffect
-- @number damageAreaOfEffect
-- @number edgeEffectiveness
-- @number explosionSpeed
-- @number gfxMod
-- @bool impactOnly
-- @bool ignoreOwner
-- @bool damageGround

--- @function Spring.SpawnExplosion
-- @number[opt=0] posX
-- @number[opt=0] posY
-- @number[opt=0] posZ
-- @number[opt=0] dirX
-- @number[opt=0] dirY
-- @number[opt=0] dirZ
-- @tparam explosionParams explosionParams
-- @treturn nil

--- @function Spring.SpawnCEG
-- @string cegname
-- @number[opt=0] posX
-- @number[opt=0] posY
-- @number[opt=0] posZ
-- @number[opt=0] dirX
-- @number[opt=0] dirY
-- @number[opt=0] dirZ
-- @number[opt=0] radius
-- @number[opt=0] damage
-- @treturn ?nil|bool success
-- @treturn number cegID

--- Equal to the UnitScript versions of EmitSFX, but takes position and direction arguments (in either unit- or piece-space) instead of a piece index.
-- @function Spring.SpawnSFX
-- @number[opt=0] unitID
-- @number[opt=0] sfxID
-- @number[opt=0] posX
-- @number[opt=0] posY
-- @number[opt=0] posZ
-- @number[opt=0] dirX
-- @number[opt=0] dirY
-- @number[opt=0] dirZ
-- @number[opt=0] radius
-- @number[opt=0] damage
-- @bool[opt] absolute
-- @treturn ?nil|bool success

--- Projectiles
-- @section projectiles

--- @table projectileParams
-- @tparam table pos
-- @number pos.x
-- @number pos.y
-- @number pos.z
-- @tparam table end
-- @number end.x
-- @number end.y
-- @number end.z
-- @tparam table speed
-- @number speed.x
-- @number speed.y
-- @number speed.z
-- @tparam table spread
-- @number spread.x
-- @number spread.y
-- @number spread.z
-- @tparam table error
-- @number error.x
-- @number error.y
-- @number error.z
-- @number owner
-- @number team
-- @number ttl
-- @number gravity
-- @number tracking
-- @number maxRange
-- @number startAlpha
-- @number endAlpha
-- @string model
-- @string cegTag

--- @function Spring.SpawnProjectile
-- @number weaponDefID
-- @tparam projectileParams projectileParams
-- @treturn ?nil|number projectileID

--- Silently removes projectiles (no explosion).
-- @function Spring.DeleteProjectile
-- @number projectileID
-- @treturn nil

--- @function Spring.SetProjectileTarget ( number projectileID, [ number targetID, number targetType ] | [ number posX = 0, number posY = 0, number posZ = 0 ] )
-- @treturn ?nil|bool validTarget
-- 
-- targetTypeStr can be one of: 
--     'u' - unit
--     'f' - feature
--     'p' - projectile
--   while targetTypeInt is one of:
--     string.byte('g') := GROUND
--     string.byte('u') := UNIT
--     string.byte('f') := FEATURE
--     string.byte('p') := PROJECTILE
-- 

--- @function Spring.SetProjectileIgnoreTrackingError
-- @number projectileID
-- @bool ignore
-- @treturn nil

--- @function Spring.SetProjectileIsIntercepted
-- @number projectileID
-- @treturn nil

--- Disables engine movecontrol, so lua can fully control the physics.
-- @function Spring.SetProjectileMoveControl
-- @number projectileID
-- @bool enable
-- @treturn nil

--- @function Spring.SetProjectilePosition
-- @number projectileID
-- @number[opt=0] posX
-- @number[opt=0] posY
-- @number[opt=0] posZ
-- @treturn nil
-- 

--- @function Spring.SetProjectileVelocity
-- @number projectileID
-- @number[opt=0] velX
-- @number[opt=0] velY
-- @number[opt=0] velZ
-- @treturn nil
-- 

--- @function Spring.SetProjectileCollision
-- @number projectileID
-- @treturn nil

--- @function Spring.SetProjectileGravity
-- @number projectileID
-- @number[opt=0] grav
-- @treturn nil

--- @function Spring.SetProjectileCEG
-- @number projectileID
-- @string ceg_name
-- @treturn nil

--- @function Spring.SetPieceProjectileParams ( number projectileID [, number explosionFlags [, number spinAngle [, number spinSpeed [, number spinVector.x [, number spinVector.y [, number spinVector.z ]]]]]] )
-- @treturn nil

--- @function Spring.SetProjectileAlwaysVisible
-- @number projectileID
-- @bool alwaysVisible
-- @treturn nil

--- @function Spring.SetProjectileDamages
-- @number unitID
-- @number weaponNum
-- @tparam damages damages
-- @treturn nil

--- @function Spring.SetProjectileDamages
-- @number unitID
-- @number weaponNum
-- @string key
-- @number value
-- @treturn nil
