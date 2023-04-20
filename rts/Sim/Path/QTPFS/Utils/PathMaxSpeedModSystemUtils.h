#ifndef PATH_MAX_SPEED_MOD_SYSTEM_UTILS_H_
#define PATH_MAX_SPEED_MOD_SYSTEM_UTILS_H_

#include "Sim/Misc/GlobalSynced.h"

#include "Sim/Path/QTPFS/Components/PathSearch.h"
#include "Sim/Path/QTPFS/Registry.h"


namespace QTPFS {

    float GetMaxSpeedModForLayer(int layerNum) {
        auto& comp = systemGlobals.GetSystemComponent<PathMaxSpeedModSystemComponent>();
        return comp.maxRelSpeedMod[layerNum];
    }

    void RequestMaxSpeedModRefreshForLayer(int layerNum) {
        auto& comp = systemGlobals.GetSystemComponent<PathMaxSpeedModSystemComponent>();
        if (comp.startRefreshOnFrame == NEXT_FRAME_NEVER)
            comp.startRefreshOnFrame = gs->frameNum + comp.refeshDelayInFrames;
        
        // auto layersView = registry.view<NodeLayerMaxSpeedSweep>();
        // for(auto entity : layersView){
        //     auto& layer = layersView.get<NodeLayerMaxSpeedSweep>(entity);
        //     if (layer.layerNum == layerNum) {
        //         layer.requestUpdate = true;
        //         break;
        //     }
        // }
    }

}

#endif