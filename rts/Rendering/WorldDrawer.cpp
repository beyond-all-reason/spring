/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Rendering/GL/myGL.h"

#include "WorldDrawer.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Features/FeatureDefHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Rendering/Env/CubeMapHandler.h"
#include "Rendering/Env/GrassDrawer.h"
#include "Rendering/Env/IGroundDecalDrawer.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Env/SunLighting.h"
#include "Rendering/Env/WaterRendering.h"
#include "Rendering/Env/MapRendering.h"
#include "Rendering/Env/IWater.h"
#include "Rendering/CommandDrawer.h"
#include "Rendering/DebugColVolDrawer.h"
#include "Rendering/DebugVisibilityDrawer.h"
#include "Rendering/LineDrawer.h"
#include "Rendering/LuaObjectDrawer.h"
#include "Rendering/Features/FeatureDrawer.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Rendering/Units/UnitDrawer.h"
#include "Rendering/IPathDrawer.h"
#include "Rendering/DepthBufferCopy.h"
#include "Rendering/SmoothHeightMeshDrawer.h"
#include "Rendering/InMapDrawView.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Map/InfoTexture/IInfoTextureHandler.h"
#include "Rendering/Models/IModelParser.h"
#include "Rendering/Models/3DModelVAO.h"
#include "Rendering/Models/ModelsLock.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Textures/ColorMap.h"
#include "Rendering/Textures/3DOTextureHandler.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/ReadMap.h"
#include "Game/Camera.h"
#include "Game/SelectedUnitsHandler.h"
#include "Game/Game.h"
#include "Game/GlobalUnsynced.h"
#include "Game/LoadScreen.h"
#include "Game/UI/CommandColors.h"
#include "Game/UI/GuiHandler.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"
#include "System/TimeProfiler.h"
#include "System/SafeUtil.h"
#include "System/Log/ILog.h"
#include "System/Config/ConfigHandler.h"
#include "System/LoadLock.h"

CONFIG(bool, PreloadModels).defaultValue(true).description("The engine will preload all models");

void CWorldDrawer::InitPre() const
{
	LuaObjectDrawer::Init();

	CColorMap::InitStatic();

	// these need to be loaded before featureHandler is created
	// (maps with features have their models loaded at startup)
	S3DModelVAO::Init();
	modelLoader.Init();

	loadscreen->SetLoadMessage("Creating Unit Textures");
	textureHandler3DO.Init();
	textureHandlerS3O.Init();

	loadscreen->SetLoadMessage("Creating Sky");

	ISky::SetSky();
	sunLighting->Init();

	CFeatureDrawer::InitStatic();
}

void CWorldDrawer::InitPost() const
{
	char buf[512] = {0};

	CModelsLock::SetThreadSafety(true);
	const bool preloadMode = configHandler->GetBool("PreloadModels");
	{
		loadscreen->SetLoadMessage("Loading Models");

		if (preloadMode) {
			for (const auto& def : unitDefHandler->GetUnitDefsVec()) {
				def.PreloadModel();
			}

			for (const auto& def : featureDefHandler->GetFeatureDefsVec()) {
				def.PreloadModel();
			}

			for (const auto& def : weaponDefHandler->GetWeaponDefsVec()) {
				def.PreloadModel();
			}
		}
	}
	auto lock = CLoadLock::GetUniqueLock();
	{
		loadscreen->SetLoadMessage("Creating ShadowHandler");
		shadowHandler.Init();
	}
	{
		// SMFGroundDrawer accesses InfoTextureHandler, create it first
		loadscreen->SetLoadMessage("Creating InfoTextureHandler");
		IInfoTextureHandler::Create();
	}
	try {
		loadscreen->SetLoadMessage("Creating GroundDrawer");
		readMap->InitGroundDrawer();
	} catch (const content_error& e) {
		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf), "[WorldDrawer::%s] caught exception \"%s\"", __func__, e.what());
	}

	{
		loadscreen->SetLoadMessage("Creating GrassDrawer");
		grassDrawer = new CGrassDrawer();
	}
	{
		inMapDrawerView = new CInMapDrawView();
		pathDrawer = IPathDrawer::GetInstance();
	}
	{
		DepthBufferCopy::Init();
	}
	{
		IGroundDecalDrawer::Init();
	}
	{
		loadscreen->SetLoadMessage("Creating ProjectileDrawer & UnitDrawer");

		CProjectileDrawer::InitStatic();
		CUnitDrawer::InitStatic();
		// see ::InitPre
		// CFeatureDrawer::InitStatic();
	}

	// rethrow to force exit
	if (buf[0] != 0)
		throw content_error(buf);

	{
		loadscreen->SetLoadMessage("Creating Water");
		IWater::SetWater(-1);
	}
	{
		ISky::GetSky()->SetupFog();
	}
	lock = {}; //unlock
	{
		loadscreen->SetLoadMessage("Finalizing Models");
		modelLoader.DrainPreloadFutures(0);
		auto& mv = S3DModelVAO::GetInstance();
		if (preloadMode) {
			{
				auto lock = CLoadLock::GetUniqueLock();
				mv.UploadVBOs();
			}
			mv.SetSafeToDeleteVectors();
			modelLoader.LogErrors();
			CModelsLock::SetThreadSafety(false); //all models are already preloaded
		}
	}
}


