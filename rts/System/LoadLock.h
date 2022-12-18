#pragma once

#include <mutex>

#include "System/Threading/SpringThreading.h"
#include "System/ScopedResource.h"
#include "Rendering/GlobalRendering.h"

class CLoadLock {
public:
	static void Lock() {
		if (!threadSafety)
			return;

		lock = std::unique_lock(mtx);
		globalRendering->MakeCurrentContext(false); //set
	}
	static void Unlock() {
		if (!threadSafety)
			return;

		globalRendering->MakeCurrentContext(true ); //clear		
		lock = {};
	}
	static auto GetScopedLock() {
		return spring::ScopedNullResource([]() { Lock(); }, []() { Unlock(); });
	}
	static void SetThreadSafety(bool b) { threadSafety = b; }
private:
	inline static bool threadSafety = false;
	inline static std::recursive_mutex mtx = {};
	inline static std::unique_lock<decltype(mtx)> lock = {};
};