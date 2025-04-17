/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include <array>
#include <functional>
#include <tuple>

#include "UnsyncedGameCommands.h"

#include "UnsyncedActionExecutor.h"
#include "SyncedGameCommands.h"
#include "SyncedActionExecutor.h"
#include "Action.h"
#include "CameraHandler.h"
#include "ConsoleHistory.h"
#include "CommandMessage.h"
#include "Game.h"
#include "GameSetup.h"
#include "GlobalUnsynced.h"
#include "SelectedUnitsHandler.h"
#include "WordCompletion.h"
#include "InMapDraw.h"
#include "InMapDrawModel.h"
#include "IVideoCapturing.h"
#ifdef _WIN32
#  include "winerror.h" // TODO someone on windows (MinGW? VS?) please check if this is required
#endif

#include "ExternalAI/AILibraryManager.h"
#include "ExternalAI/SkirmishAIHandler.h"

#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
#include "Game/UI/CommandColors.h"
#include "Game/UI/EndGameBox.h"
#include "Game/UI/GameInfo.h"
#include "Game/UI/GuiHandler.h"
#include "Game/UI/InfoConsole.h"
#include "Game/UI/InputReceiver.h"
#include "Game/UI/KeyBindings.h"
#include "Game/UI/KeyCodes.h"
#include "Game/UI/MiniMap.h"
#include "Game/UI/ProfileDrawer.h"
#include "Game/UI/QuitBox.h"
#include "Game/UI/ResourceBar.h"
#include "Game/UI/SelectionKeyHandler.h"
#include "Game/UI/ShareBox.h"
#include "Game/UI/TooltipConsole.h"
#include "Game/UI/UnitTracker.h"
#include "Game/UI/Groups/GroupHandler.h"
#include "Game/UI/PlayerRoster.h"

#include "Lua/LuaOpenGL.h"
#include "Lua/LuaUI.h"
#include "Lua/LuaMenu.h"

#include "Map/Ground.h"
#include "Map/MetalMap.h"
#include "Map/ReadMap.h"
#include "Map/SMF/SMFGroundDrawer.h"
#include "Map/SMF/ROAM/Patch.h"
#include "Map/SMF/ROAM/RoamMeshDrawer.h"

#include "Net/GameServer.h"
#include "Net/Protocol/NetProtocol.h"

#include "Rendering/DebugColVolDrawer.h"
#include "Rendering/DebugVisibilityDrawer.h"
#include "Rendering/DebugDrawerAI.h"
#include "Rendering/DebugDrawerQuadField.h"
#include "Rendering/IPathDrawer.h"
#include "Rendering/Features/FeatureDrawer.h"
#include "Rendering/HUDDrawer.h"
#include "Rendering/LuaObjectDrawer.h"
#include "Rendering/Screenshot.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/SmoothHeightMeshDrawer.h"
#include "Rendering/TeamHighlight.h"
#include "Rendering/Units/UnitDrawer.h"
#include "Rendering/VerticalSync.h"
#include "Rendering/Env/IGroundDecalDrawer.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Env/IWater.h"
#include "Rendering/Env/GrassDrawer.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Rendering/Fonts/glFont.h"
#include "Rendering/Map/InfoTexture/IInfoTextureHandler.h"
#include "Rendering/Map/InfoTexture/Modern/Path.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Textures/NamedTextures.h"
#include "Rendering/Textures/S3OTextureHandler.h"

#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/CommandAI/CommandDescription.h"

#include "System/EventHandler.h"
#include "System/GlobalConfig.h"
#include "System/SafeUtil.h"
#include "System/TimeProfiler.h"
#include "System/Log/ILog.h"
#include "System/Config/ConfigHandler.h"
#include "System/FileSystem/SimpleParser.h"
#include "System/Sound/ISound.h"
#include "System/Sound/ISoundChannels.h"
#include "System/Sync/DumpState.h"
#include "System/Sync/DumpHistory.h"

#include <SDL_events.h>
#include <SDL_video.h>

namespace { // prevents linking problems in case of duplicate symbols


	//helper functions
	using ArgTuple = std::tuple<uint32_t, bool, std::function<void()>>;

	template<typename Iterable>
	bool GenericArgsExecutor(std::vector<std::string>& args, Iterable& argsExec) {
		for (auto& arg : args) {
			StringToLowerInPlace(arg);

			auto hs = hashString(arg.c_str());
			for (auto& [ahs, aen, afun] : argsExec) {
				if (ahs == hs)
					aen = true;
			}
		}

		if (args.empty()) {
			for (auto& [ahs, aen, afun] : argsExec) {
				aen = true;
			}
		}

		for (const auto& [ahs, aen, afun] : argsExec) {
			if (aen)
				afun();
		}

		return true;
	}
	// end of helper function

/**
 * Special case executor which is used for creating aliases to other commands.
 * The inner executor will be delet'ed in this executors dtor.
 */
class AliasActionExecutor : public IUnsyncedActionExecutor {
public:
	AliasActionExecutor(IUnsyncedActionExecutor* innerExecutor, const std::string& commandAlias)
		: IUnsyncedActionExecutor(commandAlias, "Alias for command \"" + commandAlias + "\"")
		, innerExecutor(innerExecutor)
	{
		assert(innerExecutor != nullptr);
	}

	~AliasActionExecutor() override {
		delete innerExecutor;
	}

	bool Execute(const UnsyncedAction& action) const final {
		return innerExecutor->ExecuteAction(action);
	}

private:
	IUnsyncedActionExecutor* innerExecutor;
};



/**
 * Special case executor which allows to combine multiple commands into one,
 * by calling them sequentially.
 * The inner executors will be delet'ed in this executors dtor.
 */
class SequentialActionExecutor : public IUnsyncedActionExecutor {
public:
	SequentialActionExecutor(const std::string& command): IUnsyncedActionExecutor(command, "Executes the following commands in order:") {
	}

	~SequentialActionExecutor() override {
		for (IUnsyncedActionExecutor* e: innerExecutors) {
			delete e;
		}
	}

	void AddExecutor(IUnsyncedActionExecutor* innerExecutor) {
		innerExecutors.push_back(innerExecutor);
		SetDescription(GetDescription() + " " + innerExecutor->GetCommand());
	}

	bool Execute(const UnsyncedAction& action) const final {
		for (IUnsyncedActionExecutor* e: innerExecutors) {
			e->ExecuteAction(action);
		}

		return true;
	}

private:
	std::vector<IUnsyncedActionExecutor*> innerExecutors;
};



class SelectActionExecutor : public IUnsyncedActionExecutor {
public:
	SelectActionExecutor() : IUnsyncedActionExecutor("Select", "<chat command description: Select>") {
	} // TODO

	bool Execute(const UnsyncedAction& action) const final {
		selectionKeys.DoSelection(action.GetArgs()); //TODO give it a return argument?
		return true;
	}
};

class SelectUnitsActionExecutor : public IUnsyncedActionExecutor {
public:
	SelectUnitsActionExecutor() : IUnsyncedActionExecutor("SelectUnits", "<chat command description: SelectUnits>") {
	} // TODO

	bool Execute(const UnsyncedAction& action) const final {
		selectedUnitsHandler.SelectUnits(action.GetArgs()); //TODO give it a return argument?
		return true;
	}
};

class SelectCycleActionExecutor : public IUnsyncedActionExecutor {
public:
	SelectCycleActionExecutor() : IUnsyncedActionExecutor("SelectCycle", "<chat command description: SelectUnits>") {
	} // TODO

	bool Execute(const UnsyncedAction& action) const final {
		selectedUnitsHandler.SelectCycle(action.GetArgs()); //TODO give it a return argument?
		return true;
	}
};

class DeselectActionExecutor : public IUnsyncedActionExecutor {
public:
	DeselectActionExecutor() : IUnsyncedActionExecutor("Deselect", "Deselects all currently selected units") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		selectedUnitsHandler.ClearSelected();
		return true;
	}
};



class MapMeshDrawerActionExecutor : public IUnsyncedActionExecutor {
public:
	MapMeshDrawerActionExecutor() : IUnsyncedActionExecutor("mapmeshdrawer", "Switch map-mesh rendering modes: 0=GCM, 1=HLOD, 2=ROAM") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		CSMFGroundDrawer* smfDrawer = dynamic_cast<CSMFGroundDrawer*>(readMap->GetGroundDrawer());

		if (smfDrawer == nullptr)
			return false;

		if (action.GetArgs().empty()) {
			smfDrawer->SwitchMeshDrawer();
			return true;
		}

		auto args = CSimpleParser::Tokenize(action.GetArgs());
		bool parseFailure;

		int smfMeshDrawerArg = (!args.empty()) ? StringToInt(args[0], &parseFailure) : -1.0;
		if (parseFailure) smfMeshDrawerArg = -1.0;

		smfDrawer->SwitchMeshDrawer(smfMeshDrawerArg);

		return true;
	}
};


class MapBorderActionExecutor : public IUnsyncedActionExecutor {
public:
	MapBorderActionExecutor() : IUnsyncedActionExecutor("MapBorder", "Control map-border rendering", false, {
			{"", "Toggles map-border rendering"},
			{"<on|off>", "Set map-border rendering <on|off>"},
			}) {
	}

	bool Execute(const UnsyncedAction& action) const final {
		CSMFGroundDrawer* smfGD = dynamic_cast<CSMFGroundDrawer*>(readMap->GetGroundDrawer());

		if (smfGD == nullptr)
			return false;

		if (!action.GetArgs().empty()) {
			bool enable = true;
			InverseOrSetBool(enable, action.GetArgs());

			if (enable != smfGD->ToggleMapBorder())
				smfGD->ToggleMapBorder();

		} else {
			smfGD->ToggleMapBorder();
		}

		return true;
	}
};


class ShadowsActionExecutor : public IUnsyncedActionExecutor {
public:
	ShadowsActionExecutor() : IUnsyncedActionExecutor(
		"Shadows",
		"Control shadow rendering",
		false,
		{
			{"-1", "Disabled"},
			{"0", "Off"},
			{"1", "Full shadows"},
			{"2", "Skip terrain shadows"},
		}
	) {
	}

	bool Execute(const UnsyncedAction& action) const final {
		if (shadowHandler.shadowConfig < 0) {
			LOG_L(L_WARNING, "Shadows are disabled; change your configuration and restart to use them");
			return true;
		}
		if (!CShadowHandler::ShadowsSupported()) {
			LOG_L(L_WARNING, "Your hardware/driver setup does not support shadows");
			return true;
		}

		shadowHandler.Reload(((action.GetArgs()).empty())? nullptr: (action.GetArgs()).c_str());
		LOG("Set \"shadows\" config-parameter to %i", shadowHandler.shadowConfig);
		return true;
	}
};

class DumpShadowsActionExecutor : public IUnsyncedActionExecutor {
public:
	DumpShadowsActionExecutor() : IUnsyncedActionExecutor(
		"DumpShadows",
		"Save shadow map textures to files"
	)
	{}

	bool Execute(const UnsyncedAction& action) const final {
		shadowHandler.SaveShadowMapTextures();
		return true;
	}
};

class MapShadowPolyOffsetActionExecutor: public IUnsyncedActionExecutor {
public:
	MapShadowPolyOffsetActionExecutor(): IUnsyncedActionExecutor("MapShadowPolyOffset", "") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		auto args = CSimpleParser::Tokenize(action.GetArgs());

		float& pofs = (readMap->GetGroundDrawer())->spPolygonOffsetScale;
		float& pofu = (readMap->GetGroundDrawer())->spPolygonOffsetUnits;

		pofs = args.size() > 0 ? StringToInt<float>(args[0]) : 0.0;
		pofu = args.size() > 1 ? StringToInt<float>(args[1]) : 0.0;

		LOG("MapShadowPolygonOffset{Scale,Units}={%f,%f}", pofs, pofu);
		return true;
	}
};

class WaterActionExecutor : public IUnsyncedActionExecutor {
public:
	WaterActionExecutor() : IUnsyncedActionExecutor(
		"Water",
		"Set water rendering mode",
		false,
		{
			{"0", "Basic"},
			{"1", "Reflective"},
			{"2", "Dynamic"},
			{"3", "Reflective & Refractive"},
			{"4", "Bump-mapped"},
		}
	) {
	}

	bool Execute(const UnsyncedAction& action) const final {
		bool parseFailure;
		int nextWaterRendererMode = StringToInt(action.GetArgs(), &parseFailure);

		if (parseFailure)
			nextWaterRendererMode = -1;

		IWater::SetWater(nextWaterRendererMode);
		return true;
	}
};

class AdvMapShadingActionExecutor : public IUnsyncedActionExecutor {
public:
	AdvMapShadingActionExecutor() : IUnsyncedActionExecutor("AdvMapShading",
			"Control advanced map shading mode",
			false, {
			{"", "Toggles advanced map shading mode"},
			{"<on|off>", "Set advanced map shading mode <on|off>"},
			}) {}

	bool Execute(const UnsyncedAction& action) const {

		CBaseGroundDrawer* gd = readMap->GetGroundDrawer();
		static bool canUseShaders = gd->UseAdvShading();

		if (!canUseShaders)
			return false;

		InverseOrSetBool(gd->UseAdvShadingRef(), action.GetArgs());
		LogSystemStatus("map shaders", gd->UseAdvShading());
		return true;
	}
};

class UnitDrawerTypeActionExecutor : public IUnsyncedActionExecutor {
public:
	UnitDrawerTypeActionExecutor() : IUnsyncedActionExecutor("UnitDrawer",
		"Forces particular Unit drawer type") {}

	bool Execute(const UnsyncedAction& action) const {
		auto args = CSimpleParser::Tokenize(action.GetArgs());
		bool parseFailure;

		if (args.size() == 0) {
			LOG_L(L_WARNING, "/%s: wrong syntax", GetCommand().c_str());
			return false;
		}

		int prefModelDrawer = StringToInt(args[0], &parseFailure);
		if (parseFailure) return false;

		if (args.size() > 1) {
			// Note: there's StringToBool but it always returns something; so
			// using it would change the code here.
			int mtModelDrawer = StringToInt(args[1], &parseFailure);

			if (!parseFailure)
				CUnitDrawer::MTDrawerTypeRef() = static_cast<bool>(mtModelDrawer);
		}

		CUnitDrawer::PreferedDrawerTypeRef() = prefModelDrawer;

		return true;
	}
};

class FeatureDrawerTypeActionExecutor : public IUnsyncedActionExecutor {
public:
	FeatureDrawerTypeActionExecutor() : IUnsyncedActionExecutor("FeatureDrawer",
		"Forces particular Feature drawer type") {}

	bool Execute(const UnsyncedAction& action) const {
		auto args = CSimpleParser::Tokenize(action.GetArgs());
		bool parseFailure;

		if (args.size() == 0) {
			LOG_L(L_WARNING, "/%s: wrong syntax", GetCommand().c_str());
			return false;
		}

		int prefModelDrawer = StringToInt(args[0], &parseFailure);
		if (parseFailure) return false;

		if (args.size() > 1) {
			// Note: there's StringToBool but it always returns something; so
			// using it would change the code here.
			int mtModelDrawer = StringToInt(args[1], &parseFailure);

			if (!parseFailure)
				CFeatureDrawer::MTDrawerTypeRef() = static_cast<bool>(mtModelDrawer);
		}

		CFeatureDrawer::PreferedDrawerTypeRef() = prefModelDrawer;

		return true;
	}
};


class SayActionExecutor : public IUnsyncedActionExecutor {
public:
	SayActionExecutor() : IUnsyncedActionExecutor("Say",
			"Say something in (public) chat") {}

	bool Execute(const UnsyncedAction& action) const final {
		game->SendNetChat(action.GetArgs());
		return true;
	}
};



class SayPrivateActionExecutor : public IUnsyncedActionExecutor {
public:
	SayPrivateActionExecutor() : IUnsyncedActionExecutor("W", "Say something in private to a specific player, by player-name") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		auto args = CSimpleParser::Tokenize(action.GetArgs(), 1);

		if (args.size() == 0) {
			LOG_L(L_WARNING, "/w: wrong syntax (which is '/w %%playername')");
			return true;
		}

		const int playerID = playerHandler.Player(args[0]);

		if (playerID >= 0) {
			std::string message = (args.size() == 2) ? std::move(args[1]) : "";
			game->SendNetChat(std::move(message), playerID);
		} else {
			LOG_L(L_WARNING, "/w: Player not found: %s", args[0].c_str());
		}

		return true;
	}
};



class SayPrivateByPlayerIDActionExecutor : public IUnsyncedActionExecutor {
public:
	SayPrivateByPlayerIDActionExecutor() : IUnsyncedActionExecutor("WByNum", "Say something in private to a specific player, by player-ID") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		auto args = CSimpleParser::Tokenize(action.GetArgs(), 1);

		if (args.size() == 0) {
			LOG_L(L_WARNING, "/WByNum: wrong syntax (which is '/WByNum %%playerid')");
			return true;
		}

		bool parseFailure;
		const int playerID = StringToInt(args[0], &parseFailure);

		if (parseFailure) {
			LOG_L(L_WARNING, "/WByNum: wrong syntax (which is '/WByNum %%playerid')");
			return true;
		}

		if (playerID >= 0) {
			std::string message = (args.size() == 2) ? std::move(args[1]) : "";
			game->SendNetChat(std::move(message), playerID);
		} else {
			LOG_L(L_WARNING, "Player-ID invalid: %i", playerID);
		}

		return true;
	}
};



class EchoActionExecutor : public IUnsyncedActionExecutor {
public:
	EchoActionExecutor() : IUnsyncedActionExecutor("Echo", "Write a string to the log file") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		LOG("%s", action.GetArgs().c_str());
		return true;
	}
};

