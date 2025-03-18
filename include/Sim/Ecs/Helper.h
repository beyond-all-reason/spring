/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SIM_ECS_HELPER_H__
#define SIM_ECS_HELPER_H__

#include <sstream>

// Functions to help work around Lua header conflicts.

namespace Sim {
    void ClearRegistry();

    void LoadComponents(std::stringstream &iss);
    void SaveComponents(std::stringstream &oss);
}

#endif