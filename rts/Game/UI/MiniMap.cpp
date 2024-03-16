/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <array>
#include <tuple>

#include <SDL_keycode.h>
#include <SDL_mouse.h>

#include "CommandColors.h"
#include "GuiHandler.h"
#include "MiniMap.h"
#include "MouseHandler.h"
#include "TooltipConsole.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/GameHelper.h"
#include "Game/GlobalUnsynced.h"
#include "Game/SelectedUnitsHandler.h"
#include "Game/Players/Player.h"
#include "Game/UI/UnitTracker.h"
#include "Game/TraceRay.h"
#include "Sim/Misc/TeamHandler.h"
#include "Lua/LuaUnsyncedCtrl.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Rendering/CommandDrawer.h"
#include "Rendering/IconHandler.h"
#include "Rendering/LineDrawer.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/DebugVisibilityDrawer.h"
#include "Rendering/Map/InfoTexture/IInfoTextureHandler.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Rendering/Units/UnitDrawer.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/glExtra.h"
#include "Rendering/GL/RenderBuffers.h"
#include "Rendering/GL/SubState.h"
#include "Rendering/Textures/Bitmap.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/Weapon.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/StringHash.h"
#include "System/StringUtil.h"
#include "System/TimeProfiler.h"
#include "System/Input/KeyInput.h"
#include "System/FileSystem/SimpleParser.h"
#include "System/Sound/ISoundChannels.h"

#include <tracy/Tracy.hpp>

using namespace GL::State;


CONFIG(std::string, MiniMapGeometry).defaultValue("2 2 200 200");
CONFIG(bool, MiniMapFullProxy).defaultValue(true);
CONFIG(int, MiniMapButtonSize).defaultValue(16);

CONFIG(float, MiniMapUnitSize)
	.defaultValue(2.5f)
	.minimumValue(0.0f);

CONFIG(float, MiniMapUnitExp).defaultValue(0.25f);
CONFIG(float, MiniMapCursorScale).defaultValue(-0.5f);
CONFIG(bool, MiniMapIcons).defaultValue(true).headlessValue(false);

CONFIG(int, MiniMapDrawCommands).defaultValue(1).headlessValue(0).minimumValue(0);

CONFIG(bool, MiniMapDrawProjectiles).defaultValue(true).headlessValue(false);
CONFIG(bool, SimpleMiniMapColors).defaultValue(false);

CONFIG(bool, MiniMapRenderToTexture).defaultValue(true).safemodeValue(false).description("Asynchronous render MiniMap to a texture independent of screen FPS.");
CONFIG(int, MiniMapRefreshRate).defaultValue(0).minimumValue(0).description("The refresh rate of the async MiniMap texture. Needs MiniMapRenderToTexture to be true. Value of \"0\" autoselects between 10-60FPS.");

CONFIG(bool, DualScreenMiniMapAspectRatio).defaultValue(true).description("Whether minimap preserves aspect ratio on dual screen mode.");

CONFIG(bool, MiniMapCanFlip).defaultValue(false).description("Whether minimap inverts coordinates when camera Y rotation is between 90 and 270 degrees.");


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#define USE_CLIP_PLANES 0

CMiniMap* minimap = nullptr;

CMiniMap::CMiniMap()
	: CInputReceiver(BACK)
	, myColor(0.2f, 0.9f, 0.2f, 1.0f)
	, allyColor(0.3f, 0.3f, 0.9f, 1.0f)
	, enemyColor(0.9f, 0.2f, 0.2f, 1.0f)
 {
	if (!globalRendering->dualScreenMode) {
		ParseGeometry(configHandler->GetString("MiniMapGeometry"));
	}

	fullProxy = configHandler->GetBool("MiniMapFullProxy");

	unitBaseSize = configHandler->GetFloat("MiniMapUnitSize");
	unitExponent = configHandler->GetFloat("MiniMapUnitExp");

	simpleColors = configHandler->GetBool("SimpleMiniMapColors");
	minimapRefreshRate = configHandler->GetInt("MiniMapRefreshRate");
	renderToTexture = configHandler->GetBool("MiniMapRenderToTexture") && FBO::IsSupported();

	ConfigUpdate();

	configHandler->NotifyOnChange(this, {"DualScreenMiniMapAspectRatio", "MiniMapCanFlip", "MiniMapDrawProjectiles", "MiniMapCursorScale", "MiniMapIcons", "MiniMapDrawCommands", "MiniMapButtonSize"});

	UpdateGeometry();

	const float isx = mapDims.mapx / float(mapDims.pwr2mapx);
	const float isy = mapDims.mapy / float(mapDims.pwr2mapy);

	bgShader = shaderHandler->CreateProgramObject("[MiniMap]", "Background");
	bgShader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/MiniMapVertProg.glsl", "", GL_VERTEX_SHADER));
	bgShader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/MiniMapFragProg.glsl", "", GL_FRAGMENT_SHADER));
	bgShader->BindAttribLocation("vertexPos", 0);
	bgShader->BindAttribLocation("texCoords", 1);
	bgShader->Link();

	bgShader->Enable();
	bgShader->SetUniform("shadingTex", 0);
	bgShader->SetUniform("minimapTex", 1);
	bgShader->SetUniform("infomapTex", 2);
	bgShader->SetUniform("uvMult", isx, isy);
	bgShader->SetUniform("infotexMul", 0.0f);
	bgShader->Disable();

	// setup the buttons' texture and texture coordinates
	buttonsTextureID = 0;
	CBitmap bitmap;
	bool unfiltered = false;
	if (bitmap.Load("bitmaps/minimapbuttons.png")) {
		if ((bitmap.ysize == buttonSize) && (bitmap.xsize == (buttonSize * 4))) {
			unfiltered = true;
		}
		glGenTextures(1, &buttonsTextureID);
		glBindTexture(GL_TEXTURE_2D, buttonsTextureID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
								 bitmap.xsize, bitmap.ysize, 0,
								 GL_RGBA, GL_UNSIGNED_BYTE, bitmap.GetRawMem());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		if (unfiltered) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		} else {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	const float xshift = unfiltered ? 0.0f : (0.5f / bitmap.xsize);
	const float yshift = unfiltered ? 0.0f : (0.5f / bitmap.ysize);
	    moveBox.xminTx = 0.50f + xshift;
	    moveBox.xmaxTx = 0.75f - xshift;
	  resizeBox.xminTx = 0.75f + xshift;
	  resizeBox.xmaxTx = 1.00f - xshift;
	minimizeBox.xminTx = 0.00f + xshift;
	minimizeBox.xmaxTx = 0.25f - xshift;
	maximizeBox.xminTx = 0.25f + xshift;
	maximizeBox.xmaxTx = 0.50f - xshift;
	    moveBox.yminTx = 1.00f - yshift;
	  resizeBox.yminTx = 1.00f - yshift;
	minimizeBox.yminTx = 1.00f - yshift;
	maximizeBox.yminTx = 1.00f - yshift;
	    moveBox.ymaxTx = 0.00f + yshift;
	  resizeBox.ymaxTx = 0.00f + yshift;
	minimizeBox.ymaxTx = 0.00f + yshift;
	maximizeBox.ymaxTx = 0.00f + yshift;
}


CMiniMap::~CMiniMap()
{
	//ZoneScoped;
	shaderHandler->ReleaseProgramObjects("[MiniMap]");

	glDeleteTextures(1, &buttonsTextureID);
	glDeleteTextures(1, &minimapTex);

	configHandler->RemoveObserver(this);
}

void CMiniMap::ConfigUpdate()
{
	//ZoneScoped;
	aspectRatio = configHandler->GetBool("DualScreenMiniMapAspectRatio");
	buttonSize = configHandler->GetInt("MiniMapButtonSize");
	drawProjectiles = configHandler->GetBool("MiniMapDrawProjectiles");
	drawCommands = configHandler->GetInt("MiniMapDrawCommands");
	cursorScale = configHandler->GetFloat("MiniMapCursorScale");
	useIcons = configHandler->GetBool("MiniMapIcons");

	minimapCanFlip = configHandler->GetBool("MiniMapCanFlip");
	if (!minimapCanFlip)
		flipped = false;
}

void CMiniMap::ConfigNotify(const std::string& key, const std::string& value)
{
	//ZoneScoped;
	ConfigUpdate();

	if (key == "DualScreenMiniMapAspectRatio")
		UpdateGeometry();
}

void CMiniMap::ParseGeometry(const string& geostr)
{
	//ZoneScoped;
	const std::string geodef = "2 2 200 200";

	if ((sscanf(geostr.c_str(), "%i %i %i %i", &curPos.x, &curPos.y, &curDim.x, &curDim.y) == 4) && (geostr == geodef)) {
		// default geometry
		curDim.x = -200;
		curDim.y = -200;
	} else {
		if (curDim.x <= 0) { curDim.x = -200; }
		if (curDim.y <= 0) { curDim.y = -200; }
	}

	if ((curDim.x <= 0) && (curDim.y <= 0)) {
		const float hw = math::sqrt(mapDims.mapx * 1.0f / mapDims.mapy);
		curDim.x = (-curDim.x * hw);
		curDim.y = (-curDim.y / hw);
	}
	else if (curDim.x <= 0) {
		curDim.x = curDim.y * (mapDims.mapx * 1.0f / mapDims.mapy);
	}
	else if (curDim.y <= 0) {
		curDim.y = curDim.x * (mapDims.mapy * 1.0f / mapDims.mapx);
	}

	// convert to GL coords (MM origin in bottom-left corner)
	curPos.y = globalRendering->viewSizeY - curDim.y - curPos.y;
}


void CMiniMap::ToggleMaximized(bool _maxspect)
{
	//ZoneScoped;
	if ((maximized = !maximized)) {
		// stash current geometry
		oldPos = curPos;
		oldDim = curDim;
		maxspect = _maxspect;
	} else {
		// restore previous geometry
		curPos = oldPos;
		curDim = oldDim;
	}

	// needed for SetMaximizedGeometry
	UpdateGeometry();
}


void CMiniMap::SetAspectRatioGeometry(const float& viewSizeX, const float& viewSizeY,
		const float& viewPosX, const float& viewPosY, const MINIMAP_POSITION position)
{
	//ZoneScoped;
	const float  mapRatio = (float)mapDims.mapx / (float)mapDims.mapy;
	const float viewRatio = viewSizeX / float(viewSizeY);;

	if (mapRatio > viewRatio) {
		curPos.x = viewPosX;
		curDim.x = viewSizeX;
		curDim.y = viewSizeX / mapRatio;
		curPos.y = viewPosY + (viewSizeY - curDim.y) / 2;
	} else {
		curPos.y = viewPosY;
		curDim.y = viewSizeY;
		curDim.x = viewSizeY * mapRatio;
		// CENTER = 0, LEFT = 1, CENTER = 2
		//curPos.x = viewPosX + (position > 0) * (viewSizeX - curDim.x) / position;
		switch (position)
		{
			case (MINIMAP_POSITION_CENTER):
				{
					curPos.x = viewPosX + (viewSizeX - curDim.x) / 2;
				} break;
			case (MINIMAP_POSITION_LEFT):
				{
					curPos.x = viewPosX;
				} break;
			case (MINIMAP_POSITION_RIGHT):
				{
					curPos.x = viewPosX + (viewSizeX - curDim.x);
				} break;
		}
	}
}


void CMiniMap::LoadDualViewport() const {
	glEnable(GL_SCISSOR_TEST);
	glScissor(globalRendering->dualViewPosX, globalRendering->dualViewPosY, globalRendering->dualViewSizeX, globalRendering->dualViewSizeY);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_SCISSOR_TEST);

	glViewport(curPos.x, curPos.y, curDim.x, curDim.y);
}


