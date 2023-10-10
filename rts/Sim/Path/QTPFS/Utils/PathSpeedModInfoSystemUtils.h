#ifndef PATH_SPEED_MOD_INFO_SYSTEM_UTILS_H_
#define PATH_SPEED_MOD_INFO_SYSTEM_UTILS_H_

#include "Sim/Misc/GlobalSynced.h"

#include "Sim/Path/QTPFS/Components/PathSpeedModInfo.h"
#include "Sim/Path/QTPFS/Registry.h"


namespace QTPFS {

    float GetMaxSpeedModForLayer(int layerNum) {
        auto& comp = systemGlobals.GetSystemComponent<PathSpeedModInfoSystemComponent>();
        return comp.relSpeedModinfos[layerNum].max;
    }

    void RequestMaxSpeedModRefreshForLayer(int layerNum) {
        auto& comp = systemGlobals.GetSystemComponent<PathSpeedModInfoSystemComponent>();
        if (comp.startRefreshOnFrame == NEXT_FRAME_NEVER)
            comp.startRefreshOnFrame = gs->frameNum + comp.refeshDelayInFrames;
    }

}

#endif