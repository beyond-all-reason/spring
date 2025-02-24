/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IWater.h"
#include "ISky.h"
#include "BasicWater.h"
#include "AdvWater.h"
#include "BumpWater.h"
#include "DynWater.h"
#include "RefractWater.h"
#include "Game/Game.h"
#include "Game/GameHelper.h"
#include "Map/ReadMap.h"
#include "Map/BaseGroundDrawer.h"
#include "Rendering/Features/FeatureDrawer.h"
#include "Rendering/Units/UnitDrawer.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Sim/Projectiles/ExplosionListener.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"
#include "System/SafeUtil.h"
#include "System/Log/ILog.h"

#include "System/Misc/TracyDefs.h"

CONFIG(int, Water)
.defaultValue(IWater::WATER_RENDERER_REFLECTIVE)
.safemodeValue(IWater::WATER_RENDERER_BASIC)
.headlessValue(0)
.minimumValue(0)
.maximumValue(IWater::NUM_WATER_RENDERERS - 1)
.description("Defines the type of water rendering. Can be set in game. Options are: 0 = Basic water, 1 = Reflective water, 2 = Reflective and Refractive water, 3 = Dynamic water, 4 = Bumpmapped water");

IWater::IWater()
	: drawReflection(false)
	, drawRefraction(false)
	, wireFrameMode(false)
{
	CExplosionCreator::AddExplosionListener(this);
}

void IWater::ExplosionOccurred(const CExplosionParams& event) {
	RECOIL_DETAILED_TRACY_ZONE;
	AddExplosion(event.pos, event.damages.GetDefault(), event.craterAreaOfEffect);
}

void IWater::SetModelClippingPlane(const double* planeEq) {
	RECOIL_DETAILED_TRACY_ZONE;
	glPushMatrix();
	glLoadIdentity();
	glClipPlane(GL_CLIP_PLANE2, planeEq);
	glPopMatrix();
}

void IWater::SetWater(int rendererMode)
{
	RECOIL_DETAILED_TRACY_ZONE;
	static std::array<bool, NUM_WATER_RENDERERS> allowedModes = {
		true,
		GLAD_GL_ARB_fragment_program && ProgramStringIsNative(GL_FRAGMENT_PROGRAM_ARB, "ARB/water.fp"),
		GLAD_GL_ARB_fragment_program && ProgramStringIsNative(GL_FRAGMENT_PROGRAM_ARB, "ARB/waterDyn.fp"),
		GLAD_GL_ARB_fragment_program && GLAD_GL_ARB_texture_rectangle,
		GLAD_GL_ARB_shading_language_100 && GLAD_GL_ARB_fragment_shader && GLAD_GL_ARB_vertex_shader,
	};

	WATER_RENDERER selectedRendererID;
	if (rendererMode < 0) {
		if (water == nullptr) {
			// just select
			selectedRendererID = static_cast<WATER_RENDERER>(configHandler->GetInt("Water"));
			if (!allowedModes[selectedRendererID])
				selectedRendererID = WATER_RENDERER_BASIC;
		}
		else {
			// cycle
			for (int i = NUM_WATER_RENDERERS - 1; i >= 0; --i) {
				selectedRendererID = static_cast<WATER_RENDERER>((static_cast<int>(water->GetID()) + 1 + i) % NUM_WATER_RENDERERS);
				if (allowedModes[selectedRendererID])
					break;
			}
		}
	}
	else {
		// select specific one
		for (int i = 0; i < NUM_WATER_RENDERERS; ++i) {
			selectedRendererID = static_cast<WATER_RENDERER>((rendererMode + i) % NUM_WATER_RENDERERS);
			if (allowedModes[selectedRendererID])
				break;
		}
	}

	if (water && water->GetID() == selectedRendererID)
		return;

	water = nullptr;
	try {
		switch (selectedRendererID)
		{
		case WATER_RENDERER_BASIC:
			water = std::make_unique<CBasicWater>();
			break;
		case WATER_RENDERER_REFLECTIVE:
			water = std::make_unique<CAdvWater>();
			break;
		case WATER_RENDERER_DYNAMIC:
			water = std::make_unique<CDynWater>();
			break;
		case WATER_RENDERER_REFL_REFR:
			water = std::make_unique<CRefractWater>();
			break;
		case WATER_RENDERER_BUMPMAPPED:
			water = std::make_unique<CBumpWater>();
			break;
		default:
			assert(false);
			break;
		}
		if (water)
			water->InitResources();
	} catch (const content_error& ex) {		
		LOG_L(L_ERROR, "Loading \"%s\" water failed, error: %s", IWater::GetWaterName(selectedRendererID), ex.what());
		if (water)
			water->FreeResources(); //destructor is not called for an object throwing exception in a constructor
		water = nullptr;
	}

	// set it here as user preference.
	if (water)
		configHandler->Set("Water", static_cast<int>(water->GetID()));

	if (water == nullptr)
		water = std::make_unique<CBasicWater>();
}


