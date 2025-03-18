#pragma once

#include <array>
#include <mutex>

#include "SpringThreading.h"

namespace spring {
	template<typename Mutex>
	class WrappedSync {
	public:
		WrappedSync() {
			sync = {
				std::make_unique<spring::mutex_wrapper<spring::noop_mutex>>(),
				std::make_unique<spring::mutex_wrapper<             Mutex>>()
			};
		}
		void SetThreadSafety(bool b) { needThreadSafety = b;    }
		bool GetThreadSafety() const { return needThreadSafety; }

		void Lock()   { sync[needThreadSafety]->lock();   }
		void Unlock() { sync[needThreadSafety]->unlock(); }

		std::scoped_lock<spring::mutex_wrapper_concept> GetScopedLock() { return std::scoped_lock(*sync[needThreadSafety]); }
		std::unique_lock<spring::mutex_wrapper_concept> GetUniqueLock() { return std::unique_lock(*sync[needThreadSafety]); }
	protected:
		std::array<std::unique_ptr<spring::mutex_wrapper_concept>, 2> sync;
		bool needThreadSafety = true;
	};

	using WrappedSyncStdMutex       = WrappedSync<std::mutex>;
	using WrappedSyncSpringMutex    = WrappedSync<spring::mutex>;
	using WrappedSyncRecursiveMutex = WrappedSync<spring::recursive_mutex>;
	using WrappedSyncSpinLock       = WrappedSync<spring::spinlock>;
}