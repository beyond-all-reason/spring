#include "UpdateList.h"
#include "System/Misc/TracyDefs.h"

#include <algorithm>

CR_BIND(UpdateList, )
CR_REG_METADATA(UpdateList, (
	CR_MEMBER(updateList),
	CR_MEMBER(changed)
))


void UpdateList::SetNeedUpdateAll()
{
	RECOIL_DETAILED_TRACY_ZONE;
	std::fill(updateList.begin(), updateList.end(), true);
	changed = true;
}

void UpdateList::ResetNeedUpdateAll()
{
	RECOIL_DETAILED_TRACY_ZONE;
	std::fill(updateList.begin(), updateList.end(), false);
	changed = false;
}

void UpdateList::SetUpdate(const UpdateList::IteratorPair& it)
{
	RECOIL_DETAILED_TRACY_ZONE;
	std::fill(it.first, it.second, true);
	changed = true;
}

void UpdateList::SetUpdate(size_t offset)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(offset < updateList.size());
	updateList[offset] = true;
	changed = true;
}

void UpdateList::EmplaceBackUpdate()
{
	RECOIL_DETAILED_TRACY_ZONE;
	updateList.emplace_back(true);
	changed = true;
}

void UpdateList::PopBack()
{
	updateList.pop_back();
	changed = true;
}

void UpdateList::PopBack(size_t N)
{
	while (N-- >= 0)
		updateList.pop_back();

	changed = true;
}

std::optional<UpdateList::IteratorPair> UpdateList::GetNext(const std::optional<UpdateList::IteratorPair>& prev)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto beg = prev.has_value() ? prev.value().second : updateList.begin();
	     beg = std::find(beg, updateList.end(),  true);
	auto end = std::find(beg, updateList.end(), false);

	if (beg == end)
		return std::nullopt;

	return std::make_optional(std::make_pair(beg, end));
}

std::pair<size_t, size_t> UpdateList::GetOffsetAndSize(const UpdateList::IteratorPair& it)
{
	RECOIL_DETAILED_TRACY_ZONE;
	return std::make_pair(
		std::distance(updateList.begin(), it.first ),
		std::distance(it.first          , it.second)
	);
}
