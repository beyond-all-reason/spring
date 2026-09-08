/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef COB_ENGINE_H
#define COB_ENGINE_H

/*
 * Simple VM responsible for "scheduling" and running COB threads.
 * It also manages reading and caching of the actual .cob files.
 */

#include <vector>
#include <deque>

#include "CobThread.h"
#include "CobThreadID.h"
#include "CobDeferredCallin.h"
#include "System/creg/creg_cond.h"
#include "System/creg/STL_Queue.h"
#include "System/creg/STL_Deque.h"
#include "System/Cpp11Compat.hpp"

class CCobThread;
class CCobInstance;
class CCobFile;
class CCobFileHandler;

class CCobEngine
{
	CR_DECLARE_STRUCT(CCobEngine)

public:
	struct SleepingThread {
		CR_DECLARE_STRUCT(SleepingThread)

		CobThreadID id;
		int wt;
	};

	struct Slot {
		CR_DECLARE_STRUCT(Slot)

		uint32_t generation = 0;
		bool isOccupied = false;

		CCobThread thread;
	};

	struct CCobThreadComp {
	public:
		bool operator() (const SleepingThread& a, const SleepingThread& b) const {
			return a.wt > b.wt || (a.wt == b.wt && a.id > b.id);
		}
	};

public:
	void Init() {
		tickAddedThreads.reserve(128);

		runningThreadIDs.reserve(512);
		waitingThreadIDs.reserve(512);

		sleepingThreadIDs = {};

		curThread = nullptr;

		currentTime = 0;
	}
	void Kill() {
		// threadSlots is never explicitly iterated in the actual code,
		// but iterated during sync dumps, so clear it
		threadSlots.clear();
		recycledThreadSlots.clear();
		spring::clear_unordered_map(deferredCallins);
		tickAddedThreads.clear();

		runningThreadIDs.clear();
		waitingThreadIDs.clear();

		while (!sleepingThreadIDs.empty()) {
			sleepingThreadIDs.pop();
		}
	}

	void Tick(int deltaTime);
	void ShowScriptError(const std::string& msg);


	CCobThread* GetThread(CobThreadID threadID) {
		uint32_t generation;
		size_t slotIndex;
		threadID.Unpack(generation, slotIndex);
		if (slotIndex >= threadSlots.size()) {
			return nullptr;
		}

		Slot* matchingSlot = &threadSlots[slotIndex];
		if (!matchingSlot->isOccupied || matchingSlot->generation != generation) {
			return nullptr;
		}

		return &matchingSlot->thread;
	}

	bool RemoveThread(CobThreadID threadID);
	CobThreadID AddThread(CCobThread&& thread);

	void QueueAddThread(CCobThread&& thread) { tickAddedThreads.emplace_back(std::move(thread)); }
	void QueueRemoveThread(CobThreadID threadID) { tickRemovedThreads.emplace_back(threadID); }
	void ProcessQueuedThreads();

	void ScheduleThread(const CCobThread* thread);
	void SanityCheckThreads(const CCobInstance* owner);

	// HACK: cob threads currently need their id ahead of being stored. A refactor
	//       that relies on the new stability guarantees of the deque/slots means we 
	//       could AddThread instead of separating thread ids from storage.
	// this MUST be followed by an AddThread or a QueueAddThread or slots will leak.
	CobThreadID AllocateThreadID();

	const auto& GetThreadSlots() const { return threadSlots; }
//	const auto& GetTickAddedThreads() const { return tickAddedThreads; }
//	const auto& GetTickRemovedThreads() const { return tickRemovedThreads; }
//	const auto& GetRunningThreadIDs() const { return runningThreadIDs; }
	const auto& GetWaitingThreadIDs() const { return waitingThreadIDs; }
	const auto& GetSleepingThreadIDs() const { return sleepingThreadIDs; }
	const auto  GetCurrTime() const { return currentTime; }

	void AddDeferredCallin(CCobDeferredCallin&& deferredCallin);
	void RunDeferredCallins();
private:
	void TickThread(CCobThread* thread);

	void WakeSleepingThreads();
	void TickRunningThreads();

private:
	// slot pool of live threads across all script instances, indexed by id
	// (this is a perf optimization to reuse instances since theres so much churn)
	std::deque<Slot> threadSlots;
	std::vector<size_t> recycledThreadSlots;
	// threads that are spawned during Tick
	std::vector<CCobThread> tickAddedThreads;
	// threads that are killed during Tick
	std::vector<CobThreadID> tickRemovedThreads;

	std::vector<CobThreadID> runningThreadIDs;
	std::vector<CobThreadID> waitingThreadIDs;

	spring::unordered_map<int, std::vector<CCobDeferredCallin> > deferredCallins;

	// stores <id, waketime> pairs s.t. after waking up the ID can be checked
	// for validity; thread owner might get removed while a thread is sleeping
	std::priority_queue<SleepingThread, std::vector<SleepingThread>, CCobThreadComp> sleepingThreadIDs;

	CCobThread* curThread = nullptr;

	int currentTime = 0;
};


extern CCobEngine* cobEngine;

#endif // COB_ENGINE_H