void IWater::DrawReflections(const double* clipPlaneEqs, bool drawGround, bool drawSky) {
	RECOIL_DETAILED_TRACY_ZONE;
	game->SetDrawMode(CGame::gameReflectionDraw);

	{
		drawReflection = true;

		SCOPED_TIMER("Draw::Water::DrawReflections");
		SCOPED_GL_DEBUGGROUP("Draw::Water::DrawReflections");

		// opaque; do not clip skydome (is drawn in camera space)
		if (drawSky) {
			ISky::GetSky()->Draw();
		}

		glEnable(GL_CLIP_PLANE2);
		glClipPlane(GL_CLIP_PLANE2, &clipPlaneEqs[0]);

		if (drawGround)
			readMap->GetGroundDrawer()->Draw(DrawPass::WaterReflection);


		// rest needs the plane in model-space; V is combined with P
		SetModelClippingPlane(&clipPlaneEqs[4]);
		unitDrawer->Draw(true);
		featureDrawer->Draw(true);
		projectileDrawer->DrawOpaque(true);

		// transparent
		unitDrawer->DrawAlphaPass(true);
		featureDrawer->DrawAlphaPass(true);
		projectileDrawer->DrawAlpha(true, false, true, false);
		// sun-disc does not blend well with water

		eventHandler.DrawWorldReflection();
		glDisable(GL_CLIP_PLANE2);

		drawReflection = false;
	}

	game->SetDrawMode(CGame::gameNormalDraw);
}

void IWater::DrawRefractions(const double* clipPlaneEqs, bool drawGround, bool drawSky) {
	RECOIL_DETAILED_TRACY_ZONE;
	game->SetDrawMode(CGame::gameRefractionDraw);

	{
		drawRefraction = true;

		SCOPED_TIMER("Draw::Water::DrawRefractions");
		SCOPED_GL_DEBUGGROUP("Draw::Water::DrawRefractions");

		glEnable(GL_CLIP_PLANE2);
		glClipPlane(GL_CLIP_PLANE2, &clipPlaneEqs[0]);

		// opaque
		if (drawSky) {
			ISky::GetSky()->Draw();
		}
		if (drawGround) {
			readMap->GetGroundDrawer()->Draw(DrawPass::WaterRefraction);
		}


		SetModelClippingPlane(&clipPlaneEqs[4]);
		unitDrawer->Draw(false, true);
		featureDrawer->Draw(false, true);
		projectileDrawer->DrawOpaque(false, true);

		// transparent
		unitDrawer->DrawAlphaPass(false, true);
		featureDrawer->DrawAlphaPass(false, true);
		projectileDrawer->DrawAlpha(false, true, false, true);

		eventHandler.DrawWorldRefraction();
		glDisable(GL_CLIP_PLANE2);

		drawRefraction = false;
	}

	game->SetDrawMode(CGame::gameNormalDraw);
}

