#include "ModelDrawer.h"

#include "Map/Ground.h"
#include "Rendering/GL/LightHandler.h"
#include "System/Config/ConfigHandler.h"
#include "Rendering/Env/CubeMapHandler.h"
#include "Rendering/LuaObjectDrawer.h"

void CModelDrawerConcept::InitStatic()
{
	advShading = configHandler->GetBool("AdvUnitShading") && cubeMapHandler.Init();
	wireFrameMode = false;

	lightHandler.Init(2U, configHandler->GetInt("MaxDynamicModelLights"));

	deferredAllowed = configHandler->GetBool("AllowDeferredModelRendering");

	// shared with FeatureDrawer!
	geomBuffer = LuaObjectDrawer::GetGeometryBuffer();
	deferredAllowed &= geomBuffer->Valid();
}

void CModelDrawerConcept::KillStatic(bool reload)
{
	cubeMapHandler.Free();
	geomBuffer = nullptr;
}