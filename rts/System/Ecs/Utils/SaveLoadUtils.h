/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SAVE_LOAD_UTILS_H__
#define SAVE_LOAD_UTILS_H__

#include "Sim/Ecs/EcsMain.h"
#include "System/creg/Serializer.h"

namespace SystemGlobals {

class SaveLoadUtils {
public:
    static void LoadComponents(std::stringstream &iss);
    static void SaveComponents(std::stringstream &oss);
};

}

#endif