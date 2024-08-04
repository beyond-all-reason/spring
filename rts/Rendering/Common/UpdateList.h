#pragma once

#include <tuple>
#include <optional>
#include <vector>
#include "System/creg/creg.h"

class UpdateList {
	CR_DECLARE_STRUCT(UpdateList)
public:
	using IteratorPair = std::pair<std::vector<bool>::iterator, std::vector<bool>::iterator>;
public:
	UpdateList()
		: updateList()
		, changed(true)
	{}

	size_t Size() const { return updateList.size(); }
	size_t Capacity() const { return updateList.capacity(); }

	void Resize(size_t newSize) { updateList.resize(newSize); SetNeedUpdateAll(); }
	void Reserve(size_t reservedSize) { updateList.reserve(reservedSize); }

	void SetUpdate(const IteratorPair& it);
	void SetUpdate(size_t offset);

	void SetNeedUpdateAll();
	void ResetNeedUpdateAll();

	void EmplaceBackUpdate();
	void PopBack();

	bool NeedUpdate() const { return changed; }

	std::optional<IteratorPair> GetNext(const std::optional<IteratorPair>& prev = std::nullopt);
	std::pair<size_t, size_t> GetOffsetAndSize(const IteratorPair& it);
private:
	std::vector<bool> updateList;
	bool changed;
};