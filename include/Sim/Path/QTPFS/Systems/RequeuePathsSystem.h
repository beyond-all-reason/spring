/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef REQUEUE_PATHS_SYSTEM_H__
#define REQUEUE_PATHS_SYSTEM_H__

namespace QTPFS {

class RequeuePathsSystem {
public:
    static void Init();
    static void Update();
    static void Shutdown();
};

}

#endif