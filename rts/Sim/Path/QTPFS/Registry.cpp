#include "Registry.h"

entt::registry QTPFS::registry;
SystemGlobals::SystemGlobal QTPFS::systemGlobals(QTPFS::registry);
SystemUtils::SystemUtils QTPFS::systemUtils;
ecs_dll::DoubleLinkList QTPFS::linkedListHelper(QTPFS::registry);
