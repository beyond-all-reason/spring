#include "InterprocessRecursiveMutex.h"

#include <windows.h>
#include <system_error>
#include <fmt/format.h>

#include "System/Platform/Misc.h"
#include "System/Misc/SpringTime.h"

namespace Impl {
	static const auto throw_lasterror = []() {
		DWORD dwErrVal = GetLastError();
		std::error_code ec(dwErrVal, std::system_category());
		throw std::system_error(ec, Platform::GetLastErrorAsString(dwErrVal));
	};
}

InterprocessRecursiveMutex::InterprocessRecursiveMutex(const char* name) noexcept(false)
	: mtx(nullptr)
{
	const std::string extName = fmt::format("Local\\{}", name);
	const std::wstring extNameW(extName.begin(), extName.end());

	// in case the mutex has already been created in the other process
	// CreateMutexW will signal the error, but will still create the handle
	mtx = CreateMutexW(nullptr, FALSE, extNameW.c_str());

	if (mtx == nullptr) {
		Impl::throw_lasterror();
	}

	SetLastError(ERROR_SUCCESS); // clear in case the mutex was already created
}

InterprocessRecursiveMutex::~InterprocessRecursiveMutex() noexcept(false)
{
	if (!CloseHandle(mtx)) {
		Impl::throw_lasterror();
	}
}

void InterprocessRecursiveMutex::lock()
{
	if (!TryLockImpl(INFINITE)) {
		Impl::throw_lasterror();
	}
}

bool InterprocessRecursiveMutex::try_lock() noexcept
{
	return TryLockImpl(0);
}

void InterprocessRecursiveMutex::unlock()
{
	if (!ReleaseMutex(mtx)) {
		Impl::throw_lasterror();
	}
}

bool InterprocessRecursiveMutex::TryLockImpl(uint32_t timeoutMs) noexcept
{
	// const auto dwWaitResult = WaitForSingleObject(mtx, timeoutMs);
	// doesn't react properly if the mutex got abandoned in other app
	// do while loop instead
	
	auto start = spring_now();
	do {
		const auto dwWaitResult = WaitForSingleObject(mtx, 0);

		switch (dwWaitResult)
		{
		case WAIT_OBJECT_0:
			return true;
		case WAIT_ABANDONED:
			// The mutex is still acquired despite being abandoned
			return true;
		case WAIT_TIMEOUT:
			break; // of switch, not the loop
		case WAIT_FAILED:
			return false;
		default:
			return false;
		}
		spring_msecs(100).sleep(true);
	} while (static_cast<uint32_t>((spring_now() - start).toMilliSecsi()) < timeoutMs);

	return false;
}
