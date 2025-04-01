--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    callins.lua
--  brief:   array and map of call-ins
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

CallInsList = {
	"Shutdown",
	"LayoutButtons",
	"ConfigureLayout",
	"ActiveCommandChanged",
	"CameraRotationChanged",
	"CameraPositionChanged",
	"CommandNotify",

	"KeyMapChanged",
	"KeyPress",
	"KeyRelease",
	"TextInput",
	"TextEditing",
	"MouseMove",
	"MousePress",
	"MouseRelease",
	"JoyAxis",
	"JoyHat",
	"JoyButtonDown",
	"JoyButtonUp",
	"IsAbove",
	"GetTooltip",
	"AddConsoleLine",
	"GroupChanged",
	"WorldTooltip",

	"GameLoadLua",
	"GameStartPlaying",
	"GameOver",
	"TeamDied",

	"UnitCreated",
	"UnitFinished",
	"UnitFromFactory",
	"UnitReverseBuilt",
	"UnitConstructionDecayed",
	"UnitDestroyed",
	"RenderUnitDestroyed",
	"UnitTaken",
	"UnitGiven",
	"UnitIdle",
	"UnitCommand",
	"UnitSeismicPing",
	"UnitEnteredRadar",
	"UnitEnteredLos",
	"UnitLeftRadar",
	"UnitLeftLos",
	"UnitLoaded",
	"UnitUnloaded",
	"UnitHarvestStorageFull",

	"UnitEnteredUnderwater",
	"UnitEnteredWater",
	"UnitEnteredAir",
	"UnitLeftUnderwater",
	"UnitLeftWater",
	"UnitLeftAir",

	"FeatureCreated",
	"FeatureDestroyed",

	"DrawGenesis",
	"DrawWorld",
	"DrawWorldPreUnit",
	"DrawWorldPreParticles",
	"DrawWorldShadow",
	"DrawWorldReflection",
	"DrawWorldRefraction",
	"DrawGroundPreForward",
	"DrawGroundPostForward",
	"DrawGroundPreDeferred",
	"DrawGroundDeferred",
	"DrawGroundPostDeferred",
	"DrawUnitsPostDeferred",
	"DrawFeaturesPostDeferred",
	"DrawScreenEffects",
	"DrawScreenPost",
	"DrawScreen",
	"DrawInMiniMap",

	"FontsChanged",

	"SunChanged",

	"Explosion",
	"ShockFront",

	"RecvSkirmishAIMessage",

	"GameFrame",
	"CobCallback",
	"AllowCommand",
	"CommandFallback",
	"AllowUnitCreation",
	"AllowUnitTransfer",
	"AllowUnitBuildStep",
	"AllowUnitCaptureStep",
	"AllowFeatureCreation",
	"AllowFeatureBuildStep",
	"AllowResourceLevel",
	"AllowResourceTransfer",

	"GameProgress",
	"Pong",

	"DownloadQueued",
	"DownloadStarted",
	"DownloadFinished",
	"DownloadFailed",
	"DownloadProgress",
}


-- make the map
CallInsMap = {}
for _, callin in ipairs(CallInsList) do
  CallInsMap[callin] = true
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
