#pragma once

// Linux version

#include <string>
#include <cstdint>
#include <atomic>
#include <pthread.h>

class InterprocessRecursiveMutex
{
public:
	using native_handle_type = pthread_mutex_t*;

	InterprocessRecursiveMutex(const char* name)  noexcept(false);
	~InterprocessRecursiveMutex() noexcept(false);

	InterprocessRecursiveMutex(const InterprocessRecursiveMutex&) = delete;
	InterprocessRecursiveMutex& operator=(const InterprocessRecursiveMutex&) = delete;

	void lock();
	bool try_lock() noexcept;
	void unlock();

	native_handle_type native_handle() { return &shData->mtx; }
private:
	bool TryLockImpl(uint32_t timeoutMs) noexcept;
protected:
	struct shared_data {
		pthread_mutex_t mtx;
		std::atomic_uint32_t refCount;
	};
	const std::string name;
	int shmFd;
	shared_data* shData;
};