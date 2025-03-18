/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CONTAINER_UTIL_H
#define CONTAINER_UTIL_H

#include <algorithm>
#include <cassert>
#include <vector>

#include <version>
#ifdef __cpp_lib_flat_set
	#define VUS [[deprecated("VectorUniqueSorted/VectorSorted can be replaced with std::flat_set/flat_multiset")]]
#else
	#define VUS
#endif

namespace spring {
	template<typename ForwardIt, typename T, typename Compare = std::less <>>
	ForwardIt BinarySearch(ForwardIt first, ForwardIt last, const T& value, Compare comp = {})
	{
		first = std::lower_bound(first, last, value, comp);
		return (!(first == last) && !(comp(value, *first))) ? first : last;
	}

	template<typename T, typename UnaryPredicate>
	static bool VectorEraseIf(std::vector<T>& v, UnaryPredicate p)
	{
		auto it = std::find_if(v.begin(), v.end(), p);

		if (it == v.end())
			return false;

		*it = std::move(v.back());
		v.pop_back();
		return true;
	}

	template<typename T>
	static bool VectorErase(std::vector<T>& v, const T& e)
	{
		auto it = std::find(v.begin(), v.end(), e);

		if (it == v.end())
			return false;

		*it = std::move(v.back());
		v.pop_back();
		return true;
	}

	template<typename T, typename C>
	VUS static bool VectorEraseUniqueSorted(std::vector<T>& v, const T& e, const C& c)
	{
		const auto iter = std::lower_bound(v.begin(), v.end(), e, c);

		if ((iter == v.end()) || (*iter != e))
			return false;

		v.erase(iter);
		return true;
	}

	/* Removes globally and doesn't necessarily preserve order,
	 * e.g. AABBCCAA -> ACB
	 * This is unlike `std::unique` which works "locally",
	 * e.g. AABBCCAA -> ABCA */
	template<typename T, typename UniqPred = std::equal_to <>>
	VUS static void VectorUnique(std::vector<T>& v, UniqPred uniqPred = {}) {
		for (size_t i = 0; i < v.size(); i++) {
			for (size_t j = i + 1; j < v.size(); /*NOOP*/) {
				if (uniqPred(v[i], v[j])) {
					v[j] = std::move(v.back());
					v.pop_back();
				}
				else {
					j++;
				}
			}
		}
	}

	template<typename T, typename SortPred = std::less <>, typename UniqPred = std::equal_to <>>
	VUS static void VectorSortUnique(std::vector<T>& v, SortPred sortPred = {}, UniqPred uniqPred = {})
	{
		std::sort(v.begin(), v.end(), sortPred);
		auto last = std::unique(v.begin(), v.end(), uniqPred);
		v.erase(last, v.end());
	}

	template<typename T>
	VUS static bool VectorInsertUnique(std::vector<T>& v, const T& e, bool checkIfUnique = false)
	{
		// do not assume uniqueness, test for it
		if (checkIfUnique && std::find(v.begin(), v.end(), e) != v.end())
			return false;

		// assume caller knows best, skip the test
		assert(checkIfUnique || std::find(v.begin(), v.end(), e) == v.end());
		v.push_back(e);
		return true;
	}

	template<typename T, typename Pred>
	VUS typename std::vector<T>::iterator VectorInsertSorted(std::vector<T>& vec, T&& item, Pred pred)
	{
		return vec.insert(std::upper_bound(vec.begin(), vec.end(), item, pred), item);
	}

	template<
		typename T, typename Pred,
		typename = typename std::enable_if_t<std::is_pointer_v<std::remove_cv_t<T>>>
	>
	VUS typename std::vector<T>::iterator VectorInsertSorted(std::vector<T>& vec, T item, Pred pred)
	{
		return vec.insert(std::upper_bound(vec.begin(), vec.end(), item, pred), item);
	}

	template<typename T>
	VUS typename std::vector<T>::iterator VectorInsertSorted(std::vector<T>& vec, T&& item)
	{
		return vec.insert(std::upper_bound(vec.begin(), vec.end(), item), item);
	}

	template<
		typename T,
		typename = typename std::enable_if_t<std::is_pointer_v<std::remove_cv_t<T>>>
	>
	VUS typename std::vector<T>::iterator VectorInsertSorted(std::vector<T>& vec, T item)
	{
		return vec.insert(std::upper_bound(vec.begin(), vec.end(), item), item);
	}

	template<typename T, typename Pred>
	VUS static bool VectorInsertUniqueSorted(std::vector<T>& v, const T& e, Pred pred)
	{
		const auto iter = std::lower_bound(v.begin(), v.end(), e, pred);

		if ((iter != v.end()) && (*iter == e))
			return false;

		v.insert(iter, e);
		return true;
	}

	template<typename T>
	static T VectorBackPop(std::vector<T>& v) { T e = std::move(v.back()); v.pop_back(); return e; }
};

#undef VUS

#endif

