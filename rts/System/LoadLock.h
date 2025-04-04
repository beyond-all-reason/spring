#pragma once

#include "Rendering/GlobalRendering.h"
#include "System/ScopedResource.h"
#include "System/Threading/SpringThreading.h"
#include "System/Threading/WrappedSync.h"

#include <mutex>

class CLoadLockMtx {
public:
	using native_handle_type = uint32_t;

	void lock()
	{
		mtx.lock();

		// hack to protect from multiple context acquisions in case of recursive lock
		auto& thisThreadLocksCount = locksCount[Threading::IsGameLoadThread()];
		++thisThreadLocksCount;
		if (thisThreadLocksCount == 1) {
			globalRendering->MakeCurrentContext(false); // set
			globalRendering->ToggleMultisampling();
		}
	}

	void unlock()
	{
		// hack to protect from multiple context clear in case of recursive lock
		auto& thisThreadLocksCount = locksCount[Threading::IsGameLoadThread()];
		--thisThreadLocksCount;

		if (thisThreadLocksCount == 0)
			globalRendering->MakeCurrentContext(true); // clear

		mtx.unlock();
	}

	bool try_lock()
	{
		assert(false);
		return true;
	} // placeholder

	native_handle_type native_handle() { return native_handle_type{}; } // placeholder
private:
	std::array<uint32_t, 2> locksCount = {0};
	std::recursive_mutex mtx = {};
};

class CLoadLockImpl : public spring::WrappedSync<CLoadLockMtx> {
public:
	CLoadLockImpl()
	    : spring::WrappedSync<CLoadLockMtx>()
	{
		needThreadSafety = false;
	}

	auto& GetMutex() { return *sync[needThreadSafety]; }
};

class CLoadLock {
public:
	static auto& GetMutex() { return loadLockImpl.GetMutex(); }

	static auto GetUniqueLock() { return loadLockImpl.GetUniqueLock(); }

	static void SetThreadSafety(bool b) { loadLockImpl.SetThreadSafety(b); }

	static bool GetThreadSafety() { return loadLockImpl.GetThreadSafety(); }

private:
	inline static CLoadLockImpl loadLockImpl;
};
