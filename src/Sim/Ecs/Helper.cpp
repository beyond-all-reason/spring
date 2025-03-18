/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// #undef NDEBUG

#include "Helper.h"
#include "Registry.h"
#include "SaveLoadUtils.h"

void Sim::ClearRegistry() {
    Sim::registry.clear();
}

void Sim::LoadComponents(std::stringstream &iss) {
    saveLoadUtils.LoadComponents(iss);
}

void Sim::SaveComponents(std::stringstream &oss) {
    saveLoadUtils.SaveComponents(oss);
}