void CMiniMap::SetMaximizedGeometry()
{
	//ZoneScoped;
	if (!maxspect) {
		curDim.y = globalRendering->viewSizeY;
		curDim.x = curDim.y;
		curPos.x = (globalRendering->viewSizeX - globalRendering->viewSizeY) / 2;
		curPos.y = 0;
		return;
	}

	SetAspectRatioGeometry(globalRendering->viewSizeX, globalRendering->viewSizeY);
}


/******************************************************************************/

void CMiniMap::SetSlaveMode(bool newMode)
{
	//ZoneScoped;
	if (newMode) {
		proxyMode   = false;
		selecting   = false;
		maxspect    = false;
		maximized   = false;
		minimized   = false;
		mouseLook   = false;
		mouseMove   = false;
		mouseResize = false;
	}

	if (newMode != slaveDrawMode) {
		static int oldButtonSize = 16;

		if (newMode) {
			oldButtonSize = buttonSize;
			buttonSize = 0;
		} else {
			buttonSize = oldButtonSize;
		}
	}

	slaveDrawMode = newMode;
	UpdateGeometry();
}


void CMiniMap::ConfigCommand(const std::string& line)
{
	//ZoneScoped;
	const std::vector<std::string>& words = CSimpleParser::Tokenize(line, 1);
	if (words.empty())
		return;

	switch (hashStringLower(words[0].c_str())) {
		case hashString("fullproxy"): {
			fullProxy = (words.size() >= 2) ? !!atoi(words[1].c_str()) : !fullProxy;
		} break;
		case hashString("icons"): {
			useIcons = (words.size() >= 2) ? !!atoi(words[1].c_str()) : !useIcons;
		} break;
		case hashString("unitexp"): {
			if (words.size() >= 2)
				unitExponent = atof(words[1].c_str());

			UpdateGeometry();
		} break;
		case hashString("unitsize"): {
			if (words.size() >= 2)
				unitBaseSize = atof(words[1].c_str());

			unitBaseSize = std::max(0.0f, unitBaseSize);
			UpdateGeometry();
		} break;
		case hashString("drawcommands"): {
			if (words.size() >= 2) {
				drawCommands = std::max(0, atoi(words[1].c_str()));
			} else {
				drawCommands = (drawCommands > 0) ? 0 : 1;
			}
		} break;
		case hashString("drawprojectiles"): {
			drawProjectiles = (words.size() >= 2) ? !!atoi(words[1].c_str()) : !drawProjectiles;
		} break;
		case hashString("simplecolors"): {
			simpleColors = (words.size() >= 2) ? !!atoi(words[1].c_str()) : !simpleColors;
		} break;

		// the following commands can not be used in dualscreen mode
		case hashString("geo"):
		case hashString("geometry"): {
			if (globalRendering->dualScreenMode)
				return;
			if (words.size() < 2)
				return;

			ParseGeometry(words[1]);
			UpdateGeometry();
		} break;

		case hashString("min"):
		case hashString("minimize"): {
			if (globalRendering->dualScreenMode)
				return;

			minimized = (words.size() >= 2) ? !!atoi(words[1].c_str()) : !minimized;
		} break;

		case hashString("max"):
		case hashString("maximize"):
		case hashString("maxspect"): {
			if (globalRendering->dualScreenMode)
				return;

			const bool   isMaximized = maximized;
			const bool wantMaximized = (words.size() >= 2) ? !!atoi(words[1].c_str()) : !isMaximized;

			if (isMaximized != wantMaximized)
				ToggleMaximized(StrCaseStr(words[0].c_str(), "maxspect") == 0);
		} break;

		case hashString("mouseevents"): {
			mouseEvents = (words.size() >= 2) ? !!atoi(words[1].c_str()) : !mouseEvents;
		} break;

		default: {
		} break;
	}
}

/******************************************************************************/

void CMiniMap::SetGeometry(int px, int py, int sx, int sy)
{
	//ZoneScoped;
	curPos = {px, py};
	curDim = {sx, sy};

	UpdateGeometry();
}


