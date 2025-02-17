#include "CpuTopology.h"
#include <atomic>
#include <bitset>
#include <mutex>

namespace cpu_topology {

static std::mutex cacheMutex;
static volatile bool cacheActive = false;

static ProcessorMasks cachedProcessorMasks;
static int logicalCpuCount = 0;
static int physicalCpuCount = 0;

void SetCpuCounts(ProcessorMasks& masks) {
	const uint32_t logicalCountMask = (masks.efficiencyCoreMask & masks.performanceCoreMask);
	const uint32_t coreCountMask = logicalCountMask & ~masks.hyperThreadHighMask;

	logicalCpuCount = std::bitset<32>(logicalCountMask).count();
	physicalCpuCount = std::bitset<32>(coreCountMask).count();
}

void InitTopologicalData() {
	if (cacheActive)
		return;

	const std::lock_guard lock(cacheMutex);
	if (cacheActive)
		return;

	cachedProcessorMasks = GetProcessorMasks();
	SetCpuCounts(cachedProcessorMasks);
	cacheActive = true;
}

const ProcessorMasks& GetCachedProcessorMasks() {
	InitTopologicalData();

	return cachedProcessorMasks;
}

int GetNumLogicalCpuCores() {
	InitTopologicalData();

	return logicalCpuCount;
}

int GetNumPhysicalCpuCores() {
	InitTopologicalData();

	return physicalCpuCount;
}

} //namespace cpu_topology