class SetActionExecutor : public IUnsyncedActionExecutor {
public:
	SetActionExecutor(bool overlay_) : IUnsyncedActionExecutor(overlay_ ? "TSet" : "Set",
			std::string("Set a config key=value pair") +
			(overlay_ ? " in the overlay, meaning it will not be persisted for future games" : "")
		),
		overlay(overlay_) {}

	bool Execute(const UnsyncedAction& action) const final {
		auto args = CSimpleParser::Tokenize(action.GetArgs(), 1);

		if (args.size() != 2) {
			LOG_L(L_WARNING, "/%s: wrong syntax (which is '/%s %%cfgtag %%cfgvalue')", GetCommand().c_str(), GetCommand().c_str());
			return true;
		}
		configHandler->SetString(args[0], args[1], overlay);
		return true;
	}

private:
	bool overlay;
};



class EnableDrawInMapActionExecutor : public IUnsyncedActionExecutor {
public:
	EnableDrawInMapActionExecutor() : IUnsyncedActionExecutor("DrawInMap", "Enables drawing on the map") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		inMapDrawer->SetDrawMode(true);
		return true;
	}

	bool ExecuteRelease(const UnsyncedAction& action) const final {
		inMapDrawer->SetDrawMode(false);
		// TODO: false for backwards compatibility (needs in depth review before changing)
		return false;
	}
};



class DrawLabelActionExecutor : public IUnsyncedActionExecutor {
public:
	DrawLabelActionExecutor() : IUnsyncedActionExecutor("DrawLabel", "Draws a label on the map at the current mouse-pointer position") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		float3 pos;

		if (minimap != nullptr && minimap->IsInside(mouse->lastx, mouse->lasty)) {
			pos = minimap->GetMapPosition(mouse->lastx, mouse->lasty);
		} else {
			pos = mouse->GetWorldMapPos();
		}

		if (pos.x >= 0.0f) {
			inMapDrawer->SetDrawMode(false);
			inMapDrawer->PromptLabel(pos);
		} else {
			LOG_L(L_WARNING, "/DrawLabel: move mouse over the map");
		}

		return true;
	}
};



class MouseActionExecutor : public IUnsyncedActionExecutor {
public:
	MouseActionExecutor(int _button): IUnsyncedActionExecutor(
		"Mouse" + IntToString(_button),
		"Simulates a press of mouse-button " + IntToString(_button)
	) {
		button = _button;
	}

	bool Execute(const UnsyncedAction& action) const final {
		if (!action.IsRepeat())
			mouse->MousePress(mouse->lastx, mouse->lasty, button);

		return true;
	}

	bool ExecuteRelease(const UnsyncedAction& action) const final {
		if (button == 4 || button == 5) {
			// HACK   somehow weird things happen when MouseRelease is called for button 4 and 5.
			// Note that SYS_WMEVENT on windows also only sends MousePress events for these buttons.
			return false;
		}
		mouse->MouseRelease(mouse->lastx, mouse->lasty, button);
		// TODO: false for backwards compatibility (needs in depth review)
		return false;
	}

private:
	int button;
};


class MouseStateActionExecutor : public IUnsyncedActionExecutor {
public:
	MouseStateActionExecutor() : IUnsyncedActionExecutor("MouseState", "Toggles mousestate") {}

	bool Execute(const UnsyncedAction& action) const final {
		// TODO: false for backwards compatibility (needs in depth review)
		return false;
	}
	bool ExecuteRelease(const UnsyncedAction& action) const final {
		mouse->ToggleMiddleClickScroll();
		// TODO: false for backwards compatibility (needs in depth review)
		return false;
	}
};


class MouseCancelSelectionRectangleActionExecutor : public IUnsyncedActionExecutor {
public:
	MouseCancelSelectionRectangleActionExecutor(): IUnsyncedActionExecutor("MouseCancelSelectionRectangle", "") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		// MouseHandler::MouseRelease checks LMB movement against drag-selection threshold
		mouse->CancelButtonMovement(SDL_BUTTON_LEFT);
		return true;
	}
};



class ViewSelectionActionExecutor : public IUnsyncedActionExecutor {
public:
	ViewSelectionActionExecutor() : IUnsyncedActionExecutor(
		"ViewSelection",
		"Moves the camera to the center of the currently selected units"
	) {
	}

	bool Execute(const UnsyncedAction& action) const final {
		const auto& selUnits = selectedUnitsHandler.selectedUnits;

		if (selUnits.empty())
			return false;

		// XXX this code is duplicated in CGroup::CalculateCenter()
		float3 pos;

		for (const int unitID: selUnits) {
			pos += (unitHandler.GetUnit(unitID))->midPos;
		}

		camHandler->CameraTransition(0.6f);
		camHandler->GetCurrentController().SetPos(pos * (1.0f / selUnits.size()));
		return true;
	}
};



class CameraMoveActionExecutor : public IUnsyncedActionExecutor {
public:
	CameraMoveActionExecutor(
		int _moveStateIdx,
		const std::string& commandPostfix,
		const bool _halt = true
	): IUnsyncedActionExecutor("Move" + commandPostfix, "Moves the camera " + commandPostfix + " a bit") {
		moveStateIdx = _moveStateIdx;
		halt = _halt;
	}

	bool Execute(const UnsyncedAction& action) const final {
		camera->SetMovState(moveStateIdx, true);

		return halt;
	}

	bool ExecuteRelease(const UnsyncedAction& action) const final {
		camera->SetMovState(moveStateIdx, false);
		// TODO: false for backwards compatibility (needs in depth review)
		return false;
	}

private:
	int moveStateIdx;
	bool halt;
};



class AIKillReloadActionExecutor : public IUnsyncedActionExecutor {
public:
	/**
	 * @param kill whether this executor should function as the kill-
	 * or the reload-AI command
	 */
	AIKillReloadActionExecutor(bool kill_): IUnsyncedActionExecutor(
		(kill_ ? "AIKill" : "AIReload"),
		std::string(kill_ ? "Kills" : "Reloads") + " the Skirmish AI controlling a specified team"
	) {
		kill = kill_;
	}

	bool WrongSyntax() const {
		if (kill) {
			LOG("description: "
				"Kill a Skirmish AI controlling a team. The team itself will remain alive "
				"unless a second argument is given, which specifies an active team "
				"that will receive all the units of the AI team.");
			LOG("usage:   /%s teamToKill [teamToReceiveUnits]", GetCommand().c_str());
		} else {
			// reload
			LOG("description: "
				"Reload a Skirmish AI controlling a team."
				"The team itself will remain alive during the process.");
			LOG("usage:   /%s teamToReload", GetCommand().c_str());
		}
		return true;
	}

	bool Execute(const UnsyncedAction& action) const final {
		const CPlayer* fromPlayer     = playerHandler.Player(gu->myPlayerNum);
		const int      fromTeamId     = (fromPlayer != nullptr) ? fromPlayer->team : -1;

		const bool cheating           = gs->cheatEnabled;
		const bool singlePlayer       = (playerHandler.ActivePlayers() <= 1);

		std::vector<std::string> args = CSimpleParser::Tokenize(action.GetArgs());
		const std::string actionName  = StringToLower(GetCommand()).substr(2);

		if (args.empty()) {
			LOG_L(L_WARNING, "/%s: missing mandatory argument \"teamTo%s\"", GetCommand().c_str(), actionName.c_str());
			return WrongSyntax();
		}

		bool parseFailure;

		// Parse first argument (team to reload/kill)
		int teamToKillId = StringToInt(args[0], &parseFailure);
		CTeam* teamToKill = (!parseFailure && teamHandler.IsActiveTeam(teamToKillId))? teamHandler.Team(teamToKillId) : nullptr;

		// Validate first argument
		if (parseFailure || teamToKill == nullptr) {
			LOG_L(L_WARNING, "Team to %s: not a valid team number: \"%s\"", actionName.c_str(), args[0].c_str());
			return WrongSyntax();
		}

		// Parse second argument if needed (team to receive units)
		bool share = false;
		int teamToReceiveUnitsId = -1;

		if ((args.size() >= 2) && kill) {
			share = true;
			teamToReceiveUnitsId = StringToInt(args[1], &parseFailure);

			// Validate second argument
			if (parseFailure || !teamHandler.IsActiveTeam(teamToReceiveUnitsId)) {
				LOG_L(L_WARNING, "Team to receive units: not a valid team number: \"%s\"", args[1].c_str());
				return WrongSyntax();
			}
		}

		// Additional checks over first parameter.
		if (skirmishAIHandler.GetSkirmishAIsInTeam(teamToKillId).empty()) {
			LOG_L(L_WARNING, "Team to %s: not a Skirmish AI team: %i", actionName.c_str(), teamToKillId);
			return WrongSyntax();
		}

		const std::vector<uint8_t>& teamAIs = skirmishAIHandler.GetSkirmishAIsInTeam(teamToKillId, gu->myPlayerNum);
		if (teamAIs.empty()) {
			LOG_L(L_WARNING, "Team to %s: not a local Skirmish AI team: %i", actionName.c_str(), teamToKillId);
			return WrongSyntax();
		}

		size_t skirmishAIId = teamAIs[0];
		if (skirmishAIHandler.GetSkirmishAI(skirmishAIId)->isLuaAI) {
			LOG_L(L_WARNING, "Team to %s: it is not yet supported to %s Lua AIs", actionName.c_str(), actionName.c_str());
			return WrongSyntax();
		}

		{
			const bool weAreAllied  = teamHandler.AlliedTeams(fromTeamId, teamToKillId);
			const bool weAreAIHost  = (skirmishAIHandler.GetSkirmishAI(skirmishAIId)->hostPlayer == gu->myPlayerNum);
			const bool weAreLeader  = (teamToKill->GetLeader() == gu->myPlayerNum);

			if (!(weAreAIHost || weAreLeader || singlePlayer || (weAreAllied && cheating))) {
				LOG_L(L_WARNING, "Team to %s: player %s is not allowed to %s Skirmish AI controlling team %i (try with /cheat)",
						actionName.c_str(), fromPlayer->name.c_str(), actionName.c_str(), teamToKillId);
				return WrongSyntax();
			}
		}

		if (teamToKill->isDead) {
			LOG_L(L_WARNING, "Team to %s: is a dead team already: %i", actionName.c_str(), teamToKillId);
			return WrongSyntax();
		}

		// Execute the command
		if (kill) {
			if (share) {
				clientNet->Send(CBaseNetProtocol::Get().SendGiveAwayEverything(gu->myPlayerNum, teamToReceiveUnitsId, teamToKillId));
				// when the AIs team has no units left,
				// the AI will be destroyed automatically
			} else {
				if (skirmishAIHandler.IsLocalSkirmishAI(skirmishAIId))
					skirmishAIHandler.SetLocalKillFlag(skirmishAIId, 3 /* = AI killed */);
			}
		} else {
			// reload
			clientNet->Send(CBaseNetProtocol::Get().SendAIStateChanged(gu->myPlayerNum, skirmishAIId, SKIRMAISTATE_RELOADING));
		}

		LOG("Skirmish AI controlling team %i is being %sed ...", teamToKillId, actionName.c_str());

		return true;
	}

private:
	bool kill;
};



class AIControlActionExecutor : public IUnsyncedActionExecutor {
public:
	AIControlActionExecutor() : IUnsyncedActionExecutor(
		"AIControl",
		"Creates a new instance of a Skirmish AI, to let it control a specific team"
	) {
	}

	bool WrongSyntax() const {
		LOG("description: Let a Skirmish AI take over control of a team.");
		LOG("usage:   /%s teamToControl aiShortName [aiVersion] [name] [options...]", GetCommand().c_str());
		LOG("example: /%s 1 RAI 0.601 my_RAI_Friend difficulty=2 aggressiveness=3", GetCommand().c_str());

		return true;
	}

	bool Execute(const UnsyncedAction& action) const final {
		bool badArgs = false;

		const CPlayer* fromPlayer     = playerHandler.Player(gu->myPlayerNum);
		const int      fromTeamId     = (fromPlayer != nullptr) ? fromPlayer->team : -1;

		const bool cheating           = gs->cheatEnabled;
		const bool singlePlayer       = (playerHandler.ActivePlayers() <= 1);

		std::vector<std::string> args = CSimpleParser::Tokenize(action.GetArgs());

		if (args.size() < 2) {
			LOG_L(L_WARNING, "/%s: missing mandatory arguments \"teamToControl\" and \"aiShortName\"", GetCommand().c_str());
			return WrongSyntax();
		}

		bool parseFailure;

		// Note: this seems unused.
		spring::unordered_map<std::string, std::string> aiOptions;

		// First parameter
		const int teamToControlId = StringToInt(args[0], &parseFailure);
		const CTeam* teamToControl = (!parseFailure && teamHandler.IsActiveTeam(teamToControlId)) ? teamHandler.Team(teamToControlId) : nullptr;
		if (teamToControl == nullptr) {
			LOG_L(L_WARNING, "Team to control: not a valid team number: \"%s\"", args[0].c_str());
			return WrongSyntax();
		}

		// Second parameter
		std::string aiShortName = args[1];

		// Optional parameters
		std::string aiVersion;
		if (args.size() >= 3)
			aiVersion = args[2];

		std::string aiName;
		if (args.size() >= 4)
			aiName = args[3];

		// Note: Shouldn't we parse options here..?

		{
			const bool weAreAllied  = teamHandler.AlliedTeams(fromTeamId, teamToControlId);
			const bool weAreLeader  = (teamToControl->GetLeader() == gu->myPlayerNum);
			const bool noLeader     = (!teamToControl->HasLeader());

			if (!(weAreLeader || singlePlayer || (weAreAllied && (cheating || noLeader)))) {
				LOG_L(L_WARNING, "Team to control: player %s is not allowed to let a Skirmish AI take over control of team %i (try with /cheat)",
						fromPlayer->name.c_str(), teamToControlId);
				return WrongSyntax();
			}
		}
		if (teamToControl->isDead) {
			LOG_L(L_WARNING, "Team to control: is a dead team: %i", teamToControlId);
			return WrongSyntax();
		}
		// TODO remove this, if support for multiple Skirmish AIs per team is in place
		if (!skirmishAIHandler.GetSkirmishAIsInTeam(teamToControlId).empty()) {
			LOG_L(L_WARNING, "Team to control: there is already an AI controlling this team: %i", teamToControlId);
			return WrongSyntax();
		}
		if (skirmishAIHandler.GetLocalSkirmishAIInCreation(teamToControlId) != nullptr) {
			LOG_L(L_WARNING, "Team to control: there is already an AI being created for team: %i", teamToControlId);
			return WrongSyntax();
		}

		const spring::unordered_set<std::string>& luaAIImplShortNames = skirmishAIHandler.GetLuaAIImplShortNames();
		if (luaAIImplShortNames.find(aiShortName) != luaAIImplShortNames.end()) {
			LOG_L(L_WARNING, "Team to control: it is currently not supported to initialize Lua AIs mid-game");
			return WrongSyntax();
		}

		SkirmishAIKey aiKey(aiShortName, aiVersion);
		aiKey = aiLibManager->ResolveSkirmishAIKey(aiKey);

		if (aiKey.IsUnspecified()) {
			LOG_L(L_WARNING, "Skirmish AI: not a valid Skirmish AI: %s %s", aiShortName.c_str(), aiVersion.c_str());
			return WrongSyntax();
		}

		// Execute command
		const CSkirmishAILibraryInfo& aiLibInfo = aiLibManager->GetSkirmishAIInfos().find(aiKey)->second;

		SkirmishAIData aiData;
		aiData.name       = !aiName.empty() ? aiName : aiShortName;
		aiData.team       = teamToControlId;
		aiData.hostPlayer = gu->myPlayerNum;
		aiData.shortName  = aiShortName;
		aiData.version    = aiVersion;

		for (const auto& opt: aiOptions)
			aiData.optionKeys.push_back(opt.first);

		aiData.options = aiOptions;
		aiData.isLuaAI = aiLibInfo.IsLuaAI();

		skirmishAIHandler.NetCreateLocalSkirmishAI(aiData);

		return true;
	}
};



class AIListActionExecutor : public IUnsyncedActionExecutor {
public:
	AIListActionExecutor() : IUnsyncedActionExecutor("AIList", "Prints a list of all currently active Skirmish AIs") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		const auto& ais = skirmishAIHandler.GetAllSkirmishAIs();

		if (ais.empty()) {
			LOG("<There are no active Skirmish AIs in this game>");
			return false;
		}

		LOG("%s | %s | %s | %s | %s | %s",
				"ID",
				"Team",
				"Local",
				"Lua",
				"Name",
				"(Hosting player name) or (Short name & Version)");

		for (const auto& p: ais) {
			const SkirmishAIData& aiData = *(p.second);
			const bool isLocal = (aiData.hostPlayer == gu->myPlayerNum);
			std::string lastPart;

			if (isLocal) {
				lastPart = "(Key:)  " + aiData.shortName + " " + aiData.version;
			} else {
				lastPart = "(Host:) " + playerHandler.Player(gu->myPlayerNum)->name;
			}

			LOG("%i | %i | %s | %s | %s | %s",
					p.first,
					aiData.team,
					(isLocal ? "yes" : "no "),
					(aiData.isLuaAI ? "yes" : "no "),
					aiData.name.c_str(),
					lastPart.c_str());
		}

		return true;
	}
};



class TeamActionExecutor : public IUnsyncedActionExecutor {
public:
	TeamActionExecutor() : IUnsyncedActionExecutor("Team", "Lets the local user change to another team", true) {
	}

	bool Execute(const UnsyncedAction& action) const final {
		bool parseFailure;
		const int teamId = StringToInt(action.GetArgs(), &parseFailure);

		if (!parseFailure && teamHandler.IsValidTeam(teamId)) {
			clientNet->Send(CBaseNetProtocol::Get().SendJoinTeam(gu->myPlayerNum, teamId));
		} else {
			LOG_L(L_WARNING, "[%s] team %d does not exist", __func__, teamId);
		}

		return true;
	}
};



