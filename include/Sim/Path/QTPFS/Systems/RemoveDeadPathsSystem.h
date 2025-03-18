/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef REMOVE_DEAD_PATHS_SYSTEM_H__
#define REMOVE_DEAD_PATHS_SYSTEM_H__

namespace QTPFS {

class RemoveDeadPathsSystem {
public:
    static void Init();
    static void Update();
    static void Shutdown();
};

}

#endif