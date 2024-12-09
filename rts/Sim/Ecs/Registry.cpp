/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Registry.h"

entt::registry Sim::registry;
SystemGlobals::SystemGlobal Sim::systemGlobals(Sim::registry);
SystemUtils::SystemUtils Sim::systemUtils;
Sim::SaveLoadUtils Sim::saveLoadUtils(Sim::registry, Sim::systemUtils, Sim::systemGlobals);
