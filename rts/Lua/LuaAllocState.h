/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRING_LUA_ALLOC_STATE_H
#define SPRING_LUA_ALLOC_STATE_H

#include <atomic>

struct SLuaAllocLimit {
	static constexpr size_t MAX_ALLOC_BYTES_DEFAULT = 1536u * (1024u * 1024u);
	static inline size_t MAX_ALLOC_BYTES = MAX_ALLOC_BYTES_DEFAULT;
};

struct SLuaAllocState {
	std::atomic<uint64_t> allocedBytes;
	std::atomic<uint64_t> numLuaAllocs;
	std::atomic<uint64_t> luaAllocTime;
	std::atomic<uint64_t> numLuaStates;
};

#endif
