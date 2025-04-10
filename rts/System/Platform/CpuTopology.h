#pragma once

#include <cstdint>
#include <vector>

namespace cpu_topology {

static constexpr uint32_t MAX_CACHE_LEVELS = 3;

struct ProcessorMasks {
	uint32_t performanceCoreMask = 0;
	uint32_t efficiencyCoreMask = 0;
	uint32_t hyperThreadLowMask = 0;
	uint32_t hyperThreadHighMask = 0;
};

struct ProcessorGroupCaches {
	uint32_t groupMask = 0;
	uint32_t cacheSizes[MAX_CACHE_LEVELS] = {0, 0, 0};
};

struct ProcessorCaches {
	std::vector<ProcessorGroupCaches> groupCaches;
};

// OS-specific implementation to get the processor masks.
ProcessorMasks GetProcessorMasks();

// OS-specific implementation to get the logical processor masks and the L3 cache they have access to.
ProcessorCaches GetProcessorCache();

} // namespace cpu_topology
