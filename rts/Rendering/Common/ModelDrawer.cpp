#include "ModelDrawer.h"

#include "Map/Ground.h"
#include "Rendering/GL/LightHandler.h"
#include "System/Config/ConfigHandler.h"
#include "Rendering/Env/CubeMapHandler.h"
#include "Rendering/LuaObjectDrawer.h"

#include "System/Misc/TracyDefs.h"

void CModelDrawerConcept::InitStatic()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (initialized)
		return;

	cubeMapHandler.Init();
	wireFrameMode = false;

	lightHandler.Init(2U, configHandler->GetInt("MaxDynamicModelLights"));

	deferredAllowed = configHandler->GetBool("AllowDeferredModelRendering");

	// shared with FeatureDrawer!
	geomBuffer = LuaObjectDrawer::GetGeometryBuffer();
	deferredAllowed &= geomBuffer->Valid();

	IModelDrawerState::InitInstance<CModelDrawerStateGLSL>(MODEL_DRAWER_GLSL);
	IModelDrawerState::InitInstance<CModelDrawerStateGL4 >(MODEL_DRAWER_GL4 );

	initialized = true;
}

void CModelDrawerConcept::KillStatic(bool reload)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!initialized)
		return;

	cubeMapHandler.Free();
	geomBuffer = nullptr;

	for (int t = ModelDrawerTypes::MODEL_DRAWER_GLSL; t < ModelDrawerTypes::MODEL_DRAWER_CNT; ++t) {
		IModelDrawerState::KillInstance(t);
	}

	initialized = false;
}