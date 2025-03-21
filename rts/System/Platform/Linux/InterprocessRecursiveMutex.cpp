#include "InterprocessRecursiveMutex.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <system_error>

#include "System/Platform/Misc.h"
#include "System/Misc/SpringTime.h"

InterprocessRecursiveMutex::InterprocessRecursiveMutex(const char* name_) noexcept(false)
	: name(name_)
	, shmFd(-1)
	, shData(nullptr)
{
	shmFd = shm_open(name.c_str(), O_CREAT | O_RDWR, 0660);
	if (shmFd == -1) {
		const auto errNum = errno;
		throw std::system_error(errNum, std::generic_category(), Platform::GetLastErrorAsString(errNum));
	}

	if (ftruncate(shmFd, sizeof(shared_data)) == -1) {
		const auto errNum = errno;
		throw std::system_error(errNum, std::generic_category(), Platform::GetLastErrorAsString(errNum));
	}

	// Map the shared memory segment
	shData = reinterpret_cast<shared_data*>(mmap(nullptr, sizeof(shared_data), PROT_READ | PROT_WRITE, MAP_SHARED, shmFd, 0));
	if (shData == MAP_FAILED) {
		const auto errNum = errno;
		throw std::system_error(errNum, std::generic_category(), Platform::GetLastErrorAsString(errNum));
	}

	pthread_mutexattr_t mutex_attr;
	if (pthread_mutexattr_init(&mutex_attr) != 0) {
		const auto errNum = errno;
		throw std::system_error(errNum, std::generic_category(), Platform::GetLastErrorAsString(errNum));
	}

	// Set the mutex as recursive
	if (pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE) != 0) {
		const auto errNum = errno;
		throw std::system_error(errNum, std::generic_category(), Platform::GetLastErrorAsString(errNum));
	}

	// Set the mutex as process-shared
	if (pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED) != 0) {
		const auto errNum = errno;
		throw std::system_error(errNum, std::generic_category(), Platform::GetLastErrorAsString(errNum));
	}

	// Set the mutex as robust
	if (pthread_mutexattr_setrobust(&mutex_attr, PTHREAD_MUTEX_ROBUST) != 0) {
		const auto errNum = errno;
		throw std::system_error(errNum, std::generic_category(), Platform::GetLastErrorAsString(errNum));
	}

	// Initialize the mutex
	if (pthread_mutex_init(&shData->mtx, &mutex_attr) != 0) {
		const auto errNum = errno;
		throw std::system_error(errNum, std::generic_category(), Platform::GetLastErrorAsString(errNum));
	}

	// Destroy the mutex attributes
	pthread_mutexattr_destroy(&mutex_attr);
}

InterprocessRecursiveMutex::~InterprocessRecursiveMutex() noexcept(false)
{
	munmap(shData, sizeof(shared_data));
	close(shmFd);
	shm_unlink(name.c_str());
}

bool InterprocessRecursiveMutex::TryLockImpl(uint32_t timeoutMs) noexcept
{
	auto start = spring_now();
	do {
		auto lockResult = pthread_mutex_trylock(&shData->mtx);
		switch (lockResult)
		{
		case 0:
			return true;
		case EOWNERDEAD: {
			// The mutex is still acquired despite being abandoned
			// Mark as acquired
			pthread_mutex_consistent(&shData->mtx);
			return true;
		}
		case EBUSY:
			break; // of switch, not the loop
		default:
			return false;
		}
		spring_msecs(100).sleep(true);
	} while (static_cast<uint32_t>((spring_now() - start).toMilliSecsi()) < timeoutMs);

	return false;
}


void InterprocessRecursiveMutex::lock()
{
	if (!TryLockImpl(0xFFFFFFFF)) {
		const auto errNum = errno;
		throw std::system_error(errNum, std::generic_category(), Platform::GetLastErrorAsString(errNum));
	}
}

bool InterprocessRecursiveMutex::try_lock() noexcept
{
	return TryLockImpl(0);
}

void InterprocessRecursiveMutex::unlock()
{
	if (pthread_mutex_unlock(&shData->mtx) != 0) {
		const auto errNum = errno;
		throw std::system_error(errNum, std::generic_category(), Platform::GetLastErrorAsString(errNum));
	}
}