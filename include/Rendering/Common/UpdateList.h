#pragma once

#include <tuple>
#include <optional>
#include <vector>
#include "System/creg/creg_cond.h"

class UpdateList {
	CR_DECLARE_STRUCT(UpdateList)
public:
	using ConstIteratorPair = std::pair<std::vector<bool>::const_iterator, std::vector<bool>::const_iterator>;
	using IteratorPair = std::pair<std::vector<bool>::iterator, std::vector<bool>::iterator>;
public:
	UpdateList()
		: updateList()
		, changed(false)
	{}
	UpdateList(size_t initialSize)
		: updateList(initialSize)
		, changed(initialSize > 0)
	{}

	size_t Size() const { return updateList.size(); }
	size_t Capacity() const { return updateList.capacity(); }
	bool Empty() const { return updateList.empty(); }

	void Trim(size_t newLessThanOrEqualSize);
	void Resize(size_t newSize) { updateList.resize(newSize); SetNeedUpdateAll(); }
	void Reserve(size_t reservedSize) { updateList.reserve(reservedSize); }
	void Clear() { *this = std::move(UpdateList()); }

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
	std::vector<bool> updateList;
	bool changed;
};