#pragma once

// Windows version

#include <string>
#include <cstdint>


class InterprocessRecursiveMutex
{
public:
	using native_handle_type = void*;

	InterprocessRecursiveMutex(const char* name)  noexcept(false);
	~InterprocessRecursiveMutex() noexcept(false);

	InterprocessRecursiveMutex(const InterprocessRecursiveMutex&) = delete;
	InterprocessRecursiveMutex& operator=(const InterprocessRecursiveMutex&) = delete;

	void lock();
	bool try_lock() noexcept;
	void unlock();

	native_handle_type native_handle() { return mtx; }
private:
	bool TryLockImpl(uint32_t timeoutMs) noexcept;
protected:
	native_handle_type mtx;
};