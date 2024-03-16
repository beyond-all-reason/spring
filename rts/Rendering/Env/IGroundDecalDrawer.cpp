/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IGroundDecalDrawer.h"
#include "Rendering/Env/Decals/GroundDecalHandler.h"
#include "System/Config/ConfigHandler.h"
#include "System/Exceptions.h"
#include "System/SafeUtil.h"
#include "System/Log/ILog.h"

#include <tracy/Tracy.hpp>


CONFIG(int, GroundDecals).defaultValue(3).headlessValue(0).description("Controls whether ground decals underneath buildings and ground scars from explosions will be rendered. Values >1 define how long such decals will stay.");

static NullGroundDecalDrawer nullDecalDrawer;
IGroundDecalDrawer* IGroundDecalDrawer::singleton = &nullDecalDrawer;
int IGroundDecalDrawer::decalLevel = 0;


static IGroundDecalDrawer* GetInstance()
{
	//ZoneScoped;
	IGroundDecalDrawer* instance = &nullDecalDrawer;
	if (!IGroundDecalDrawer::GetDrawDecals()) {
		LOG_L(L_INFO, "Loaded DecalsDrawer: %s", "null");
		return instance;
	}

	if (instance == &nullDecalDrawer) {
		instance = new CGroundDecalHandler();
		LOG_L(L_INFO, "Loaded DecalsDrawer: %s", "standard");
	}

	return instance;
}


void IGroundDecalDrawer::Init()
{
	//ZoneScoped;
	decalLevel = configHandler->GetInt("GroundDecals");

	FreeInstance();
	singleton = GetInstance();
}


void IGroundDecalDrawer::FreeInstance()
{
	//ZoneScoped;
	if (singleton != &nullDecalDrawer)
		spring::SafeDelete(singleton);
}


void IGroundDecalDrawer::SetDrawDecals(bool v)
{
	//ZoneScoped;
	if (v) {
		decalLevel =  std::abs(decalLevel);
	} else {
		decalLevel = -std::abs(decalLevel);
	}

	if (groundDecals == &nullDecalDrawer) {
		groundDecals = GetInstance();
	}

	groundDecals->OnDecalLevelChanged();
}