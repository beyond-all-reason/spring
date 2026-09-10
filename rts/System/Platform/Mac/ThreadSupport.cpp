/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#include "System/Platform/Threading.h"

#include <memory>
#include <pthread.h>

// macOS thread-control support.
//
// The Linux implementation suspends/resumes threads via a SIGUSR1 signal
// handler plus getcontext()/ucontext_t, which has no portable macOS
// equivalent. macOS therefore provides no-op Suspend()/Resume().
//
// Performance-core preference is NOT applied here: it is a QoS hint applied
// in Threading::SetAffinity(), which the main and worker threads already
// reach through SetAffinityHelper().

namespace Threading {

void SetupCurrentThreadControls(std::shared_ptr<ThreadControls>& threadCtls)
{
	threadCtls.reset(new Threading::ThreadControls());
	threadCtls->handle = pthread_self();
}

void ThreadStart(
	std::function<void()> taskFunc,
	std::shared_ptr<ThreadControls>* threadCtls,
	ThreadControls* tempCtls)
{
	if (threadCtls != nullptr)
		SetupCurrentThreadControls(*threadCtls);

	// notify the caller that this thread is running
	{
		std::lock_guard<spring::mutex> lock(tempCtls->mutSuspend);
		tempCtls->condInitialized.notify_one();
	}

	taskFunc();
}

SuspendResult ThreadControls::Suspend()
{
	// no-op on macOS (no signal-based suspend)
	return Threading::THREADERR_NOT_RUNNING;
}

SuspendResult ThreadControls::Resume()
{
	// no-op on macOS
	return Threading::THREADERR_NONE;
}

} // namespace Threading