class SpectatorActionExecutor : public IUnsyncedActionExecutor {
public:
	SpectatorActionExecutor() : IUnsyncedActionExecutor(
		"Spectator",
		"Lets the local user give up control over a team and start spectating"
	) {
	}

	bool Execute(const UnsyncedAction& action) const final {
		if (gu->spectating)
			return false;

		clientNet->Send(CBaseNetProtocol::Get().SendResign(gu->myPlayerNum));
		return true;
	}
};



class SpecTeamActionExecutor : public IUnsyncedActionExecutor {
public:
	SpecTeamActionExecutor() : IUnsyncedActionExecutor(
		"SpecTeam",
		"Lets the local user specify the team to follow if he is a spectator"
	) {
	}

	bool Execute(const UnsyncedAction& action) const final {
		if (!gu->spectating)
			return false;

		bool parseFailure;
		const int teamId = StringToInt(action.GetArgs(), &parseFailure);

		if (parseFailure || !teamHandler.IsValidTeam(teamId))
			return false;

		if (gu->myTeam == teamId)
			return true;

		gu->myTeam = teamId;
		gu->myAllyTeam = teamHandler.AllyTeam(teamId);

		CLuaUI::UpdateTeams();

		// NOTE: unsynced event
		eventHandler.PlayerChanged(gu->myPlayerNum);

		const int specFVMode =
			(gu->spectatingFullView   ? 1 : 0) +
			(gu->spectatingFullSelect ? 2 : 0);

		if (specFVMode == 0 || specFVMode == 2)
			readMap->CopySyncedToUnsynced();

		LOG("[SpecTeamAction] local client %d now spectating team %d and allyteam %d", gu->myPlayerNum, gu->myTeam, gu->myAllyTeam);
		return true;
	}
};



class SpecFullViewActionExecutor : public IUnsyncedActionExecutor {
public:
	SpecFullViewActionExecutor() : IUnsyncedActionExecutor(
		"SpecFullView",
		"Sets or toggles LOS settings if the local user is a spectator. Fullview: See everything, otherwise visibility is determined by the current team. Fullselect: Whether all units can be selected",
		false, 
		{
			{"", "Toggles both Fullview and Fullselect from current values"},
			{"0", "Not Fullview, Not Fullselect"},
			{"1", "Fullview, Not Fullselect"},
			{"2", "Not Fullview, Fullselect"},
			{"3", "Fullview, Fullselect (default)"},
		}
	) {
	}

	bool Execute(const UnsyncedAction& action) const final {
		if (!gu->spectating)
			return false;

		const int oldMode =
			(gu->spectatingFullView   ? 1 : 0) +
			(gu->spectatingFullSelect ? 2 : 0);

		if (!action.GetArgs().empty()) {
			const int mode = StringToInt(action.GetArgs());
			gu->spectatingFullView   = !!(mode & 1);
			gu->spectatingFullSelect = !!(mode & 2);
		} else {
			gu->spectatingFullView = !gu->spectatingFullView;
			gu->spectatingFullSelect = gu->spectatingFullView;
		}

		const int newMode =
			(gu->spectatingFullView   ? 1 : 0) +
			(gu->spectatingFullSelect ? 2 : 0);

		CLuaUI::UpdateTeams();

		// NOTE: unsynced event
		eventHandler.PlayerChanged(gu->myPlayerNum);
		/*
		from "View Chosen Player" (0) and "Select Any Unit"(2) to "View All"(1) and "View All & Select Any"(3)
		if we are going from global view (for example "View All") there should be no need for such update
		*/
		if ((oldMode == 0 || oldMode == 2) && (newMode == 1 || newMode == 3))
			readMap->CopySyncedToUnsynced();
		return true;
	}
};



class AllyActionExecutor : public IUnsyncedActionExecutor {
public:
	AllyActionExecutor() : IUnsyncedActionExecutor(
		"Ally",
		"Starts/Ends alliance of the local player's ally-team with another ally-team"
	) {
	}

	bool WrongSyntax() const {
		LOG_L(L_WARNING, "/%s: wrong parameters (usage: /%s <other team> [0|1])", GetCommand().c_str(), GetCommand().c_str());
		return true;
	}

	bool Execute(const UnsyncedAction& action) const final {
		if (gu->spectating)
			return false;

		if (gameSetup->fixedAllies) {
			LOG_L(L_WARNING, "In-game alliances are not allowed");
			return true;
		}

		auto args = CSimpleParser::Tokenize(action.GetArgs());
		if (args.size() < 2)
			return WrongSyntax();

		bool parseFailure;

		int otherAllyTeam = StringToInt(args[0], &parseFailure);
		if (parseFailure)
			return WrongSyntax();

		int state = StringToInt(args[1], &parseFailure);
		if (parseFailure)
			return WrongSyntax();

		if (!(state >= 0 && state < 2 && otherAllyTeam >= 0 && otherAllyTeam != gu->myAllyTeam))
			return WrongSyntax();

		clientNet->Send(CBaseNetProtocol::Get().SendSetAllied(gu->myPlayerNum, otherAllyTeam, state));

		return true;
	}
};



class GroupActionExecutor : public IUnsyncedActionExecutor {
public:
	GroupActionExecutor() : IUnsyncedActionExecutor("Group", "Manage control groups", false, {
			{"<n>", "Select group <n>, also focuses on second call (deprecated)"},
			{"select <n>", "Select group <n>"},
			{"focus <n>", "Focus camera on group <n>"},
			{"set <n>", "Set current selected units as group <n>"},
			{"add <n>", "Add current selected units to group <n>"},
			{"unset", "Deassign control group for currently selected units"},
			{"selectadd <n>", "Add members from group <n> to currently selected units"},
			{"selectclear <n>", "Remove members from group <n> from currently selected units"},
			{"selecttoggle <n>", "Toggle members from group <n> from currently selected units"},
			}) {
	}

	bool WrongSyntax(std::string description = "wrong syntax") const {
		LOG_L(L_WARNING, "/%s error: %s", GetCommand().c_str(), description.c_str());
		return false;
	}

	bool Execute(const UnsyncedAction& action) const final {
		if (action.IsRepeat())
			return false;

		const auto args = CSimpleParser::Tokenize(action.GetArgs());

		if (args.size() == 0)
			return WrongSyntax();

		std::string subCommand = "";
		int groupId;
		bool parseFailure = true;

		switch (args.size()) {
			case 1:
				if (args[0] == "unset") {
					selectedUnitsHandler.SetGroup(nullptr);
					return true;
				}
				groupId = StringToInt(args[0], &parseFailure);
				break;
			case 2:
				subCommand = args[0];
				groupId = StringToInt(args[1], &parseFailure);
				break;
			default:
				return WrongSyntax();
		};

		if (parseFailure)
			return WrongSyntax();
		// This check is important because GroupCommand doesn't check the range
		// and we can go OOB.
		if (groupId < 0 || groupId > 9)
			return WrongSyntax("groupId must be single digit number");

		// Finally, actually run the command.
		bool error;
		const bool halt = uiGroupHandlers[gu->myTeam].GroupCommand(groupId, subCommand, error);

		if (error)
			return WrongSyntax("subcommand " + subCommand + " not found");

		return halt;
	}
};



class GroupIDActionExecutor : public IUnsyncedActionExecutor {
public:
	GroupIDActionExecutor(int _groupId): IUnsyncedActionExecutor(
		"Group" + IntToString(_groupId),
		"Allows modifying the members of group " + IntToString(_groupId)
	) {
		groupId = _groupId;
	}

	bool Execute(const UnsyncedAction& action) const final {
		if (action.IsRepeat())
			return false;

		return uiGroupHandlers[gu->myTeam].GroupCommand(groupId);
	}

private:
	int groupId;
};



class LastMessagePositionActionExecutor : public IUnsyncedActionExecutor {
public:
	LastMessagePositionActionExecutor() : IUnsyncedActionExecutor(
		"LastMsgPos",
		"Moves the camera to show the position of the last message"
	) {
	}

	bool Execute(const UnsyncedAction& action) const final {
		if (infoConsole->GetMsgPosCount() == 0)
			return false;

		// cycle through the positions
		camHandler->CameraTransition(0.6f);
		camHandler->GetCurrentController().SetPos(infoConsole->GetMsgPos());
		return true;
	}
};



class ChatActionExecutor : public IUnsyncedActionExecutor {
public:
	ChatActionExecutor(
		const std::string& _commandPostfix,
		const std::string& _userInputPrefix,
		bool _setUserInputPrefix
	): IUnsyncedActionExecutor(
		"Chat" + _commandPostfix,
		"Starts waiting for intput to be sent to " + _commandPostfix
	),
		userInputPrefix(_userInputPrefix),
		setUserInputPrefix(_setUserInputPrefix)
	{}

public:
	bool Execute(const UnsyncedAction& action) const final {
		SDL_StartTextInput();

		gameTextInput.PromptInput(setUserInputPrefix? &userInputPrefix: nullptr);
		gameConsoleHistory.ResetPosition();
		inMapDrawer->SetDrawMode(false);
		return true;
	}

private:
	const std::string userInputPrefix;
	const bool setUserInputPrefix;
};


class TrackActionExecutor : public IUnsyncedActionExecutor {
public:
	TrackActionExecutor() : IUnsyncedActionExecutor("Track",
			"Start/stop following the selected unit(s) with the camera", false, {
			{"", "Toggles tracking"},
			{"<on|off>", "Set tracking <on|off> <unitID unitID ...>"},
			}) {}

	bool Execute(const UnsyncedAction& action) const final {
		auto args = CSimpleParser::Tokenize(action.GetArgs());
		bool enableTracking = unitTracker.Enabled();
		std::vector<int> unitIDs = {};

		switch (args.size())
		{
			case 0:
				InverseOrSetBool(enableTracking, ""); break;
			case 1:
				InverseOrSetBool(enableTracking, args.at(0)); break;
			default: {
				InverseOrSetBool(enableTracking, args.at(0));
				if (enableTracking) {
					for (size_t i = 1; i < args.size(); ++i)
						unitIDs.emplace_back(StringToInt(args.at(i)));
				}
			} break;
		}

		if (enableTracking)
			unitTracker.Track(std::move(unitIDs));
		else
			unitTracker.Disable();

		return true;
	}
};

class TrackModeActionExecutor : public IUnsyncedActionExecutor {
public:
	TrackModeActionExecutor() : IUnsyncedActionExecutor("TrackMode",
			"Shift through different ways of following selected unit(s)") {}

	bool Execute(const UnsyncedAction& action) const final {
		unitTracker.IncMode();
		return true;
	}
};



class PauseActionExecutor : public IUnsyncedActionExecutor {
public:
	PauseActionExecutor() : IUnsyncedActionExecutor("Pause",
			"Pause/Unpause the game",
			false, {
			{"", "Toggles tracking"},
			{"<on|off>", "Set tracking <on|off>"},
			}) {}

	bool Execute(const UnsyncedAction& action) const final {
		// disallow pausing prior to start of game proper
		if (!game->playing)
			return false;

		// do not need to update lastReadNetTime, gets
		// done when NETMSG_PAUSE makes the round-trip
		InverseOrSetBool(game->paused, action.GetArgs());
		clientNet->Send(CBaseNetProtocol::Get().SendPause(gu->myPlayerNum, game->paused));
		return true;
	}

};



class DebugActionExecutor : public IUnsyncedActionExecutor {
public:
	DebugActionExecutor() : IUnsyncedActionExecutor("Debug", "Enable/Disable debug rendering mode") {}

	bool Execute(const UnsyncedAction& action) const final {
		auto& profiler = CTimeProfiler::GetInstance();
		bool drawDebug = false;
		bool draw4Real = false;
		bool changeSorting = false;

		auto args = CSimpleParser::Tokenize(action.GetArgs());

		if (args.size() == 0) {
			drawDebug = !globalRendering->drawDebug;
			draw4Real = drawDebug;
		}
		if (args.size() > 0) {
			if (args[0] == "reset") {
				ProfileDrawer::SetEnabled(false);
				profiler.SetEnabled(false);
				profiler.ResetState();
				profiler.SetEnabled(globalRendering->drawDebug);
				ProfileDrawer::SetEnabled(globalRendering->drawDebug);
				return true;
			}
			else if(args[0] == "sort") {
				drawDebug = globalRendering->drawDebug;
				draw4Real = drawDebug;
				changeSorting = true;
				profiler.SetSortingType(CTimeProfiler::SortType::ST_ALPHABETICAL); //default
			}
			else {
				drawDebug = StringToBool(args[0]);
				draw4Real = drawDebug;
			}
		}
		if (args.size() > 1) {
			if (changeSorting) {
				StringToLowerInPlace(args[1]);
				switch (hashString(args[1].c_str()))
				{
				case hashString("default"): [[fallthrough]];
				case hashString("abc"): [[fallthrough]];
				case hashString("alphabetical"): {
					profiler.SetSortingType(CTimeProfiler::SortType::ST_ALPHABETICAL);
				} break;
				case hashString("totaltime"): [[fallthrough]];
				case hashString("total"): {
					profiler.SetSortingType(CTimeProfiler::SortType::ST_TOTALTIME);
				} break;
				case hashString("currenttime"): [[fallthrough]];
				case hashString("curr"): [[fallthrough]];
				case hashString("cur"): [[fallthrough]];
				case hashString("current"): {
					profiler.SetSortingType(CTimeProfiler::SortType::ST_CURRENTTIME);
				} break;
				case hashString("maxtime"): [[fallthrough]];
				case hashString("max"): {
					profiler.SetSortingType(CTimeProfiler::SortType::ST_MAXTIME);
				} break;
				case hashString("lag"): {
					profiler.SetSortingType(CTimeProfiler::SortType::ST_LAG);
				} break;
				default: {} break;
				}

				return true;
			}
			else {
				draw4Real = StringToBool(args[1]);
			}
		}


		globalRendering->drawDebug = drawDebug;

		if (draw4Real && globalRendering->drawDebug) {
			if (!ProfileDrawer::IsEnabled())
				ProfileDrawer::SetEnabled(true);
		}
		else {
			if (ProfileDrawer::IsEnabled())
				ProfileDrawer::SetEnabled(false);
		}

		profiler.SetEnabled(globalRendering->drawDebug);
		return true;
	}
};

class DebugCubeMapActionExecutor : public IUnsyncedActionExecutor {
public:
	DebugCubeMapActionExecutor() : IUnsyncedActionExecutor("DebugCubeMap", "Use debug cubemap texture instead of the sky") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		globalRendering->drawDebugCubeMap = !globalRendering->drawDebugCubeMap;
		ISky::SetSky();
		return true;
	}
};

class DebugQuadFieldActionExecutor : public IUnsyncedActionExecutor {
public:
	DebugQuadFieldActionExecutor() : IUnsyncedActionExecutor("DebugQuadField", "Draw quadfield sectors around GuiTraceRay and selected units") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		bool enabled = DebugDrawerQuadField::IsEnabled();
		DebugDrawerQuadField::SetEnabled(!enabled);
		LogSystemStatus("quadfield debug", !enabled);
		return true;
	}
};

class DrawSkyActionExecutor : public IUnsyncedActionExecutor {
public:
	DrawSkyActionExecutor() : IUnsyncedActionExecutor("DrawSky", "Whether to actually draw sky") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		globalRendering->drawSky = !globalRendering->drawSky;
		return true;
	}
};

class DebugGLActionExecutor : public IUnsyncedActionExecutor {
public:
	DebugGLActionExecutor() : IUnsyncedActionExecutor("DebugGL", "Enable/Disable OpenGL debug-context output") {}

	bool Execute(const UnsyncedAction& action) const final {
		bool debugEnabled = !globalRendering->glDebug;

		uint32_t msgSrceIdx = 0;
		uint32_t msgTypeIdx = 0;
		uint32_t msgSevrIdx = 0;

		auto args = CSimpleParser::Tokenize(action.GetArgs());

		if (args.size() > 0) {
			int options = StringToInt(args[0]);
			if (options > 1) {
				debugEnabled = !!(options & (1 << 1));
				configHandler->Set("DebugGLReportGroups", !!(options & (1 << 2)), true);
				configHandler->Set("DebugGLStacktraces",  !!(options & (1 << 3)), true);
			}
			else {
				debugEnabled = !!(options & 1);
			}
		}

		if (args.size() > 1)
			msgSrceIdx = StringToInt(args[1]);
		if (args.size() > 2)
			msgTypeIdx = StringToInt(args[2]);
		if (args.size() > 3)
			msgSevrIdx = StringToInt(args[3]);

		globalRendering->glDebug = debugEnabled;
		globalRendering->ToggleGLDebugOutput(msgSrceIdx, msgTypeIdx, msgSevrIdx);

		return true;
	}
};

class DebugGLErrorsActionExecutor : public IUnsyncedActionExecutor {
public:
	DebugGLErrorsActionExecutor() : IUnsyncedActionExecutor("DebugGLErrors", "Enable/Disable OpenGL debug-errors") {}

	bool Execute(const UnsyncedAction& action) const final {
		LogSystemStatus("GL debug-errors", globalRendering->glDebugErrors = !globalRendering->glDebugErrors);
		return true;
	}
};


class MuteActionExecutor : public IUnsyncedActionExecutor {
public:
	MuteActionExecutor() : IUnsyncedActionExecutor("MuteSound", "Mute/Unmute the current sound system") {}

	bool Execute(const UnsyncedAction& action) const final {
		// toggle
		sound->Mute();
		LogSystemStatus("Mute", sound->IsMuted());
		return true;
	}
};

class SoundActionExecutor : public IUnsyncedActionExecutor {
public:
	SoundActionExecutor() : IUnsyncedActionExecutor("SoundDevice",
			"Switch the sound output system (currently only OpenAL / NullAudio)") {}

	bool Execute(const UnsyncedAction& action) const final {
		// toggle
		LogSystemStatus("Sound", !sound->ChangeOutput());
		return true;
	}
};