void CWorldDrawer::Kill()
{
	spring::SafeDelete(infoTextureHandler);

	IWater::KillWater();
	ISky::KillSky();
	spring::SafeDelete(grassDrawer);
	spring::SafeDelete(pathDrawer);
	shadowHandler.Kill();
	spring::SafeDelete(inMapDrawerView);

	CFeatureDrawer::KillStatic(gu->globalReload);
	CUnitDrawer::KillStatic(gu->globalReload); // depends on unitHandler, cubeMapHandler
	CProjectileDrawer::KillStatic(gu->globalReload);

	S3DModelVAO::Kill();
	modelLoader.Kill();

	textureHandler3DO.Kill();
	textureHandlerS3O.Kill();

	readMap->KillGroundDrawer();
	IGroundDecalDrawer::FreeInstance();
	DepthBufferCopy::Kill();
	LuaObjectDrawer::Kill();
	SmoothHeightMeshDrawer::FreeInstance();

	numUpdates = 0;
}




void CWorldDrawer::Update(bool newSimFrame)
{
	SCOPED_TIMER("Update::WorldDrawer");

	LuaObjectDrawer::Update(numUpdates == 0);
	readMap->UpdateDraw(numUpdates == 0);

	if (globalRendering->drawGround) {
		ZoneScopedN("GroundDrawer::Update");
		(readMap->GetGroundDrawer())->Update();
	}
	// XXX: done in CGame, needs to get updated even when !doDrawWorld
	// (it updates unitdrawpos which is used for maximized minimap too)
	// unitDrawer->Update();
	// lineDrawer.UpdateLineStipple();
	CUnitDrawer::UpdateStatic();
	CFeatureDrawer::UpdateStatic();
	projectileDrawer->UpdateDrawFlags();

	if (newSimFrame) {
		projectileDrawer->UpdateTextures();

		{
			SCOPED_TIMER("Update::WorldDrawer::{Sky,Water}");

			ISky::GetSky()->Update();
			IWater::GetWater()->Update();
		}

		// once every simframe is frequent enough here
		// NB: errors will not be logged until frame 0
		modelLoader.LogErrors();
	}

	numUpdates += 1;
}



void CWorldDrawer::GenerateIBLTextures() const
{

	if (shadowHandler.ShadowsLoaded()) {
		SCOPED_TIMER("Draw::World::CreateShadows");
		SCOPED_GL_DEBUGGROUP("Draw::World::CreateShadows");

		game->SetDrawMode(CGame::gameShadowDraw);
		shadowHandler.CreateShadows();
		game->SetDrawMode(CGame::gameNormalDraw);
	}

	{
		SCOPED_TIMER("Draw::World::UpdateReflTex");
		SCOPED_GL_DEBUGGROUP("Draw::World::UpdateReflTex");
		cubeMapHandler.UpdateReflectionTexture();
	}

	SCOPED_GL_DEBUGGROUP("Draw::World::UpdateMisc");
	bool sunDirUpd = ISky::GetSky()->GetLight()->Update();
	bool sunLightUpd = sunLighting->IsUpdated();
	bool skyUpd = ISky::GetSky()->IsUpdated();
	bool waterUpd = waterRendering->IsUpdated();

	if (sunDirUpd) {
		SCOPED_TIMER("Draw::World::UpdateSpecTex");
		cubeMapHandler.UpdateSpecularTexture();
	}
	if (sunDirUpd || skyUpd) {
		SCOPED_TIMER("Draw::World::UpdateSkyTex");
		ISky::GetSky()->UpdateSkyTexture();
	}
	if (sunDirUpd || sunLightUpd || waterUpd) {
		SCOPED_TIMER("Draw::World::UpdateShadingTex");
		readMap->UpdateShadingTexture();
	}
}

void CWorldDrawer::ResetMVPMatrices() const
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, 1, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}



