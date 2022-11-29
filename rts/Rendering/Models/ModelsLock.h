#pragma once

#include "System/Threading/WrappedSync.h"

struct CModelsLock {
	inline static spring::WrappedSyncRecursiveMutex lock = {};
};