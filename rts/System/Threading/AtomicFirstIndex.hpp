#pragma once

#include <concepts>
#include <atomic>

#include "../ScopedResource.h"

namespace Recoil {
	template <typename T>
	requires std::unsigned_integral<T>
	class AtomicFirstIndex {
	public:
		using ValueType = T;
	public:
		void SetMaxBitsMask(T maxBitsMask_) { maxBitsMask = maxBitsMask_; }

		auto AcquireScoped() {
			return spring::ScopedResource(
				[this]() { return Acquire(); }(),
				[this](int tidx) { if (tidx >= 0) Release(tidx); }
			);
		}
	private:
		int Acquire() {
			T currMask = busyMask.load(std::memory_order_acquire);
			while (true) {
				// All bits set - no zeros available
				if (currMask == maxBitsMask) {
					assert(false);
					// this is not expected, but continue looping
					// instead of causing crash with returning -1
					//return -1;
				}

				// Find first 0 bit (first set bit in complement)
				const auto pos = std::countr_one(currMask);
				const T mask = T(1) << pos;

				// Attempt atomic compare-and-swap
				const T desiredMask = currMask | mask;

				if (busyMask.compare_exchange_weak(currMask, desiredMask, std::memory_order_acq_rel, std::memory_order_relaxed)) {
					return pos;
				}
				// CAS failed (value changed), retry with new current value
			}
		}
		void Release(int idx) {
			const T resetMask = ~(T(1) << idx);
			busyMask.fetch_and(resetMask, std::memory_order_release);
		}
	private:
		std::atomic<T> busyMask = T(0);
		T maxBitsMask = T(-1);
	};
}