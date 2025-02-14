#ifndef CPU_TOPOLOGY_H__
#define CPU_TOPOLOGY_H__

#include <cstdint>

namespace cpu_topology {

struct ProcessorMasks {
    uint32_t performanceCoreMask = 0;
    uint32_t efficiencyCoreMask = 0;
    uint32_t hyperThreadLowMask = 0;
    uint32_t hyperThreadHighMask = 0;
};

ProcessorMasks GetProcessorMasks();

}

#endif