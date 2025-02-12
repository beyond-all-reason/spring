/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Threading.h"
#include "System/Log/ILog.h"
#include "System/Platform/CpuID.h"

#ifndef DEDICATED
	#include "System/Sync/FPUCheck.h"
#endif

#include <functional>
#include <memory>
#include <cinttypes>
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__)
#elif defined(_WIN32)
	#include <windows.h>
#else
	#include <unistd.h>
	#if defined(__USE_GNU)
		#include <sys/prctl.h>
	#endif
	#include <sched.h>
#endif

#ifndef _WIN32
	#include "Linux/ThreadSupport.h"
#endif

#include "System/Misc/TracyDefs.h"

namespace Threading {
#ifndef _WIN32
	thread_local std::shared_ptr<ThreadControls> localThreadControls;
#endif

	static NativeThreadId nativeThreadIDs[THREAD_IDX_LAST] = {};
	static Error threadError;

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__)
#elif defined(_WIN32)
	static DWORD_PTR cpusSystem = 0;
#else
	static cpu_set_t cpusSystem;
#endif


	void DetectCores()
	{
	#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__)
		// no-op

	#elif defined(_WIN32)
		// Get the available cores
		DWORD_PTR curMask;
		GetProcessAffinityMask(GetCurrentProcess(), &curMask, &cpusSystem);

		LOG("%s: cpu mask %" PRIx64, __func__, cpusSystem);
	#else
		// Get the available cores
		CPU_ZERO(&cpusSystem);
		sched_getaffinity(0, sizeof(cpu_set_t), &cpusSystem);

		// std::uint64_t curMask = 0;
		// std::uint32_t maskLimit = sizeof(curMask)*8;
		// std::uint32_t nproc = std:min(sysconf(_SC_NPROCESSORS_ONLN), maskLimit);
		// for (i=0; i<nproc; ++i) {
		// 	curMask |= (static_cast<std::uint64_t>(CPU_ISSET(i, &cpusSystem)) << i);
		// }
		// LOG("%s: cpu mask %" PRIx64, __func__, curMask);
	#endif

		GetPhysicalCpuCores(); // (uses a static, too)
	}



	#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__)
	#elif defined(_WIN32)
	#else
	static std::uint32_t CalcCoreAffinityMask(const cpu_set_t* cpuSet) {
		std::uint32_t coreMask = 0;

		// without the min(..., 32), `(1 << n)` could overflow
		const int numCPUs = std::min(CPU_COUNT(&cpusSystem), 32);

		for (int n = numCPUs - 1; n >= 0; --n) {
			if (CPU_ISSET(n, cpuSet))
				coreMask |= (1 << n);
		}

		return coreMask;
	}

	static void SetWantedCoreAffinityMask(cpu_set_t* cpuDstSet, std::uint32_t coreMask) {
		CPU_ZERO(cpuDstSet);

		const int numCPUs = std::min(CPU_COUNT(&cpusSystem), 32);

		for (int n = numCPUs - 1; n >= 0; --n) {
			if ((coreMask & (1 << n)) != 0)
				CPU_SET(n, cpuDstSet);
		}

		CPU_AND(cpuDstSet, cpuDstSet, &cpusSystem);
	}
	#endif

