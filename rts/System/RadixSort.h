/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <vector>

// Sorts elems ascending by an unsigned integer key, using a least-significant
// digit radix sort: linear, input-independent cost, no comparison predicate.
// keyOf maps an element to its key; pack composite orderings into one integer
// (see e.g. the alpha-particle sort in ProjectileDrawer for a float-distance
// packing example). Key bytes that are identical across all elements cost one
// counting pass and no scatter, so sparse keys sort proportionally faster.
//
// The radix path is stable (equal keys keep their input order). Arrays below
// smallNThreshold fall back to std::sort on the key, which is faster at that
// size but not stable; both paths are deterministic for identical input.
//
// scratch is a caller-owned ping-pong buffer, kept as a parameter so repeated
// sorts (e.g. once per frame) can reuse its capacity.
template<typename T, typename KeyFunc>
inline void RadixSortByKey(std::vector<T>& elems, std::vector<T>& scratch, KeyFunc keyOf, size_t smallNThreshold = 1024)
{
	using KeyType = std::decay_t<std::invoke_result_t<KeyFunc, const T&>>;
	static_assert(std::is_unsigned_v<KeyType>, "radix sort key must be an unsigned integer type");

	const size_t n = elems.size();

	if (n < 2)
		return;

	if (n < smallNThreshold) {
		std::sort(elems.begin(), elems.end(), [&keyOf](const T& a, const T& b) { return keyOf(a) < keyOf(b); });
		return;
	}

	scratch.resize(n);

	T* src = elems.data();
	T* dst = scratch.data();

	for (uint32_t shift = 0; shift < sizeof(KeyType) * 8; shift += 8) {
		size_t bucketOffsets[256] = { 0 };

		for (size_t i = 0; i < n; ++i)
			bucketOffsets[(keyOf(src[i]) >> shift) & 0xFF] += 1;

		// this byte is identical across all keys; nothing to reorder
		if (std::any_of(std::begin(bucketOffsets), std::end(bucketOffsets), [n](size_t c) { return c == n; }))
			continue;

		// exclusive prefix sum: counts -> output offsets
		size_t sum = 0;
		for (size_t b = 0; b < 256; ++b) {
			const size_t count = bucketOffsets[b];
			bucketOffsets[b] = sum;
			sum += count;
		}

		for (size_t i = 0; i < n; ++i)
			dst[bucketOffsets[(keyOf(src[i]) >> shift) & 0xFF]++] = src[i];

		std::swap(src, dst);
	}

	if (src != elems.data())
		std::copy(src, src + n, elems.data());
}
