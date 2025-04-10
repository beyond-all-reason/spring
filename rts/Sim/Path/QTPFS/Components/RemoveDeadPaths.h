/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_SYSTEMS_REMOVE_DEAD_PATHS_H__
#define QTPFS_SYSTEMS_REMOVE_DEAD_PATHS_H__

#include "System/Ecs/Components/BaseComponents.h"

#include <cstddef>

namespace QTPFS {

ALIAS_COMPONENT(PathDelayedDelete, int);

struct RemoveDeadPathsComponent {
	static constexpr std::size_t page_size = 1;

	int refreshRate = 15;
	int refreshOffset = 1;
};

} // namespace QTPFS

#endif
