#ifndef QTPFS_REGISTRY_H__
#define QTPFS_REGISTRY_H__

/**
 * Important! Group ownerships used on this registry:
 *
 * group<PathSearch, ProcessPath>()
 *  used to group path searches to be processed this frame
 *  those searches are removed afterwards; however if a request was a raw path and failed, then
 *  a new path gets scheduled for a regular path. By using groups, added new paths does not break
 *  the loop used to cleared out processed paths.
 */

#include "System/Ecs/EcsMain.h"
#include "System/Ecs/Utils/DoubleLinkedList.h"
#include "System/Ecs/Utils/SystemGlobalUtils.h"
#include "System/Ecs/Utils/SystemUtils.h"

namespace QTPFS {
extern entt::registry registry;
extern SystemGlobals::SystemGlobal systemGlobals;
extern SystemUtils::SystemUtils systemUtils;
extern ecs_dll::DoubleLinkList linkedListHelper;
} // namespace QTPFS

#endif
