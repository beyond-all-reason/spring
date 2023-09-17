/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "System/TimeProfiler.h"
#include "System/Platform/Threading.h"
#include "System/Threading/SpringThreading.h"

#include "taskflow/taskflow.hpp"
#include "taskflow/algorithm/for_each.hpp"

#include  <array>
#include <vector>
#include <functional>
#include <numeric>
#include <atomic>
#include <future>
#include <memory>
#include <tuple>

#ifdef UNITSYNC
	#undef SCOPED_MT_TIMER
	#define SCOPED_MT_TIMER(x)
#endif

namespace ThreadPool {
	extern std::unique_ptr<tf::Executor> executor;
	extern std::shared_ptr<tf::TFProfObserver> profObserver;

	template<typename Func, typename ... Args>
	auto Enqueue(Func&& func, Args&&... args) {
#if 0
		auto BoundFunc = [func = std::forward<Func>(func), at = std::forward_as_tuple(args...)]() {
			std::apply(func, at);
		};
#else
		auto BoundFunc = [func = std::forward<Func>(func), ... args = std::forward<Args>(args)]() mutable {
			func(args...);
		};
#endif
		return executor->async(BoundFunc);
	}
	void SetMaximumThreadCount();
	void SetDefaultThreadCount();
	void SetThreadCount(int num);
	int GetThreadNum();
	bool HasThreads();
	int GetMaxThreads();
	int GetNumThreads();



	extern bool inMultiThreadedSection;

	static constexpr int MAX_THREADS = 256;
}

template <typename F>
static inline void for_mt(int start, int end, int step, F&& f)
{
	ThreadPool::inMultiThreadedSection = true;

	if (!ThreadPool::HasThreads() || ((end - start) < step)) {
		for (int i = start; i < end; i += step) {
			f(i);
		}
	}
	else {
		SCOPED_MT_TIMER("ThreadPool::AddTask");

		tf::Taskflow taskflow;
		taskflow.for_each_index(start, end, step, f);
		ThreadPool::executor->run(taskflow).wait();
	}

	ThreadPool::inMultiThreadedSection = false;
}

template <typename F>
static inline void for_mt(int start, int end, F&& f)
{
	for_mt(start, end, 1, f);
}

template <typename F>
static inline void for_mt_chunk(int b, int e, F&& f, int chunkOrMinChinkSize = 0)
{
	const int numElems = e - b;
	if (numElems <= 0)
		return;

	static const int maxThreads = ThreadPool::GetNumThreads();

	int chunkSize = chunkOrMinChinkSize;
	if (chunkOrMinChinkSize <= 0) {
		chunkSize = numElems / maxThreads + (numElems % maxThreads != 0); //split the work evenly. Does for_mt() do the same?
		chunkSize = std::max(chunkSize, -chunkOrMinChinkSize);
	}
	chunkSize = std::max(chunkSize, 1);

	const int numChunks = numElems / chunkSize + (numElems % chunkSize != 0);

	if (numChunks == 1) {
		for (int i = b; i < e; ++i)
			f(i);

		return;
	}

	const int chunksPerThread = numChunks / maxThreads + (numChunks % maxThreads != 0);
	const int numThreads = std::min(numChunks, maxThreads);

	for_mt(0, numThreads, 1, [&f, b, e, chunkSize, chunksPerThread](const int jobId) {
		const int bb = b + jobId * chunksPerThread * chunkSize;
		const int ee = std::min(bb + chunksPerThread * chunkSize, e);

		for (int i = bb; i < ee; ++i)
			std::forward<F>(f)(i);
	});
}

template<class Generate, class Reduce>
static inline auto parallel_reduce(Generate&& generate, Reduce&& reduce) -> std::invoke_result_t<Generate> {
	using RetType = std::invoke_result_t<Generate>;
	std::array<RetType, ThreadPool::MAX_THREADS> results;

	// first job in a reduction usually wants to run on the main thread
	results[0] = generate();

	auto GenResult = [&results, generate = std::forward<Generate>(generate)](size_t idx) {
		results[idx] = generate();
	};

	const auto NT = ThreadPool::GetNumThreads();
	tf::Taskflow taskflow;
	taskflow.for_each_index(1, NT, 1, GenResult, tf::StaticPartitioner());
	ThreadPool::executor->run(taskflow).wait();

	return std::accumulate(results.begin(), results.begin() + NT, 0, reduce);
}