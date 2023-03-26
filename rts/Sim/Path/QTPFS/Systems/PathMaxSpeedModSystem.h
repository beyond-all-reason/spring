/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATH_MAX_SPEED_MOD_SYSTEM_H__
#define PATH_MAX_SPEED_MOD_SYSTEM_H__

namespace QTPFS {

class PathMaxSpeedModSystem {
public:
    static void Init();
    static void Update();
    static void Shutdown();
};

}

#endif