void CWorldDrawer::Draw() const
{
	SCOPED_TIMER("Draw::World");
	SCOPED_GL_DEBUGGROUP("Draw::World");

	const auto& sky = ISky::GetSky();
	glClearColor(sky->fogColor.x, sky->fogColor.y, sky->fogColor.z, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	camera->Update();

	DrawOpaqueObjects();
	DrawAlphaObjects();
	{
		SCOPED_TIMER("Draw::World::DrawWorld");
		SCOPED_GL_DEBUGGROUP("Draw::World::DrawWorld");
		eventHandler.DrawWorld();
	}


	DrawMiscObjects();
	DrawBelowWaterOverlay();

	glDisable(GL_FOG);
}


void CWorldDrawer::DrawOpaqueObjects() const
{
	CBaseGroundDrawer* gd = readMap->GetGroundDrawer();

	if (globalRendering->drawGround) {
		{
			SCOPED_TIMER("Draw::World::Terrain");
			SCOPED_GL_DEBUGGROUP("Draw::World::Terrain");
			gd->Draw(DrawPass::Normal);
			depthBufferCopy->MakeDepthBufferCopy();
		}
		{
			eventHandler.DrawPreDecals();
			SCOPED_TIMER("Draw::World::Decals");
			SCOPED_GL_DEBUGGROUP("Draw::World::Decals");
			groundDecals->Draw();
			projectileDrawer->DrawGroundFlashes();
		}
		{
			SCOPED_TIMER("Draw::World::Foliage");
			SCOPED_GL_DEBUGGROUP("Draw::World::Foliage");
			grassDrawer->Draw();
		}
		smoothHeightMeshDrawer->Draw(1.0f);
	}

	// not an opaque rendering, but makes sense to run after the terrain was rendered
	{
		const auto& sky = ISky::GetSky();
		sky->Draw();
	}

	selectedUnitsHandler.Draw();
	eventHandler.DrawWorldPreUnit();

	{
		SCOPED_TIMER("Draw::World::Models::Opaque");
		SCOPED_GL_DEBUGGROUP("Draw::World::Models::Opaque");
		unitDrawer->Draw(false);
		featureDrawer->Draw(false);
	}
	{
		SCOPED_TIMER("Draw::World::Models::Projectiles");
		SCOPED_GL_DEBUGGROUP("Draw::World::Models::Projectiles");
		projectileDrawer->DrawOpaque(false);
	}
	{
		SCOPED_TIMER("Draw::OpaqueObjects::Debug");
		SCOPED_GL_DEBUGGROUP("Draw::OpaqueObjects::Debug");
		DebugColVolDrawer::Draw();
		DebugVisibilityDrawer::DrawWorld();
		pathDrawer->DrawAll();
	}
}

void CWorldDrawer::DrawAlphaObjects() const
{
	// transparent objects
	glEnable(GL_BLEND);
	glDepthFunc(GL_LEQUAL);

	static const double belowPlaneEq[4] = {0.0f, -1.0f, 0.0f, 0.0f};
	static const double abovePlaneEq[4] = {0.0f,  1.0f, 0.0f, 0.0f};

	const bool hasWaterRendering = globalRendering->drawWater && readMap->HasVisibleWater();

	{
		SCOPED_TIMER("Draw::World::Models::Alpha");
		SCOPED_GL_DEBUGGROUP("Draw::World::Models::Alpha");
		// clip in model-space
		if (hasWaterRendering) {
			glPushMatrix();
			glLoadIdentity();
			glClipPlane(GL_CLIP_PLANE3, belowPlaneEq);
			glPopMatrix();
			glEnable(GL_CLIP_PLANE3);
		}

		// draw alpha-objects below water surface (farthest)
		unitDrawer->DrawAlphaPass(false);
		featureDrawer->DrawAlphaPass(false);
	}
	{
		SCOPED_TIMER("Draw::World::Particles");
		SCOPED_GL_DEBUGGROUP("Draw::World::Particles");
		projectileDrawer->DrawAlpha(!hasWaterRendering, true, false, false);

		if (hasWaterRendering)
			glDisable(GL_CLIP_PLANE3);
	}

	if (!hasWaterRendering)
		return;

	// draw water (in-between)
	{
		SCOPED_TIMER("Draw::World::Water");
		SCOPED_GL_DEBUGGROUP("Draw::World::Water");

		const auto& water = IWater::GetWater();
		{
			ZoneScopedN("Draw::World::Water::UpdateWater");
			water->UpdateWater(game);
		}
		water->Draw();
		eventHandler.DrawWaterPost();
	}

	{
		SCOPED_TIMER("Draw::World::Models::Alpha");
		SCOPED_GL_DEBUGGROUP("Draw::World::Alpha");
		glPushMatrix();
		glLoadIdentity();
		glClipPlane(GL_CLIP_PLANE3, abovePlaneEq);
		glPopMatrix();
		glEnable(GL_CLIP_PLANE3);

		// draw alpha-objects above water surface (closest)
		unitDrawer->DrawAlphaPass(false);
		featureDrawer->DrawAlphaPass(false);
	}
	{
		SCOPED_TIMER("Draw::World::Particles");
		SCOPED_GL_DEBUGGROUP("Draw::World::Particles");
		projectileDrawer->DrawAlpha(true, false, false, false);

		glDisable(GL_CLIP_PLANE3);
	}
}

void CWorldDrawer::DrawMiscObjects() const
{

	{
		// note: duplicated in CMiniMap::DrawWorldStuff()
		commandDrawer->DrawLuaQueuedUnitSetCommands();

		if (cmdColors.AlwaysDrawQueue() || guihandler->GetQueueKeystate()) {
			selectedUnitsHandler.DrawCommands();
		}
	}

	// either draw from here, or make {Dyn,Bump}Water use blending
	// pro: icons are drawn only once per frame, not every pass
	// con: looks somewhat worse for underwater / obscured icons
	if (!CUnitDrawer::UseScreenIcons())
		unitDrawer->DrawUnitIcons();

	lineDrawer.DrawAll();
	cursorIcons.Draw();

	mouse->DrawSelectionBox();
	guihandler->DrawMapStuff(false);

	if (globalRendering->drawMapMarks && !game->hideInterface) {
		inMapDrawerView->Draw();
	}
}



void CWorldDrawer::DrawBelowWaterOverlay() const
{

	if (!globalRendering->drawWater)
		return;
	if (mapRendering->voidWater)
		return;
	if (camera->GetPos().y >= 0.0f)
		return;

	{
		glEnableClientState(GL_VERTEX_ARRAY);

		const float3& cpos = camera->GetPos();
		const float vr = camera->GetFarPlaneDist() * 0.5f;

		glDepthMask(GL_FALSE);
		glDisable(GL_TEXTURE_2D);
		glColor4f(0.0f, 0.5f, 0.3f, 0.50f);

		{
			const float3 verts[] = {
				float3(cpos.x - vr, 0.0f, cpos.z - vr),
				float3(cpos.x - vr, 0.0f, cpos.z + vr),
				float3(cpos.x + vr, 0.0f, cpos.z + vr),
				float3(cpos.x + vr, 0.0f, cpos.z - vr)
			};

			glVertexPointer(3, GL_FLOAT, 0, verts);
			glDrawArrays(GL_QUADS, 0, 4);
		}

		{
			const float3 verts[] = {
				float3(cpos.x - vr, 0.0f, cpos.z - vr),
				float3(cpos.x - vr,  -vr, cpos.z - vr),
				float3(cpos.x - vr, 0.0f, cpos.z + vr),
				float3(cpos.x - vr,  -vr, cpos.z + vr),
				float3(cpos.x + vr, 0.0f, cpos.z + vr),
				float3(cpos.x + vr,  -vr, cpos.z + vr),
				float3(cpos.x + vr, 0.0f, cpos.z - vr),
				float3(cpos.x + vr,  -vr, cpos.z - vr),
				float3(cpos.x - vr, 0.0f, cpos.z - vr),
				float3(cpos.x - vr,  -vr, cpos.z - vr),
			};

			glVertexPointer(3, GL_FLOAT, 0, verts);
			glDrawArrays(GL_QUAD_STRIP, 0, 10);
		}

		glDepthMask(GL_TRUE);
		glDisableClientState(GL_VERTEX_ARRAY);
	}

	{
		// draw water-coloration quad in raw screenspace
		ResetMVPMatrices();

		glEnableClientState(GL_VERTEX_ARRAY);
		glDisable(GL_TEXTURE_2D);
		glColor4f(0.0f, 0.2f, 0.8f, 0.333f);

		const float3 verts[] = {
			float3(0.0f, 0.0f, -1.0f),
			float3(1.0f, 0.0f, -1.0f),
			float3(1.0f, 1.0f, -1.0f),
			float3(0.0f, 1.0f, -1.0f),
		};

		glVertexPointer(3, GL_FLOAT, 0, verts);
		glDrawArrays(GL_QUADS, 0, 4);
		glDisableClientState(GL_VERTEX_ARRAY);
	}
}