#pragma GCC push_options
#pragma GCC optimize ("O0")

	// Mingw v12 is the minimum version that gives us a GetLogicalProcessorInformationEx() with efficiency core
	// detection. Unfortunately mingw v12 also introduces a ticking time bomb change that can cause the toolchain to
	// produce a crashing exe because of a change in static initialized variable space.
	// See bug report for more information: https://sourceforge.net/p/mingw-w64/bugs/992/
	// The net result is that we have to use GetProcAddress() on the kernel32 ourselves and provide a modified set
	// of data definitions with the information we need.
	// We can do away with this if 1) mingw fix their bug or 2) the build system shifts over to using MSVC.
	#if defined(_WIN32)
	namespace spring_overrides {

		typedef struct _GROUP_AFFINITY {
			KAFFINITY Mask;
			WORD      Group;
			WORD      Reserved[3];
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

		typedef struct _PROCESSOR_RELATIONSHIP {
			BYTE Flags;
			BYTE EfficiencyClass;
			BYTE Reserved[20];
			WORD GroupCount;
			GROUP_AFFINITY GroupMask[ANYSIZE_ARRAY];
		  } PROCESSOR_RELATIONSHIP,*PPROCESSOR_RELATIONSHIP;

		struct _SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX {
			LOGICAL_PROCESSOR_RELATIONSHIP Relationship;
			DWORD Size;
			__C89_NAMELESS union {
			  PROCESSOR_RELATIONSHIP Processor;
			  //NUMA_NODE_RELATIONSHIP NumaNode;
			  //CACHE_RELATIONSHIP Cache;
			  //GROUP_RELATIONSHIP Group;
			};
		  };

		  typedef struct _SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX, *PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX;
	  
		#if _WIN32_WINNT >= 0x0601
		typedef BOOL (WINAPI *GetLogicalProcessorInformationExFunc)(
			LOGICAL_PROCESSOR_RELATIONSHIP,
			PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX,
			PDWORD);
		//WINBASEAPI WINBOOL WINAPI GetLogicalProcessorInformationEx (LOGICAL_PROCESSOR_RELATIONSHIP RelationshipType, PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX Buffer, PDWORD ReturnedLength);
		#endif
	}
	#endif

	struct ProcessorMasks {
		uint32_t performanceCoreMask = 0;
		uint32_t efficiencyCoreMask = 0;
		uint32_t hyperThreadLowMask = 0;
		uint32_t hyperThreadHighMask = 0;
	};

	#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__)
	// noop
	#elif defined(_WIN32)
	ProcessorMasks GetProcessorMasksWindows() {
		spring_overrides::GetLogicalProcessorInformationExFunc Local_GetLogicalProcessorInformationEx;
		spring_overrides::PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX buffer = NULL;
		spring_overrides::PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX ptr = NULL;
		DWORD returnLength = 0;
		DWORD byteOffset = 0;
		BOOL done = FALSE;
		ProcessorMasks processorMasks;

		Local_GetLogicalProcessorInformationEx = (spring_overrides::GetLogicalProcessorInformationExFunc) GetProcAddress(
			GetModuleHandle(TEXT("kernel32")),
			"GetLogicalProcessorInformationEx");
		if (NULL == Local_GetLogicalProcessorInformationEx)
		{
			LOG("GetLogicalProcessorInformation is not supported.\n");
			return processorMasks;
		}

		while (!done)
		{
			DWORD rc = Local_GetLogicalProcessorInformationEx(spring_overrides::RelationProcessorCore, buffer, &returnLength);

			if (FALSE == rc)
			{
				if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
				{
					if (buffer)
						free(buffer);

					buffer = (spring_overrides::PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)malloc(returnLength);

					if (NULL == buffer)
						return processorMasks;
				}
				else
					return processorMasks;
			}
			else
				done = TRUE;
		}
		ptr = buffer;

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

				if (ptr->Processor.EfficiencyClass){
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
	#else
	ProcessorMasks GetProcessorMasksWindows() {
		ProcessorMasks processorMasks;
		processorMasks.performanceCoreMask = 0xfffffff;
		// fopen("/proc/cpuinfo");
		return processorMasks;
	}
	#endif

	uint32_t GetSystemAffinityMask() {
		#if defined(_WIN32)
		ProcessorMasks pm = GetProcessorMasksWindows();
		// need an entry for linux.
		#else
		ProcessorMasks pm;
		pm.performanceCoreMask = 0xfffffff;
		#endif

		LOG("CPU Affinity Mask Details detected:");
		LOG("-- Performance Core Mask:      0x%08x", pm.performanceCoreMask);
		LOG("-- Efficiency  Core Mask:      0x%08x", pm.efficiencyCoreMask);
		LOG("-- Hyper Thread/SMT Low Mask:  0x%08x", pm.hyperThreadLowMask);
		LOG("-- Hyper Thread/SMT High Mask: 0x%08x", pm.hyperThreadHighMask);

		// Engine worker thread policy:
		// 1. Only use general/performance cores. Do not use efficiency cores.
		// 2. Do not use Hyper Threading or SMT. If present use only one of the HW threads per core.
		return pm.performanceCoreMask & (~pm.hyperThreadHighMask);
	}

	std::uint32_t GetAffinity()
	{
	#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__)
		// no-op
		return 0;

	#elif defined(_WIN32)
		DWORD_PTR curMask;
		DWORD_PTR systemCpus;
		GetProcessAffinityMask(GetCurrentProcess(), &curMask, &systemCpus);
		return curMask;
	#else
		cpu_set_t curAffinity;
		CPU_ZERO(&curAffinity);
		sched_getaffinity(0, sizeof(cpu_set_t), &curAffinity);

		return (CalcCoreAffinityMask(&curAffinity));
	#endif
	}

#pragma GCC pop_options

	std::uint32_t SetAffinity(std::uint32_t coreMask, bool hard)
	{
		if (coreMask == 0)
			return (~0);

	#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__)
		// no-op
		return 0;

	#elif defined(_WIN32)
		// create mask
		DWORD_PTR cpusWanted = (coreMask & cpusSystem);
		DWORD_PTR result = 0;

		HANDLE thread = GetCurrentThread();

		// set the affinity
		if (hard) {
			result = SetThreadAffinityMask(thread, cpusWanted);
		} else {
			result = SetThreadIdealProcessor(thread, (DWORD)cpusWanted);
		}

		// return final mask
		return ((static_cast<std::uint32_t>(cpusWanted)) * (result > 0));
	#else
		cpu_set_t cpusWanted;

		// create wanted mask
		SetWantedCoreAffinityMask(&cpusWanted, coreMask);

		// set the affinity; return final mask (can differ from wanted)
		if (sched_setaffinity(0, sizeof(cpu_set_t), &cpusWanted) == 0)
			return (CalcCoreAffinityMask(&cpusWanted));

		return 0;
	#endif
	}

	void SetAffinityHelper(const char* threadName, std::uint32_t affinity) {
		const std::uint32_t cpuMask = Threading::SetAffinity(affinity);

		if (cpuMask == ~0u) {
			LOG("[Threading] %s thread CPU affinity not set", threadName);
			return;
		}
		if (cpuMask == 0) {
			LOG_L(L_ERROR, "[Threading] %s thread CPU affinity mask failed: 0x%x", threadName, affinity);
			return;
		}
		if (cpuMask != affinity) {
			LOG("[Threading] %s thread CPU affinity mask set: 0x%x (config is %x)", threadName, cpuMask, affinity);
			return;
		}

		LOG("[Threading] %s thread CPU affinity mask set: 0x%x", threadName, cpuMask);
	}


	std::uint32_t GetAvailableCoresMask()
	{
	#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__)
		// no-op
		return (~0);
	#elif defined(_WIN32)
		return cpusSystem;
	#else
		return (CalcCoreAffinityMask(&cpusSystem));
	#endif
	}


	int GetLogicalCpuCores() {
		// auto-detect number of system threads (including hyperthreading)
		//return spring::thread::hardware_concurrency();
		return springproc::CPUID::GetInstance().GetNumLogicalCores();
	}

	/** Function that returns the number of real cpu cores (not
	    hyperthreading ones). These are the total cores in the system
	    (across all existing processors, if more than one)*/
	int GetPhysicalCpuCores() {
		return springproc::CPUID::GetInstance().GetNumPhysicalCores();
	}

	bool HasHyperThreading() { return (GetLogicalCpuCores() > GetPhysicalCpuCores()); }


	void SetThreadScheduler()
	{
	#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__)
		// no-op

	#elif defined(_WIN32)
		//TODO add MMCSS (http://msdn.microsoft.com/en-us/library/ms684247.aspx)
		//Note: only available with mingw64!!!

	#else
		if (GetLogicalCpuCores() > 1) {
			// Change os scheduler for this process.
			// This way the kernel knows that we are a CPU-intensive task
			// and won't randomly move us across the cores and tries
			// to maximize the runtime (_slower_ wakeups, less yields)
			//Note:
			// It _may_ be possible that this has negative impact in case
			// threads are waiting for mutexes (-> less yields).
			int policy;
			struct sched_param param;
			pthread_getschedparam(Threading::GetCurrentThread(), &policy, &param);
			pthread_setschedparam(Threading::GetCurrentThread(), SCHED_BATCH, &param);
		}
	#endif
	}


	NativeThreadHandle GetCurrentThread()
	{
	#ifdef _WIN32
		// we need to use this cause GetCurrentThread() just returns a pseudo handle,
		// which returns in all threads the current active one, so we need to translate it
		// with DuplicateHandle to an absolute handle valid in our watchdog thread
		NativeThreadHandle hThread;
		::DuplicateHandle(::GetCurrentProcess(), ::GetCurrentThread(), ::GetCurrentProcess(), &hThread, 0, TRUE, DUPLICATE_SAME_ACCESS);
		return hThread;
	#else
		return pthread_self();
	#endif
	}


	NativeThreadId GetCurrentThreadId()
	{
	#ifdef _WIN32
		return ::GetCurrentThreadId();
	#else
		return pthread_self();
	#endif
	}



	ThreadControls::ThreadControls():
		handle(0),
		running(false)
	{
#ifndef _WIN32
		memset(&ucontext, 0, sizeof(ucontext_t));
#endif
	}


#ifndef _WIN32
	std::shared_ptr<ThreadControls> GetCurrentThreadControls() { return localThreadControls; }
#endif


	spring::thread CreateNewThread(std::function<void()> taskFunc, std::shared_ptr<Threading::ThreadControls>* threadCtls)
	{
#ifndef _WIN32
		// only used as locking mechanism, not installed by thread
		Threading::ThreadControls tempCtls;

		std::unique_lock<spring::mutex> lock(tempCtls.mutSuspend);
		spring::thread localthread(std::bind(Threading::ThreadStart, taskFunc, threadCtls, &tempCtls));

		// wait so that we know the thread is running and fully initialized before returning
		tempCtls.condInitialized.wait(lock);
#else
		spring::thread localthread(taskFunc);
#endif

		return localthread;
	}



	static void SetThreadID(unsigned int threadIndex) {
		// NOTE:
		//   LOAD and SND thread ID's always have to be set unconditionally
		//   (threads are joined and respawned when reloading, so KISS here)
		//   other threads never call Set*Thread more than once, no need for
		//   caching
		nativeThreadIDs[threadIndex] = Threading::GetCurrentThreadId();

		switch (threadIndex) {
			case THREAD_IDX_LOAD: {
				// do nothing if Load is actually Main (LoadingMT=0 case)
				if (IsMainThread())
					return;
			} break;
			#ifndef _WIN32
			// both heartBeatThread and soundThread make use of CreateNewThread -> ThreadStart
			// other threads under the eye of watchdog have their control structure setup here
			case THREAD_IDX_SND : { return; } break;
			#endif
			case THREAD_IDX_WDOG: { return; } break;
		}
	#ifndef _WIN32
		SetupCurrentThreadControls(localThreadControls);
	#endif
	}

	void     SetMainThread() { SetThreadID(THREAD_IDX_MAIN); }
	void SetGameLoadThread() { SetThreadID(THREAD_IDX_LOAD); }
	void    SetAudioThread() { SetThreadID(THREAD_IDX_SND ); }
	void  SetFileSysThread() { SetThreadID(THREAD_IDX_VFSI); }
	void SetWatchDogThread() { SetThreadID(THREAD_IDX_WDOG); }

	bool IsMainThread(NativeThreadId threadID) { return NativeThreadIdsEqual(threadID, nativeThreadIDs[THREAD_IDX_MAIN]); }
	bool IsMainThread(                       ) { return IsMainThread(Threading::GetCurrentThreadId()); }

	bool IsGameLoadThread(NativeThreadId threadID) { return NativeThreadIdsEqual(threadID, nativeThreadIDs[THREAD_IDX_LOAD]); }
	bool IsGameLoadThread(                       ) { return IsGameLoadThread(Threading::GetCurrentThreadId()); }

	bool IsAudioThread(NativeThreadId threadID) { return NativeThreadIdsEqual(threadID, nativeThreadIDs[THREAD_IDX_SND]); }
	bool IsAudioThread(                       ) { return IsAudioThread(Threading::GetCurrentThreadId()); }

	bool IsFileSysThread(NativeThreadId threadID) { return NativeThreadIdsEqual(threadID, nativeThreadIDs[THREAD_IDX_VFSI]); }
	bool IsFileSysThread(                       ) { return IsFileSysThread(Threading::GetCurrentThreadId()); }

	bool IsWatchDogThread(NativeThreadId threadID) { return NativeThreadIdsEqual(threadID, nativeThreadIDs[THREAD_IDX_WDOG]); }
	bool IsWatchDogThread(                       ) { return IsWatchDogThread(Threading::GetCurrentThreadId()); }



	void SetThreadName(const std::string& newname)
	{
	#if defined(TRACY_ENABLE)
		tracy::SetThreadName(newname.c_str());
	#endif
	#if defined(__USE_GNU) && !defined(_WIN32)
		//alternative: pthread_setname_np(pthread_self(), newname.c_str());
		prctl(PR_SET_NAME, newname.c_str(), 0, 0, 0);
	#elif _MSC_VER
		const DWORD MS_VC_EXCEPTION = 0x406D1388;

		#pragma pack(push,8)
		struct THREADNAME_INFO
		{
			DWORD dwType; // Must be 0x1000.
			LPCSTR szName; // Pointer to name (in user addr space).
			DWORD dwThreadID; // Thread ID (-1=caller thread).
			DWORD dwFlags; // Reserved for future use, must be zero.
		} info;
		#pragma pack(pop)

		info.dwType = 0x1000;
		info.szName = newname.c_str();
		info.dwThreadID = (DWORD)-1;
		info.dwFlags = 0;

		__try {
			RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*) &info);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
		}
	#endif
	}

	// NB: no protection against two threads posting at the same time
	const Error* GetThreadErrorC() { return &threadError; }
	      Error* GetThreadErrorM() { return &threadError; }
}

