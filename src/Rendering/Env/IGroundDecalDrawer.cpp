/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IGroundDecalDrawer.h"
#include "Rendering/Env/Decals/GroundDecalHandler.h"
#include "System/Config/ConfigHandler.h"
#include "System/Exceptions.h"
#include "System/SafeUtil.h"
#include "System/Log/ILog.h"
#include "Sim/Units/Unit.h"

#include "System/Misc/TracyDefs.h"


CONFIG(bool, GroundDecals).defaultValue(true).headlessValue(false).description("Controls whether ground decals underneath buildings, unit tracks & footprints as well as ground scars from explosions will be rendered.");

CR_BIND_INTERFACE(IGroundDecalDrawer)
CR_REG_METADATA(IGroundDecalDrawer, (
	CR_MEMBER(decals)
))

CR_BIND_DERIVED(NullGroundDecalDrawer, IGroundDecalDrawer, )
CR_REG_METADATA(NullGroundDecalDrawer,  )

void IGroundDecalDrawer::Init()
{
	RECOIL_DETAILED_TRACY_ZONE;
	SetDrawDecals(configHandler->GetBool("GroundDecals"));
}


void IGroundDecalDrawer::FreeInstance()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (singleton)
		spring::SafeDelete(singleton);
}

void IGroundDecalDrawer::SetDrawDecals(bool v)
{
	RECOIL_DETAILED_TRACY_ZONE;

	if (singleton && v == GetDrawDecals())
		return;

	FreeInstance();

	hasDecals = v;
	assert(!singleton);
	if (IGroundDecalDrawer::GetDrawDecals()) {
		singleton = new CGroundDecalHandler();
		LOG_L(L_INFO, "Loaded DecalsDrawer: %s", "standard");
	}
	else {
		singleton = new NullGroundDecalDrawer();
		LOG_L(L_INFO, "Loaded DecalsDrawer: %s", "null");
	}

	configHandler->Set("GroundDecals", hasDecals);
}

void NullGroundDecalDrawer::SetUnitLeaveTracks(CUnit* unit, bool leaveTracks)
{
	unit->leaveTracks = leaveTracks;
}
