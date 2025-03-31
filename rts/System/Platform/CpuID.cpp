/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CpuID.h"
#include "System/MainDefines.h"
#include "System/Platform/Threading.h"
#include "System/Log/ILog.h"
#include "System/ScopedResource.h"

#ifdef _MSC_VER
	#include <intrin.h>
#endif

#include "System/Threading/SpringThreading.h"
#include "System/UnorderedSet.hpp"

#include <algorithm>
#include <bitset>
#include <cassert>
#include <tuple>


namespace springproc {
	enum {
		REG_EAX = 0,
		REG_EBX = 1,
		REG_ECX = 2,
		REG_EDX = 3,
		REG_CNT = 4,
	};


#if (__is_x86_arch__ == 1 && defined(__GNUC__))
	// function inlining breaks the asm
	// NOLINTNEXTLINE{readability-non-const-parameter}
	_noinline void ExecCPUID(unsigned int* a, unsigned int* b, unsigned int* c, unsigned int* d)
	{
		#ifndef __APPLE__
		__asm__ __volatile__(
			"cpuid"
			: "=a" (*a), "=b" (*b), "=c" (*c), "=d" (*d)
			: "0" (*a), "2" (*c)
		);

		#else

		#ifdef __x86_64__
			__asm__ __volatile__(
				"pushq %%rbx\n\t"
				"cpuid\n\t"
				"movl %%ebx, %1\n\t"
				"popq %%rbx"
				: "=a" (*a), "=r" (*b), "=c" (*c), "=d" (*d)
				: "0" (*a)
			);
		#else
			__asm__ __volatile__(
				"pushl %%ebx\n\t"
				"cpuid\n\t"
				"movl %%ebx, %1\n\t"
				"popl %%ebx"
				: "=a" (*a), "=r" (*b), "=c" (*c), "=d" (*d)
				: "0" (*a)
			);
		#endif
		#endif
	}

#elif (__is_x86_arch__ == 1 && defined(_MSC_VER) && (_MSC_VER >= 1310))

	void ExecCPUID(unsigned int* a, unsigned int* b, unsigned int* c, unsigned int* d)
	{
		int regs[REG_CNT] = {0};
		__cpuid(regs, *a);
		*a = regs[0];
		*b = regs[1];
		*c = regs[2];
		*d = regs[3];
	}

#else

	// no-op on other compilers / platforms (ARM has no cpuid instruction, etc)
	void ExecCPUID(unsigned int* a, unsigned int* b, unsigned int* c, unsigned int* d) {}
#endif

	CPUID& CPUID::GetInstance() {
		static CPUID cpuid;
		return cpuid;
	}

	CPUID::CPUID()
	{
		uint32_t regs[REG_CNT] = {0, 0, 0, 0};

		EnumerateCores();
	}

	void CPUID::EnumerateCores() {
		processorMasks = cpu_topology::GetProcessorMasks();
		processorCaches = cpu_topology::GetProcessorCache();

		// Sort the logical processor groups to move the groups with the largest cache to the front. This will make it
		// easier for policies to make decisions on logical processor groups based on cache size.
		std::ranges::stable_sort
			( processorCaches.groupCaches
			// sort larger to the bottom.
			, [](const auto &lh, const auto &rh) -> bool { return lh.cacheSizes[2] > rh.cacheSizes[2]; });

		std::ranges::for_each
			( processorCaches.groupCaches
			, [](const auto& cache) -> void { LOG("Found logical processors (mask 0x%08x) using L3 cache (sized %dKB) ", cache.groupMask, cache.cacheSizes[2] / 1024); });

		const uint32_t logicalCountMask  = (processorMasks.efficiencyCoreMask | processorMasks.performanceCoreMask);
		const uint32_t perfCoreCountMask = processorMasks.performanceCoreMask & ~processorMasks.hyperThreadHighMask;
		const uint32_t coreCountMask     = logicalCountMask & ~processorMasks.hyperThreadHighMask;
	
		numLogicalCores     = std::popcount(logicalCountMask);
		numPhysicalCores    = std::popcount(coreCountMask);
		numPerformanceCores = std::popcount(perfCoreCountMask);

		smtDetected = !!( processorMasks.hyperThreadLowMask | processorMasks.hyperThreadHighMask );
	}

}
