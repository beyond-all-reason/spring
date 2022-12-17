#pragma once

#include "System/Threading/WrappedSync.h"

class CModelsLock {
public:
	static auto GetScopedLock() { return lock.GetScopedLock(); }
	static auto GetUniqueLock() { return lock.GetUniqueLock(); }
	static void SetThreadSafety(bool b) { lock.SetThreadSafety(b); }
private:
	inline static spring::WrappedSyncRecursiveMutex lock = {};
};