class SoundChannelEnableActionExecutor : public IUnsyncedActionExecutor {
public:
	SoundChannelEnableActionExecutor() : IUnsyncedActionExecutor(
		"SoundChannelEnable",
		"Enable/Disable specific sound channels: UnitReply, General, Battle, UserInterface, Music"
	) {}

	bool Execute(const UnsyncedAction& action) const final {
		auto args = CSimpleParser::Tokenize(action.GetArgs());

		if (args.size() < 2) {
			LOG_L(L_WARNING, "/%s: wrong syntax (which is '/%s channelName 0/1')", GetCommand().c_str(), GetCommand().c_str());
			return true;
		}

		std::string channel = std::move(args[0]);
		bool enable = StringToBool(args[1]);

		if (channel == "UnitReply")
			Channels::UnitReply->Enable(enable);
		else if (channel == "General")
			Channels::General->Enable(enable);
		else if (channel == "Battle")
			Channels::Battle->Enable(enable);
		else if (channel == "UserInterface")
			Channels::UserInterface->Enable(enable);
		else if (channel == "Music")
			Channels::BGMusic->Enable(enable);
		else
			LOG_L(L_WARNING, "/%s: wrong channel name \"%s\"", GetCommand().c_str(), channel.c_str());

		return true;
	}
};



class CreateVideoActionExecutor : public IUnsyncedActionExecutor {
public:
	CreateVideoActionExecutor() : IUnsyncedActionExecutor("CreateVideo", "Start/Stop capturing a video of the game in progress") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		// toggle
		videoCapturing->SetCapturing(!videoCapturing->IsCapturing());
		LogSystemStatus("Video capturing", videoCapturing->IsCapturing());
		return true;
	}
};

class NetPingActionExecutor : public IUnsyncedActionExecutor {
public:
	NetPingActionExecutor() : IUnsyncedActionExecutor("NetPing", "Send a ping request to the server") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		// tag=0 if no args
		clientNet->Send(CBaseNetProtocol::Get().SendPing(gu->myPlayerNum, StringToInt(action.GetArgs()), spring_tomsecs(spring_now())));
		return true;
	}
};

class NetMsgSmoothingActionExecutor : public IUnsyncedActionExecutor {
public:
	NetMsgSmoothingActionExecutor() : IUnsyncedActionExecutor(
		"NetMsgSmoothing",
		"Toggles whether client will use net-message smoothing; better for unstable connections"
	) {
	}

	bool Execute(const UnsyncedAction& action) const final {
		const char* fmt = "net-message smoothing %s";
		const char* strs[] = {"disabled", "enabled"};

		LOG(fmt, strs[globalConfig.useNetMessageSmoothingBuffer = !globalConfig.useNetMessageSmoothingBuffer]);
		return true;
	}
};

class SpeedControlActionExecutor : public IUnsyncedActionExecutor {
public:
	SpeedControlActionExecutor() : IUnsyncedActionExecutor(
		"SpeedControl",
		"Sets how server adjusts speed according to player's CPU load, 1: use average, 0: use highest"
	) {
	}

	bool Execute(const UnsyncedAction& action) const final {
		if (gameServer == nullptr)
			return false;

		int speedCtrl = game->speedControl;

		if (action.GetArgs().empty()) {
			// switch to next value (0 <-> 1);
			speedCtrl = (speedCtrl == 0) ? 1 : 0;
		} else {
			// set value
			speedCtrl = std::clamp(StringToInt(action.GetArgs()), 0, 1);
		}

		// constrain to bounds
		gameServer->UpdateSpeedControl(game->speedControl = speedCtrl);
		return true;
	}
};


class LuaGarbageCollectControlExecutor: public IUnsyncedActionExecutor {
public:
	LuaGarbageCollectControlExecutor() : IUnsyncedActionExecutor(
		"LuaGCControl",
		"Toggle between 1/f and 30/s Lua garbage collection rate"
	) {}

	bool Execute(const UnsyncedAction& action) const final {
		constexpr const char* strs[] = {"1/f", "30/s"};

		const std::string& args = action.GetArgs();

		if (!args.empty()) {
			LOG("Lua garbage collection rate: %s", strs[game->luaGCControl = std::clamp(StringToInt(args), 0, 1)]);
		} else {
			LOG("Lua garbage collection rate: %s", strs[game->luaGCControl = 1 - game->luaGCControl]);
		}

		return true;
	}
};




class GameInfoActionExecutor : public IUnsyncedActionExecutor {
public:
	GameInfoActionExecutor() : IUnsyncedActionExecutor("GameInfo", "Enables/Disables game-info panel rendering") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		if (!action.IsRepeat()) {
			if (!CGameInfo::IsActive()) {
				CGameInfo::Enable();
			} else {
				CGameInfo::Disable();
			}
		}

		return true;
	}
};


class GameInfoCloseActionExecutor : public IUnsyncedActionExecutor {
public:
	GameInfoCloseActionExecutor() : IUnsyncedActionExecutor("GameInfoClose", "Closes game info") {}

	bool Execute(const UnsyncedAction& action) const final {
		// TODO: false for backwards compatibility (may or may not make sense to keep it, probably not)
		return false;
	}

	bool ExecuteRelease(const UnsyncedAction& action) const final {
		if (CGameInfo::IsActive()) {
			// TODO: handled in release, and return false for backwards compatibility
			// (needs in depth review)
			CGameInfo::Disable();
			return false;
		}
		return false;
	}
};



class HideInterfaceActionExecutor : public IUnsyncedActionExecutor {
public:
	HideInterfaceActionExecutor() : IUnsyncedActionExecutor("HideInterface", "Hide/Show the GUI controlls") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		InverseOrSetBool(game->hideInterface, action.GetArgs());
		return true;
	}
};



class HardwareCursorActionExecutor : public IUnsyncedActionExecutor {
public:
	HardwareCursorActionExecutor() : IUnsyncedActionExecutor("HardwareCursor", "Enables/Disables hardware mouse-cursor support") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		const bool enable = StringToBool(action.GetArgs());
		mouse->ToggleHwCursor(enable);
		configHandler->Set("HardwareCursor", enable);
		LogSystemStatus("Hardware mouse-cursor", enable);
		return true;
	}
};



class FullscreenActionExecutor : public IUnsyncedActionExecutor {
public:
	FullscreenActionExecutor() : IUnsyncedActionExecutor("Fullscreen",
			"Switches fullscreen mode") {}

	bool Execute(const UnsyncedAction& action) const final {
		bool b = globalRendering->fullScreen;
		InverseOrSetBool(b, action.GetArgs());

		configHandler->Set("Fullscreen", b);
		return true;
	}
};

class WindowBorderlessActionExecutor : public IUnsyncedActionExecutor {
public:
	WindowBorderlessActionExecutor() : IUnsyncedActionExecutor("WindowBorderless",
		"Switches borderless/decorated mode") {}

	bool Execute(const UnsyncedAction& action) const final {
		bool b = globalRendering->borderless;
		InverseOrSetBool(b, action.GetArgs());

		configHandler->Set("WindowBorderless", b);
		return true;
	}
};




class IncreaseViewRadiusActionExecutor : public IUnsyncedActionExecutor {
public:
	IncreaseViewRadiusActionExecutor() : IUnsyncedActionExecutor("IncreaseViewRadius", "Increase terrain tessellation level") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		readMap->GetGroundDrawer()->IncreaseDetail();
		return true;
	}
};

class DecreaseViewRadiusActionExecutor : public IUnsyncedActionExecutor {
public:
	DecreaseViewRadiusActionExecutor() : IUnsyncedActionExecutor("DecreaseViewRadius",
			"Decrease the view radius (higher performance, uglier view)") {}

	bool Execute(const UnsyncedAction& action) const final {
		readMap->GetGroundDrawer()->DecreaseDetail();
		return true;
	}
};



class GroundDetailActionExecutor : public IUnsyncedActionExecutor {
public:
	GroundDetailActionExecutor() : IUnsyncedActionExecutor("GroundDetail",
			"Set the level of ground detail") {}

	bool Execute(const UnsyncedAction& action) const final {
		int detail;
		if (action.GetArgs().empty()) {
			LOG_L(L_WARNING, "/%s: missing argument", GetCommand().c_str());
			return false;
		}
		detail = StringToInt((action.GetArgs()).c_str());

		readMap->GetGroundDrawer()->SetDetail(detail);
		return true;
	}

};



class MoreCloudsActionExecutor : public IUnsyncedActionExecutor {
public:
	MoreCloudsActionExecutor() : IUnsyncedActionExecutor("MoreClouds",
			"Increases the density of clouds (lower performance)") {}

	bool Execute(const UnsyncedAction& action) const final {
		ISky::GetSky()->IncreaseCloudDensity();
		ReportCloudDensity();
		return true;
	}

	static void ReportCloudDensity() {
		LOG("Cloud density %f", 1.0f / ISky::GetSky()->GetCloudDensity());
	}
};


class LessCloudsActionExecutor : public IUnsyncedActionExecutor {
public:
	LessCloudsActionExecutor() : IUnsyncedActionExecutor("LessClouds",
			"Decreases the density of clouds (higher performance)") {}

	bool Execute(const UnsyncedAction& action) const final {
		ISky::GetSky()->DecreaseCloudDensity();
		MoreCloudsActionExecutor::ReportCloudDensity();
		return true;
	}
};

class RotateSkyActionExecutor : public IUnsyncedActionExecutor {
public:
	RotateSkyActionExecutor() : IUnsyncedActionExecutor("RotateSky",
		"Rotates the sky around axis by angle") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		auto args = CSimpleParser::Tokenize(action.GetArgs());
		if (args.size() != 4)
			return false;

		const auto axisAngle = float4{
			StringToInt<float>(args[0]),
			StringToInt<float>(args[1]),
			StringToInt<float>(args[2]),
			StringToInt<float>(args[3])
		};

		ISky::GetSky()->SetSkyAxisAngle(axisAngle);
		return true;
	}
};

class FeatureFadeDistActionExecutor : public IUnsyncedActionExecutor {
public:
	FeatureFadeDistActionExecutor(): IUnsyncedActionExecutor("FeatureFadeDistance", "") {}

	bool Execute(const UnsyncedAction& action) const final {
		featureDrawer->ConfigNotify(action.GetCmd(), action.GetArgs());
		return true;
	}
};

class FeatureDrawDistActionExecutor : public IUnsyncedActionExecutor {
public:
	FeatureDrawDistActionExecutor(): IUnsyncedActionExecutor("FeatureDrawDistance", "") {}

	bool Execute(const UnsyncedAction& action) const final {
		featureDrawer->ConfigNotify(action.GetCmd(), action.GetArgs());
		return true;
	}
};


class SpeedUpActionExecutor : public IUnsyncedActionExecutor {
public:
	SpeedUpActionExecutor() : IUnsyncedActionExecutor("SpeedUp",
			"Increases the simulation speed."
			" The engine will try to simulate more frames per second") {}

	bool Execute(const UnsyncedAction& action) const final {
		float speed = gs->wantedSpeedFactor;
		if (speed < 5) {
			speed += (speed < 2) ? 0.1f : 0.2f;
			float fPart = speed - (int)speed;
			if (fPart < 0.01f || fPart > 0.99f)
				speed = math::round(speed);
		} else if (speed < 10) {
			speed += 0.5f;
		} else {
			speed += 1.0f;
		}
		clientNet->Send(CBaseNetProtocol::Get().SendUserSpeed(gu->myPlayerNum, speed));
		return true;
	}
};



class SlowDownActionExecutor : public IUnsyncedActionExecutor {
public:
	SlowDownActionExecutor() : IUnsyncedActionExecutor(
		"SlowDown",
		"Decreases the simulation speed. The engine will try to simulate less frames per second"
	) {}

	bool Execute(const UnsyncedAction& action) const final {
		int index = 0;

		float speed = gs->wantedSpeedFactor;
		float fract = 0.0f;

		constexpr float range[] = {2.0f, 5.0f, 10.0f, std::numeric_limits<float>::max()};
		constexpr float steps[] = {0.1f, 0.2f, 0.5f, 1.0f};

		while (index < 4 && speed > range[index])
			index++;

		speed -= steps[index];
		fract  = speed - std::floor(speed);

		clientNet->Send(CBaseNetProtocol::Get().SendUserSpeed(gu->myPlayerNum, mix(speed, std::round(speed), fract < 0.01f || fract > 0.99f)));
		return true;
	}
};

class SetGamespeedActionExecutor : public IUnsyncedActionExecutor {
public:
	SetGamespeedActionExecutor() : IUnsyncedActionExecutor(
		"SetSpeed",
		"Set the simulation speed to any positive value, bounded by the minimum and maximum game speed settings."
	) {}

	bool Execute(const UnsyncedAction& action) const final {
		if ((action.GetArgs()).empty())
			return false;

		bool parseFailure;
		const float speed = StringToInt<float>(action.GetArgs(), &parseFailure);

		if (parseFailure || speed <= 0.0)
			return false;

		clientNet->Send(CBaseNetProtocol::Get().SendUserSpeed(gu->myPlayerNum, speed));
		return true;
	}
};

class ControlUnitActionExecutor : public IUnsyncedActionExecutor {
public:
	ControlUnitActionExecutor() : IUnsyncedActionExecutor("ControlUnit", "Start to first-person-control a unit") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		if (gu->spectating)
			return false;

		// we must cause the to-be-controllee to be put in
		// netSelected[myPlayerNum] by giving it an order
		selectedUnitsHandler.SendCommand(Command(CMD_STOP));
		clientNet->Send(CBaseNetProtocol::Get().SendDirectControl(gu->myPlayerNum));
		return true;
	}
};



class ShowStandardActionExecutor : public IUnsyncedActionExecutor {
public:
	ShowStandardActionExecutor() : IUnsyncedActionExecutor("ShowStandard", "Disable rendering of all auxiliary map overlays") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		infoTextureHandler->SetMode("");
		return true;
	}
};

class ShowElevationActionExecutor : public IUnsyncedActionExecutor {
public:
	ShowElevationActionExecutor() : IUnsyncedActionExecutor("ShowElevation", "Enable rendering of the auxiliary height-map overlay") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		infoTextureHandler->ToggleMode("height");
		return true;
	}
};

class ShowMetalMapActionExecutor : public IUnsyncedActionExecutor {
public:
	ShowMetalMapActionExecutor() : IUnsyncedActionExecutor("ShowMetalMap", "Enable rendering of the auxiliary metal-map overlay") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		infoTextureHandler->ToggleMode("metal");
		return true;
	}
};

class ShowPathTravActionExecutor : public IUnsyncedActionExecutor {
public:
	ShowPathTravActionExecutor() : IUnsyncedActionExecutor("ShowPathTraversability", "Enable rendering of the path traversability-map overlay") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		CPathTexture* pathTexInfo = dynamic_cast<CPathTexture*>(infoTextureHandler->GetInfoTexture("path"));

		if (pathTexInfo != nullptr)
			pathTexInfo->ShowMoveDef(-1);

		infoTextureHandler->ToggleMode("path");
		return true;
	}
};

class ShowPathHeatActionExecutor : public IUnsyncedActionExecutor {
public:
	ShowPathHeatActionExecutor() : IUnsyncedActionExecutor("ShowPathHeat", "Enable/Disable rendering of the path heat-map overlay", true) {
	}

	bool Execute(const UnsyncedAction& action) const final {
		infoTextureHandler->ToggleMode("heat");
		return true;
	}
};

class ShowPathFlowActionExecutor : public IUnsyncedActionExecutor {
public:
	ShowPathFlowActionExecutor() : IUnsyncedActionExecutor("ShowPathFlow", "Enable/Disable rendering of the path flow-map overlay", true) {
	}

	bool Execute(const UnsyncedAction& action) const final {
		infoTextureHandler->ToggleMode("flow");
		return true;
	}
};

class ShowPathCostActionExecutor : public IUnsyncedActionExecutor {
public:
	ShowPathCostActionExecutor() : IUnsyncedActionExecutor("ShowPathCost", "Enable rendering of the path cost-map overlay", true) {
	}

	bool Execute(const UnsyncedAction& action) const final {
		infoTextureHandler->ToggleMode("pathcost");
		return true;
	}
};

class ToggleLOSActionExecutor : public IUnsyncedActionExecutor {
public:
	ToggleLOSActionExecutor() : IUnsyncedActionExecutor("ToggleLOS", "Enable rendering of the auxiliary LOS-map overlay") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		infoTextureHandler->ToggleMode("los");
		return true;
	}
};

class ToggleInfoActionExecutor : public IUnsyncedActionExecutor {
public:
	ToggleInfoActionExecutor() : IUnsyncedActionExecutor("ToggleInfo", "Toggles current info texture view") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		infoTextureHandler->ToggleMode(action.GetArgs());
		return true;
	}
};


class ShowPathTypeActionExecutor : public IUnsyncedActionExecutor {
public:
	ShowPathTypeActionExecutor() : IUnsyncedActionExecutor(
		"ShowPathType",
		"Shows path traversability for a given MoveDefName, MoveDefID or UnitDefName"
	) {
	}

