#ifndef SLOW_UPDATE_H__
#define SLOW_UPDATE_H__

#include <stdint.h>

#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/GlobalSynced.h"


struct SlowUpdateRefreshCheckByGameFrame {
    bool operator()(int updateRate) {
        return ((gs->frameNum % updateRate)==0);
    }

    void Init() {};
};

struct SlowUpdateRefreshCheckOnce {
    bool hasAlreadyUpdated = false;

    bool operator()(int updateRate) {
        if (!hasAlreadyUpdated){
            hasAlreadyUpdated = true;
            return true;
        }
        return false;
    }

    void Init() { hasAlreadyUpdated = false; };
};

template <class RefreshCheck = SlowUpdateRefreshCheckByGameFrame>
struct SlowUpdateSubSystemT {

    template<class T, class UnaryFunction>
    void Update(T group, UnaryFunction f) {
        constexpr int updateRate = ECONOMY_SLOWUPDATE_RATE;
        if (refreshNeeded(updateRate)){
            curIndex = 0;
            lastFrameGroupSize = group.size();
            chunkSize = (lastFrameGroupSize / updateRate) + 1*(lastFrameGroupSize % updateRate != 0);
        }
        else
        {
            // iteration works backwards in EnTT.
            auto changeInSize = group.size() - lastFrameGroupSize;
            if (changeInSize > 0){
                lastFrameGroupSize += changeInSize;
                curIndex += changeInSize;
            }
        }
        auto stopBeforeIndex = std::min(curIndex+chunkSize, lastFrameGroupSize);

        for (; curIndex<stopBeforeIndex; ++curIndex){
            auto entity = group[curIndex];
            f(entity);
        }
    }

    void Init() {
        curIndex = 0;
        lastFrameGroupSize = 0;
        chunkSize = 0;
        refreshNeeded.Init();
    }

    bool IsCycleDone() {
        return (curIndex >= lastFrameGroupSize);
    }

    uint32_t curIndex = 0;
    uint32_t lastFrameGroupSize = 0;
    uint32_t chunkSize = 0;

    RefreshCheck refreshNeeded;
};

typedef SlowUpdateSubSystemT<> SlowUpdateSubSystem;
typedef SlowUpdateSubSystemT<SlowUpdateRefreshCheckOnce> SlowUpdateOnceSubSystem;

#endif