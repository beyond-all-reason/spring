/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CpuID.h"
#include "lib/libcpuid/libcpuid/libcpuid.h"
#include "System/MainDefines.h"
#include "System/Platform/Threading.h"
#include "System/Log/ILog.h"
#include "System/ScopedResource.h"

#ifdef _MSC_VER
	#include <intrin.h>
#endif

#include "System/Threading/SpringThreading.h"
#include "System/UnorderedSet.hpp"

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
		: shiftCore(0)
		, shiftPackage(0)

		, maskVirtual(0)
		, maskCore(0)
		, maskPackage(0)

		, hasLeaf11(false)
	{
		uint32_t regs[REG_CNT] = {0, 0, 0, 0};

		SetDefault();

		// check for CPUID presence
		if (!cpuid_present()) {
			LOG_L(L_WARNING, "[CpuId] failed cpuid_present check");
			return;
		}

		EnumerateCores();
	}

	void CPUID::EnumerateCores() {
		const auto oldAffinity = Threading::GetAffinity();

		LOG("%s: thread affinity %x", __func__, Threading::GetAffinity());

		availableProceesorAffinityMask = 0;
		numLogicalCores = 0;
		numPhysicalCores = 0;

		auto cpuID = spring::ScopedResource(
			[]() {
				cpu_raw_data_array_t raw_array;
				system_id_t system;

				bool badResult = false;
				if (cpuid_get_all_raw_data(&raw_array) < 0) //necessary to call as it calls raw_array constructor
					badResult = true;

				if (cpu_identify_all(&raw_array, &system) < 0) //necessary to call as it calls system constructor
					badResult = true;

				return std::make_tuple(raw_array, system, badResult);
			}(),
			[](auto&& item) {
				cpuid_free_raw_data_array(&std::get<0>(item));
				cpuid_free_system_id(&std::get<1>(item));
			}
		);

		auto& [raw_array, system, badResult] = cpuID.Get();

		Threading::SetAffinity(oldAffinity);
		LOG("%s: thread affinity %x ...", __func__, Threading::GetAffinity());
		if (badResult) {
			LOG_L(L_WARNING, "[CpuId] error: %s", cpuid_error());
			return;
		}

		for (int group = 0; group < system.num_cpu_types; ++group) {
			cpu_id_t cpu_id = system.cpu_types[group];
			switch(cpu_id.purpose) {
			case PURPOSE_GENERAL:
			case PURPOSE_PERFORMANCE:
				availableProceesorAffinityMask |= *(uint64_t*)&cpu_id.affinity_mask;
				numLogicalCores += cpu_id.num_logical_cpus;
				numPhysicalCores += cpu_id.num_cores;
				LOG("[CpuId] found %d cores and %d logical cpus (mask: 0x%x) of type %s"
						, cpu_id.num_cores
						, cpu_id.num_logical_cpus
						, *(int*)&cpu_id.affinity_mask
						, cpu_purpose_str(cpu_id.purpose));
				LOG("[CpuId] setting logical cpu affinity mask to 0x%x", (int)availableProceesorAffinityMask);
			// ignore case PURPOSE_EFFICIENCY:
			}
		}
	}

	void CPUID::SetDefault()
	{
		numLogicalCores = spring::thread::hardware_concurrency();
		numPhysicalCores = numLogicalCores >> 1; //In 2022 HyperThreading is likely more common rather than not
		availableProceesorAffinityMask = 1;
		totalNumPackages = 1;

		// affinity mask is a uint64_t, but spring uses uint32_t
		assert(numLogicalCores <= MAX_PROCESSORS);

		static_assert(sizeof(affinityMaskOfCores   ) == (MAX_PROCESSORS * sizeof(affinityMaskOfCores   [0])), "");
		static_assert(sizeof(affinityMaskOfPackages) == (MAX_PROCESSORS * sizeof(affinityMaskOfPackages[0])), "");

		memset(affinityMaskOfCores   , 0, sizeof(affinityMaskOfCores   ));
		memset(affinityMaskOfPackages, 0, sizeof(affinityMaskOfPackages));
		memset(processorApicIds      , 0, sizeof(processorApicIds      ));

		for (int i = 0; i<numLogicalCores; ++i)
			availableProceesorAffinityMask |= 1 << i;

		// failed to determine CPU anatomy, just set affinity mask to (-1)
		for (int i = 0; i < numLogicalCores; i++) {
			affinityMaskOfCores[i] = affinityMaskOfPackages[i] = -1;
		}
	}

}
