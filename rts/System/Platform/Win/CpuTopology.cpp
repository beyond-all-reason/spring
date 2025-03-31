#include "System/Platform/CpuTopology.h"

#include "System/Log/ILog.h"

#include <algorithm>
#include <bit>
#include <set>
#include <windows.h>

namespace cpu_topology {

// Mingw v12 is the minimum version that gives us a GetLogicalProcessorInformationEx() with efficiency core
// detection. Unfortunately mingw v12 also introduces a ticking time bomb change that can cause the toolchain to
// produce a crashing exe because of a change in static initialized variable space.
// See bug report for more information: https://sourceforge.net/p/mingw-w64/bugs/992/
// The net result is that we have to use GetProcAddress() on the kernel32 ourselves and provide a modified set
// of data definitions with the information we need.
// We can do away with this if 1) mingw fix their bug or 2) the build system shifts over to using MSVC.
namespace spring_overrides {

	typedef struct _GROUP_AFFINITY {
		KAFFINITY Mask;
		WORD	  Group;
		WORD	  Reserved[3];
		} GROUP_AFFINITY, *PGROUP_AFFINITY;

	typedef enum _LOGICAL_PROCESSOR_RELATIONSHIP {
		RelationProcessorCore,
		RelationNumaNode,
		RelationCache,
		RelationProcessorPackage,
		RelationGroup,
		RelationProcessorDie,
		RelationNumaNodeEx,
		RelationProcessorModule,
		RelationAll = 0xffff
		} LOGICAL_PROCESSOR_RELATIONSHIP;

	typedef struct {
		BYTE Flags;
		BYTE EfficiencyClass;
		BYTE Reserved[20];
		WORD GroupCount;
		GROUP_AFFINITY GroupMask[ANYSIZE_ARRAY];
	} PROCESSOR_RELATIONSHIP,*PPROCESSOR_RELATIONSHIP;

	typedef struct {
		BYTE                 Level;
		BYTE                 Associativity;
		WORD                 LineSize;
		DWORD                CacheSize;
		PROCESSOR_CACHE_TYPE Type;
		BYTE                 Reserved[18];
		WORD                 GroupCount;
		union {
		  GROUP_AFFINITY GroupMask;
		  GROUP_AFFINITY GroupMasks[ANYSIZE_ARRAY];
		} DUMMYUNIONNAME;
	} CACHE_RELATIONSHIP, *PCACHE_RELATIONSHIP;

	typedef struct _SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX {
		LOGICAL_PROCESSOR_RELATIONSHIP Relationship;
		DWORD                          Size;
		union {
			PROCESSOR_RELATIONSHIP Processor;
			NUMA_NODE_RELATIONSHIP NumaNode;
			CACHE_RELATIONSHIP     Cache;
			GROUP_RELATIONSHIP     Group;
		} DUMMYUNIONNAME;
	} SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX, *PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX;
	
	#if _WIN32_WINNT >= 0x0601
	typedef BOOL (WINAPI *GetLogicalProcessorInformationExFunc)(
		LOGICAL_PROCESSOR_RELATIONSHIP,
		PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX,
		PDWORD);
	//WINBASEAPI WINBOOL WINAPI GetLogicalProcessorInformationEx (LOGICAL_PROCESSOR_RELATIONSHIP RelationshipType, PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX Buffer, PDWORD ReturnedLength);
	#endif
}


BOOL GetProcessorInformation
	( spring_overrides::PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX& buffer
	, DWORD& returnLength
	, cpu_topology::spring_overrides::LOGICAL_PROCESSOR_RELATIONSHIP queryRelationshipType
	)
{
	spring_overrides::GetLogicalProcessorInformationExFunc Local_GetLogicalProcessorInformationEx;
	BOOL done = FALSE;

	Local_GetLogicalProcessorInformationEx = (spring_overrides::GetLogicalProcessorInformationExFunc) GetProcAddress(
		GetModuleHandle(TEXT("kernel32")),
		"GetLogicalProcessorInformationEx");
	if (NULL == Local_GetLogicalProcessorInformationEx)
	{
		LOG("GetLogicalProcessorInformation is not supported.\n");
		return FALSE;
	}

	while (!done)
	{
		DWORD rc = Local_GetLogicalProcessorInformationEx(queryRelationshipType, buffer, &returnLength);

		if (FALSE == rc)
		{
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
				if (buffer)
					free(buffer);

				buffer = (spring_overrides::PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)malloc(returnLength);

				if (NULL == buffer)
					return FALSE;
			}
			else
				return FALSE;
		}
		else
			done = TRUE;
	}
	return TRUE;
}

