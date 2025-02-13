/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <cstddef>
#include <vector>
#include <memory>

#include "System/MemPoolTypes.h"
#include "System/UnorderedMap.hpp"

class CLuaHandle;
class LuaMemPool {
public:
	explicit LuaMemPool(bool isEnabled);
	explicit LuaMemPool(size_t lmpIndex);

	~LuaMemPool() {
		Clear();

		if (!LuaMemPool::enabled)
			return;

		luaMemPoolImpl = nullptr;
	}

	LuaMemPool(const LuaMemPool& p) = delete;
	LuaMemPool(LuaMemPool&& p) = delete;

	LuaMemPool& operator = (const LuaMemPool& p) = delete;
	LuaMemPool& operator = (LuaMemPool&& p) = delete;

public:
	static size_t GetPoolCount();

	static LuaMemPool* GetSharedPtr();
	static LuaMemPool* AcquirePtr(bool shared, bool owned);
	static void ReleasePtr(LuaMemPool* p, const CLuaHandle* o);

	static void FreeShared();
	static void InitStatic(bool enable);
	static void KillStatic();

public:
	void Clear();
	void* Alloc(size_t size);
	void* Realloc(void* ptr, size_t nsize, size_t osize);
	void Free(void* ptr, size_t size);

	void LogStats(const char* handle, const char* lctype);

	size_t  GetGlobalIndex() const { return globalIndex; }
	size_t  GetSharedCount() const { return sharedCount; }
	size_t& GetSharedCount()       { return sharedCount; }

	void SetThreadSafety(bool b) { assert(luaMemPoolImpl); luaMemPoolImpl->SetThreadSafety(b); }
public:
	static bool enabled;
private:
	static constexpr size_t LUA_MEM_POOL_SIZE_MB = 128;
	static constexpr size_t MAX_INTERNAL_ALLOC_SIZE = 1 << 16;

	using LuaMemPoolImpl = PassThroughPool<LUA_MEM_POOL_SIZE_MB* (1024 * 1024), MAX_INTERNAL_ALLOC_SIZE>;

	std::unique_ptr<LuaMemPoolImpl> luaMemPoolImpl;

	enum {
		STAT_NAI = 0, // number of internal allocs
		STAT_NAF = 1, // number of int fail allocs
		STAT_NAE = 2, // number of external allocs
		STAT_NBI = 3, // number of bytes alloced (internal)
		STAT_NBF = 4, // number of bytes alloced (int fail)
		STAT_NBE = 5, // number of bytes alloced (external)
		STAT_NTI = 6, // cumulative time spent on internal allocs
		STAT_NTF = 7, // cumulative time spent on int fail allocs
		STAT_NTE = 8, // cumulative time spent on external allocs
	};

	std::array<uint64_t, 9> allocStats = { 0, 0, 0, 0, 0 };

	size_t globalIndex = 0;
	size_t sharedCount = 0;
};