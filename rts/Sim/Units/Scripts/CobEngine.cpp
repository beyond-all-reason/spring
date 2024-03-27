/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "CobEngine.h"
#include "CobThread.h"
#include "CobFile.h"

#include <cstdint>
#include <tracy/Tracy.hpp>

CR_BIND(CCobEngine, )

CR_REG_METADATA(CCobEngine, (
	CR_MEMBER(threadInstances),
	CR_MEMBER(tickAddedThreads),
	CR_MEMBER(tickRemovedThreads),
	CR_MEMBER(runningThreadIDs),
	CR_MEMBER(sleepingThreadIDs),
	// always null/empty when saving
	CR_IGNORED(waitingThreadIDs),

	CR_IGNORED(curThread),

	CR_MEMBER(currentTime),
	CR_MEMBER(threadCounter)
))

CR_BIND(CCobEngine::SleepingThread, )
CR_REG_METADATA(CCobEngine::SleepingThread, (
	CR_MEMBER(id),
	CR_MEMBER(wt)
))

static const char* const numCobThreadsPlot = "CobThreads";

int CCobEngine::AddThread(CCobThread&& thread)
{
	//ZoneScoped;
	if (thread.GetID() == -1)
		thread.SetID(GenThreadID());

	CCobInstance* o = thread.cobInst;
	CCobThread& t = threadInstances[thread.GetID()];

	// move thread into registry, hand its ID to owner
	t = std::move(thread);
	o->AddThreadID(t.GetID());

	TracyPlot(numCobThreadsPlot, static_cast<int64_t>(threadInstances.size()));

	return (t.GetID());
}

bool CCobEngine::RemoveThread(int threadID) {
	//ZoneScoped;
	const auto it = threadInstances.find(threadID);

	if (it != threadInstances.end()) {
		threadInstances.erase(it);
		TracyPlot(numCobThreadsPlot, static_cast<int64_t>(threadInstances.size()));
		return true;
	}

	return false;
}

void CCobEngine::ProcessQueuedThreads() {
	ZoneScoped;

	// Remove threads killed during Tick by other thread (SIGNAL), we do it
	// here as nothing is actively referencing any thread's memory here.
	for (int threadID: tickRemovedThreads) {
		RemoveThread(threadID);
	}
	tickRemovedThreads.clear();

	// move new threads spawned by START into threadInstances;
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
	//ZoneScoped;
	switch (thread->GetState()) {
		case CCobThread::Run: {
			waitingThreadIDs.push_back(thread->GetID());
		} break;
		case CCobThread::Sleep: {
			sleepingThreadIDs.push(SleepingThread{thread->GetID(), thread->GetWakeTime()});
		} break;
		default: {
			LOG_L(L_ERROR, "[COBEngine::%s] unknown state %d for thread %d", __func__, thread->GetState(), thread->GetID());
		} break;
	}
}

void CCobEngine::SanityCheckThreads(const CCobInstance* owner)
{
	//ZoneScoped;
	if (false) {
		// no threads belonging to owner should be left
		for (const auto& p: threadInstances) {
			assert(p.second.cobInst != owner);
		}
		for (const CCobThread& t: tickAddedThreads) {
			assert(t.cobInst != owner);
		}
	}
}


void CCobEngine::TickThread(CCobThread* thread)
{
	//ZoneScoped;
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
				LOG_L(L_ERROR, "[COBEngine::%s] unknown state %d for thread %d", __func__, zzzThread->GetState(), zzzThread->GetID());
			} break;
		}
	}
}

void CCobEngine::TickRunningThreads()
{
	ZoneScoped;
	// advance all currently running threads
	for (const int threadID: runningThreadIDs) {
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
	//ZoneScoped;
	if (curThread != nullptr) {
		curThread->ShowError(msg.c_str());
		return;
	}

	LOG_L(L_ERROR, "[COBEngine::%s] \"%s\" outside script execution", __func__, msg.c_str());
}

