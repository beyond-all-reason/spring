#pragma once

#include <cstdint>
#include <tuple>
#include <optional>
#include <vector>
#include <algorithm>

#include "System/creg/creg_cond.h"
#include "System/Misc/TracyDefs.h"

template<typename ValueT>
class UpdateListTemplate {
	CR_DECLARE_STRUCT(UpdateListTemplate)
public:
	using ConstIterator = typename std::vector<ValueT>::const_iterator;
	using ConstIteratorPair = std::pair<ConstIterator, ConstIterator>;

	using Iterator = typename std::vector<ValueT>::iterator;
	using IteratorPair = std::pair<Iterator, Iterator>;
public:
	UpdateListTemplate()
		: updateList()
		, changed(false)
	{}
	UpdateListTemplate(size_t initialSize)
		: updateList(initialSize)
		, changed(initialSize > 0)
	{}

	size_t Size() const { return updateList.size(); }
	size_t Capacity() const { return updateList.capacity(); }
	bool Empty() const { return updateList.empty(); }

	void Trim(size_t newLessThanOrEqualSize);
	void Resize(size_t newSize) { updateList.resize(newSize); SetNeedUpdateAll(); }
	void Reserve(size_t reservedSize) { updateList.reserve(reservedSize); }
	void Clear() { *this = std::move(UpdateListTemplate<ValueT>()); }

	void SetUpdate(size_t first, size_t count);
	void SetUpdate(const IteratorPair& it);
	void SetUpdate(size_t offset);

	void SetNeedUpdateAll();
	void ResetNeedUpdateAll();

	void EmplaceBackUpdate();
	void PopBack();
	void PopBack(size_t N);

	bool NeedUpdate() const { return changed; }

	std::optional<ConstIteratorPair> GetNext(const std::optional<ConstIteratorPair>& prev = std::nullopt) const;
	std::pair<size_t, size_t> GetOffsetAndSize(const ConstIteratorPair& it) const;
private:
	static constexpr ValueT TypedTrue  = ValueT{ 1 };
	static constexpr ValueT TypedFalse = ValueT{ 0 };
	std::vector<ValueT> updateList;
	bool changed;
};

template<typename ValueT>
inline void UpdateListTemplate<ValueT>::SetNeedUpdateAll()
{
	RECOIL_DETAILED_TRACY_ZONE;
	std::fill(updateList.begin(), updateList.end(), TypedTrue);
	changed = true;
}

template<typename ValueT>
inline void UpdateListTemplate<ValueT>::ResetNeedUpdateAll()
{
	RECOIL_DETAILED_TRACY_ZONE;
	std::fill(updateList.begin(), updateList.end(), TypedFalse);
	changed = false;
}

template<typename ValueT>
inline void UpdateListTemplate<ValueT>::Trim(size_t newLessThanOrEqualSize)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(newLessThanOrEqualSize <= updateList.size());
	updateList.resize(newLessThanOrEqualSize);
	// no need to modify the update status
}

template<typename ValueT>
inline void UpdateListTemplate<ValueT>::SetUpdate(size_t first, size_t count)
{
	RECOIL_DETAILED_TRACY_ZONE;

	auto beg = updateList.begin() + first;
	auto end = beg + count;

	SetUpdate(IteratorPair(beg, end));
}

template<typename ValueT>
inline void UpdateListTemplate<ValueT>::SetUpdate(const IteratorPair& it)
{
	RECOIL_DETAILED_TRACY_ZONE;
	std::fill(it.first, it.second, TypedTrue);
	changed = true;
}

template<typename ValueT>
inline void UpdateListTemplate<ValueT>::SetUpdate(size_t offset)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(offset < updateList.size());
	updateList[offset] = TypedTrue;
	changed = true;
}

template<typename ValueT>
inline void UpdateListTemplate<ValueT>::EmplaceBackUpdate()
{
	RECOIL_DETAILED_TRACY_ZONE;
	updateList.emplace_back(TypedTrue);
	changed = true;
}

template<typename ValueT>
inline void UpdateListTemplate<ValueT>::PopBack()
{
	updateList.pop_back();
	changed = true;
}

template<typename ValueT>
inline void UpdateListTemplate<ValueT>::PopBack(size_t N)
{
	while (N-- >= 0)
		updateList.pop_back();

	changed = true;
}

template<typename ValueT>
inline std::optional<typename UpdateListTemplate<ValueT>::ConstIteratorPair> UpdateListTemplate<ValueT>::GetNext(const std::optional<ConstIteratorPair>& prev) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto beg = prev.has_value() ? prev.value().second : updateList.begin();
	beg = std::find(beg, updateList.end(), TypedTrue);
	auto end = std::find(beg, updateList.end(), TypedFalse);

	if (beg == end)
		return std::nullopt;

	return std::make_optional(std::make_pair(beg, end));
}

template<typename ValueT>
inline std::pair<size_t, size_t> UpdateListTemplate<ValueT>::GetOffsetAndSize(const ConstIteratorPair& it) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return std::make_pair(
		std::distance(updateList.begin(), it.first),
		std::distance(it.first, it.second)
	);
}

// can't use these due to creg limitations
//using UpdateList = UpdateListTemplate<bool>;
//using UpdateListMT = UpdateListTemplate<uint8_t>;

class UpdateList : public UpdateListTemplate<bool> {
	CR_DECLARE_STRUCT(UpdateList)
};
using UpdateListST = UpdateList;
class UpdateListMT : public UpdateListTemplate<uint8_t> {
	CR_DECLARE_STRUCT(UpdateListMT)
};

