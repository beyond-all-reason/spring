#pragma once

#include <mutex>

#include "System/Threading/SpringThreading.h"
#include "System/Threading/WrappedSync.h"
#include "System/ScopedResource.h"
#include "Rendering/GlobalRendering.h"

class CLoadLockMtx {
public:
	using native_handle_type = uint32_t;

	void lock() {
		mtx.lock();
		globalRendering->MakeCurrentContext(false); //set
	}
	void unlock() {
		globalRendering->MakeCurrentContext(true ); //clear
		mtx.unlock();
	}
	bool try_lock() { assert(false); return true; } // placeholder
	native_handle_type native_handle() { return native_handle_type{}; } // placeholder
private:
	inline static std::recursive_mutex mtx = {};
};

class CLoadLockImpl : public spring::WrappedSync<CLoadLockMtx> {
public:
	auto& GetMutex() {
		return *sync[needThreadSafety];
	}
};

class CLoadLock {
public:
	static auto& GetMutex() {
		return loadLockImpl.GetMutex();
	}
	static auto GetUniqueLock() {
		return loadLockImpl.GetUniqueLock();
	}
	static void SetThreadSafety(bool b) { loadLockImpl.SetThreadSafety(b); }
	static bool GetThreadSafety() { return loadLockImpl.GetThreadSafety(); }
private:
	inline static CLoadLockImpl loadLockImpl;
};