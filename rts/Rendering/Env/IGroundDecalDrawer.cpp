/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IGroundDecalDrawer.h"
#include "Rendering/Env/Decals/GroundDecalHandler.h"
#include "System/Config/ConfigHandler.h"
#include "System/Exceptions.h"
#include "System/SafeUtil.h"
#include "System/Log/ILog.h"
#include "Sim/Units/Unit.h"


CONFIG(int, GroundDecals).defaultValue(3).headlessValue(0).description("Controls whether ground decals underneath buildings and ground scars from explosions will be rendered. Values >1 define how long such decals will stay.");

static NullGroundDecalDrawer nullDecalDrawer;
IGroundDecalDrawer* IGroundDecalDrawer::singleton = &nullDecalDrawer;
int IGroundDecalDrawer::decalLevel = 0;


CR_BIND_INTERFACE(IGroundDecalDrawer)
CR_REG_METADATA(IGroundDecalDrawer, (
	CR_MEMBER(decals)
))

CR_BIND_DERIVED(NullGroundDecalDrawer, IGroundDecalDrawer, )
CR_REG_METADATA(NullGroundDecalDrawer,  )

static IGroundDecalDrawer* GetInstance()
{
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
	decalLevel = configHandler->GetInt("GroundDecals");

	FreeInstance();
	singleton = GetInstance();
}


void IGroundDecalDrawer::FreeInstance()
{
	if (singleton != &nullDecalDrawer)
		spring::SafeDelete(singleton);
}


void IGroundDecalDrawer::SetDrawDecals(bool v)
{
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

void NullGroundDecalDrawer::SetUnitLeaveTracks(CUnit* unit, bool leaveTracks)
{
	unit->leaveTracks = leaveTracks;
}