ProcessorMasks GetProcessorMasks() {
	spring_overrides::PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX buffer = NULL;
	spring_overrides::PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX ptr = NULL;
	DWORD returnLength = 0;
	DWORD byteOffset = 0;
	BYTE performanceClass = 0;
	ProcessorMasks processorMasks;

	if (GetProcessorInformation(buffer, returnLength, spring_overrides::RelationProcessorCore) == FALSE){
		return processorMasks;
	}

	// The number of EfficiencyClass values depends on the processor.The highest numbered class is always the
	// performance core.
	ptr = buffer;
	byteOffset = 0;
	while (byteOffset < returnLength)
	{
		if (ptr->Relationship == spring_overrides::RelationProcessorCore)
		{
			BYTE ef = ptr->Processor.EfficiencyClass;
			performanceClass = std::max(performanceClass, ef);
		}
		byteOffset += ptr->Size;
		ptr = (spring_overrides::PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)(((char*)buffer) + byteOffset);
	}

	// Now collect the logical processor masks.
	ptr = buffer;
	byteOffset = 0;
	while (byteOffset < returnLength)
	{
		if (ptr->Relationship == spring_overrides::RelationProcessorCore)
		{
			const uint32_t supportedMask = static_cast<uint32_t>(ptr->Processor.GroupMask[0].Mask);
			if (supportedMask == 0) {
				LOG("Info: Processor group %d has a thread mask outside of the supported range."
					, int(ptr->Processor.GroupCount));
				break;
			}

			const bool hyperThreading = !std::has_single_bit(supportedMask);
			if (hyperThreading) {
				processorMasks.hyperThreadLowMask |= ( 1 << std::countr_zero(supportedMask) );
				processorMasks.hyperThreadHighMask |= ( 0x80000000 >> std::countl_zero(supportedMask) );
			}

			if (ptr->Processor.EfficiencyClass != performanceClass){
				processorMasks.efficiencyCoreMask |= supportedMask;
			} else {
				processorMasks.performanceCoreMask |= supportedMask;
			}
		}
		byteOffset += ptr->Size;
		ptr = (spring_overrides::PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)(((char*)buffer) + byteOffset);
	}

	if (buffer)
		free(buffer);

	return processorMasks;
}

// Notes. We're only interested in L3 cache sizes at the moment becasue we're only using this
// information to find a performance core with the most cache to pin the main thread to.
ProcessorCaches GetProcessorCache() {
	spring_overrides::PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX buffer = NULL;
	spring_overrides::PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX ptr = NULL;
	DWORD returnLength = 0;
	DWORD byteOffset = 0;
	ProcessorCaches processorCaches;

	if (GetProcessorInformation(buffer, returnLength, spring_overrides::RelationCache) == FALSE) {
		return processorCaches;
	}

	ptr = buffer;
	byteOffset = 0;
	while (byteOffset < returnLength)
	{
		if (ptr->Relationship == spring_overrides::RelationCache && ptr->Cache.Level == 3)
		{
			const uint32_t supportedMask = static_cast<uint32_t>(ptr->Cache.GroupMasks[0].Mask);
			if (supportedMask == 0) {
				LOG("Info: Processor group %d has a thread mask outside of the supported range."
					, int(ptr->Processor.GroupCount));
				break;
			}

			ProcessorGroupCaches groupCache;
			groupCache.groupMask = supportedMask;
			groupCache.cacheSizes[ptr->Cache.Level - 1] = ptr->Cache.CacheSize;

			processorCaches.groupCaches.push_back(groupCache);
		}

		byteOffset += ptr->Size;
		ptr = (spring_overrides::PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)(((char*)buffer) + byteOffset);
	}

	if (buffer)
		free(buffer);

	std::stable_sort
		( processorCaches.groupCaches.begin()
		, processorCaches.groupCaches.end()
		// sort larger to the bottom.
		, [](const auto &lh, const auto &rh) -> bool { return lh.cacheSizes[2] > rh.cacheSizes[2]; });

	std::for_each
		( processorCaches.groupCaches.begin()
		, processorCaches.groupCaches.end()
		, [](const auto& cache) -> void { LOG("Found logical processors (mask 0x%08x) using L3 cache (sized %dKB) ", cache.groupMask, cache.cacheSizes[2] / 1024); });

	return processorCaches;
}

} //namespace cpu_topology
