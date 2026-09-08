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
    enum class entity : std::uint32_t {};
}

// Create a special entity type which allows for all paths to be used from Lua.
// Lua is using floats, which allows for 23 bits mantissa. So we can shave excess bits off of the entity version number.
template<>
struct entt::internal::entt_traits<QTPFS::entity> {
    using value_type = QTPFS::entity;
    using entity_type = std::uint32_t;
    using version_type = std::uint8_t;

    static constexpr entity_type entity_mask = 0xFFFFF;
    static constexpr entity_type version_mask = 0x7;
    static constexpr std::size_t entity_shift = 20u;
};

namespace QTPFS {
    extern entt::basic_registry<QTPFS::entity> registry;
    extern SystemGlobals::SystemGlobal<decltype(registry)> systemGlobals;
    extern SystemUtils::SystemUtils systemUtils;
    extern ecs_dll::DoubleLinkList<decltype(registry)> linkedListHelper;
}

#endif