/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SPRING_HASH_H_
#define _SPRING_HASH_H_

#include "lib/xxhash/xxh3.h"
#include <cstdint>
#include <string>
#include <type_traits>
#include <memory>
#include <bit>

namespace spring {
	static inline uint32_t LiteHash(const void* p, unsigned size, uint32_t cs0 = 0) {
		return static_cast<uint32_t>(XXH3_64bits_withSeed(p, static_cast<size_t>(size), static_cast<XXH64_hash_t>(cs0)));
	}

	template<typename T>
	static inline uint32_t LiteHash(const T& p, uint32_t cs0 = 0) { return LiteHash(std::addressof(p), sizeof(T), cs0); }

	template<typename T>
	static inline uint32_t LiteHash(const T* p, uint32_t cs0 = 0) { return LiteHash(p, sizeof(T), cs0); }


	template<typename T>
	struct synced_hash {
		uint32_t operator()(const T& s) const;
	};

	template<>
	struct synced_hash<int64_t> {
	public:
		uint32_t operator()(const int64_t& i) const
		{
			return static_cast<uint32_t>(i) ^ static_cast<uint32_t>(i >> 32);
		}
	};

	template<>
	struct synced_hash<uint64_t> {
	public:
		uint32_t operator()(const uint64_t& i) const
		{
			return static_cast<uint32_t>(i) ^ static_cast<uint32_t>(i >> 32);
		}
	};

	template<>
	struct synced_hash<std::string> {
	public:
		uint32_t operator()(const std::string& s) const
		{
			return LiteHash(s.data(), static_cast<uint32_t>(s.size()), 0);
		}
	};

	template<typename T1, typename T2>
	struct synced_hash<std::pair<T1, T2>> {
	public:
		uint32_t operator()(const std::pair<T1, T2>& p) const
		{
			synced_hash<T1> h1;
			synced_hash<T2> h2;
			return h1(p.first) ^ h2(p.second);
		}
	};

	template<typename ... Ts>
	struct synced_hash<std::tuple<Ts ...>> {
	public:
		uint32_t operator()(const std::tuple<Ts ...>& t) const
		{
			uint32_t result = 0;
			std::apply([&result]<typename ... Tss>(const Tss&... args) { ((result ^= synced_hash<Tss>()(args)), ...); }, t);
			return result;
		}
	};

	// Best effort implementation for types not covered explicitly
	template<typename T>
	inline uint32_t synced_hash<T>::operator()(const T& s) const {
		if constexpr (sizeof(T) <= sizeof(uint32_t)) {
			if constexpr (std::is_integral_v<T>) {
				return static_cast<uint32_t>(s);
			} else {
				static_assert(std::has_unique_object_representations_v<T>, "synced_hash not auto-implemented for this type");
				return std::bit_cast<uint32_t>(s);
			}
		} else {
			if constexpr (std::is_pointer_v<T>) {
				synced_hash<uintptr_t> h;
				return h(std::bit_cast<uintptr_t>(s));
			}
			else {
				static_assert(std::has_unique_object_representations_v<T>, "synced_hash not auto-implemented for this type");
				return LiteHash(s);
			}
		}
	}
}

#endif //_SPRING_HASH_H_
