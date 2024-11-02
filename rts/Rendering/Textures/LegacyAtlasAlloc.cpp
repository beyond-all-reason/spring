/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LegacyAtlasAlloc.h"
#include <algorithm>
#include <vector>
#include <list>
#include <set>
#include <bit>

#include "System/Misc/TracyDefs.h"

inline bool CLegacyAtlasAlloc::CompareTex(const SAtlasEntry* tex1, const SAtlasEntry* tex2)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// sort by large to small

	if (tex1->size.y > tex2->size.y) return true;
	if (tex2->size.y > tex1->size.y) return false;

	if (tex1->size.x > tex2->size.x) return true;
	if (tex2->size.x > tex1->size.x) return false;

	// silly but will help stabilizing the placement on reload
	if (tex1->name > tex2->name) return true;
	if (tex2->name > tex1->name) return false;

	return false;
}


bool CLegacyAtlasAlloc::IncreaseSize()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (atlasSize.y < atlasSize.x) {
		if ((atlasSize.y * 2) <= maxsize.y) {
			atlasSize.y *= 2;
			return true;
		}
		if ((atlasSize.x * 2) <= maxsize.x) {
			atlasSize.x *= 2;
			return true;
		}
	} else {
		if ((atlasSize.x * 2) <= maxsize.x) {
			atlasSize.x *= 2;
			return true;
		}
		if ((atlasSize.y * 2) <= maxsize.y) {
			atlasSize.y *= 2;
			return true;
		}
	}

	return false;
}


bool CLegacyAtlasAlloc::Allocate()
{
	RECOIL_DETAILED_TRACY_ZONE;
	std::vector<SAtlasEntry*> memtextures;
	memtextures.reserve(entries.size());

	std::set<std::string> sortedNames;
	for (auto& entry : entries) {
		sortedNames.insert(entry.first);
	}

	for (auto& name : sortedNames) {
		memtextures.push_back(&entries[name]);
	}

	std::stable_sort(memtextures.begin(), memtextures.end(), CLegacyAtlasAlloc::CompareTex);

	bool success = true;
	bool recalc = false;

	int2 max;
	int2 cur;

	std::list<int2> nextSub;
	std::list<int2> thisSub;

	int padding = 1 << GetNumTexLevels();

	for (int a = 0; a < static_cast<int>(memtextures.size()); ++a) {
		SAtlasEntry* curtex = memtextures[a];

		bool done = false;
		while (!done) {
			if (thisSub.empty()) {
				if (nextSub.empty()) {
					cur.y = max.y;
					max.y += curtex->size.y + padding;

					if (max.y > atlasSize.y) {
						if (IncreaseSize()) {
							nextSub.clear();
							thisSub.clear();
							cur.y = max.y = cur.x = 0;
							recalc = true;
							break;
						}
						else {
							success = false;
							break;
						}
					}
					thisSub.push_back(int2(0, cur.y));
				}
				else {
					thisSub = nextSub;
					nextSub.clear();
				}
			}

			if ((thisSub.front().x + curtex->size.x + padding) > atlasSize.x) {
				thisSub.clear();
				continue;
			}
			if (thisSub.front().y + curtex->size.y > max.y) {
				thisSub.pop_front();
				continue;
			}

			// found space in both dimensions s.t. texture
			// PLUS margin fits within current atlas bounds
			curtex->texCoords.x1 = thisSub.front().x;
			curtex->texCoords.y1 = thisSub.front().y;
			curtex->texCoords.x2 = thisSub.front().x + curtex->size.x - 1;
			curtex->texCoords.y2 = thisSub.front().y + curtex->size.y - 1;

			cur.x = thisSub.front().x + curtex->size.x + padding;
			max.x = std::max(max.x, cur.x);

			done = true;

			if ((thisSub.front().y + curtex->size.y + padding) < max.y) {
				nextSub.push_back(int2(thisSub.front().x + padding, thisSub.front().y + curtex->size.y + padding));
			}

			thisSub.front().x += (curtex->size.x + padding);

			while (thisSub.size() > 1 && thisSub.front().x >= (++thisSub.begin())->x) {
				(++thisSub.begin())->x = thisSub.front().x;
				thisSub.erase(thisSub.begin());
			}
		}

		if (recalc) {
			// reset all existing texcoords
			for (auto it = memtextures.begin(); it != memtextures.end(); ++it) {
				(*it)->texCoords = float4();
			}
			recalc = false;
			a = -1;
			continue;
		}
	}

	atlasSize = max;

	return success;
}

int CLegacyAtlasAlloc::GetNumTexLevels() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return std::min(
		std::bit_width(static_cast<uint32_t>(GetMinDim())),
		numLevels
	);
}