	bool Execute(const UnsyncedAction& action) const final {
		CPathTexture* pathTexInfo = dynamic_cast<CPathTexture*>(infoTextureHandler->GetInfoTexture("path"));

		if (pathTexInfo != nullptr) {
			bool shown = false;
			bool failed = false;

			if (!action.GetArgs().empty()) {
				unsigned int i = StringToInt(action.GetArgs(), &failed);

				if (failed)
					i = -1;

				const MoveDef* md = moveDefHandler.GetMoveDefByName(action.GetArgs());
				const UnitDef* ud = unitDefHandler->GetUnitDefByName(action.GetArgs());

				if (md == nullptr && i < moveDefHandler.GetNumMoveDefs())
					md = moveDefHandler.GetMoveDefByPathType(i);

				shown = (md != nullptr || ud != nullptr);

				if (md != nullptr) {
					pathTexInfo->ShowMoveDef(md->pathType);
					LOG("Showing PathView for MoveDef: %s", md->name.c_str());
				} else {
					if (ud != nullptr) {
						pathTexInfo->ShowUnitDef(ud->id);
						LOG("Showing BuildView for UnitDef: %s", ud->name.c_str());
					}
				}
			}

			if (!shown) {
				pathTexInfo->ShowMoveDef(-1);
				LOG("Switching back to automatic PathView");
			} else if (infoTextureHandler->GetMode() != "path") {
				infoTextureHandler->ToggleMode("path");
			}
		}

		return true;
	}
};



class ShareDialogActionExecutor : public IUnsyncedActionExecutor {
public:
	ShareDialogActionExecutor() : IUnsyncedActionExecutor(
		"ShareDialog",
		"Opens the share dialog for sending units and resources to other players"
	) {
	}

	bool Execute(const UnsyncedAction& action) const final {
		if (gu->spectating)
			return false;

		// already shown?
		const auto& inputReceivers = CInputReceiver::GetReceivers();

		if (inputReceivers.empty() || (dynamic_cast<CShareBox*>(inputReceivers.front()) != nullptr))
			return false;

		new CShareBox();
		return true;
	}
};



class QuitMessageActionExecutor : public IUnsyncedActionExecutor {
public:
	QuitMessageActionExecutor() : IUnsyncedActionExecutor(
		"QuitMessage",
		"Deprecated, see /Quit instead (was used to quit the game immediately)"
	) {
	}

	bool Execute(const UnsyncedAction& action) const final {
		// already shown?
		const auto& inputReceivers = CInputReceiver::GetReceivers();

		if (inputReceivers.empty() || dynamic_cast<CQuitBox*>(inputReceivers.front()) != nullptr)
			return false;

		const CKeyBindings::HotkeyList& quitList = keyBindings.GetHotkeys("quitmenu");
		const std::string& quitKey = quitList.empty() ? "<none>" : *quitList.begin();

		LOG("Press %s to access the quit menu", quitKey.c_str());
		return true;
	}
};



class QuitMenuActionExecutor : public IUnsyncedActionExecutor {
public:
	QuitMenuActionExecutor() : IUnsyncedActionExecutor("QuitMenu", "Opens the quit-menu, if it is not already open") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		// already shown?
		const auto& inputReceivers = CInputReceiver::GetReceivers();

		if (inputReceivers.empty() || dynamic_cast<CQuitBox*>(inputReceivers.front()) != nullptr)
			return false;

		new CQuitBox();
		return true;
	}
};



class QuitActionExecutor : public IUnsyncedActionExecutor {
public:
	QuitActionExecutor() : IUnsyncedActionExecutor("QuitForce", "Exits game to system") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		LOG("[QuitAction] user exited to system");

		gu->globalQuit = true;
		return true;
	}
};

class ReloadActionExecutor : public IUnsyncedActionExecutor {
public:
	ReloadActionExecutor() : IUnsyncedActionExecutor("ReloadForce", "Exits game to menu") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		LOG("[ReloadAction] user exited to menu");

		gameSetup->reloadScript = "";
		gu->globalReload = true;
		return true;
	}
};



class IncreaseGUIOpacityActionExecutor : public IUnsyncedActionExecutor {
public:
	IncreaseGUIOpacityActionExecutor() : IUnsyncedActionExecutor("IncGUIOpacity",
			"Increases the the opacity(see-through-ness) of GUI elements") {}

	bool Execute(const UnsyncedAction& action) const final {

		CInputReceiver::guiAlpha = std::min(CInputReceiver::guiAlpha + 0.1f, 1.0f);
		configHandler->Set("GuiOpacity", CInputReceiver::guiAlpha);
		return true;
	}
};



class DecreaseGUIOpacityActionExecutor : public IUnsyncedActionExecutor {
public:
	DecreaseGUIOpacityActionExecutor() : IUnsyncedActionExecutor("DecGUIOpacity",
			"Decreases the the opacity(see-through-ness) of GUI elements") {}

	bool Execute(const UnsyncedAction& action) const final {

		CInputReceiver::guiAlpha = std::max(CInputReceiver::guiAlpha - 0.1f, 0.0f);
		configHandler->Set("GuiOpacity", CInputReceiver::guiAlpha);
		return true;
	}
};



class ScreenShotActionExecutor : public IUnsyncedActionExecutor {
public:
	ScreenShotActionExecutor() : IUnsyncedActionExecutor("ScreenShot", "Take a screen-shot of the current view") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		auto args = CSimpleParser::Tokenize(action.GetArgs());

		if (args.size() == 0) {
			TakeScreenshot("", 80);
		} else {
			int quality = args.size() > 1 ? StringToInt(args[1]) : 80;
			LOG_L(L_WARNING, "/%s: quality = %d", GetCommand().c_str(), quality);
			quality = std::clamp(quality, 1, 99);
			TakeScreenshot(args[0], quality);
		}
		return true;
	}
};



class GrabInputActionExecutor : public IUnsyncedActionExecutor {
public:
	GrabInputActionExecutor() : IUnsyncedActionExecutor("GrabInput", "Prevents/Enables the mouse from leaving the game window (windowed mode only)") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		const std::string& args = action.GetArgs();

		if (mouse->offscreen)
			return false;

		if (args.empty()) {
			LogSystemStatus("Input grabbing", globalRendering->ToggleWindowInputGrabbing());
		} else {
			const bool preState = globalRendering->GetWindowInputGrabbing();
			const bool reqState = StringToBool(args);
			if (reqState != preState)
				LogSystemStatus("Input grabbing", globalRendering->SetWindowInputGrabbing(reqState));
			else
				globalRendering->SetWindowInputGrabbing(reqState);
		}

		return true;
	}
};



class ClockActionExecutor : public IUnsyncedActionExecutor {
public:
	ClockActionExecutor() : IUnsyncedActionExecutor("Clock", "Shows a small digital clock indicating the local time") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		InverseOrSetBool(game->showClock, action.GetArgs());
		configHandler->Set("ShowClock", game->showClock ? 1 : 0);
		LogSystemStatus("small digital clock", game->showClock);
		return true;
	}
};



class CrossActionExecutor : public IUnsyncedActionExecutor {
public:
	CrossActionExecutor() : IUnsyncedActionExecutor(
		"Cross",
		"Allows one to exchange and modify the appearance of the"
		" cross/mouse-pointer in first-person-control view"
	) {
	}

	bool Execute(const UnsyncedAction& action) const final {
		if (action.GetArgs().empty()) {
			if (mouse->crossSize > 0.0f) {
				mouse->crossSize = -mouse->crossSize;
			} else {
				mouse->crossSize = std::max(1.0f, -mouse->crossSize);
			}
		} else {
			auto args = CSimpleParser::Tokenize(action.GetArgs());

			const float size = StringToInt<float>(args[0]);

			if (args.size() > 1) {
				const float alpha = StringToInt<float>(args[1]);
				configHandler->Set("CrossAlpha", mouse->crossAlpha = alpha);
			}
			if (args.size() > 2) {
				const float scale = StringToInt<float>(args[2]);
				configHandler->Set("CrossMoveScale", mouse->crossMoveScale = scale);
			}

			mouse->crossSize = size;
		}

		configHandler->Set("CrossSize", mouse->crossSize);
		return true;
	}
};



class FPSActionExecutor : public IUnsyncedActionExecutor {
public:
	FPSActionExecutor() : IUnsyncedActionExecutor("FPS", "Shows/Hides the frames-per-second indicator") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		InverseOrSetBool(game->showFPS, action.GetArgs());
		configHandler->Set("ShowFPS", game->showFPS ? 1 : 0);
		LogSystemStatus("frames-per-second indicator", game->showFPS);
		return true;
	}
};



class SpeedActionExecutor : public IUnsyncedActionExecutor {
public:
	SpeedActionExecutor() : IUnsyncedActionExecutor("Speed", "Shows/Hides the simulation speed indicator") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		InverseOrSetBool(game->showSpeed, action.GetArgs());
		configHandler->Set("ShowSpeed", game->showSpeed ? 1 : 0);
		LogSystemStatus("simulation speed indicator", game->showSpeed);
		return true;
	}
};


class TeamHighlightActionExecutor : public IUnsyncedActionExecutor {
public:
	TeamHighlightActionExecutor() : IUnsyncedActionExecutor("TeamHighlight", "Enables/Disables uncontrolled team blinking") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		if (action.GetArgs().empty()) {
			globalConfig.teamHighlight = abs(globalConfig.teamHighlight + 1) % CTeamHighlight::HIGHLIGHT_SIZE;
		} else {
			globalConfig.teamHighlight = abs(StringToInt(action.GetArgs())) % CTeamHighlight::HIGHLIGHT_SIZE;
		}

		LOG("Team highlighting: %s",
				((globalConfig.teamHighlight == CTeamHighlight::HIGHLIGHT_PLAYERS) ? "Players only"
				: ((globalConfig.teamHighlight == CTeamHighlight::HIGHLIGHT_ALL) ? "Players and spectators"
				: "Disabled")));

		configHandler->Set("TeamHighlight", globalConfig.teamHighlight);
		return true;
	}
};



class InfoActionExecutor : public IUnsyncedActionExecutor {
public:
	InfoActionExecutor() : IUnsyncedActionExecutor("Info", "Shows/Hides the player roster") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		if (action.GetArgs().empty()) {
			if (playerRoster.GetSortType() == PlayerRoster::Disabled) {
				playerRoster.SetSortTypeByCode(PlayerRoster::Allies);
			} else {
				playerRoster.SetSortTypeByCode(PlayerRoster::Disabled);
			}
		} else {
			playerRoster.SetSortTypeByName(action.GetArgs());
		}

		if (playerRoster.GetSortType() != PlayerRoster::Disabled)
			LOG("Sorting roster by %s", playerRoster.GetSortName());

		configHandler->Set("ShowPlayerInfo", (int)playerRoster.GetSortType());
		return true;
	}
};



class CmdColorsActionExecutor : public IUnsyncedActionExecutor {
public:
	CmdColorsActionExecutor() : IUnsyncedActionExecutor("CmdColors", "Reloads cmdcolors.txt") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		const std::string& fileName = action.GetArgs().empty() ? "cmdcolors.txt" : action.GetArgs();
		cmdColors.LoadConfigFromFile(fileName);
		LOG("Reloaded cmdcolors from file: %s", fileName.c_str());
		return true;
	}
};



class CtrlPanelActionExecutor : public IUnsyncedActionExecutor {
public:
	CtrlPanelActionExecutor() : IUnsyncedActionExecutor("CtrlPanel", "Reloads GUI config") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		guihandler->ReloadConfigFromFile(action.GetArgs());
		return true;
	}
};



class FontActionExecutor : public IUnsyncedActionExecutor {
public:
	FontActionExecutor() : IUnsyncedActionExecutor("Font", "Reloads default or custom fonts") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		auto args = CSimpleParser::Tokenize(action.GetArgs(), 1);
		std::string newLargeFontFile;
		std::string newSmallFontFile;
		switch (args.size())
		{
		case 1: {
			newSmallFontFile = std::move(args[0]);
		} break;
		case 2: {
			newSmallFontFile = std::move(args[0]);
			newLargeFontFile = std::move(args[1]);
		} break;
		default:
			// nothing
			break;
		}
		CglFont::LoadCustomFonts(newSmallFontFile, newLargeFontFile);
		return true;
	}
};



class VSyncActionExecutor : public IUnsyncedActionExecutor {
public:
	VSyncActionExecutor() : IUnsyncedActionExecutor("VSync",
			"Enables/Disables vertical-sync (Graphics setting)") {}

	bool Execute(const UnsyncedAction& action) const final {
		if (action.GetArgs().empty()) {
			verticalSync->Toggle();
		} else {
			verticalSync->SetInterval(StringToInt(action.GetArgs()));
		}

		return true;
	}
};



class SafeGLActionExecutor : public IUnsyncedActionExecutor {
public:
	SafeGLActionExecutor() : IUnsyncedActionExecutor("SafeGL",
			"Enables/Disables OpenGL safe-mode") {}

	bool Execute(const UnsyncedAction& action) const final {
		bool safeMode = LuaOpenGL::GetSafeMode();
		InverseOrSetBool(safeMode, action.GetArgs());
		LuaOpenGL::SetSafeMode(safeMode);
		LogSystemStatus("OpenGL safe-mode", LuaOpenGL::GetSafeMode());
		return true;
	}
};



class ResBarActionExecutor : public IUnsyncedActionExecutor {
public:
	ResBarActionExecutor() : IUnsyncedActionExecutor("ResBar",
			"Shows/Hides team resource storage indicator bar") {}

	bool Execute(const UnsyncedAction& action) const final {
		if (resourceBar == nullptr)
			return false;

		InverseOrSetBool(resourceBar->enabled, action.GetArgs());
		return true;
	}
};



class ToolTipActionExecutor : public IUnsyncedActionExecutor {
public:
	ToolTipActionExecutor() : IUnsyncedActionExecutor(
		"ToolTip",
		"Enables/Disables the general tool-tips, displayed when hovering over units. features or the map"
	) {
	}

	bool Execute(const UnsyncedAction& action) const final {
		if (tooltip == nullptr)
			return false;

		InverseOrSetBool(tooltip->enabled, action.GetArgs());
		return true;
	}
};



class ConsoleActionExecutor : public IUnsyncedActionExecutor {
public:
	ConsoleActionExecutor() : IUnsyncedActionExecutor("Console", "Enables/Disables the in-game console") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		InverseOrSetBool(infoConsole->enabled, action.GetArgs());
		return true;
	}
};



class EndGraphActionExecutor : public IUnsyncedActionExecutor {
public:
	EndGraphActionExecutor() : IUnsyncedActionExecutor("EndGraph", "Enables/Disables the statistics graphs shown at the end of the game") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		const auto& args = action.GetArgs();

		const char* fmt = "EndGame Graph %s";
		const char* strs[] = { "disabled", "enabled, causes quit", "enabled, causes reload" };

		if (args.empty()) {
			CEndGameBox::enabledMode = (CEndGameBox::enabledMode + 1) % 3;
		} else {
			CEndGameBox::enabledMode = std::clamp(StringToInt(args), 0, 2);
		}
		LOG(fmt, strs[CEndGameBox::enabledMode]);

		return true;
	}
};



class FPSHudActionExecutor : public IUnsyncedActionExecutor {
public:
	FPSHudActionExecutor() : IUnsyncedActionExecutor("FPSHud", "Enables/Disables HUD (GUI interface) shown in first-person-control mode") {
	}

	bool Execute(const UnsyncedAction& action) const final {

		bool drawHUD = hudDrawer->GetDraw();
		InverseOrSetBool(drawHUD, action.GetArgs());
		hudDrawer->SetDraw(drawHUD);
		return true;
	}
};



class DebugDrawAIActionExecutor : public IUnsyncedActionExecutor {
public:
	DebugDrawAIActionExecutor() : IUnsyncedActionExecutor("DebugDrawAI", "Enables/Disables debug drawing for AIs") {
	}

	bool Execute(const UnsyncedAction& action) const final {

		bool aiDebugDraw = debugDrawerAI->GetDraw();
		InverseOrSetBool(aiDebugDraw, action.GetArgs());
		debugDrawerAI->SetDraw(aiDebugDraw);
		LogSystemStatus("SkirmishAI debug drawing", debugDrawerAI->GetDraw());
		return true;
	}
};



class MapMarksActionExecutor : public IUnsyncedActionExecutor {
public:
	MapMarksActionExecutor() : IUnsyncedActionExecutor("MapMarks",
			"Enables/Disables map marks rendering") {}

	bool Execute(const UnsyncedAction& action) const final {

		InverseOrSetBool(globalRendering->drawMapMarks, action.GetArgs());
		LogSystemStatus("map marks rendering", globalRendering->drawMapMarks);
		return true;
	}
};



class AllMapMarksActionExecutor : public IUnsyncedActionExecutor {
public:
	AllMapMarksActionExecutor() : IUnsyncedActionExecutor("AllMapMarks",
			"Show/Hide all map marks drawn so far", true) {}

	bool Execute(const UnsyncedAction& action) const final {

		bool allMarksVisible = inMapDrawerModel->GetAllMarksVisible();
		InverseOrSetBool(allMarksVisible, action.GetArgs());
		inMapDrawerModel->SetAllMarksVisible(allMarksVisible);
		return true;
	}
};



class ClearMapMarksActionExecutor : public IUnsyncedActionExecutor {
public:
	ClearMapMarksActionExecutor() : IUnsyncedActionExecutor("ClearMapMarks",
			"Remove all map marks drawn so far") {}

	bool Execute(const UnsyncedAction& action) const final {
		inMapDrawerModel->EraseAll();
		return true;
	}
};



// XXX unlucky command-name, remove the "No"
class NoLuaDrawActionExecutor : public IUnsyncedActionExecutor {
public:
	NoLuaDrawActionExecutor() : IUnsyncedActionExecutor("NoLuaDraw", "Allow/Disallow Lua to draw on the map") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		bool luaMapDrawingAllowed = inMapDrawer->GetLuaMapDrawingAllowed();
		InverseOrSetBool(luaMapDrawingAllowed, action.GetArgs());
		inMapDrawer->SetLuaMapDrawingAllowed(luaMapDrawingAllowed);
		return true;
	}
};



class LuaUIActionExecutor : public IUnsyncedActionExecutor {
public:
	LuaUIActionExecutor() : IUnsyncedActionExecutor("LuaUI",
			"Allows one to reload or disable LuaUI, or alternatively to send"
			" a chat message to LuaUI") {}

