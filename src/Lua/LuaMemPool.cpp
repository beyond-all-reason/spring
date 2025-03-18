/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <algorithm> // std::min
#include <cstdint> // std::uint8_t
#include <cstring> // std::mem{cpy,set}
#include <new>

#include "LuaMemPool.h"
#include "System/MainDefines.h"
#include "System/SafeUtil.h"
#include "System/Log/ILog.h"
#include "System/Threading/SpringThreading.h"
#include "lib/fmt/printf.h"

#include "System/Misc/TracyDefs.h"

// global, affects all pool instances
bool LuaMemPool::enabled = false;

static LuaMemPool* gSharedPool = nullptr;

static std::array<uint8_t, sizeof(LuaMemPool)> gSharedPoolMem;
static std::vector<LuaMemPool*> gPools;
static std::vector<size_t> gIndcs;
static std::atomic<size_t> gCount = {0};
static spring::mutex gMutex;


size_t LuaMemPool::GetPoolCount() { return (gCount.load()); }

LuaMemPool* LuaMemPool::GetSharedPtr() { return gSharedPool; }
LuaMemPool* LuaMemPool::AcquirePtr(bool shared, bool owned)
{
	LuaMemPool* p = GetSharedPtr();

	if (!shared) {
		// caller can be any thread; cf LuaParser context-data ctors
		// (the shared pool must *not* be used by different threads)
		gMutex.lock();

		if (gIndcs.empty()) {
			gPools.push_back(p = new LuaMemPool(gPools.size()));
		} else {
			p = gPools[gIndcs.back()];
			gIndcs.pop_back();
		}

		gMutex.unlock();
	}

	// wipe statistics and blocks if we are the first to request p
	if ((p->GetSharedCount() += shared) <= 1) {
		p->Clear();
	}

	// track the number of active state-owned pools (for /debug)
	gCount += owned;
	return p;
}

void LuaMemPool::ReleasePtr(LuaMemPool* p, const CLuaHandle* o)
{
	RECOIL_DETAILED_TRACY_ZONE;
	gCount -= (o != nullptr);

	if (p == GetSharedPtr()) {
		p->GetSharedCount() -= 1;
		return;
	}

	gMutex.lock();
	gIndcs.push_back(p->GetGlobalIndex());
	gMutex.unlock();
}

void LuaMemPool::FreeShared() { gSharedPool->Clear(); }
void LuaMemPool::InitStatic(bool enable) { gSharedPool = new (gSharedPoolMem.data()) LuaMemPool(LuaMemPool::enabled = enable); }
void LuaMemPool::KillStatic()
{
	RECOIL_DETAILED_TRACY_ZONE;
	for (LuaMemPool*& p: gPools) {
		spring::SafeDelete(p);
	}

	gPools.clear();
	gIndcs.clear();

	spring::SafeDestruct(gSharedPool);
}



LuaMemPool::LuaMemPool(bool isEnabled): LuaMemPool(size_t(-1)) { assert(isEnabled == LuaMemPool::enabled); }
LuaMemPool::LuaMemPool(size_t lmpIndex): globalIndex(lmpIndex)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!LuaMemPool::enabled)
		return;

	luaMemPoolImpl = std::make_unique<LuaMemPool::LuaMemPoolImpl>();
}

void LuaMemPool::Clear()
{
	RECOIL_DETAILED_TRACY_ZONE;
	//allocStats = {};
}

void* LuaMemPool::Alloc(size_t size)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!LuaMemPool::enabled) {
		allocStats[STAT_NAE] += 1 * (size > 0);
		allocStats[STAT_NBE] += size;
		auto t0 = spring_now();
		void* ptr = ::operator new(size);
		allocStats[STAT_NTE] += (spring_now() - t0).toMicroSecsi();
		return ptr;
	}

	auto t0 = spring_now();
	auto* ptr = luaMemPoolImpl->allocMem(size);
	
	if (size > NUM_BUCKETS * BUCKET_STEP) {
		allocStats[STAT_NAE] += 1 * (size > 0);
		allocStats[STAT_NBE] += size;
		allocStats[STAT_NTE] += (spring_now() - t0).toMicroSecsi();
	} else if (luaMemPoolImpl->isAllocInternal(ptr)) {
		allocStats[STAT_NAI] += 1 * (size > 0);
		allocStats[STAT_NBI] += size;
		allocStats[STAT_NTI] += (spring_now() - t0).toMicroSecsi();
	} else {
		allocStats[STAT_NAF] += 1 * (size > 0);
		allocStats[STAT_NBF] += size;
		allocStats[STAT_NTF] += (spring_now() - t0).toMicroSecsi();
	}

	return ptr;
}