void CMiniMap::UpdateGeometry()
{
	//ZoneScoped;
	// keep the same distance to the top
	if (globalRendering->dualScreenMode) {
		if (aspectRatio) {
			const MINIMAP_POSITION position = globalRendering->viewPosX > globalRendering->dualViewPosX ? MINIMAP_POSITION_RIGHT : MINIMAP_POSITION_LEFT;
			SetAspectRatioGeometry(globalRendering->dualViewSizeX, globalRendering->dualViewSizeY, globalRendering->dualViewPosX, globalRendering->dualViewPosY, position);
		} else {
			curPos.x = globalRendering->dualViewPosX;
			curPos.y = globalRendering->dualViewPosY;
			curDim.x = globalRendering->dualViewSizeX;
			curDim.y = globalRendering->dualViewSizeY;
		}
	}
	else if (maximized) {
		SetMaximizedGeometry();
	}
	else {
		curDim.x = std::clamp(curDim.x, 1, globalRendering->viewSizeX);
		curDim.y = std::clamp(curDim.y, 1, globalRendering->viewSizeY);

		curPos.y = std::max(slaveDrawMode ? 0 : buttonSize, curPos.y);
		curPos.y = std::min(globalRendering->viewSizeY - curDim.y, curPos.y);
		curPos.x = std::clamp(curPos.x, 0, globalRendering->viewSizeX - curDim.x);
	}

	{
		// Draw{WorldStuff} transform
		viewMats[0].LoadIdentity();
		viewMats[0].Translate(UpVector);
		viewMats[0].Scale({ +1.0f / (mapDims.mapx * SQUARE_SIZE), -1.0f / (mapDims.mapy * SQUARE_SIZE), 1.0f });
		viewMats[0].RotateX(90.0f * math::DEG_TO_RAD); // rotate to match real 'world' coordinates
		viewMats[0].Scale(XZVector + UpVector * 0.0001f); // (invertibly) flatten; LuaOpenGL::DrawScreen uses persp-proj so z-values influence x&y

		viewMats[1].LoadIdentity();
		viewMats[1].Translate(UpVector);
		// heightmap (squares) to minimap
		viewMats[1].Scale({ 1.0f / mapDims.mapx, -1.0f / mapDims.mapy, 1.0f });
		// worldmap (elmos) to minimap
		// viewMats[1].Scale({1.0f / (mapDims.mapx * SQUARE_SIZE), -1.0f / (mapDims.mapy * SQUARE_SIZE), 1.0f});

		viewMats[2].LoadIdentity();
		viewMats[2].Scale({ 1.0f / curDim.x, 1.0f / curDim.y, 1.0f });

		projMats[0] = CMatrix44f::ClipOrthoProj01();
		projMats[1] = CMatrix44f::ClipOrthoProj(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, -1.0f, globalRendering->supportClipSpaceControl * 1.0f);
		projMats[2] = projMats[1];
	}

	// setup the unit scaling
	const float w = float(curDim.x);
	const float h = float(curDim.y);
	const float mapx = float(mapDims.mapx * SQUARE_SIZE);
	const float mapy = float(mapDims.mapy * SQUARE_SIZE);
	const float ref  = unitBaseSize / math::pow((200.0f * 200.0f), unitExponent);
	const float dpr  = ref * math::pow((w * h), unitExponent);

	unitSizeX = dpr * (mapx / w);
	unitSizeY = dpr * (mapy / h);
	unitSelectRadius = fastmath::apxsqrt(unitSizeX * unitSizeY);

	// in mouse coordinates
	mapBox.xmin = curPos.x;
	mapBox.xmax = mapBox.xmin + curDim.x - 1;
	if (globalRendering->dualScreenMode) {
		mapBox.ymin = globalRendering->winSizeY - (curPos.y + curDim.y) + std::max(globalRendering->viewWindowOffsetY, globalRendering->dualWindowOffsetY);
	} else { // below should be equal to above regardless of dual screen, but we want to be safe
		mapBox.ymin = globalRendering->viewSizeY - (curPos.y + curDim.y);
	}
	mapBox.ymax = mapBox.ymin + curDim.y - 1;

	// FIXME:
	//   also need to make sure we can leave maximized-mode when !maxspect (in
	//   which case the buttons should be drawn on top of map, not outside it)
	if (!maximized || maxspect) {
		// work right (resizeBox) to left (minimizeBox)
		resizeBox.xmax   = mapBox.xmax;
		resizeBox.xmin   = resizeBox.xmax - (buttonSize - 1);

		moveBox.xmax     = resizeBox.xmax   - buttonSize;
		moveBox.xmin     = resizeBox.xmin   - buttonSize;

		maximizeBox.xmax = moveBox.xmax     - buttonSize;
		maximizeBox.xmin = moveBox.xmin     - buttonSize;

		minimizeBox.xmax = maximizeBox.xmax - buttonSize;
		minimizeBox.xmin = maximizeBox.xmin - buttonSize;

		const int ymin = (mapBox.ymax + 1) + 3; // 3 for the white|black|white
		const int ymax = ymin + (buttonSize - 1);
		resizeBox.ymin = moveBox.ymin = maximizeBox.ymin = minimizeBox.ymin = ymin;
		resizeBox.ymax = moveBox.ymax = maximizeBox.ymax = minimizeBox.ymax = ymax;

		buttonBox.xmin = minimizeBox.xmin;
		buttonBox.xmax = mapBox.xmax;
		buttonBox.ymin = ymin - 3;
		buttonBox.ymax = ymax;
	} else {
		// work left to right
		minimizeBox.xmin = mapBox.xmin;
		minimizeBox.xmax = minimizeBox.xmin + (buttonSize - 1);

		maximizeBox.xmin = minimizeBox.xmin + buttonSize;
		maximizeBox.xmax = minimizeBox.xmax + buttonSize;

		// dead buttons
		resizeBox.xmin = resizeBox.ymin = moveBox.xmin = moveBox.ymin =  0;
		resizeBox.xmax = resizeBox.ymax = moveBox.xmax = moveBox.ymax = -1;

		const int ymin = mapBox.ymin;
		const int ymax = ymin + (buttonSize - 1);
		maximizeBox.ymin = minimizeBox.ymin = ymin;
		maximizeBox.ymax = minimizeBox.ymax = ymax;

		buttonBox.xmin = minimizeBox.xmin;
		buttonBox.xmax = maximizeBox.xmax;
		buttonBox.ymin = ymin - 3;
		buttonBox.ymax = ymax;
	}
}


/******************************************************************************/

void CMiniMap::MoveView(const float3& mapPos)
{
	//ZoneScoped;
	camHandler->CameraTransition(0.0f);
	camHandler->GetCurrentController().SetPos({mapPos.x, 0.0f, mapPos.z});
	unitTracker.Disable();
}


void CMiniMap::SelectUnits(int x, int y)
{
	//ZoneScoped;
	const CUnit *_lastClicked = lastClicked;
	lastClicked = nullptr;

	if (!KeyInput::GetKeyModState(KMOD_SHIFT) && !KeyInput::GetKeyModState(KMOD_CTRL))
		selectedUnitsHandler.ClearSelected();

	CMouseHandler::ButtonPressEvt& bp = mouse->buttons[SDL_BUTTON_LEFT];

	if (fullProxy && (bp.movement > mouse->dragSelectionThreshold)) {
		// use a selection box
		const float3 newMapPos = GetMapPosition(x, y);
		const float3 oldMapPos = GetMapPosition(bp.x, bp.y);

		const float xmin = std::min(oldMapPos.x, newMapPos.x);
		const float xmax = std::max(oldMapPos.x, newMapPos.x);
		const float zmin = std::min(oldMapPos.z, newMapPos.z);
		const float zmax = std::max(oldMapPos.z, newMapPos.z);

		const float4  planeRight(-RgtVector,  xmin);
		const float4   planeLeft( RgtVector, -xmax);
		const float4    planeTop( FwdVector, -zmax);
		const float4 planeBottom(-FwdVector,  zmin);

		selectedUnitsHandler.HandleUnitBoxSelection(planeRight, planeLeft, planeTop, planeBottom);
	} else {
		// Single unit
		const float3 pos = GetMapPosition(x, y);

		CUnit* unit;
		if (gu->spectatingFullSelect) {
			unit = CGameHelper::GetClosestUnit(pos, unitSelectRadius);
		} else {
			unit = CGameHelper::GetClosestFriendlyUnit(nullptr, pos, unitSelectRadius, gu->myAllyTeam);
		}

		selectedUnitsHandler.HandleSingleUnitClickSelection(lastClicked = unit, false, bp.lastRelease >= (gu->gameTime - mouse->doubleClickTime) && unit == _lastClicked);
	}

	bp.lastRelease = gu->gameTime;
}

/******************************************************************************/

void CMiniMap::MouseWheel(bool up, float delta)
{
	//ZoneScoped;
	float3 mapPos = GetMapPosition(mouse->lastx, mouse->lasty);
	mapPos.y = CGround::GetHeightAboveWater(mapPos.x, mapPos.z, false);

	// If cursor position in minimap refers to a point outside camera view just move to it
	if (!camera->InView(mapPos)) {
		MoveView(mapPos);
		return;
	}

	float3 newDir = (mapPos - camera->pos);
	newDir.LengthNormalize();

	camHandler->GetCurrentController().MouseWheelMove(delta * mouse->scrollWheelSpeed, newDir);
}

bool CMiniMap::MousePress(int x, int y, int button)
{
	//ZoneScoped;
	if (!mouseEvents)
		return false;

	if (minimized) {
		if ((x < buttonSize) && (y < buttonSize)) {
			minimized = false;
			return true;
		}

		return false;
	}

	const bool inMap = mapBox.Inside(x, y);
	const bool inButtons = buttonBox.Inside(x, y);

	if (!inMap && !inButtons)
		return false;

	if (button == SDL_BUTTON_LEFT) {
		if (inMap && (guihandler->inCommand >= 0)) {
			proxyMode = true;
			ProxyMousePress(x, y, button);
			return true;
		}
		if (showButtons && inButtons) {
			if (moveBox.Inside(x, y)) {
				mouseMove = true;
				return true;
			}
			else if (resizeBox.Inside(x, y)) {
				mouseResize = true;
				return true;
			}
			else if (minimizeBox.Inside(x, y) || maximizeBox.Inside(x, y)) {
				return true;
			}
		}
		if (inMap && !mouse->buttons[SDL_BUTTON_LEFT].chorded) {
			selecting = true;
			return true;
		}
	}
	else if (inMap) {
		if ((fullProxy && (button == SDL_BUTTON_MIDDLE)) || (!fullProxy && (button == SDL_BUTTON_RIGHT))) {
			MoveView(x, y);

			if (maximized) {
				ToggleMaximized(false);
			} else {
				mouseLook = true;
			}

			return true;
		}
		else if (fullProxy && (button == SDL_BUTTON_RIGHT)) {
			proxyMode = true;
			ProxyMousePress(x, y, button);
			return true;
		}
	}

	return false;
}


void CMiniMap::MouseMove(int x, int y, int dx, int dy, int button)
{
	//ZoneScoped;
	// if Press is not handled, should never get Move
	assert(mouseEvents);

	if (mouseMove) {
		curPos.x += dx;
		curPos.y -= dy;
		curPos.x = std::max(0, curPos.x);

		if (globalRendering->dualScreenMode) {
			curPos.x = std::min((2 * globalRendering->viewSizeX) - curDim.x, curPos.x);
		} else {
			curPos.x = std::min(globalRendering->viewSizeX - curDim.x, curPos.x);
		}
		curPos.y = std::max(5, std::min(globalRendering->viewSizeY - curDim.y, curPos.y));

		UpdateGeometry();
		return;
	}

	if (mouseResize) {
		curPos.y -= dy;
		curDim.x += dx;
		curDim.y += dy;
		curDim.y  = std::min(globalRendering->viewSizeY, curDim.y);

		if (globalRendering->dualScreenMode) {
			curDim.x = std::min(2 * globalRendering->viewSizeX, curDim.x);
		} else {
			curDim.x = std::min(globalRendering->viewSizeX, curDim.x);
		}

		if (KeyInput::GetKeyModState(KMOD_SHIFT))
			curDim.x = (curDim.y * mapDims.mapx) / mapDims.mapy;

		curDim.x = std::max(5, curDim.x);
		curDim.y = std::max(5, curDim.y);
		curPos.y = std::min(globalRendering->viewSizeY - curDim.y, curPos.y);

		UpdateGeometry();
		return;
	}

	if (mouseLook && mapBox.Inside(x, y)) {
		MoveView(x,y);
		return;
	}
}


