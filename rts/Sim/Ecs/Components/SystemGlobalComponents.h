#ifndef SYSTEM_GLOBAL_COMPONENTS_H__
#define SYSTEM_GLOBAL_COMPONENTS_H__

#include "Sim/Ecs/EcsMain.h"

#include "System/float3.h"

namespace SystemGlobals {

    // entt::component_traits<EnvResourceComponent> systemComponentTraits;
    // systemComponentTraits.page_size = 1;

struct EnvResourceComponent {
    static constexpr std::size_t page_size = 1;

	float curTidalStrength = 0.0f;
	float curWindStrength = 0.0f;
	float newWindStrength = 0.0f;

	float minWindStrength = 0.0f;
	float maxWindStrength = 0.0f;

	float3 curWindDir;
	float3 curWindVec; // curWindDir * curWindStrength
	float3 newWindVec;
	float3 oldWindVec;

	int windDirTimer = 0;
};

}

#endif