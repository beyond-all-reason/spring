/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ThreadPool.h"
#include "System/Exceptions.h"
#include "System/SpringMath.h"
#include "System/TimeProfiler.h"
#include "System/StringUtil.h"
#ifndef UNIT_TEST
	#include "System/Config/ConfigHandler.h"
#endif
#include "System/Log/ILog.h"
#include "System/Platform/CpuID.h"
#include "System/Platform/Threading.h"
#include "System/Threading/SpringThreading.h"

#include <utility>
#include <functional>
#include <cinttypes>

#define USE_TASK_STATS_TRACKING

#ifndef UNIT_TEST
CONFIG(int, WorkerThreadCount).defaultValue(-1).safemodeValue(0).minimumValue(-1).description("Number of workers (including the main thread!) used by ThreadPool.");
#endif





bool ThreadPool::inMultiThreadedSection;

namespace ThreadPool {
	std::unique_ptr<tf::Executor> executor;
	std::shared_ptr<tf::TFProfObserver> profObserver;

	int GetThreadNum() { return executor->this_worker_id(); }

	static int GetConfigNumWorkers() {
		#ifndef UNIT_TEST
		return configHandler->GetInt("WorkerThreadCount");
		#else
		return -1;
		#endif
	}

	static int GetDefaultNumWorkers() {
		const int maxNumThreads = GetMaxThreads(); // min(MAX_THREADS, logicalCpus)
		const int cfgNumWorkers = GetConfigNumWorkers();

		if (cfgNumWorkers < 0) {
			return Threading::GetPhysicalCpuCores();
		}

		if (cfgNumWorkers > maxNumThreads) {
			LOG_L(L_WARNING, "[ThreadPool::%s] workers set to %i, but there are just %i hardware threads!", __func__, cfgNumWorkers, maxNumThreads);
			return maxNumThreads;
		}

		return cfgNumWorkers;
	}


	// FIXME: mutex/atomic?
	// NOTE: +1 because we also count the main thread, workers start at 1
	int GetNumThreads() { return (executor ? executor->num_workers() : 0) + 1; }
	int GetMaxThreads() { return std::min(MAX_THREADS, Threading::GetLogicalCpuCores()); }

	bool HasThreads() { return (executor ? executor->num_workers() : 0) != 0; }

	static std::uint32_t FindWorkerThreadCore(std::int32_t index, std::uint32_t availCores, std::uint32_t avoidCores)
	{
		// find an unused core for worker-thread <index>
		const auto FindCore = [&index](std::uint32_t targetCores) {
			std::uint32_t workerCore = 1;
			std::int32_t n = index;

			while ((workerCore != 0) && !(workerCore & targetCores))
				workerCore <<= 1;

			// select n'th bit in targetCores
			// counts down because hyper-thread cores are appended to the end
			// and we prefer those for our worker threads (physical cores are
			// preferred for task specific threads)
			while (n--)
				do workerCore <<= 1; while ((workerCore != 0) && !(workerCore & targetCores));

			return workerCore;
		};

		const std::uint32_t threadAvailCore = FindCore(availCores);
		const std::uint32_t threadAvoidCore = FindCore(avoidCores);

		if (threadAvailCore != 0)
			return threadAvailCore;
		// select one of the main-thread cores if no others are available
		if (threadAvoidCore != 0)
			return threadAvoidCore;

		// fallback; use all
		return (~0u);
	}




	void SetThreadCount(int wantedNumThreads)
	{
		const int curNumThreads = GetNumThreads(); // includes main
		const int wtdNumThreads = std::clamp(wantedNumThreads, 0, GetMaxThreads());

		if (curNumThreads != wtdNumThreads) {
			if (profObserver) {
				std::ostringstream ss;
				profObserver->summary(ss);
				profObserver = nullptr;
				executor = nullptr;
			}
			if (wtdNumThreads == 0) //only main
				return;

			executor = std::make_unique<tf::Executor>(wtdNumThreads);
			profObserver = executor->make_observer<tf::TFProfObserver>();
		}
	}

	void SetMaximumThreadCount()
	{
		if (HasThreads()) {
			// NOTE:
			//   do *not* remove, this makes sure the profiler instance
			//   exists before any thread creates a timer that accesses
			//   it on destruction
			#ifndef UNIT_TEST
			CTimeProfiler::GetInstance().ResetState();
			#endif
		}

		if (GetConfigNumWorkers() <= 0)
			return;

		SetThreadCount(GetMaxThreads());
	}

	void SetDefaultThreadCount()
	{
		std::uint32_t systemCores  = springproc::CPUID::GetInstance().GetAvailableProceesorAffinityMask();
		std::uint32_t mainAffinity = systemCores;

		#ifndef UNIT_TEST
		mainAffinity &= configHandler->GetUnsigned("SetCoreAffinity");
		#endif

		std::uint32_t workerAvailCores = systemCores & ~mainAffinity;

		SetThreadCount(GetDefaultNumWorkers());

		{
			// parallel_reduce now folds over shared_ptrs to futures
			// const auto ReduceFunc = [](std::uint32_t a, std::future<std::uint32_t>& b) -> std::uint32_t { return (a | b.get()); };
			const auto ReduceFunc = [](std::uint32_t a, std::uint32_t b) -> std::uint32_t { return (a | b); };
			const auto AffinityFunc = [&]() -> std::uint32_t {
				const int i = ThreadPool::GetThreadNum();

				// -1 is the main thread, skip
				if (i == -1)
					return 0;

				const std::uint32_t workerCore = FindWorkerThreadCore(i, workerAvailCores, mainAffinity);
				// const std::uint32_t workerCore = workerAvailCores;

				Threading::SetThreadName(IntToString(i, "worker%i"));
				Threading::SetAffinityHelper(IntToString(i, "Worker %i").c_str(), workerCore);
				return workerCore;
			};

			const std::uint32_t poolCoreAffinity = parallel_reduce(AffinityFunc, ReduceFunc);
			const std::uint32_t mainCoreAffinity = Threading::HasHyperThreading() ? ~poolCoreAffinity : ~0;

			if (mainAffinity == 0)
				mainAffinity = systemCores;

			Threading::SetAffinityHelper("Main", mainAffinity & mainCoreAffinity);
		}
	}
}