void CMiniMap::MouseRelease(int x, int y, int button)
{
	//ZoneScoped;
	// if Press is not handled, should never get Release
	assert(mouseEvents);

	if (mouseMove || mouseResize || mouseLook) {
		mouseMove = false;
		mouseResize = false;
		mouseLook = false;
		proxyMode = false;
		return;
	}

	if (proxyMode) {
		ProxyMouseRelease(x, y, button);
		proxyMode = false;
		return;
	}

	if (selecting) {
		SelectUnits(x, y);
		selecting = false;
		return;
	}

	if (button == SDL_BUTTON_LEFT) {
		if (showButtons && maximizeBox.Inside(x, y)) {
			ToggleMaximized(!!KeyInput::GetKeyModState(KMOD_SHIFT));
			return;
		}

		if (showButtons && minimizeBox.Inside(x, y)) {
			minimized = true;
			return;
		}
	}
}

/******************************************************************************/

CUnit* CMiniMap::GetSelectUnit(const float3& pos) const
{
	//ZoneScoped;
	CUnit* unit = CGameHelper::GetClosestUnit(pos, unitSelectRadius);

	if (unit == nullptr)
		return unit;

	if ((unit->losStatus[gu->myAllyTeam] & (LOS_INLOS | LOS_INRADAR)) || gu->spectatingFullView)
		return unit;

	return nullptr;
}


float3 CMiniMap::GetMapPosition(int x, int y) const
{
	//ZoneScoped;
	const float mapX = float3::maxxpos + 1.0f;
	const float mapZ = float3::maxzpos + 1.0f;

	// mouse coordinate-origin is top-left of screen while
	// the minimap origin is in its bottom-left corner, so
	//   (x =     0, y =     0) maps to world-coors (   0, h,    0)
	//   (x = dim.x, y =     0) maps to world-coors (mapX, h,    0)
	//   (x = dim.x, y = dim.y) maps to world-coors (mapX, h, mapZ)
	//   (x =     0, y = dim.y) maps to world-coors (   0, h, mapZ)

	// translate mouse coords orientation and origin to map coords
	y = y - globalRendering->viewPosY + curDim.y - globalRendering->viewSizeY;

	float sx = std::clamp(float(x - tmpPos.x) / curDim.x, 0.0f, 1.0f);
	float sz = std::clamp(float(y + tmpPos.y) / curDim.y, 0.0f, 1.0f);

	if (flipped) {
		sx = 1 - sx;
		sz = 1 - sz;
	}

	return {mapX * sx, readMap->GetCurrMaxHeight(), mapZ * sz};
}


void CMiniMap::ProxyMousePress(int x, int y, int button)
{
	//ZoneScoped;
	float3 mapPos = GetMapPosition(x, y);
	const CUnit* unit = GetSelectUnit(mapPos);

	if (unit != nullptr) {
		if (gu->spectatingFullView) {
			mapPos = unit->midPos;
		} else {
			mapPos = unit->GetObjDrawErrorPos(gu->myAllyTeam);
			mapPos.y = readMap->GetCurrMaxHeight();
		}
	}

	CMouseHandler::ButtonPressEvt& bp = mouse->buttons[button];
	bp.camPos = mapPos;
	bp.dir = -UpVector;

	guihandler->MousePress(x, y, -button);
}


void CMiniMap::ProxyMouseRelease(int x, int y, int button)
{
	//ZoneScoped;
	float3 mapPos = GetMapPosition(x, y);
	const CUnit* unit = GetSelectUnit(mapPos);

	if (unit != nullptr) {
		if (gu->spectatingFullView) {
			mapPos = unit->midPos;
		} else {
			mapPos = unit->GetObjDrawErrorPos(gu->myAllyTeam);
			mapPos.y = readMap->GetCurrMaxHeight();
		}
	}

	guihandler->MouseRelease(x, y, -button, mapPos, -UpVector);
}


/******************************************************************************/
bool CMiniMap::IsInside(int x, int y)
{
	//ZoneScoped;
	return !minimized && mapBox.Inside(x, y);
}


bool CMiniMap::IsAbove(int x, int y)
{
	//ZoneScoped;
	if (minimized)
		return ((x < buttonSize) && (y < buttonSize));

	if (mapBox.Inside(x, y))
		return true;

	if (showButtons && buttonBox.Inside(x, y))
		return true;

	return false;
}


std::string CMiniMap::GetTooltip(int x, int y)
{
	//ZoneScoped;
	if (minimized)
		return "Unminimize map";

	if (buttonBox.Inside(x, y)) {
		if (resizeBox.Inside(x, y))
			return "Resize map\n(SHIFT to maintain aspect ratio)";

		if (moveBox.Inside(x, y))
			return "Move map";

		if (maximizeBox.Inside(x, y)) {
			if (!maximized)
				return "Maximize map\n(SHIFT to maintain aspect ratio)";

			return "Unmaximize map";
		}

		if (minimizeBox.Inside(x, y))
			return "Minimize map";
	}

	const std::string buildTip = guihandler->GetBuildTooltip();
	if (!buildTip.empty())
		return buildTip;

	const float3 wpos = GetMapPosition(x, y);
	const CUnit* unit = GetSelectUnit(wpos);
	if (unit != nullptr)
		return CTooltipConsole::MakeUnitString(unit);

	const std::string selTip = selectedUnitsHandler.GetTooltip();
	if (!selTip.empty())
		return selTip;

	return CTooltipConsole::MakeGroundString({wpos.x, CGround::GetHeightReal(wpos.x, wpos.z, false), wpos.z});
}


void CMiniMap::AddNotification(float3 pos, float3 color, float alpha)
{
	//ZoneScoped;
	Notification n;
	n.pos = pos;
	n.color[0] = color.x;
	n.color[1] = color.y;
	n.color[2] = color.z;
	n.color[3] = alpha;
	n.creationTime = gu->gameTime;

	notes.push_back(n);
}


/******************************************************************************/

void CMiniMap::DrawCircle(TypedRenderBuffer<VA_TYPE_C>& rb, const float3& pos, SColor color, float radius) const
{
	//ZoneScoped;
	const float xPixels = radius * float(curDim.x) / float(mapDims.mapx * SQUARE_SIZE);
	const float yPixels = radius * float(curDim.y) / float(mapDims.mapy * SQUARE_SIZE);
	const auto lod = static_cast<int>(0.25 * math::log2(1.0f + (xPixels * yPixels)));
	const int divs = 1 << (std::clamp(lod, 0, 6 - 1) + 3);

	for (int d = 0; d < divs; d++) {
		const float step0 = static_cast<float>(d + 0) / static_cast<float>(divs);
		const float step1 = static_cast<float>(d + 1) / static_cast<float>(divs);
		const float rads0 = math::TWOPI * step0;
		const float rads1 = math::TWOPI * step1;

		const float3 vtx0 = { std::sin(rads0), 0.0f, std::cos(rads0) };
		const float3 vtx1 = { std::sin(rads1), 0.0f, std::cos(rads1) };

		rb.AddVertex({ pos + vtx0 * radius, color });
		rb.AddVertex({ pos + vtx1 * radius, color });
	}
}

void CMiniMap::ApplyConstraintsMatrix() const
{
	//ZoneScoped;
	if (!renderToTexture) {
		if (globalRendering->dualScreenMode) {
			glTranslatef(curPos.x, curPos.y, 0.0f);
		} else {
			glTranslatef(curPos.x * globalRendering->pixelX, curPos.y * globalRendering->pixelY, 0.0f);
		}
		glScalef(curDim.x * globalRendering->pixelX, curDim.y * globalRendering->pixelY, 1.0f);
	}
}

float CMiniMap::GetRotation() {
	//ZoneScoped;
	return flipped ? math::PI : 0;
}

/******************************************************************************/