	bool Execute(const UnsyncedAction& action) const final {
		if (guihandler == nullptr)
			return false;

		const std::string& command = action.GetArgs();

		if (command == "reload" || command == "enable") {
			guihandler->EnableLuaUI(command == "enable");
			return true;
		}
		if (command == "disable") {
			guihandler->DisableLuaUI();
			return true;
		}
		if (luaUI != nullptr) {
			luaUI->GotChatMsg(command, 0);
			return true;
		}

		LOG_L(L_DEBUG, "LuaUI is not loaded");
		return true;
	}
};

class LuaMenuActionExecutor : public IUnsyncedActionExecutor {
public:
	LuaMenuActionExecutor() : IUnsyncedActionExecutor("LuaMenu",
		"Allows one to reload or disable LuaMenu, or alternatively to send"
		" a chat message to LuaMenu") {}

	bool Execute(const UnsyncedAction& action) const final {
		const std::string& command = action.GetArgs();

		if (command == "reload" || command == "enable") {
			CLuaMenu::Enable(command == "enable");
			return true;
		}
		if (command == "disable") {
			CLuaMenu::Disable();
			return true;
		}
		if (luaMenu != nullptr) {
			luaMenu->GotChatMsg(command, 0);
			return true;
		}

		LOG_L(L_DEBUG, "LuaMenu is not loaded");
		return true;
	}
};

class MiniMapActionExecutor : public IUnsyncedActionExecutor {
public:
	MiniMapActionExecutor() : IUnsyncedActionExecutor("MiniMap", "FIXME document subcommands") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		if (minimap == nullptr)
			return false;

		minimap->ConfigCommand(action.GetArgs());
		return true;
	}
};



class GroundDecalsActionExecutor : public IUnsyncedActionExecutor {
public:
	GroundDecalsActionExecutor() : IUnsyncedActionExecutor(
		"GroundDecals",
		"Enable/Disable ground-decal rendering"
	) {
	}

	bool Execute(const UnsyncedAction& action) const final {
		if (action.GetArgs().empty()) {
			IGroundDecalDrawer::SetDrawDecals(!IGroundDecalDrawer::GetDrawDecals()); //inverse
		}
		else {
			bool failed;
			auto dl = StringToInt(action.GetArgs(), &failed);
			if (!failed)
				IGroundDecalDrawer::SetDrawDecals(static_cast<bool>(dl));
		}

		static constexpr const char* fmt = "Ground-decal rendering %s";
		LOG(fmt, IGroundDecalDrawer::GetDrawDecals() ? "enabled" : "disabled");
		return true;
	}
};



class DistSortProjectilesActionExecutor: public IUnsyncedActionExecutor {
public:
	DistSortProjectilesActionExecutor(): IUnsyncedActionExecutor(
		"DistSortProjectiles",
		"Enable/Disable sorting drawn projectiles by camera distance"
	) {
	}

	bool Execute(const UnsyncedAction& action) const final {
		const auto& args = action.GetArgs();

		const char* fmt = "ProjectileDrawer distance-sorting %s";
		const char* strs[] = {"disabled", "enabled"};

		if (!args.empty()) {
			LOG(fmt, strs[projectileDrawer->EnableSorting(StringToInt(args))]);
		} else {
			LOG(fmt, strs[projectileDrawer->ToggleSorting()]);
		}

		return true;
	}
};

class ParticleSoftenActionExecutor : public IUnsyncedActionExecutor {
public:
	ParticleSoftenActionExecutor() : IUnsyncedActionExecutor(
		"SoftParticles",
		"Enable/Disable particles softening: 0=disabled, 1=enabled"
	) { }

	bool Execute(const UnsyncedAction& action) const final {
		const auto& args = action.GetArgs();

		const char* fmt = "ProjectileDrawer particles-softening %s";
		const char* strs[] = { "disabled", "enabled" };

		if (!args.empty()) {
			LOG(fmt, strs[projectileDrawer->EnableSoften(StringToInt(args))]);
		}
		else {
			LOG(fmt, strs[projectileDrawer->ToggleSoften()]);
		}

		return true;
	}
};

class ParticleDrawOrderActionExecutor : public IUnsyncedActionExecutor {
public:
	ParticleDrawOrderActionExecutor() : IUnsyncedActionExecutor(
		"DrawOrderParticles",
		"Enable/Disable particles draw order: 0=disabled, 1=enabled"
	) { }

	bool Execute(const UnsyncedAction& action) const final {
		const auto& args = action.GetArgs();

		const char* fmt = "ProjectileDrawer draw order %s";
		const char* strs[] = { "disabled", "enabled" };

		if (!args.empty()) {
			LOG(fmt, strs[projectileDrawer->EnableDrawOrder(StringToInt(args))]);
		}
		else {
			LOG(fmt, strs[projectileDrawer->ToggleDrawOrder()]);
		}

		return true;
	}
};

class MaxParticlesActionExecutor : public IUnsyncedActionExecutor {
public:
	MaxParticlesActionExecutor() : IUnsyncedActionExecutor(
		"MaxParticles",
		"Set the maximum number of particles (Graphics setting)"
	) {
	}

	bool Execute(const UnsyncedAction& action) const final {
		const auto& args = action.GetArgs();

		if (!args.empty()) {
			projectileHandler.SetMaxParticles(StringToInt(args));
			LOG("Set maximum particles to: %i", projectileHandler.maxParticles);
		} else {
			LOG_L(L_WARNING, "/%s: wrong syntax", GetCommand().c_str());
		}

		return true;
	}
};

class MaxNanoParticlesActionExecutor : public IUnsyncedActionExecutor {
public:
	MaxNanoParticlesActionExecutor() : IUnsyncedActionExecutor(
		"MaxNanoParticles",
		"Set the maximum number of nano-particles (Graphic setting)"
	) {
	}

	bool Execute(const UnsyncedAction& action) const final {
		const auto& args = action.GetArgs();

		if (!args.empty()) {
			projectileHandler.SetMaxNanoParticles(StringToInt(args));
			LOG("Set maximum nano-particles to: %i", projectileHandler.maxNanoParticles);
		} else {
			LOG_L(L_WARNING, "/%s: wrong syntax", GetCommand().c_str());
		}

		return true;
	}
};



class MinViewRangeActionExecutor : public IUnsyncedActionExecutor {
public:
	MinViewRangeActionExecutor() : IUnsyncedActionExecutor("MinViewRange", "Set minimum view-distance") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		const std::string& args = action.GetArgs();

		if (args.empty())
			return false;

		globalRendering->minViewRange = std::clamp(StringToInt<float>(args), CGlobalRendering::MIN_ZNEAR_DIST, globalRendering->maxViewRange);
		return true;
	}
};

class MaxViewRangeActionExecutor : public IUnsyncedActionExecutor {
public:
	MaxViewRangeActionExecutor() : IUnsyncedActionExecutor("MaxViewRange", "Set maximum view-distance") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		const std::string& args = action.GetArgs();

		if (args.empty())
			return false;

		globalRendering->maxViewRange = std::clamp(StringToInt<float>(args), globalRendering->minViewRange, CGlobalRendering::MAX_VIEW_RANGE);
		return true;
	}
};



class GatherModeActionExecutor : public IUnsyncedActionExecutor {
public:
	GatherModeActionExecutor() : IUnsyncedActionExecutor("GatherMode", "Enter/Leave gather-wait command mode") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		if (guihandler == nullptr)
			return false;

		bool gatherMode = guihandler->GetGatherMode();
		InverseOrSetBool(gatherMode, action.GetArgs());
		guihandler->SetGatherMode(gatherMode);
		LogSystemStatus("Gather-Mode", guihandler->GetGatherMode());
		return true;
	}
};



class PasteTextActionExecutor : public IUnsyncedActionExecutor {
public:
	PasteTextActionExecutor() : IUnsyncedActionExecutor(
		"PasteText",
		"Paste either the argument string(s) or the content of the clip-board to chat input"
	) {
	}

	bool Execute(const UnsyncedAction& action) const final {
		return (gameTextInput.CheckHandlePasteCommand(action.GetInnerAction().rawline));
	}
};

class BufferTextActionExecutor : public IUnsyncedActionExecutor {
public:
	BufferTextActionExecutor() : IUnsyncedActionExecutor("BufferText", "Write the argument string(s) directly to the console history") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		// we cannot use extra commands because tokenization strips multiple
		// spaces or even trailing spaces, the text should be copied verbatim
		const std::string bufferCmd = "buffertext ";
		const std::string& rawLine = action.GetInnerAction().rawline;

		if (rawLine.length() > bufferCmd.length() ) {
			gameConsoleHistory.AddLine(rawLine.substr(bufferCmd.length(), rawLine.length() - bufferCmd.length()));
		} else {
			LOG_L(L_WARNING, "/%s: wrong syntax", GetCommand().c_str());
		}

		return true;
	}
};



class InputTextGeoActionExecutor : public IUnsyncedActionExecutor {
public:
	InputTextGeoActionExecutor() : IUnsyncedActionExecutor(
		"InputTextGeo",
		"Move and/or resize the input-text field (the \"Say: \" thing)"
	) {
	}

	bool Execute(const UnsyncedAction& action) const final {
		if (!action.GetArgs().empty()) {
			game->ParseInputTextGeometry(action.GetArgs());
		} else {
			LOG_L(L_WARNING, "/%s: wrong syntax", GetCommand().c_str());
		}

		return true;
	}
};



class DistIconActionExecutor : public IUnsyncedActionExecutor {
public:
	DistIconActionExecutor() : IUnsyncedActionExecutor("DistIcon",
			"Set the distance between units and camera, at which they turn"
			" into icons (Graphic setting)") {}

	bool Execute(const UnsyncedAction& action) const final {
		if (!action.GetArgs().empty()) {
			const int iconDist = StringToInt(action.GetArgs());
			CUnitDrawer::SetUnitIconDist(static_cast<float>(iconDist));
			configHandler->Set("UnitIconDist", iconDist);
			LOG("Set UnitIconDist to %i", iconDist);
		} else {
			LOG_L(L_WARNING, "/%s: wrong syntax", GetCommand().c_str());
		}

		return true;
	}
};

class IconsAsUIActionExecutor : public IUnsyncedActionExecutor {
public:
	IconsAsUIActionExecutor() : IUnsyncedActionExecutor("IconsAsUI",
			"Set whether unit icons are drawn as an UI element (true) or old LOD-like style (false, default).") {}

	bool Execute(const UnsyncedAction& action) const final {
		InverseOrSetBool(CUnitDrawer::UseScreenIcons(), action.GetArgs());
		configHandler->Set("UnitIconsAsUI", CUnitDrawer::UseScreenIcons() ? 1 : 0);
		LogSystemStatus("Draw unit icons as UI: ", CUnitDrawer::UseScreenIcons());
		return true;
	}
};

class IconScaleActionExecutor : public IUnsyncedActionExecutor {
public:
	IconScaleActionExecutor() : IUnsyncedActionExecutor("IconScaleUI",
			"Set the multiplier for the size of the UI unit icons") {}

	bool Execute(const UnsyncedAction& action) const final
	{
		if (!action.GetArgs().empty()) {
			const float iconScale = StringToInt<float>(action.GetArgs());
			unitDrawer->SetUnitIconScaleUI(iconScale);
			configHandler->Set("UnitIconScaleUI", iconScale);
			LOG("Set UnitIconScaleUI to %f", iconScale);
		}
		else
			LOG("UnitIconScaleUI is %f", unitDrawer->GetUnitIconScaleUI());

		return true;
	}
};

class IconFadeStartActionExecutor : public IUnsyncedActionExecutor {
public:
	IconFadeStartActionExecutor() : IUnsyncedActionExecutor("IconFadeStart",
			"Set the distance where unit icons became completely opaque at") {}

	bool Execute(const UnsyncedAction& action) const final
	{
		if (!action.GetArgs().empty())
		{
			const float iconFadeStart = StringToInt<float>(action.GetArgs());
			unitDrawer->SetUnitIconFadeStart(iconFadeStart);
			configHandler->Set("UnitIconFadeStart", iconFadeStart);
			LOG("Set UnitIconFadeStart to %f", iconFadeStart);
		}
		else
			LOG("UnitIconFadeStart is %f", unitDrawer->GetUnitIconFadeStart());

		return true;
	}
};

class IconFadeVanishActionExecutor : public IUnsyncedActionExecutor {
public:
	IconFadeVanishActionExecutor() : IUnsyncedActionExecutor("IconFadeVanish",
			"Set the distance where unit icons fade out at") {}

	bool Execute(const UnsyncedAction& action) const final
	{
		if (!action.GetArgs().empty())
		{
			const float iconFadeVanish = StringToInt<float>(action.GetArgs());
			CUnitDrawer::SetUnitIconFadeVanish(iconFadeVanish);
			configHandler->Set("UnitIconFadeVanish", iconFadeVanish);
			LOG("Set UnitIconFadeVanish to %f", iconFadeVanish);
		}
		else
			LOG("UnitIconFadeVanish is %f", CUnitDrawer::GetUnitIconFadeVanish());

		return true;
	}
};

class IconsHideWithUIActionExecutor : public IUnsyncedActionExecutor {
public:
	IconsHideWithUIActionExecutor() : IUnsyncedActionExecutor("IconsHideWithUI",
			"Set whether unit icons are hidden when UI is hidden.") {}

	bool Execute(const UnsyncedAction& action) const final {
		InverseOrSetBool(CUnitDrawer::IconHideWithUI(), action.GetArgs());
		configHandler->Set("UnitIconsHideWithUI", CUnitDrawer::IconHideWithUI() ? 1 : 0);
		LogSystemStatus("Hide unit icons with UI: ", CUnitDrawer::IconHideWithUI());
		return true;
	}
};

class LODScaleActionExecutor : public IUnsyncedActionExecutor {
public:
	LODScaleActionExecutor() : IUnsyncedActionExecutor(
		"LODScale",
		"Set the scale for either of: LOD (level-of-detail), shadow-LOD, reflection-LOD, refraction-LOD"
	) {
	}

	bool Execute(const UnsyncedAction& action) const final {
		if (action.GetArgs().empty()) {
			LOG_L(L_WARNING, "/%s: wrong syntax", GetCommand().c_str());
			return true;
		}
		const vector<string> &args = CSimpleParser::Tokenize(action.GetArgs());

		if (args.size() == 2) {
			const int objType = std::clamp(StringToInt(args[0]), int(LUAOBJ_UNIT), int(LUAOBJ_FEATURE));
			const float lodScale = StringToInt<float>(args[1]);

			LuaObjectDrawer::SetLODScale(objType, lodScale);
		}
		else if (args.size() == 3) {
			const int objType = std::clamp(StringToInt(args[1]), int(LUAOBJ_UNIT), int(LUAOBJ_FEATURE));
			const float lodScale = StringToInt<float>(args[2]);

			switch (hashString(args[0].c_str())) {
				case hashString("shadow"): {
					LuaObjectDrawer::SetLODScaleShadow(objType, lodScale);
				} break;
				case hashString("reflection"): {
					LuaObjectDrawer::SetLODScaleReflection(objType, lodScale);
				} break;
				case hashString("refraction"): {
					LuaObjectDrawer::SetLODScaleRefraction(objType, lodScale);
				} break;
			}
		} else {
			LOG_L(L_WARNING, "/%s: wrong syntax", GetCommand().c_str());
		}

		return true;
	}
};



class AirMeshActionExecutor: public IUnsyncedActionExecutor {
public:
	AirMeshActionExecutor(): IUnsyncedActionExecutor("airmesh", "Show/Hide the smooth air-mesh map overlay", true) {
	}

	bool Execute(const UnsyncedAction& action) const final {
		InverseOrSetBool(smoothHeightMeshDrawer->DrawEnabled(), action.GetArgs());
		LogSystemStatus("smooth air-mesh map overlay", smoothHeightMeshDrawer->DrawEnabled());
		return true;
	}
};


class WireModelActionExecutor: public IUnsyncedActionExecutor {
public:
	WireModelActionExecutor(): IUnsyncedActionExecutor("WireModel", "Toggle wireframe-mode drawing of model geometry") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		// note: affects feature and projectile render-state for free
		LogSystemStatus("wireframe model-drawing mode", unitDrawer->WireFrameModeRef() = !unitDrawer->WireFrameModeRef());
		return true;
	}
};

class WireMapActionExecutor: public IUnsyncedActionExecutor {
public:
	WireMapActionExecutor(): IUnsyncedActionExecutor("WireMap", "Toggle wireframe-mode drawing of map geometry") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		CBaseGroundDrawer* gd = readMap->GetGroundDrawer();

		LogSystemStatus("wireframe map-drawing mode", gd->WireFrameModeRef() = !gd->WireFrameModeRef());
		return true;
	}
};

class WireSkyActionExecutor: public IUnsyncedActionExecutor {
public:
	WireSkyActionExecutor(): IUnsyncedActionExecutor("WireSky", "Toggle wireframe-mode drawing of skydome geometry") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		const auto& sky = ISky::GetSky();
		LogSystemStatus("wireframe sky-drawing mode", sky->WireFrameModeRef() = !sky->WireFrameModeRef());
		return true;
	}
};

class WireWaterActionExecutor: public IUnsyncedActionExecutor {
public:
	WireWaterActionExecutor(): IUnsyncedActionExecutor("WireWater", "Toggle wireframe-mode drawing of water geometry") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		const auto& water = IWater::GetWater();
		LogSystemStatus("wireframe water-drawing mode", water->WireFrameModeRef() = !water->WireFrameModeRef());
		return true;
	}
};



class DebugColVolDrawerActionExecutor : public IUnsyncedActionExecutor {
public:
	DebugColVolDrawerActionExecutor(): IUnsyncedActionExecutor("DebugColVol", "Enable/Disable drawing of collision volumes") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		InverseOrSetBool(DebugColVolDrawer::enable, action.GetArgs());
		return true;
	}
};


