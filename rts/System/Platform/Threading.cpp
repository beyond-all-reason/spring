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
	#include "System/Platform/Win/DllLib.h"
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


	std::once_flag affinityMaskDetailsLogFlag;

	uint32_t GetSystemAffinityMask() {
		cpu_topology::ProcessorMasks pm = springproc::CPUID::GetInstance().GetAvailableProcessorAffinityMask();

		std::call_once(affinityMaskDetailsLogFlag, [&](){
			LOG("CPU Affinity Mask Details detected:");
			LOG("-- Performance Core Mask:      0x%08x", pm.performanceCoreMask);
			LOG("-- Efficiency  Core Mask:      0x%08x", pm.efficiencyCoreMask);
			LOG("-- Hyper Thread/SMT Low Mask:  0x%08x", pm.hyperThreadLowMask);
			LOG("-- Hyper Thread/SMT High Mask: 0x%08x", pm.hyperThreadHighMask);
		});

		// Engine worker thread pool are primarily for mutli-threading activies of simulation; though, they are
		// available to be used by other system while simulation is not running. As such the policy for pinning worker
		// threads are to maximise performance of the multi-threaded tasks of simulation, which are a poor fit for
		// cpu hardware threads (SMT/Hyper-Threading) and low-power cores.
		//
		// Engine worker thread policy:
		// 1. Only use general/performance cores. Do not use efficiency cores.
		// 2. Do not use Hyper Threading or SMT. If present use only one of the HW threads per core.
		//
		// This doesn't preclude systems from using separate unpinned threads, which the OS should logically try to
		// move to under used resources, such as low-power cores for example.
		#if defined(THREADPOOL)
		const uint32_t policy = pm.performanceCoreMask & (~pm.hyperThreadHighMask);
		#else

		/* Allow any core; keep it a "proper" mask though
		 * since that has less risk of blowing up than 0 or 0xFF..FF */
		const uint32_t policy = pm.performanceCoreMask | pm.efficiencyCoreMask;
		#endif

		return policy;
	}

	std::once_flag preferredMaskDetailsLogFlag;

	uint32_t GetPreferredMainThreadMask() {
		cpu_topology::ProcessorCaches pc = springproc::CPUID::GetInstance().GetProcessorCaches();

	#if defined(THREADPOOL)
		const uint32_t affinityMask = GetSystemAffinityMask();

		// The cache groups from GetProcessorCaches() are sorted in order of largest first. Find the first group that
		// has a logical processor that will be used to pin the main/worker threads.
		auto preferredCache = std::ranges::find_if(pc.groupCaches
			, [affinityMask](const auto& gc) -> bool { return !!(affinityMask & gc.groupMask); });
		
		std::call_once(preferredMaskDetailsLogFlag, [&](){
			if (preferredCache != pc.groupCaches.end())
				LOG("[Threading] Preferred performance cache mask is: 0x%08x (L3 sized: %dKB)", preferredCache->groupMask, preferredCache->cacheSizes[2]/1024);
			else
				LOG_L(L_WARNING, "[Threading] Failed to find a preferred performance cache mask");
		});

		const uint32_t policy = affinityMask
			& ( (preferredCache != pc.groupCaches.end()) ? preferredCache->groupMask : 0xffffffff );
	#else
		/* Allow any core; keep it a "proper" mask though
		 * since that has less risk of blowing up than 0 or 0xFF..FF */
		cpu_topology::ProcessorMasks pm = springproc::CPUID::GetInstance().GetAvailableProcessorAffinityMask();
		const uint32_t policy = pm.performanceCoreMask | pm.efficiencyCoreMask;
	#endif

		// Choose last logical processor in the list.
		return ( 0x80000000 >> std::countl_zero(policy) );
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

	int GetPerformanceCpuCores() {
		return springproc::CPUID::GetInstance().GetNumPerformanceCores();
	}

	bool HasHyperThreading() {
		return springproc::CPUID::GetInstance().HasHyperThreading();
	}


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
	#ifndef _WIN32
		//alternative: pthread_setname_np(pthread_self(), newname.c_str());
		prctl(PR_SET_NAME, newname.c_str(), 0, 0, 0);
	#else
		// adapted from SDL2 code
		DllLib k32Lib("kernel32.dll");
		DllLib kbaseLib("KernelBase.dll");

		using GetCurrentThreadFuncT = HANDLE WINAPI(VOID);
		using SetThreadDescriptionFuncT = HRESULT WINAPI(HANDLE, PCWSTR);

		auto GetCurrentThreadFunc = k32Lib.FindAddressTyped<GetCurrentThreadFuncT*>("GetCurrentThread");
		auto SetThreadDescriptionFunc = k32Lib.FindAddressTyped<SetThreadDescriptionFuncT*>("SetThreadDescription");
		if (!SetThreadDescriptionFunc)
			SetThreadDescriptionFunc = kbaseLib.FindAddressTyped<SetThreadDescriptionFuncT*>("SetThreadDescription");

		if (GetCurrentThreadFunc && SetThreadDescriptionFunc) {
			std::wstring newnameW(newname.begin(), newname.end());
			SetThreadDescriptionFunc(GetCurrentThreadFunc(), newnameW.c_str());
		}
	#endif
	}

	// NB: no protection against two threads posting at the same time
	const Error* GetThreadErrorC() { return &threadError; }
	      Error* GetThreadErrorM() { return &threadError; }
}

