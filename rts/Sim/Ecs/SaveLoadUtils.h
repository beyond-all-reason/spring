/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SAVE_LOAD_UTILS_H__
#define SAVE_LOAD_UTILS_H__

#include "System/Ecs/EcsMain.h"
#include "System/Ecs/Utils/SystemGlobalUtils.h"
#include "System/Ecs/Utils/SystemUtils.h"

#include <sstream>

// #include "System/creg/Serializer.h"

namespace Sim {

class SaveLoadUtils {
public:
	SaveLoadUtils(entt::registry& registryReference,
	    SystemUtils::SystemUtils& systemUtilsReference,
	    SystemGlobals::SystemGlobal& systemGlobalsReference)
	    : registry(registryReference)
	    , systemUtils(systemUtilsReference)
	    , systemGlobals(systemGlobalsReference)
	{
	}

	void LoadComponents(std::stringstream& iss);
	void SaveComponents(std::stringstream& oss);

private:
	entt::registry& registry;
	SystemUtils::SystemUtils& systemUtils;
	SystemGlobals::SystemGlobal& systemGlobals;
};

} // namespace Sim

#endif
