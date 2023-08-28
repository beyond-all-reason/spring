#ifndef QTPFS_REGISTRY_H__
#define QTPFS_REGISTRY_H__

#include "System/Ecs/EcsMain.h"
#include "System/Ecs/Utils/SystemGlobalUtils.h"
#include "System/Ecs/Utils/SystemUtils.h"

namespace Sim {
    extern entt::registry registry;
    extern SystemGlobals::SystemGlobal systemGlobals;
    extern SystemUtils::SystemUtils systemUtils;
}

#endif