void* LuaMemPool::Realloc(void* ptr, size_t nsize, size_t osize)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (ptr == nullptr || osize == 0)
		return Alloc(nsize);

	if (!LuaMemPool::enabled) {
		void* newPtr = ::operator new(nsize);

		if (newPtr == nullptr)
			return nullptr;

		allocStats[STAT_NBE] -= osize;
		allocStats[STAT_NBE] += nsize;

		auto t0 = spring_now();
		std::memcpy(newPtr, ptr, std::min(nsize, osize));
		std::memset(ptr, 0, osize);
		::operator delete(ptr);
		allocStats[STAT_NTE] += (spring_now() - t0).toMicroSecsi();

		return newPtr;
	}

	auto t0 = spring_now();
	auto* ret = luaMemPoolImpl->reAllocMem(ptr, nsize);
	if (nsize > NUM_BUCKETS * BUCKET_STEP) {
		allocStats[STAT_NAE] += 1 * (nsize > 0);
		allocStats[STAT_NBE] += nsize;
		allocStats[STAT_NTE] += (spring_now() - t0).toMicroSecsi();
	}
	else if (luaMemPoolImpl->isAllocInternal(ret)) {
		allocStats[STAT_NAI] += 1 * (nsize > 0);
		allocStats[STAT_NBI] += nsize;
		allocStats[STAT_NTI] += (spring_now() - t0).toMicroSecsi();
	}
	else {
		allocStats[STAT_NAF] += 1 * (nsize > 0);
		allocStats[STAT_NBF] += nsize;
		allocStats[STAT_NTF] += (spring_now() - t0).toMicroSecsi();
	}
	return ret;
}

void LuaMemPool::Free(void* ptr, size_t size)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!LuaMemPool::enabled) {
		::operator delete(ptr);
		return;
	}

	luaMemPoolImpl->freeMem(ptr);
}

void LuaMemPool::LogStats(const char* handle, const char* lctype)
{
	RECOIL_DETAILED_TRACY_ZONE;
	static constexpr auto one = uint64_t(1);
	const float intPerc = 100.0f * static_cast<float>(allocStats[STAT_NAI]) / static_cast<float>(std::max(allocStats[STAT_NAI] + allocStats[STAT_NAF] + allocStats[STAT_NAE], one));
	const float avgAllocTimeI = static_cast<float>(allocStats[STAT_NTI]) / static_cast<float>(std::max(allocStats[STAT_NAI], one));
	const float avgAllocTimeF = static_cast<float>(allocStats[STAT_NTF]) / static_cast<float>(std::max(allocStats[STAT_NAF], one));
	const float avgAllocTimeE = static_cast<float>(allocStats[STAT_NTE]) / static_cast<float>(std::max(allocStats[STAT_NAE], one));
	std::string msg = fmt::sprintf(
		"[LuaMemPool::%s][handle=%s (%s)] index=%u numAllocs{int+, int-, ext, int_p}={%u, %u, %u, %.1f} allocedSize{int+, int-, ext}={%u, %u, %u}, avgAllocTime{int+, int-, ext}={%.4f, %.4f, %.4f}, cumAllocTime={int+, int-, ext}={%u, %u, %u}",
		__func__,
		handle,
		lctype,
		globalIndex,
		allocStats[STAT_NAI],
		allocStats[STAT_NAF],
		allocStats[STAT_NAE],
		intPerc,
		allocStats[STAT_NBI],
		allocStats[STAT_NBF],
		allocStats[STAT_NBE],
		avgAllocTimeI,
		avgAllocTimeF,
		avgAllocTimeE,
		allocStats[STAT_NTI],
		allocStats[STAT_NTF],
		allocStats[STAT_NTE]
	);
	LOG("%s", msg.c_str());
	allocStats = {};
}
