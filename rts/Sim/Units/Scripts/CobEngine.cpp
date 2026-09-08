/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "CobEngine.h"

#include "CobDeferredCallin.h"
#include "CobThread.h"
#include "CobFile.h"

#include <cstddef>
#include <cstdint>
#include "System/Misc/TracyDefs.h"
#include "Lua/LuaUI.h"

CR_BIND(CCobEngine, )

CR_REG_METADATA(CCobEngine, (
	CR_MEMBER(threadSlots),
	CR_MEMBER(recycledThreadSlots),
	CR_MEMBER(tickAddedThreads),
	CR_MEMBER(tickRemovedThreads),
	CR_MEMBER(runningThreadIDs),
	CR_MEMBER(sleepingThreadIDs),
	// always null/empty when saving
	CR_IGNORED(waitingThreadIDs),

	CR_IGNORED(curThread),
	CR_IGNORED(deferredCallins),

	CR_MEMBER(currentTime)
))

CR_BIND(CCobEngine::SleepingThread, )
CR_REG_METADATA(CCobEngine::SleepingThread, (
	CR_MEMBER(id),
	CR_MEMBER(wt)
))
CR_BIND(CCobEngine::Slot, )
CR_REG_METADATA(CCobEngine::Slot, (
	CR_MEMBER(generation),
	CR_MEMBER(isOccupied),
	CR_MEMBER(thread)
))

static const char* const numCobThreadsPlot = "CobThreads";

CobThreadID CCobEngine::AllocateThreadID()
{
	size_t slotIndex;
    
	if (recycledThreadSlots.empty()) {
		slotIndex = threadSlots.size();
		threadSlots.emplace_back();
	} else {
		slotIndex = recycledThreadSlots.back();
		recycledThreadSlots.pop_back();
	}
	
	Slot* slot = &threadSlots[slotIndex];
	slot->generation = (slot->generation + 1) & CobThreadID::GEN_MAX;
	slot->isOccupied = true;
	return CobThreadID::Pack(slot->generation, slotIndex);
}

CobThreadID CCobEngine::AddThread(CCobThread&& thread)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!thread.GetID().IsValid()) {
		thread.SetID(AllocateThreadID());
	}
	assert(thread.GetID().IsValid());

	uint32_t generation;
    size_t slotIndex;
    thread.GetID().Unpack(generation, slotIndex);

	Slot* slot = &threadSlots[slotIndex];
	slot->thread = std::move(thread);
	slot->thread.cobInst->AddThreadID(slot->thread.GetID());

	TracyPlot(numCobThreadsPlot, static_cast<int64_t>(threadSlots.size() - recycledThreadSlots.size()));
	return slot->thread.GetID();
}

bool CCobEngine::RemoveThread(CobThreadID threadID) {
	RECOIL_DETAILED_TRACY_ZONE;
	assert(threadID.IsValid());

    size_t slotIndex;
	uint32_t generation;
	threadID.Unpack(generation, slotIndex);
	if unlikely(slotIndex >= threadSlots.size()) {
    	return false;
	}

	Slot* matchingSlot = &threadSlots[slotIndex];
	if (!matchingSlot->isOccupied || matchingSlot->generation != generation) {
		return false;
	}

	std::destroy_at(&matchingSlot->thread);
	std::construct_at(&matchingSlot->thread); // avoids double destruct when the deque tears down at engine shutdown

	matchingSlot->isOccupied = false;
	recycledThreadSlots.push_back(slotIndex);

	TracyPlot(numCobThreadsPlot, static_cast<int64_t>(threadSlots.size() - recycledThreadSlots.size()));
	return true;
}

void CCobEngine::ProcessQueuedThreads() {
	ZoneScoped;

	// Remove threads killed during Tick by other thread (SIGNAL), we do it
	// here as nothing is actively referencing any thread's memory here.
	for (CobThreadID threadID: tickRemovedThreads) {
		RemoveThread(threadID);
	}
	tickRemovedThreads.clear();

	// move new threads spawned by START into threadSlots;
	// their ID's will already have been scheduled into either
	// waitingThreadIDs or sleepingThreadIDs
	for (CCobThread& t: tickAddedThreads) {
		AddThread(std::move(t));
	}

	tickAddedThreads.clear();
}