class DebugVisibilityDrawerActionExecutor : public IUnsyncedActionExecutor {
public:
	DebugVisibilityDrawerActionExecutor(): IUnsyncedActionExecutor("DebugVisibility", "Enable/Disable drawing of visible terrain") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		InverseOrSetBool(DebugVisibilityDrawer::enable, action.GetArgs());
		return true;
	}
};


class DebugPathDrawerActionExecutor : public IUnsyncedActionExecutor {
public:
	DebugPathDrawerActionExecutor(): IUnsyncedActionExecutor("DebugPath", "Enable/Disable drawing of pathfinder debug-data") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		LogSystemStatus("path-debug rendering mode", pathDrawer->ToggleEnabled());
		return true;
	}
};


class DebugTraceRayDrawerActionExecutor : public IUnsyncedActionExecutor {
public:
	DebugTraceRayDrawerActionExecutor(): IUnsyncedActionExecutor("DebugTraceRay", "Enable/Disable drawing of traceray debug-data") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		LogSystemStatus("traceray debug rendering mode", globalRendering->drawDebugTraceRay = !globalRendering->drawDebugTraceRay);
		return true;
	}
};

class DebugShadowFrustum : public IUnsyncedActionExecutor {
public:
	DebugShadowFrustum() : IUnsyncedActionExecutor("DebugShadowFrustum", "Enable/Disable drawing of shadow frustum shape") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		InverseOrSetBool(shadowHandler.DebugFrustumRef(), action.GetArgs());
		LogSystemStatus("shadow frustum debug rendering mode", shadowHandler.DebugFrustumRef());
		return true;
	}
};

class CrashActionExecutor : public IUnsyncedActionExecutor {
public:
	CrashActionExecutor() : IUnsyncedActionExecutor("Crash", "Invoke an artificial crash through a NULL-pointer dereference (SIGSEGV)", true) {
	}

	bool Execute(const UnsyncedAction& action) const final {
		std::abort();
		return true;
	}
};

class HangActionExecutor : public IUnsyncedActionExecutor {
public:
	HangActionExecutor() : IUnsyncedActionExecutor("Hang", "Invoke an artificial hang", true) {
	}

	bool Execute(const UnsyncedAction& action) const final {
		const std::string& args = action.GetArgs();

		const spring_time t0 = spring_now();
		const spring_time t1 = t0 + spring_time((args.empty() ? 20.0f : StringToInt<float>(args)) * 1000.0f);

		for (spring_time t = t0; t < t1; t = spring_now()) {
			// prevent compiler from removing this
			SCOPED_TIMER("HangAction::Execute");
		}

		return true;
	}
};

class ExceptionActionExecutor : public IUnsyncedActionExecutor {
public:
	ExceptionActionExecutor() : IUnsyncedActionExecutor("Exception", "Invoke an artificial crash by throwing an std::runtime_error", true) {
	}

	bool Execute(const UnsyncedAction& action) const final {
		throw std::runtime_error("Exception test");
		return true;
	}
};

class DivByZeroActionExecutor : public IUnsyncedActionExecutor {
public:
	DivByZeroActionExecutor() : IUnsyncedActionExecutor("DivByZero", "Invoke an artificial crash by performing a division-by-zero", true) {
	}

	bool Execute(const UnsyncedAction& action) const final {
		float a = 0.0f; //can't be constexpr since MSVC dies
		LOG("Result: %f", 1.0f / a);
		return true;
	}
};



class GiveActionExecutor : public IUnsyncedActionExecutor {
public:
	GiveActionExecutor() : IUnsyncedActionExecutor(
		"Give",
		"Places one or multiple units of a single or multiple types on the map, instantly; by default to your own team", true
	) {
	}

	bool Execute(const UnsyncedAction& action) const final {
		if (action.GetArgs().find('@') == string::npos) {
			CInputReceiver* ir = nullptr;

			if (!game->hideInterface && !mouse->offscreen)
				ir = CInputReceiver::GetReceiverAt(mouse->lastx, mouse->lasty);

			float3 givePos;

			if (ir == minimap) {
				givePos = minimap->GetMapPosition(mouse->lastx, mouse->lasty);
			} else {
				const float3& pos = camera->GetPos();
				const float3& dir = mouse->dir;
				const float dist = CGround::LineGroundCol(pos, pos + (dir * 30000.0f));
				givePos = pos + (dir * dist);
			}

			char message[256];
			SNPRINTF(message, sizeof(message),
					"%s %s @%.0f,%.0f,%.0f",
					GetCommand().c_str(), action.GetArgs().c_str(),
					givePos.x, givePos.y, givePos.z);

			CommandMessage pckt(message, gu->myPlayerNum);
			clientNet->Send(pckt.Pack());
		} else {
			// forward (as synced command)
			CommandMessage pckt(action.GetInnerAction(), gu->myPlayerNum);
			clientNet->Send(pckt.Pack());
		}

		return true;
	}
};


class BaseDestroyActionExecutor : public IUnsyncedActionExecutor {
public:
	BaseDestroyActionExecutor(const std::string& command, const std::string& description)
		: IUnsyncedActionExecutor(command, description, true) {}

	bool Execute(const UnsyncedAction& action) const final {
		if (selectedUnitsHandler.selectedUnits.empty()) {
			return false;
		}

		std::stringstream ss;
		ss << action.GetCmd();
		for (const int unitID : selectedUnitsHandler.selectedUnits) {
			ss << " " << unitID;
		}

		CommandMessage pckt(ss.str(), gu->myPlayerNum);
		clientNet->Send(pckt.Pack());
		return true;
	}
};


class DestroyActionExecutor : public BaseDestroyActionExecutor {
public:
	DestroyActionExecutor() : BaseDestroyActionExecutor("Destroy", "Destroys one or multiple units by unitID immediately") {}
};


class RemoveActionExecutor : public BaseDestroyActionExecutor {
public:
	RemoveActionExecutor() : BaseDestroyActionExecutor("Remove", "Removes one or multiple units by unitID immediately, bypassing death sequence") {}
};


class SendActionExecutor : public IUnsyncedActionExecutor {
public:
	SendActionExecutor() : IUnsyncedActionExecutor("Send", "Send a string as raw network message to the game host (for debugging only)") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		CommandMessage pckt(Action(action.GetArgs()), gu->myPlayerNum);
		clientNet->Send(pckt.Pack());
		return true;
	}
};



class DumpStateActionExecutor : public IUnsyncedActionExecutor {
public:
	DumpStateActionExecutor() : IUnsyncedActionExecutor("DumpState", "dump game-state to file") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		std::vector<std::string> args = CSimpleParser::Tokenize(action.GetArgs());

		switch (args.size()) {
			case 1: {
					int dumpId = DumpState(StringToInt(args[0]), StringToInt(args[0]), 1, false);
					DumpHistory(dumpId, false);
				} break;
			case 2: { DumpState(StringToInt(args[0]), StringToInt(args[1]),                    1,                 false); } break;
			case 3: { DumpState(StringToInt(args[0]), StringToInt(args[1]), StringToInt(args[2]),                 false); } break;
			case 4: { DumpState(StringToInt(args[0]), StringToInt(args[1]), StringToInt(args[2]), StringToBool(args[3])); } break;
			default: {
				LOG_L(L_WARNING, "/DumpState: wrong syntax");
			} break;
		}

		return true;
	}
};

class DumpRNGActionExecutor : public IUnsyncedActionExecutor {
public:
	DumpRNGActionExecutor() : IUnsyncedActionExecutor("DumpRNG", "dump SyncedRNG-state to file") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		std::vector<std::string> args = CSimpleParser::Tokenize(action.GetArgs());

		switch (args.size()) {
		case 1: { DumpRNG(StringToInt(args[0]), StringToInt(args[0])); } break;
		case 2: { DumpRNG(StringToInt(args[0]), StringToInt(args[1])); } break;
		default: {
			LOG_L(L_WARNING, "/DumpRNG: wrong syntax");
		} break;
		}

		return true;
	}
};



/// /save [-y ]<savename>
class SaveActionExecutor : public IUnsyncedActionExecutor {
public:
	SaveActionExecutor(bool _usecreg) : IUnsyncedActionExecutor(
		(_usecreg)? "Save" : "LuaSave",
		"Save the game state to a specific file, add -y to overwrite when file is already present"
	) {
		usecreg = _usecreg;
	}

	bool Execute(const UnsyncedAction& action) const final {
		std::vector<std::string> args = CSimpleParser::Tokenize(action.GetArgs());

		switch (args.size()) {
			case  1: { game->Save("Saves/" + args[0] + (usecreg? ".ssf": ".slsf"),                 ""); return  true; } break;
			case  2: { game->Save("Saves/" + args[0] + (usecreg? ".ssf": ".slsf"), std::move(args[1])); return  true; } break;
			default: {                                                                                                } break;
		}

		return false;
	}
private:
	bool usecreg;
};



class ReloadShadersActionExecutor : public IUnsyncedActionExecutor {
public:
	ReloadShadersActionExecutor() : IUnsyncedActionExecutor("ReloadShaders", "Reloads all engine shaders") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		LOG("Reloading all engine shaders");
		//FIXME make threadsafe!
		shaderHandler->ReloadAll();
		return true;
	}
};

class ReloadTexturesActionExecutor : public IUnsyncedActionExecutor {
public:
	ReloadTexturesActionExecutor() : IUnsyncedActionExecutor("ReloadTextures", "Reloads textures") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		auto luaFunc = []() {
			LOG("Reloading Lua textures");
			CNamedTextures::Reload();
		};
		auto s3oFunc = []() {
			LOG("Reloading S3O textures");
			textureHandlerS3O.Reload();
		};
		auto smfFunc = []() {
			LOG("Reloading SMF textures");
			readMap->ReloadTextures();
		};
		auto cegFunc = []() {
			LOG("Reloading CEG textures");
			projectileDrawer->textureAtlas->ReloadTextures();
			projectileDrawer->groundFXAtlas->ReloadTextures();
		};
		auto decalFunc = []() {
			LOG("Reloading Decal textures");
			groundDecals->ReloadTextures();
		};

		std::array argsExec = {
			ArgTuple(hashString("lua") , false, luaFunc),
			ArgTuple(hashString("s3o") , false, s3oFunc),
			ArgTuple(hashString("ssmf"), false, smfFunc),
			ArgTuple(hashString("smf") , false, smfFunc),
			ArgTuple(hashString("cegs"), false, cegFunc),
			ArgTuple(hashString("ceg") , false, cegFunc),
			ArgTuple(hashString("decal")  , false, decalFunc),
			ArgTuple(hashString("decals") , false, decalFunc),
		};

		auto args = CSimpleParser::Tokenize(action.GetArgs(), 1);
		return GenericArgsExecutor(args, argsExec);
	}
};

class DumpAtlasActionExecutor : public IUnsyncedActionExecutor {
public:
	DumpAtlasActionExecutor() : IUnsyncedActionExecutor("DumpAtlas", "Save Some Atlases") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		auto projFunc = []() {
			LOG("Dumping projectile textures");
			projectileDrawer->textureAtlas->DumpTexture("TextureAtlas");
			projectileDrawer->groundFXAtlas->DumpTexture("GroundFXAtlas");
		};
		auto threeDoFunc = []() {
			LOG("Dumping 3do atlas textures");
			glSaveTexture(textureHandler3DO.GetAtlasTex1ID(), "3doTex1.png");
			glSaveTexture(textureHandler3DO.GetAtlasTex2ID(), "3doTex2.png");
		};
		auto decalsFunc = []() {
			LOG("Dumping decal atlas textures");
			groundDecals->DumpAtlasTextures();
		};
		std::array argsExec = {
			ArgTuple(hashString("proj"), false, projFunc),
			ArgTuple(hashString("3do"), false, threeDoFunc),
			ArgTuple(hashString("decal"), false, decalsFunc),
			ArgTuple(hashString("decals"), false, decalsFunc)
		};

		auto args = CSimpleParser::Tokenize(action.GetArgs(), 1);
		return GenericArgsExecutor(args, argsExec);
	}
};


class DebugInfoActionExecutor : public IUnsyncedActionExecutor {
public:
	DebugInfoActionExecutor() : IUnsyncedActionExecutor(
		"DebugInfo",
		"Print debug info to the chat/log-file about either sound, profiling, or command-descriptions"
	) {
	}

	bool Execute(const UnsyncedAction& action) const final {
		const std::string& args = action.GetArgs();

		switch (hashString(args.c_str())) {
			case hashString("sound"): {
				sound->PrintDebugInfo();
			} break;
			case hashString("profiling"): {
				CTimeProfiler::GetInstance().PrintProfilingInfo();
			} break;
			case hashString("cmddescrs"): {
				commandDescriptionCache.Dump(true);
			} break;
			default: {
				LOG_L(L_WARNING, "[DbgInfoAction::%s] unknown argument \"%s\" (use \"sound\", \"profiling\", or \"cmddescrs\")", __func__, args.c_str());
			} break;
		}

		return true;
	}
};



class RedirectToSyncedActionExecutor : public IUnsyncedActionExecutor {
public:
	RedirectToSyncedActionExecutor(const std::string& command): IUnsyncedActionExecutor(
		command,
		"Redirects command /" + command + " to its synced processor"
	) {
	}

	bool Execute(const UnsyncedAction& action) const final {
		// redirect as a synced command
		CommandMessage pckt(action.GetInnerAction(), gu->myPlayerNum);
		clientNet->Send(pckt.Pack());
		return true;
	}
};



class CommandListActionExecutor : public IUnsyncedActionExecutor {
public:
	CommandListActionExecutor() : IUnsyncedActionExecutor(
		"CommandList",
		"Prints all the available chat commands with description (if available) to the console"
	) {
	}

	bool Execute(const UnsyncedAction& action) const final {
		LOG("Chat commands plus description");
		LOG("==============================");

		PrintToConsole(syncedGameCommands->GetSortedActionExecutors());
		PrintToConsole(unsyncedGameCommands->GetSortedActionExecutors());
		return true;
	}

	template<class action_t, bool synced_v>
	static void PrintExecutorToConsole(const IActionExecutor<action_t, synced_v>* executor) {
		const std::string& cmd = executor->GetCommand();
		const std::string& dsc = executor->GetDescription();
		LOG("/%-30s  (%s)  %s", cmd.c_str(), (executor->IsSynced()? "  synced" : "unsynced"), dsc.c_str());
	}

private:
	template<typename TActionExec>
	static void PrintToConsole(const std::vector< std::pair<std::string, TActionExec*> >& executors) {
		for (const auto& p: executors) {
			PrintExecutorToConsole(p.second);
		}
	}
};



class CommandHelpActionExecutor : public IUnsyncedActionExecutor {
public:
	CommandHelpActionExecutor() : IUnsyncedActionExecutor("CommandHelp", "Prints info about a specific chat command") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		const std::vector<std::string> args = CSimpleParser::Tokenize(action.GetArgs(), 1);

		if (!args.empty()) {
			const std::string commandLower = StringToLower(args[0]);

			// try if an unsynced chat command with this name is available
			const IUnsyncedActionExecutor* unsyncedExecutor = unsyncedGameCommands->GetActionExecutor(commandLower);

			if (unsyncedExecutor != nullptr) {
				PrintExecutorHelpToConsole(unsyncedExecutor);
				return true;
			}

			// try if a synced chat command with this name is available
			const ISyncedActionExecutor* syncedExecutor = syncedGameCommands->GetActionExecutor(commandLower);

			if (syncedExecutor != nullptr) {
				PrintExecutorHelpToConsole(syncedExecutor);
				return true;
			}

			LOG_L(L_WARNING, "No chat command registered with name \"%s\" (case-insensitive)", args[0].c_str());
		} else {
			LOG_L(L_WARNING, "missing command-name");
		}

		return true;
	}

private:
	template<class action_t, bool synced_v>
	static void PrintExecutorHelpToConsole(const IActionExecutor<action_t, synced_v>* executor)
	{
		// XXX extend this in case more info about commands are available (for example "Usage: name {args}")
		CommandListActionExecutor::PrintExecutorToConsole(executor);
	}
};

} // namespace (unnamed)


bool UnsyncedGameCommands::ActionPressed(const Action& action, bool isRepeat)
{
	const IUnsyncedActionExecutor* executor = GetActionExecutor(action.command);

	if (executor != nullptr) {
		// an executor for that action was found
		if (executor->ExecuteAction(UnsyncedAction(action, isRepeat)))
			return true;
	}

	return false;
}

bool UnsyncedGameCommands::ActionReleased(const Action& action)
{
	const IUnsyncedActionExecutor* executor = GetActionExecutor(action.command);

	if (executor != nullptr) {
		// an executor for that action was found
		if (executor->ExecuteActionRelease(UnsyncedAction(action, false)))
			return true;
	}

	return false;
}




