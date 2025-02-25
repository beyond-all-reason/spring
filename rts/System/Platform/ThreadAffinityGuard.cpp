#include "ThreadAffinityGuard.h"

#include "System/Log/ILog.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <sched.h>
#include <unistd.h>
#include <syscall.h>
#endif

// Constructor: Saves the current thread's affinity
ThreadAffinityGuard::ThreadAffinityGuard() : affinitySaved(false) {
#ifdef _WIN32
	threadHandle = GetCurrentThread();  // Get the current thread handle
	savedAffinity = SetThreadAffinityMask(threadHandle, ~0);
	affinitySaved = ( savedAffinity != 0 );
	if (!affinitySaved) {
		LOG_L(L_WARNING, "GetThreadAffinityMask failed with error code: %lu", GetLastError());
	}
#else
	tid = syscall(SYS_gettid);  // Get thread ID
	CPU_ZERO(&savedAffinity);
	if (sched_getaffinity(tid, sizeof(cpu_set_t), &savedAffinity) == 0) {
		affinitySaved = true;
	} else {
		LOG_L(L_WARNING, "Failed to save thread affinity.");
	}
#endif
}

// Destructor: Restores the saved affinity if it was successfully stored
ThreadAffinityGuard::~ThreadAffinityGuard() {
	if (affinitySaved) {
#ifdef _WIN32
		if (!SetThreadAffinityMask(threadHandle, savedAffinity)) {
			LOG_L(L_WARNING, "SetThreadAffinityMask failed with error code: %lu", GetLastError());
		}
#else
		if (sched_setaffinity(tid, sizeof(cpu_set_t), &savedAffinity) != 0) {
			LOG_L(L_WARNING, "Failed to restore thread affinity.");
		}
#endif
	}
}
