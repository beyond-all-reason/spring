/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TKPFS_PATHINGSTATESYSTEM_H
#define TKPFS_PATHINGSTATESYSTEM_H

#include <string>
#include <vector>

namespace TKPFS {

class PathingState {
public:

    void Init(PathingState* parentState, unsigned int BLOCK_SIZE, const std::string& peFileName, const std::string& mapFileName);

    void Terminate() {};

    float GetMaxSpeedMod(unsigned int pathType) const { return maxSpeedMods[pathType]; };

private:
    PathingState* nextPathState = nullptr;

    std::vector<float> maxSpeedMods;
};

}

#endif