void UnsyncedGameCommands::AddDefaultActionExecutors()
{
	if (!actionExecutors.empty())
		return;

	AddActionExecutor(AllocActionExecutor<SelectActionExecutor>());
	AddActionExecutor(AllocActionExecutor<SelectUnitsActionExecutor>());
	AddActionExecutor(AllocActionExecutor<SelectCycleActionExecutor>());
	AddActionExecutor(AllocActionExecutor<DeselectActionExecutor>());
	AddActionExecutor(AllocActionExecutor<ShadowsActionExecutor>());
	AddActionExecutor(AllocActionExecutor<DumpShadowsActionExecutor>());
	AddActionExecutor(AllocActionExecutor<MapShadowPolyOffsetActionExecutor>());
	AddActionExecutor(AllocActionExecutor<MapMeshDrawerActionExecutor>());
	AddActionExecutor(AllocActionExecutor<MapBorderActionExecutor>());
	AddActionExecutor(AllocActionExecutor<WaterActionExecutor>());
	AddActionExecutor(AllocActionExecutor<AdvMapShadingActionExecutor>()); // [maint]
	AddActionExecutor(AllocActionExecutor<UnitDrawerTypeActionExecutor>()); // [maint]
	AddActionExecutor(AllocActionExecutor<FeatureDrawerTypeActionExecutor>()); // [maint]
	AddActionExecutor(AllocActionExecutor<SayActionExecutor>());
	AddActionExecutor(AllocActionExecutor<SayPrivateActionExecutor>());
	AddActionExecutor(AllocActionExecutor<SayPrivateByPlayerIDActionExecutor>());
	AddActionExecutor(AllocActionExecutor<EchoActionExecutor>());
	AddActionExecutor(AllocActionExecutor<SetActionExecutor>(true));
	AddActionExecutor(AllocActionExecutor<SetActionExecutor>(false));
	AddActionExecutor(AllocActionExecutor<EnableDrawInMapActionExecutor>());
	AddActionExecutor(AllocActionExecutor<DrawLabelActionExecutor>());
	AddActionExecutor(AllocActionExecutor<MouseActionExecutor>(1));
	AddActionExecutor(AllocActionExecutor<MouseActionExecutor>(2));
	AddActionExecutor(AllocActionExecutor<MouseActionExecutor>(3));
	AddActionExecutor(AllocActionExecutor<MouseActionExecutor>(4));
	AddActionExecutor(AllocActionExecutor<MouseActionExecutor>(5));
	AddActionExecutor(AllocActionExecutor<MouseCancelSelectionRectangleActionExecutor>());
	AddActionExecutor(AllocActionExecutor<ViewSelectionActionExecutor>());
	AddActionExecutor(AllocActionExecutor<CameraMoveActionExecutor>(CCamera::MOVE_STATE_FWD, "Forward"     ));
	AddActionExecutor(AllocActionExecutor<CameraMoveActionExecutor>(CCamera::MOVE_STATE_BCK, "Back"        ));
	AddActionExecutor(AllocActionExecutor<CameraMoveActionExecutor>(CCamera::MOVE_STATE_LFT, "Left"        ));
	AddActionExecutor(AllocActionExecutor<CameraMoveActionExecutor>(CCamera::MOVE_STATE_RGT, "Right"       ));
	AddActionExecutor(AllocActionExecutor<CameraMoveActionExecutor>(CCamera::MOVE_STATE_UP , "Up"          ));
	AddActionExecutor(AllocActionExecutor<CameraMoveActionExecutor>(CCamera::MOVE_STATE_DWN, "Down"        ));
	AddActionExecutor(AllocActionExecutor<CameraMoveActionExecutor>(CCamera::MOVE_STATE_FST, "Fast"  , false));
	AddActionExecutor(AllocActionExecutor<CameraMoveActionExecutor>(CCamera::MOVE_STATE_SLW, "Slow"  , false));
	AddActionExecutor(AllocActionExecutor<CameraMoveActionExecutor>(CCamera::MOVE_STATE_TLT, "Tilt"  , false));
	AddActionExecutor(AllocActionExecutor<CameraMoveActionExecutor>(CCamera::MOVE_STATE_RST, "Reset" , false));
	AddActionExecutor(AllocActionExecutor<CameraMoveActionExecutor>(CCamera::MOVE_STATE_RTT, "Rotate", false));
	AddActionExecutor(AllocActionExecutor<MouseStateActionExecutor>());
	AddActionExecutor(AllocActionExecutor<AIKillReloadActionExecutor>(true));
	AddActionExecutor(AllocActionExecutor<AIKillReloadActionExecutor>(false));
	AddActionExecutor(AllocActionExecutor<AIControlActionExecutor>());
	AddActionExecutor(AllocActionExecutor<AIListActionExecutor>());
	AddActionExecutor(AllocActionExecutor<TeamActionExecutor>());
	AddActionExecutor(AllocActionExecutor<SpectatorActionExecutor>());
	AddActionExecutor(AllocActionExecutor<SpecTeamActionExecutor>());
	AddActionExecutor(AllocActionExecutor<SpecFullViewActionExecutor>());
	AddActionExecutor(AllocActionExecutor<AllyActionExecutor>());
	AddActionExecutor(AllocActionExecutor<GroupActionExecutor>());
	AddActionExecutor(AllocActionExecutor<GroupIDActionExecutor>(0));
	AddActionExecutor(AllocActionExecutor<GroupIDActionExecutor>(1));
	AddActionExecutor(AllocActionExecutor<GroupIDActionExecutor>(2));
	AddActionExecutor(AllocActionExecutor<GroupIDActionExecutor>(3));
	AddActionExecutor(AllocActionExecutor<GroupIDActionExecutor>(4));
	AddActionExecutor(AllocActionExecutor<GroupIDActionExecutor>(5));
	AddActionExecutor(AllocActionExecutor<GroupIDActionExecutor>(6));
	AddActionExecutor(AllocActionExecutor<GroupIDActionExecutor>(7));
	AddActionExecutor(AllocActionExecutor<GroupIDActionExecutor>(8));
	AddActionExecutor(AllocActionExecutor<GroupIDActionExecutor>(9));
	AddActionExecutor(AllocActionExecutor<LastMessagePositionActionExecutor>());

	AddActionExecutor(AllocActionExecutor<ChatActionExecutor>("",     "",   false));
	AddActionExecutor(AllocActionExecutor<ChatActionExecutor>("All",  "",   true));
	AddActionExecutor(AllocActionExecutor<ChatActionExecutor>("Ally", "a:", true));
	AddActionExecutor(AllocActionExecutor<ChatActionExecutor>("Spec", "s:", true));

	AddActionExecutor(AllocActionExecutor<TrackActionExecutor>());
	AddActionExecutor(AllocActionExecutor<TrackModeActionExecutor>());
	AddActionExecutor(AllocActionExecutor<PauseActionExecutor>());
	AddActionExecutor(AllocActionExecutor<DebugActionExecutor>());
	AddActionExecutor(AllocActionExecutor<DebugCubeMapActionExecutor>());
	AddActionExecutor(AllocActionExecutor<DebugQuadFieldActionExecutor>());
	AddActionExecutor(AllocActionExecutor<DrawSkyActionExecutor>());
	AddActionExecutor(AllocActionExecutor<DebugGLActionExecutor>());
	AddActionExecutor(AllocActionExecutor<DebugGLErrorsActionExecutor>());
	AddActionExecutor(AllocActionExecutor<DebugColVolDrawerActionExecutor>());
	AddActionExecutor(AllocActionExecutor<DebugVisibilityDrawerActionExecutor>());
	AddActionExecutor(AllocActionExecutor<DebugPathDrawerActionExecutor>());
	AddActionExecutor(AllocActionExecutor<DebugTraceRayDrawerActionExecutor>());
	AddActionExecutor(AllocActionExecutor<DebugShadowFrustum>());
	AddActionExecutor(AllocActionExecutor<MuteActionExecutor>());
	AddActionExecutor(AllocActionExecutor<SoundActionExecutor>());
	AddActionExecutor(AllocActionExecutor<SoundChannelEnableActionExecutor>());
	AddActionExecutor(AllocActionExecutor<CreateVideoActionExecutor>());
	// [devel] AddActionExecutor(AllocActionExecutor<DrawGrassActionExecutor>());
	AddActionExecutor(AllocActionExecutor<NetPingActionExecutor>());
	AddActionExecutor(AllocActionExecutor<NetMsgSmoothingActionExecutor>());
	AddActionExecutor(AllocActionExecutor<SpeedControlActionExecutor>());
	AddActionExecutor(AllocActionExecutor<GameInfoActionExecutor>());
	AddActionExecutor(AllocActionExecutor<GameInfoCloseActionExecutor>());
	AddActionExecutor(AllocActionExecutor<HideInterfaceActionExecutor>());
	AddActionExecutor(AllocActionExecutor<HardwareCursorActionExecutor>());
	AddActionExecutor(AllocActionExecutor<FullscreenActionExecutor>());
	AddActionExecutor(AllocActionExecutor<WindowBorderlessActionExecutor>());
	// [devel] AddActionExecutor(AllocActionExecutor<GammaExponentActionExecutor>());
	AddActionExecutor(AllocActionExecutor<IncreaseViewRadiusActionExecutor>());
	AddActionExecutor(AllocActionExecutor<DecreaseViewRadiusActionExecutor>());
	AddActionExecutor(AllocActionExecutor<GroundDetailActionExecutor>());
	// [devel] AddActionExecutor(AllocActionExecutor<MoreGrassActionExecutor>());
	// [devel] AddActionExecutor(AllocActionExecutor<LessGrassActionExecutor>());
	// [devel] AddActionExecutor(AllocActionExecutor<RotateSkyActionExecutor>());
	AddActionExecutor(AllocActionExecutor<FeatureFadeDistActionExecutor>());
	AddActionExecutor(AllocActionExecutor<FeatureDrawDistActionExecutor>());
	AddActionExecutor(AllocActionExecutor<SpeedUpActionExecutor>());
	AddActionExecutor(AllocActionExecutor<SlowDownActionExecutor>());
	AddActionExecutor(AllocActionExecutor<SetGamespeedActionExecutor>());
	AddActionExecutor(AllocActionExecutor<ControlUnitActionExecutor>());
	AddActionExecutor(AllocActionExecutor<ShowStandardActionExecutor>());
	AddActionExecutor(AllocActionExecutor<ShowElevationActionExecutor>());
	AddActionExecutor(AllocActionExecutor<ShowMetalMapActionExecutor>());
	AddActionExecutor(AllocActionExecutor<ShowPathTravActionExecutor>());
	AddActionExecutor(AllocActionExecutor<ShowPathHeatActionExecutor>());
	AddActionExecutor(AllocActionExecutor<ShowPathFlowActionExecutor>());
	AddActionExecutor(AllocActionExecutor<ShowPathCostActionExecutor>());
	AddActionExecutor(AllocActionExecutor<ToggleLOSActionExecutor>());
	AddActionExecutor(AllocActionExecutor<ToggleInfoActionExecutor>());
	AddActionExecutor(AllocActionExecutor<ShowPathTypeActionExecutor>());
	AddActionExecutor(AllocActionExecutor<ShareDialogActionExecutor>());
	AddActionExecutor(AllocActionExecutor<QuitMessageActionExecutor>());
	AddActionExecutor(AllocActionExecutor<QuitMenuActionExecutor>());
	AddActionExecutor(AllocActionExecutor<QuitActionExecutor>());
	AddActionExecutor(AllocActionExecutor<ReloadActionExecutor>());
	AddActionExecutor(AllocActionExecutor<IncreaseGUIOpacityActionExecutor>());
	AddActionExecutor(AllocActionExecutor<DecreaseGUIOpacityActionExecutor>());
	AddActionExecutor(AllocActionExecutor<ScreenShotActionExecutor>());
	AddActionExecutor(AllocActionExecutor<GrabInputActionExecutor>());
	AddActionExecutor(AllocActionExecutor<ClockActionExecutor>());
	AddActionExecutor(AllocActionExecutor<CrossActionExecutor>());
	AddActionExecutor(AllocActionExecutor<FPSActionExecutor>());
	AddActionExecutor(AllocActionExecutor<SpeedActionExecutor>());
	AddActionExecutor(AllocActionExecutor<TeamHighlightActionExecutor>());
	AddActionExecutor(AllocActionExecutor<InfoActionExecutor>());
	AddActionExecutor(AllocActionExecutor<CmdColorsActionExecutor>());
	AddActionExecutor(AllocActionExecutor<CtrlPanelActionExecutor>());
	AddActionExecutor(AllocActionExecutor<FontActionExecutor>());
	AddActionExecutor(AllocActionExecutor<VSyncActionExecutor>());
	AddActionExecutor(AllocActionExecutor<SafeGLActionExecutor>());
	AddActionExecutor(AllocActionExecutor<ResBarActionExecutor>());
	AddActionExecutor(AllocActionExecutor<ToolTipActionExecutor>());
	AddActionExecutor(AllocActionExecutor<ConsoleActionExecutor>());
	AddActionExecutor(AllocActionExecutor<EndGraphActionExecutor>());
	AddActionExecutor(AllocActionExecutor<FPSHudActionExecutor>());
	AddActionExecutor(AllocActionExecutor<DebugDrawAIActionExecutor>());
	AddActionExecutor(AllocActionExecutor<MapMarksActionExecutor>());
	AddActionExecutor(AllocActionExecutor<AllMapMarksActionExecutor>());
	AddActionExecutor(AllocActionExecutor<ClearMapMarksActionExecutor>());
	AddActionExecutor(AllocActionExecutor<NoLuaDrawActionExecutor>());
	AddActionExecutor(AllocActionExecutor<LuaUIActionExecutor>());
	AddActionExecutor(AllocActionExecutor<LuaMenuActionExecutor>());
	AddActionExecutor(AllocActionExecutor<LuaGarbageCollectControlExecutor>());
	AddActionExecutor(AllocActionExecutor<MiniMapActionExecutor>());
	AddActionExecutor(AllocActionExecutor<GroundDecalsActionExecutor>());

	AddActionExecutor(AllocActionExecutor<DistSortProjectilesActionExecutor>());
	AddActionExecutor(AllocActionExecutor<ParticleSoftenActionExecutor>());
	AddActionExecutor(AllocActionExecutor<ParticleDrawOrderActionExecutor>());
	AddActionExecutor(AllocActionExecutor<MaxParticlesActionExecutor>());
	AddActionExecutor(AllocActionExecutor<MaxNanoParticlesActionExecutor>());
	AddActionExecutor(AllocActionExecutor<MinViewRangeActionExecutor>());
	AddActionExecutor(AllocActionExecutor<MaxViewRangeActionExecutor>());

	AddActionExecutor(AllocActionExecutor<GatherModeActionExecutor>());
	AddActionExecutor(AllocActionExecutor<PasteTextActionExecutor>());
	AddActionExecutor(AllocActionExecutor<BufferTextActionExecutor>());
	AddActionExecutor(AllocActionExecutor<InputTextGeoActionExecutor>());
	AddActionExecutor(AllocActionExecutor<DistIconActionExecutor>());
	AddActionExecutor(AllocActionExecutor<IconsAsUIActionExecutor>());
	AddActionExecutor(AllocActionExecutor<IconScaleActionExecutor>());
	AddActionExecutor(AllocActionExecutor<IconFadeStartActionExecutor>());
	AddActionExecutor(AllocActionExecutor<IconFadeVanishActionExecutor>());
	AddActionExecutor(AllocActionExecutor<IconsHideWithUIActionExecutor>());
	AddActionExecutor(AllocActionExecutor<LODScaleActionExecutor>());
	AddActionExecutor(AllocActionExecutor<AirMeshActionExecutor>());
	AddActionExecutor(AllocActionExecutor<WireModelActionExecutor>());
	AddActionExecutor(AllocActionExecutor<WireMapActionExecutor>());
	AddActionExecutor(AllocActionExecutor<WireSkyActionExecutor>());
	AddActionExecutor(AllocActionExecutor<WireWaterActionExecutor>());
	AddActionExecutor(AllocActionExecutor<CrashActionExecutor>());
	AddActionExecutor(AllocActionExecutor<HangActionExecutor>());
	AddActionExecutor(AllocActionExecutor<ExceptionActionExecutor>());
	AddActionExecutor(AllocActionExecutor<DivByZeroActionExecutor>());
	AddActionExecutor(AllocActionExecutor<GiveActionExecutor>());
	AddActionExecutor(AllocActionExecutor<DestroyActionExecutor>());
	AddActionExecutor(AllocActionExecutor<RemoveActionExecutor>());
	AddActionExecutor(AllocActionExecutor<SendActionExecutor>());
	AddActionExecutor(AllocActionExecutor<DumpStateActionExecutor>());
	AddActionExecutor(AllocActionExecutor<DumpRNGActionExecutor>());
	AddActionExecutor(AllocActionExecutor<SaveActionExecutor>(true));
	AddActionExecutor(AllocActionExecutor<SaveActionExecutor>(false));
	AddActionExecutor(AllocActionExecutor<ReloadShadersActionExecutor>());
	AddActionExecutor(AllocActionExecutor<ReloadTexturesActionExecutor>());
	AddActionExecutor(AllocActionExecutor<DumpAtlasActionExecutor>());
	AddActionExecutor(AllocActionExecutor<DebugInfoActionExecutor>());

	// XXX are these redirects really required?
	AddActionExecutor(AllocActionExecutor<RedirectToSyncedActionExecutor>("ATM"));
#ifdef DEBUG
	AddActionExecutor(AllocActionExecutor<RedirectToSyncedActionExecutor>("Desync"));
#endif
	AddActionExecutor(AllocActionExecutor<RedirectToSyncedActionExecutor>("Resync"));
	if (modInfo.allowTake)
		AddActionExecutor(AllocActionExecutor<RedirectToSyncedActionExecutor>("Take"));

	AddActionExecutor(AllocActionExecutor<RedirectToSyncedActionExecutor>("LuaRules"));
	AddActionExecutor(AllocActionExecutor<RedirectToSyncedActionExecutor>("LuaGaia"));
	AddActionExecutor(AllocActionExecutor<CommandListActionExecutor>());
	AddActionExecutor(AllocActionExecutor<CommandHelpActionExecutor>());
}


alignas(UnsyncedGameCommands) static std::byte ugcSingletonMem[sizeof(UnsyncedGameCommands)];

void UnsyncedGameCommands::CreateInstance() {
	UnsyncedGameCommands*& singleton = GetInstance();

	if (singleton != nullptr)
		return;

	singleton = new (ugcSingletonMem) UnsyncedGameCommands();
}

void UnsyncedGameCommands::DestroyInstance(bool reload) {
	UnsyncedGameCommands*& singleton = GetInstance();

	// executors should be inaccessible in between reloads
	if (reload)
		return;

	spring::SafeDestruct(singleton);
	std::memset(ugcSingletonMem, 0, sizeof(ugcSingletonMem));
}