void CMiniMap::Update()
{
	//ZoneScoped;
	// need this because UpdateTextureCache sets curPos={0,0}
	// (while calling DrawForReal, which can reach GetMapPos)
	tmpPos = curPos;

	if (minimized || curDim.x == 0 || curDim.y == 0)
		return;

	if (!renderToTexture)
		return;

	static spring_time nextDrawScreen = spring_gettime();

	if (spring_gettime() <= nextDrawScreen)
		return;

	/* Below the renderToTexture check above,
	 * since that other rendering pipeline
	 * does not support minimap flipping. */
	if (minimapCanFlip) {
		const float rotY = fmod(abs(camHandler->GetCurrentController().GetRot().y), 2 * math::PI);
		flipped = rotY > math::PI/2 && rotY <= 3 * math::PI/2;
	}

	float refreshRate = minimapRefreshRate;

	if (minimapRefreshRate == 0) {
		const float viewArea = globalRendering->viewSizeX * globalRendering->viewSizeY;
		const float mmapArea = (curDim.x * curDim.y) / viewArea;
		refreshRate = (mmapArea >= 0.45f) ? 60 : (mmapArea > 0.15f) ? 25 : 15;
	}
	nextDrawScreen = spring_gettime() + spring_msecs(1000.0f / refreshRate);

	fbo.Bind();
	if (minimapTexSize != curDim)
		ResizeTextureCache();

	fbo.Bind();
	UpdateTextureCache();

	// gets done in CGame
	// fbo.Unbind();
}


