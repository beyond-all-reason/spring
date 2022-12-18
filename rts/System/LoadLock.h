#pragma once

#include <mutex>

#include "System/Threading/SpringThreading.h"
#include "System/ScopedResource.h"
#include "Rendering/GlobalRendering.h"

class CLoadLockImplUnsafe {
public:
	static void Lock() {}
	static void Unlock() {}
};

class CLoadLockImplSafe {
public:
	static void Lock() {
		lock = std::unique_lock(mtx);
		globalRendering->MakeCurrentContext(false); //set
	}
	static void Unlock() {
		globalRendering->MakeCurrentContext(true ); //clear		
		lock = {};
	}
private:
	inline static std::recursive_mutex mtx = {};
	inline static std::unique_lock<decltype(mtx)> lock = {};
};

class CLoadLock {
public:
	static auto GetScopedLock() {
		return spring::ScopedNullResource([]() { locks[threadSafety](); }, []() { unlocks[threadSafety](); });
	}
	static void SetThreadSafety(bool b) { threadSafety = b; }
private:
	inline static std::array<void(*)(), 2>   locks = { CLoadLockImplUnsafe::Lock  , CLoadLockImplSafe::Lock   };
	inline static std::array<void(*)(), 2> unlocks = { CLoadLockImplUnsafe::Unlock, CLoadLockImplSafe::Unlock };
	inline static bool threadSafety = false;
};