// a thread wants to continue running at a later time, and adds itself to the scheduler
void CCobEngine::ScheduleThread(const CCobThread* thread)
{
	RECOIL_DETAILED_TRACY_ZONE;
	switch (thread->GetState()) {
		case CCobThread::Run: {
			waitingThreadIDs.push_back(thread->GetID());
		} break;
		case CCobThread::Sleep: {
			sleepingThreadIDs.push(SleepingThread{thread->GetID(), thread->GetWakeTime()});
		} break;
		default: {
			LOG_L(L_ERROR, "[COBEngine::%s] unknown state %d for thread %d", __func__, thread->GetState(), thread->GetID().Raw());
		} break;
	}
}

void CCobEngine::SanityCheckThreads(const CCobInstance* owner)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (false) {
		// no threads belonging to owner should be left
		for (const auto& p: threadSlots) {
			assert(p.thread.cobInst != owner || p.isOccupied == false);
		}
		for (const CCobThread& t: tickAddedThreads) {
			assert(t.cobInst != owner);
		}
	}
}


void CCobEngine::TickThread(CCobThread* thread)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// for error messages originating in CUnitScript
	curThread = thread;

	// NB: threadID is still in <runningThreadIDs> here, TickRunningThreads clears it
	if (thread != nullptr && !thread->Tick())
		RemoveThread(thread->GetID());

	curThread = nullptr;
}

void CCobEngine::WakeSleepingThreads()
{
	ZoneScoped;
	// check on the sleeping threads, remove any whose owner died
	while (!sleepingThreadIDs.empty()) {
		CCobThread* zzzThread = GetThread((sleepingThreadIDs.top()).id);

		if (zzzThread == nullptr) {
			sleepingThreadIDs.pop();
			continue;
		}

		// not yet time to execute this thread or any subsequent sleepers
		if (zzzThread->GetWakeTime() >= currentTime)
			break;

		// remove executing thread from the queue
		sleepingThreadIDs.pop();

		// wake up the thread and tick it (if not dead)
		// this can quite possibly re-add the thread to <sleepingThreadIDs>
		// again, but any thread is guaranteed to sleep for at least 1 tick
		switch (zzzThread->GetState()) {
			case CCobThread::Sleep: {
				zzzThread->SetState(CCobThread::Run);
				TickThread(zzzThread);
			} break;
			case CCobThread::Dead: {
				RemoveThread(zzzThread->GetID());
			} break;
			default: {
				LOG_L(L_ERROR, "[COBEngine::%s] unknown state %d for thread %d", __func__, zzzThread->GetState(), zzzThread->GetID().Raw());
			} break;
		}
	}
}

void CCobEngine::TickRunningThreads()
{
	ZoneScoped;
	// advance all currently running threads
	for (const CobThreadID threadID: runningThreadIDs) {
		TickThread(GetThread(threadID));
	}

	// a thread can never go from running->running, so clear the list
	// note: if preemption was to be added, this would no longer hold
	// however, TA scripts can not run preemptively anyway since there
	// aren't any synchronization methods available
	runningThreadIDs.clear();

	// prepare threads that will run next frame
	std::swap(runningThreadIDs, waitingThreadIDs);
}

void CCobEngine::Tick(int deltaTime)
{
	ZoneScoped;
	currentTime += deltaTime;

	TickRunningThreads();
	ProcessQueuedThreads();

	WakeSleepingThreads();
	ProcessQueuedThreads();
}


void CCobEngine::ShowScriptError(const std::string& msg)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (curThread != nullptr) {
		curThread->ShowError(msg.c_str());
		return;
	}

	LOG_L(L_ERROR, "[COBEngine::%s] \"%s\" outside script execution", __func__, msg.c_str());
}


void CCobEngine::AddDeferredCallin(CCobDeferredCallin&& deferredCallin)
{
	deferredCallins[deferredCallin.funcHash].push_back(deferredCallin);
}


void CCobEngine::RunDeferredCallins()
{
	std::vector<int> funcHashes;
	funcHashes.reserve(deferredCallins.size());
	for(auto& it: deferredCallins)
		funcHashes.push_back(it.first);

	for(auto funcHash: funcHashes) {
		auto it = deferredCallins.find(funcHash); // 'it' has to necessarily be present at this point

		auto callins = std::move(it->second);
		deferredCallins.erase(it);

		const LuaHashString cmdStr = LuaHashString(callins[0].funcName.c_str());
		luaRules->unsyncedLuaHandle.Cob2LuaBatch(cmdStr, callins);
		if (luaUI)
			luaUI->Cob2LuaBatch(cmdStr, callins);
	}
}