void CMiniMap::ResizeTextureCache()
{
	//ZoneScoped;
	minimapTexSize = curDim;
	multisampledFBO = (FBO::GetMaxSamples() > 1);

	if (multisampledFBO) {
		// multisampled FBO we are render to
		fbo.Detach(GL_COLOR_ATTACHMENT0_EXT); // delete old RBO
		fbo.CreateRenderBufferMultisample(GL_COLOR_ATTACHMENT0_EXT, GL_RGBA8, minimapTexSize.x, minimapTexSize.y, 4);
		//fbo.CreateRenderBuffer(GL_DEPTH_ATTACHMENT_EXT, GL_DEPTH_COMPONENT16, minimapTexSize.x, minimapTexSize.y);

		if (!fbo.CheckStatus("MINIMAP")) {
			fbo.Detach(GL_COLOR_ATTACHMENT0_EXT);
			multisampledFBO = false;
		}
	}

	glDeleteTextures(1, &minimapTex);
	glGenTextures(1, &minimapTex);

	glBindTexture(GL_TEXTURE_2D, minimapTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, minimapTexSize.x, minimapTexSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	if (multisampledFBO) {
		// resolve FBO with attached final texture target
		fboResolve.Bind();
		fboResolve.AttachTexture(minimapTex);

		if (!fboResolve.CheckStatus("MINIMAP-RESOLVE")) {
			renderToTexture = false;
			return;
		}
	} else {
		// directly render to texture without multisampling (fallback solution)
		fbo.Bind();
		fbo.AttachTexture(minimapTex);

		if (!fbo.CheckStatus("MINIMAP-RESOLVE")) {
			renderToTexture = false;
			return;
		}
	}
}


void CMiniMap::UpdateTextureCache()
{
	//ZoneScoped;
	// draws minimap into FBO
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0,1,0,1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	{
		curPos = {0, 0};

		glViewport(0, 0, minimapTexSize.x, minimapTexSize.y);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		DrawForReal(false, true, false);

		curPos = tmpPos;
	}

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	// resolve multisampled FBO if there is one
	if (multisampledFBO) {
		const std::array rect = { 0, 0, minimapTexSize.x, minimapTexSize.y };
		FBO::Blit(fbo.fboId, fboResolve.fboId, rect, rect, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	}
}


/******************************************************************************/

void CMiniMap::Draw()
{
	ZoneScopedN("MiniMap::Draw");
	if (slaveDrawMode)
		return;

	// Draw Border
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		auto state = GL::SubState(
			DepthTest(GL_FALSE),
			DepthFunc(GL_LEQUAL),
			DepthMask(GL_FALSE));

		glDisable(GL_TEXTURE_2D);
		glMatrixMode(GL_MODELVIEW);

		if (minimized) {
			DrawMinimizedButtonQuad();
			DrawMinimizedButtonLoop();
			glEnable(GL_TEXTURE_2D);
			return;
		}

		// draw the frameborder
		if (!globalRendering->dualScreenMode) {
			DrawFrame();
			DrawButtons();
		}
	}

	// draw minimap itself
	DrawForReal(true, false, false);
}

void CMiniMap::DrawMinimizedButtonQuad() const
{
	//ZoneScoped;
	auto& rb = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_2DTC>();
	rb.AssertSubmission();

	const float px = globalRendering->pixelX;
	const float py = globalRendering->pixelY;

	const float xmin = (globalRendering->viewPosX + 1 +          0) * px;
	const float xmax = (globalRendering->viewPosX + 1 + buttonSize) * px;
	const float ymin = 1.0f - (1 + buttonSize) * py;
	const float ymax = 1.0f - (1 +          0) * py;

	glBindTexture(GL_TEXTURE_2D, buttonsTextureID);

	rb.AddQuadTriangles(
		{ xmin, ymin, minimizeBox.xminTx, minimizeBox.yminTx, {1.0f, 1.0f, 1.0f, 1.0f} },
		{ xmax, ymin, minimizeBox.xmaxTx, minimizeBox.yminTx, {1.0f, 1.0f, 1.0f, 1.0f} },
		{ xmax, ymax, minimizeBox.xmaxTx, minimizeBox.ymaxTx, {1.0f, 1.0f, 1.0f, 1.0f} },
		{ xmin, ymax, minimizeBox.xminTx, minimizeBox.ymaxTx, {1.0f, 1.0f, 1.0f, 1.0f} }
	);

	auto& sh = rb.GetShader();
	sh.Enable();
	rb.DrawElements(GL_TRIANGLES);
	sh.Disable();
}

void CMiniMap::DrawMinimizedButtonLoop() const
{
	//ZoneScoped;
	auto& rb = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_2DC>();
	rb.AssertSubmission();

	const float px = globalRendering->pixelX;
	const float py = globalRendering->pixelY;

	const float xmin = (globalRendering->viewPosX + 1 +          0) * px;
	const float xmax = (globalRendering->viewPosX + 1 + buttonSize) * px;
	const float ymin = 1.0f - (1 + buttonSize) * py;
	const float ymax = 1.0f - (1 +          0) * py;

	auto& sh = rb.GetShader();
	sh.Enable();

	// highlight
	if (((mouse->lastx + 1) <= buttonSize) && ((mouse->lasty + 1) <= buttonSize)) {
		rb.AddQuadTriangles(
			{ xmin, ymin, {1.0f, 1.0f, 1.0f, 0.4f} },
			{ xmax, ymin, {1.0f, 1.0f, 1.0f, 0.4f} },
			{ xmax, ymax, {1.0f, 1.0f, 1.0f, 0.4f} },
			{ xmin, ymax, {1.0f, 1.0f, 1.0f, 0.4f} }
		);
		rb.DrawElements(GL_TRIANGLES);
	}

	// outline
	rb.AddQuadLines(
		{ xmin - 0.5f * px, ymax + 0.5f * py, {0.0f, 0.0f, 0.0f, 1.0f} },
		{ xmin - 0.5f * px, ymin - 0.5f * py, {0.0f, 0.0f, 0.0f, 1.0f} },
		{ xmax + 0.5f * px, ymin - 0.5f * py, {0.0f, 0.0f, 0.0f, 1.0f} },
		{ xmax + 0.5f * px, ymax + 0.5f * py, {0.0f, 0.0f, 0.0f, 1.0f} }
	);
	rb.Submit(GL_LINES);

	sh.Disable();
}


void CMiniMap::DrawForReal(bool useNormalizedCoors, bool updateTex, bool luaCall)
{
	ZoneScopedN("MiniMap::DrawForReal");
	if (minimized)
		return;

	glActiveTexture(GL_TEXTURE0);

	if (!updateTex) {
		RenderCachedTexture(useNormalizedCoors);
		return;
	}

	glPushAttrib(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_FALSE);
	glDisable(GL_TEXTURE_2D);
	glMatrixMode(GL_MODELVIEW);

	if (useNormalizedCoors) {
		glPushMatrix();

		// switch to normalized minimap coords
		if (globalRendering->dualScreenMode) {
			LoadDualViewport();
		} else {
			glTranslatef(curPos.x * globalRendering->pixelX, curPos.y * globalRendering->pixelY, 0.0f);
			glScalef(curDim.x * globalRendering->pixelX, curDim.y * globalRendering->pixelY, 1.0f);
		}
	}

	cursorIcons.Enable(false);

#if USE_CLIP_PLANES
	// clip everything outside of the minimap box
	SetClipPlanes(false);
	glEnable(GL_CLIP_PLANE0);
	glEnable(GL_CLIP_PLANE1);
	glEnable(GL_CLIP_PLANE2);
	glEnable(GL_CLIP_PLANE3);
#endif
	DrawBackground();

	// allow Lua scripts to overdraw the background image
#if USE_CLIP_PLANES
	SetClipPlanes(true);
#endif
	eventHandler.DrawInMiniMapBackground();
#if USE_CLIP_PLANES
	SetClipPlanes(false);
#endif

	DrawUnitIcons();
	DrawWorldStuff();

	if (useNormalizedCoors)
		glPopMatrix();

	glPopAttrib();
	glEnable(GL_TEXTURE_2D);

	// allow Lua scripts to draw into the minimap
	SetClipPlanes(true);
	eventHandler.DrawInMiniMap();

	if (!updateTex) {
		glPushMatrix();
			if (globalRendering->dualScreenMode) {
				glTranslatef(curPos.x, curPos.y, 0.0f);
			} else {
				glTranslatef(curPos.x * globalRendering->pixelX, curPos.y * globalRendering->pixelY, 0.0f);
				glScalef(curDim.x * globalRendering->pixelX, curDim.y * globalRendering->pixelY, 1.0f);
			}
			DrawCameraFrustumAndMouseSelection();
		glPopMatrix();
	}

	// Finish
	// Reset of GL state
	if (useNormalizedCoors && globalRendering->dualScreenMode)
		globalRendering->LoadViewport();

	// disable ClipPlanes
#if USE_CLIP_PLANES
	glDisable(GL_CLIP_PLANE0);
	glDisable(GL_CLIP_PLANE1);
	glDisable(GL_CLIP_PLANE2);
	glDisable(GL_CLIP_PLANE3);
#endif
	cursorIcons.Enable(true);
}


/******************************************************************************/
/******************************************************************************/

void CMiniMap::DrawCameraFrustumAndMouseSelection()
{
	//ZoneScoped;
	glEnable(GL_SCISSOR_TEST);
	glScissor(curPos.x, curPos.y, curDim.x, curDim.y);

	// switch to top-down map/world coords (z is twisted with y compared to the real map/world coords)
	glPushMatrix();
	if (flipped) {
		glTranslatef(+1.0f, 0.0f, 0.0f);
		glScalef(-1.0f / (mapDims.mapx * SQUARE_SIZE), +1.0f / (mapDims.mapy * SQUARE_SIZE), 1.0f);
	} else {
		glTranslatef(0.0f, +1.0f, 0.0f);
		glScalef(+1.0f / (mapDims.mapx * SQUARE_SIZE), -1.0f / (mapDims.mapy * SQUARE_SIZE), 1.0f);
	}

	static auto& rb = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_2D0>();
	rb.AssertSubmission();
	auto& sh = rb.GetShader();

	if (!minimap->maximized) {
		// draw the camera frustum lines
		// CCamera* cam = CCameraHandler::GetCamera(CCamera::CAMTYPE_SHADOW);
		CCamera* cam = CCameraHandler::GetCamera(CCamera::CAMTYPE_PLAYER);

		//this one is bugged, probably because CalcFrustumLines is bugged as well
		// TODO: Investigate
#if 0
		cam->CalcFrustumLines(readMap->GetCurrAvgHeight(), readMap->GetCurrAvgHeight(), 1.0f, true);
		cam->ClipFrustumLines(-100.0f, mapDims.mapy * SQUARE_SIZE + 100.0f, true);

		const CCamera::FrustumLine* negLines = cam->GetNegFrustumLines();

		CVertexArray* va = GetVertexArray();
		va->Initialize();
		va->EnlargeArrays(4 * 2, 0, VA_SIZE_2D0);


		for (int idx = 0; idx < /*negLines[*/4/*].sign*/; idx++) {
			const CCamera::FrustumLine& fl = negLines[idx];

			if (fl.minz >= fl.maxz)
				continue;

			va->AddVertexQ2d0((fl.dir * fl.minz) + fl.base, fl.minz);
			va->AddVertexQ2d0((fl.dir * fl.maxz) + fl.base, fl.maxz);
		}
#else
		const auto& pos = cam->GetPos();
		const auto& dir = cam->GetForward();

		const CUnit* unit = nullptr;
		const CFeature* feature = nullptr;

		const float rawRange = cam->GetFarPlaneDist() * 1.4f;
		const float badRange = rawRange - 300.0f;

		const float traceDist = TraceRay::GuiTraceRay(pos, dir, rawRange, nullptr, unit, feature, true, true, true);
		const float3 tracePos = pos + (dir * traceDist);

		float Y = tracePos.y;

		if (traceDist < 0.0f || traceDist > badRange)
			Y = readMap->GetCurrAvgHeight();

		constexpr float3 plane = UpVector;

		std::array<std::pair<float, float>, 4> pts;

		uint8_t negCount = 0;
		for (int i = 0; i < 4; ++i) {
			const auto& fv = cam->GetFrustumVert(4 + i);
			const float3 ray = (fv - pos);

			float denom = plane.dot(ray);
			if (denom == 0.0f)
				continue;

			float t = (Y - plane.dot(pos)) / denom;
			if (t < 0.0f) { //if intersection happens "behind" the "pos", hack "t" to still point in front of the camera
				t = 1.0f - t;
				++negCount;
			}

			float3 xpos = pos + ray * t;
			pts[i] = std::make_pair(xpos.x, xpos.z);
		}

		//if negCount == 4, then all intersections happen behind the "pos", doesn't make sense to draw anything but a small box
		if (negCount == 4) {
			constexpr float bias = 16.0f;
			pts[0] = std::make_pair(pos.x - bias, pos.z + bias); //TL
			pts[1] = std::make_pair(pos.x + bias, pos.z + bias); //TR
			pts[2] = std::make_pair(pos.x + bias, pos.z - bias); //BR
			pts[3] = std::make_pair(pos.x - bias, pos.z - bias); //BL
		}

		for (int i = 0; i < pts.size(); ++i) {
			rb.AddVertex({ pts[i].first, pts[i].second });
		}

		glLineWidth(2.5f);
		sh.Enable();

		sh.SetUniform("ucolor", 0.0f, 0.0f, 0.0f, 0.5f);
		rb.DrawArrays(GL_LINE_LOOP, false);

		glLineWidth(1.5f);
		sh.SetUniform("ucolor", 1.0f, 1.0f, 1.0f, 0.75f);
		rb.DrawArrays(GL_LINE_LOOP);

		sh.SetUniform("ucolor", 1.0f, 1.0f, 1.0f, 1.0f);
		sh.Disable();

		glLineWidth(1.0f);
#endif
	}


	// selection box
	CMouseHandler::ButtonPressEvt& bp = mouse->buttons[SDL_BUTTON_LEFT];
	if (selecting && fullProxy && (bp.movement > mouse->dragSelectionThreshold)) {
		const float3 oldMapPos = GetMapPosition(bp.x, bp.y);
		const float3 newMapPos = GetMapPosition(mouse->lastx, mouse->lasty);

		//glBlendFunc((GLenum)cmdColors.MouseBoxBlendSrc(),
		//            (GLenum)cmdColors.MouseBoxBlendDst());
		glLineWidth(cmdColors.MouseBoxLineWidth());

		rb.AddVertices({
			{oldMapPos.x, oldMapPos.z},
			{newMapPos.x, oldMapPos.z},
			{newMapPos.x, newMapPos.z},
			{oldMapPos.x, newMapPos.z}
		});
		sh.Enable();
		sh.SetUniform("ucolor", cmdColors.mouseBox[0], cmdColors.mouseBox[1], cmdColors.mouseBox[2], cmdColors.mouseBox[3]);
		rb.DrawArrays(GL_LINE_LOOP);
		sh.SetUniform("ucolor", 1.0f, 1.0f, 1.0f, 1.0f);
		sh.Disable();
		glLineWidth(1.0f);
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	DrawNotes();


	// disable ClipPlanes
#if USE_CLIP_PLANES
	glDisable(GL_CLIP_PLANE0);
	glDisable(GL_CLIP_PLANE1);
	glDisable(GL_CLIP_PLANE2);
	glDisable(GL_CLIP_PLANE3);
#endif

	glPopMatrix();

	glDisable(GL_SCISSOR_TEST);
	glEnable(GL_TEXTURE_2D);
}

void CMiniMap::DrawFrame()
{
	//ZoneScoped;
	auto& rb = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_2DC>();
	rb.AssertSubmission();

	const float px = globalRendering->pixelX;
	const float py = globalRendering->pixelY;

	auto& sh = rb.GetShader();
	sh.Enable();

	rb.AddVertex({ float(curPos.x            - 2 + 0.5f) * px, float(curPos.y            - 2 + 0.5f) * py, {0.0f, 0.0f, 0.0f, 1.0f} });
	rb.AddVertex({ float(curPos.x            - 2 + 0.5f) * px, float(curPos.y + curDim.y + 2 - 0.5f) * py, {0.0f, 0.0f, 0.0f, 1.0f} });
	rb.AddVertex({ float(curPos.x + curDim.x + 2 - 0.5f) * px, float(curPos.y + curDim.y + 2 - 0.5f) * py, {0.0f, 0.0f, 0.0f, 1.0f} });
	rb.AddVertex({ float(curPos.x + curDim.x + 2 - 0.5f) * px, float(curPos.y            - 2 + 0.5f) * py, {0.0f, 0.0f, 0.0f, 1.0f} });
	rb.DrawArrays(GL_LINE_LOOP);

	rb.AddVertex({ float(curPos.x            - 1 + 0.5f) * px, float(curPos.y            - 1 + 0.5f) * py, {1.0f, 1.0f, 1.0f, 1.0f} });
	rb.AddVertex({ float(curPos.x            - 1 + 0.5f) * px, float(curPos.y + curDim.y + 1 - 0.5f) * py, {1.0f, 1.0f, 1.0f, 1.0f} });
	rb.AddVertex({ float(curPos.x + curDim.x + 1 - 0.5f) * px, float(curPos.y + curDim.y + 1 - 0.5f) * py, {1.0f, 1.0f, 1.0f, 1.0f} });
	rb.AddVertex({ float(curPos.x + curDim.x + 1 - 0.5f) * px, float(curPos.y            - 1 + 0.5f) * py, {1.0f, 1.0f, 1.0f, 1.0f} });
	rb.DrawArrays(GL_LINE_LOOP);

	sh.Disable();
}

void CMiniMap::IntBox::GetBoxRenderData(TypedRenderBuffer<VA_TYPE_2DC>& rb, SColor col) const
{
	//ZoneScoped;
	const float px = globalRendering->pixelX;
	const float py = globalRendering->pixelY;

	rb.AddQuadTriangles(
		{ float(xmin + 0) * px, 1.0f - float(ymin + 0) * py, col },
		{ float(xmax + 1) * px, 1.0f - float(ymin + 0) * py, col },
		{ float(xmax + 1) * px, 1.0f - float(ymax + 1) * py, col },
		{ float(xmin + 0) * px, 1.0f - float(ymax + 1) * py, col }
	);
}

void CMiniMap::IntBox::GetTextureBoxRenderData(TypedRenderBuffer<VA_TYPE_2DT>& rb) const
{
	//ZoneScoped;
	const float px = globalRendering->pixelX;
	const float py = globalRendering->pixelY;

	rb.AddQuadTriangles(
		{ float(xmin + 0) * px, 1.0f - float(ymin + 0) * py, xminTx, yminTx },
		{ float(xmax + 1) * px, 1.0f - float(ymin + 0) * py, xmaxTx, yminTx },
		{ float(xmax + 1) * px, 1.0f - float(ymax + 1) * py, xmaxTx, ymaxTx },
		{ float(xmin + 0) * px, 1.0f - float(ymax + 1) * py, xminTx, ymaxTx }
	);
}


void CMiniMap::DrawButtons()
{
	//ZoneScoped;
	const int x = mouse->lastx;
	const int y = mouse->lasty;

	// update the showButtons state
	if (!showButtons) {
		if (mapBox.Inside(x, y) && (buttonSize > 0) && !globalRendering->dualScreenMode) {
			showButtons = true;
		} else {
			return;
		}
	} else if (!mouseMove && !mouseResize &&
	           !mapBox.Inside(x, y) && !buttonBox.Inside(x, y)) {
		showButtons = false;
		return;
	}

	auto& rbBox = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_2DC>();
	rbBox.AssertSubmission();
	auto& shBox = rbBox.GetShader();

	if (buttonsTextureID) {
		glBindTexture(GL_TEXTURE_2D, buttonsTextureID);

		auto& rb = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_2DT>();
		rb.AssertSubmission();
		auto& sh = rb.GetShader();
		sh.Enable();
		sh.SetUniform("ucolor", 1.0f, 1.0f, 1.0f, 1.0f);

		resizeBox.GetTextureBoxRenderData(rb);
		moveBox.GetTextureBoxRenderData(rb);
		maximizeBox.GetTextureBoxRenderData(rb);
		minimizeBox.GetTextureBoxRenderData(rb);

		rb.DrawElements(GL_TRIANGLES);
		sh.Disable();
	} else {
		resizeBox.GetBoxRenderData  (rbBox, { 0.1f, 0.1f, 0.8f, 0.8f }); // blue
		moveBox.GetBoxRenderData    (rbBox, { 0.0f, 0.8f, 0.0f, 0.8f }); // green
		maximizeBox.GetBoxRenderData(rbBox, { 0.8f, 0.8f, 0.0f, 0.8f }); // yellow
		minimizeBox.GetBoxRenderData(rbBox, { 0.8f, 0.0f, 0.0f, 0.8f }); // red

		shBox.Enable();
		rbBox.DrawElements(GL_TRIANGLES);
		shBox.Disable();
	}

	// highlight
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	SColor boxColor = SColor(1.0f, 1.0f, 1.0f, 0.4f);
	if (mouseResize || (!mouseMove && resizeBox.Inside(x, y))) {
		if (!buttonsTextureID) { boxColor = SColor(0.3f, 0.4f, 1.0f, 0.9f); }
		resizeBox.GetBoxRenderData(rbBox, boxColor);
	}
	else if (mouseMove || (!mouseResize && moveBox.Inside(x, y))) {
		if (!buttonsTextureID) { boxColor = SColor(1.0f, 1.0f, 1.0f, 0.3f); }
		moveBox.GetBoxRenderData(rbBox, boxColor);
	}
	else if (!mouseMove && !mouseResize) {
		if (minimizeBox.Inside(x, y)) {
			if (!buttonsTextureID) { boxColor = SColor(1.0f, 0.2f, 0.2f, 0.6f); }
			minimizeBox.GetBoxRenderData(rbBox, boxColor);
		} else if (maximizeBox.Inside(x, y)) {
			if (!buttonsTextureID) { boxColor = SColor(1.0f, 1.0f, 1.0f, 0.3f); }
			maximizeBox.GetBoxRenderData(rbBox, boxColor);
		}
	}

	shBox.Enable();
	rbBox.DrawElements(GL_TRIANGLES);

	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// outline the button box
	{
		const float px = globalRendering->pixelX;
		const float py = globalRendering->pixelY;

		auto c0 = SColor(0.0f, 0.0f, 0.0f, 1.0f);
		rbBox.AddVertices({
			{ float(buttonBox.xmin - 1 - 0.5f) * px, 1.0f - float(buttonBox.ymin + 2 - 0.5f) * py, c0 },
			{ float(buttonBox.xmin - 1 - 0.5f) * px, 1.0f - float(buttonBox.ymax + 2 + 0.5f) * py, c0 },
			{ float(buttonBox.xmax + 2 + 0.5f) * px, 1.0f - float(buttonBox.ymax + 2 + 0.5f) * py, c0 },
			{ float(buttonBox.xmax + 2 + 0.5f) * px, 1.0f - float(buttonBox.ymin + 2 - 0.5f) * py, c0 }
		});
		rbBox.DrawArrays(GL_LINE_LOOP);

		auto c1 = SColor(1.0f, 1.0f, 1.0f, 1.0f);
		rbBox.AddVertices({
			{ float(buttonBox.xmin - 0 - 0.5f) * px, 1.0f - float(buttonBox.ymin + 3 - 0.5f) * py, c1 },
			{ float(buttonBox.xmin - 0 - 0.5f) * px, 1.0f - float(buttonBox.ymax + 1 + 0.5f) * py, c1 },
			{ float(buttonBox.xmax + 1 + 0.5f) * px, 1.0f - float(buttonBox.ymax + 1 + 0.5f) * py, c1 },
			{ float(buttonBox.xmax + 1 + 0.5f) * px, 1.0f - float(buttonBox.ymin + 3 - 0.5f) * py, c1 }
		});
		rbBox.DrawArrays(GL_LINE_LOOP);
	}

	shBox.Disable();
}


void CMiniMap::DrawNotes()
{
	//ZoneScoped;
	if (notes.empty()) {
		return;
	}

	const float baseSize = mapDims.mapx * SQUARE_SIZE;
	static auto& rb = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_C>();
	rb.AssertSubmission();
	auto& shader = rb.GetShader();

	std::deque<Notification>::iterator ni = notes.begin();
	while (ni != notes.end()) {
		const float age = gu->gameTime - ni->creationTime;
		if (age > 2) {
			ni = notes.erase(ni);
			continue;
		}

		SColor color(ni->color[0], ni->color[1], ni->color[2], ni->color[3]);
		for (int a = 0; a < 3; ++a) {
			const float modage = age + a * 0.1f;
			const float rot = modage * 3;
			float size = baseSize - modage * baseSize * 0.9f;
			if (size < 0){
				if (size < -baseSize * 0.4f) {
					continue;
				} else if (size > -baseSize * 0.2f) {
					size = modage * baseSize * 0.9f - baseSize;
				} else {
					size = baseSize * 1.4f - modage * baseSize * 0.9f;
				}
			}
			color.a = (255 * ni->color[3]) / (3 - a);
			const float sinSize = fastmath::sin(rot) * size;
			const float cosSize = fastmath::cos(rot) * size;
			rb.AddVertex({ float3(ni->pos.x + sinSize, ni->pos.z + cosSize, 0.0f), color });
			rb.AddVertex({ float3(ni->pos.x + cosSize, ni->pos.z - sinSize, 0.0f), color });
			rb.AddVertex({ float3(ni->pos.x + cosSize, ni->pos.z - sinSize, 0.0f), color });
			rb.AddVertex({ float3(ni->pos.x - sinSize, ni->pos.z - cosSize, 0.0f), color });
			rb.AddVertex({ float3(ni->pos.x - sinSize, ni->pos.z - cosSize, 0.0f), color });
			rb.AddVertex({ float3(ni->pos.x - cosSize, ni->pos.z + sinSize, 0.0f), color });
			rb.AddVertex({ float3(ni->pos.x - cosSize, ni->pos.z + sinSize, 0.0f), color });
			rb.AddVertex({ float3(ni->pos.x + sinSize, ni->pos.z + cosSize, 0.0f), color });
		}
		++ni;
	}

	shader.Enable();
	rb.DrawArrays(GL_LINES);
	shader.Disable();
}



bool CMiniMap::RenderCachedTexture(bool useNormalizedCoors)
{
	//ZoneScoped;
	if (!renderToTexture)
		return false;

	glPushAttrib(GL_COLOR_BUFFER_BIT);
	glBindTexture(GL_TEXTURE_2D, minimapTex);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);

	if (useNormalizedCoors) {
		glPushMatrix();

		if (globalRendering->dualScreenMode) {
			LoadDualViewport();
		} else {
			glTranslatef(curPos.x * globalRendering->pixelX, curPos.y * globalRendering->pixelY, 0.0f);
			glScalef(curDim.x * globalRendering->pixelX, curDim.y * globalRendering->pixelY, 1.0f);
		}
	}

	auto& rb = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_2DT>();
	rb.AssertSubmission();

	rb.AddQuadTriangles(
		{ 0.0f, 0.0f, 0.0, 0.0 },
		{ 1.0f, 0.0f, 1.0, 0.0 },
		{ 1.0f, 1.0f, 1.0, 1.0 },
		{ 0.0f, 1.0f, 0.0, 1.0 }
	);

	auto& sh = rb.GetShader();
	sh.Enable();
	sh.SetUniform("ucolor", 1.0f, 1.0f, 1.0f, 1.0f);
	rb.DrawElements(GL_TRIANGLES);
	sh.Disable();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	DrawCameraFrustumAndMouseSelection();

	if (useNormalizedCoors) {
		if (globalRendering->dualScreenMode)
			globalRendering->LoadViewport();

		glPopMatrix();
	}

	glDisable(GL_TEXTURE_2D);
	glPopAttrib();
	return true;
}

void CMiniMap::DrawBackground() const
{
	//ZoneScoped;
	auto& rb = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_2DT>();
	rb.AssertSubmission();

	if (flipped) {
		rb.AddQuadTriangles(
			{ 1.0f, 1.0f, 0.0f, 1.0f }, // tl
			{ 0.0f, 1.0f, 1.0f, 1.0f }, // tr
			{ 0.0f, 0.0f, 1.0f, 0.0f }, // br
			{ 1.0f, 0.0f, 0.0f, 0.0f }  // bl
		);
	} else {
		rb.AddQuadTriangles(
			{ 0.0f, 0.0f, 0.0f, 1.0f }, // tl
			{ 1.0f, 0.0f, 1.0f, 1.0f }, // tr
			{ 1.0f, 1.0f, 1.0f, 0.0f }, // br
			{ 0.0f, 1.0f, 0.0f, 0.0f }  // bl
		);
	}

	//glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadMatrixf(projMats[0]);

	// draw the map
	glDisable(GL_BLEND);

	readMap->BindMiniMapTextures();
	bgShader->Enable();
	bgShader->SetUniform("infotexMul", static_cast<float>(infoTextureHandler->IsEnabled()));
	rb.DrawElements(GL_TRIANGLES);
	bgShader->Disable();

	glEnable(GL_BLEND);

	//glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

void CMiniMap::DrawUnitIcons() const
{
	ZoneScopedN("MiniMap::DrawUnitIcons");
#if USE_CLIP_PLANES
	for (int i = 0; i < 4; ++i)
		glDisable(GL_CLIP_PLANE0 + i);
#endif
	glEnable(GL_SCISSOR_TEST);
	glScissor(curPos.x, curPos.y, curDim.x, curDim.y);

	// switch to top-down map/world coords (z is twisted with y compared to the real map/world coords)
	glPushMatrix();
	glTranslatef(0.0f, +1.0f, 0.0f);
	glScalef(+1.0f / (mapDims.mapx * SQUARE_SIZE), -1.0f / (mapDims.mapy * SQUARE_SIZE), 1.0f);

	unitDrawer->DrawUnitMiniMapIcons();

	glDisable(GL_TEXTURE_2D); //maybe later stages need it

	glPopMatrix();

	glDisable(GL_SCISSOR_TEST);

#if USE_CLIP_PLANES
	for (int i = 0; i < 4; ++i)
		glEnable(GL_CLIP_PLANE0 + i);
#endif
}


void CMiniMap::DrawUnitRanges() const
{
	//ZoneScoped;
	// draw unit ranges
	const auto& selUnits = selectedUnitsHandler.selectedUnits;
	auto& rb = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_C>();
	rb.AssertSubmission();

	for (const int unitID: selUnits) {
		const CUnit* unit = unitHandler.GetUnit(unitID);

		// LOS Ranges
		if (unit->radarRadius && !unit->beingBuilt && unit->activated) {
			DrawCircle(rb, unit->pos, cmdColors.rangeRadar, static_cast<float>(unit->radarRadius));
		}
		if (unit->sonarRadius && !unit->beingBuilt && unit->activated) {
			DrawCircle(rb, unit->pos, cmdColors.rangeSonar, static_cast<float>(unit->sonarRadius));
		}
		if (unit->jammerRadius && !unit->beingBuilt && unit->activated) {
			DrawCircle(rb, unit->pos, cmdColors.rangeJammer, static_cast<float>(unit->jammerRadius));
		}

		// Interceptor Ranges
		for (const CWeapon* w: unit->weapons) {
			auto& wd = *w->weaponDef;
			if ((w->range > 300) && wd.interceptor) {
				SColor rangeColor = (w->numStockpiled || !wd.stockpile) ?
					cmdColors.rangeInterceptorOn : cmdColors.rangeInterceptorOff;

				DrawCircle(rb, unit->pos, rangeColor, wd.coverageRange);
			}
		}
	}

	auto& sh = rb.GetShader();
	sh.Enable();
	rb.DrawArrays(GL_LINES);
	sh.Disable();
}


void CMiniMap::DrawWorldStuff() const
{
	//ZoneScoped;
	glPushMatrix();

	if (flipped) {
		glTranslatef(+1.0f, 0.0f, 0.0f);
		glScalef(-1.0f / (mapDims.mapx * SQUARE_SIZE), +1.0f / (mapDims.mapy * SQUARE_SIZE), 1.0f);
	} else {
		glTranslatef(0.0f, +1.0f, 0.0f);
		glScalef(+1.0f / (mapDims.mapx * SQUARE_SIZE), -1.0f / (mapDims.mapy * SQUARE_SIZE), 1.0f);
	}

	glRotatef(-90.0f, +1.0f, 0.0f, 0.0f); // real 'world' coordinates
	glScalef(1.0f, 0.0f, 1.0f); // skip the y-coord (Lua's DrawScreen is perspective and so any z-coord in it influence the x&y, too)

	// draw the projectiles
	if (drawProjectiles) {
		projectileDrawer->DrawProjectilesMiniMap();
	}

	shadowHandler.DrawFrustumDebug();
	DebugVisibilityDrawer::DrawMinimap();

	{
		// draw the queued commands
		commandDrawer->DrawLuaQueuedUnitSetCommands();

		// NOTE: this needlessly adds to the CursorIcons list, but at least
		//       they are not drawn  (because the input receivers are drawn
		//       after the command queues)
		if ((drawCommands > 0) && guihandler->GetQueueKeystate()) {
			selectedUnitsHandler.DrawCommands();
		}
	}


	glLineWidth(2.5f);
	lineDrawer.DrawAll();
	glLineWidth(1.0f);

	// draw the selection shape, and some ranges
	if (drawCommands > 0)
		guihandler->DrawMapStuff(true);

	DrawUnitRanges();

	glPopMatrix();
}


void CMiniMap::SetClipPlanes(const bool lua) const
{
	//ZoneScoped;
	if (lua) {
		// prepare ClipPlanes for Lua's DrawInMinimap Modelview matrix

		// quote from glClipPlane spec:
		// "When glClipPlane is called, equation is transformed by the inverse of the modelview matrix and stored in the resulting eye coordinates.
		//  Subsequent changes to the modelview matrix have no effect on the stored plane-equation components."
		// -> we have to use the same modelview matrix when calling glClipPlane and later draw calls

		// set the modelview matrix to the same as used in Lua's DrawInMinimap
		glPushMatrix();
		glLoadIdentity();
		glScalef(1.0f / curDim.x, 1.0f / curDim.y, 1.0f);

		const double plane0[4] = { 0, -1, 0, double(curDim.y)};
		const double plane1[4] = { 0,  1, 0,                0};
		const double plane2[4] = {-1,  0, 0, double(curDim.x)};
		const double plane3[4] = { 1,  0, 0,                0};

		glClipPlane(GL_CLIP_PLANE0, plane0); // clip bottom
		glClipPlane(GL_CLIP_PLANE1, plane1); // clip top
		glClipPlane(GL_CLIP_PLANE2, plane2); // clip right
		glClipPlane(GL_CLIP_PLANE3, plane3); // clip left

		glPopMatrix();
	} else {
		// clip everything outside of the minimap box
		const double plane0[4] = { 0,-1, 0, 1};
		const double plane1[4] = { 0, 1, 0, 0};
		const double plane2[4] = {-1, 0, 0, 1};
		const double plane3[4] = { 1, 0, 0, 0};

		glClipPlane(GL_CLIP_PLANE0, plane0); // clip bottom
		glClipPlane(GL_CLIP_PLANE1, plane1); // clip top
		glClipPlane(GL_CLIP_PLANE2, plane2); // clip right
		glClipPlane(GL_CLIP_PLANE3, plane3); // clip left
	}
}



/******************************************************************************/

