#include "Registry.h"

entt::registry Sim::registry;
SystemGlobals::SystemGlobal Sim::systemGlobals(Sim::registry);
SystemUtils::SystemUtils Sim::systemUtils;
