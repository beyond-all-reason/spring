/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef COMMAND_PARAMS_POOL_H
#define COMMAND_PARAMS_POOL_H

#include <cassert>
#include <algorithm>
#include <vector>

#include "System/creg/creg_cond.h"

/* Commands can normally have up to N parameters inline (where N is something low), for performance reasons.
 * However advanced commands can have an arbitrary number of parameters and sometimes the command needs to
 * be atomic (i.e. can't split into multiple commands with N params each since it would change the meaning).
 *
 * Some examples would be:
 *  -  terraform area defined by lasso-selection at coordinates X1, Z1, X2, Z2, ..., X328, Z328.
 *  -  cast soul link (think wc3 tauren spiritwalker) on units A, B, ..., Z.
 *  -  set targeting priority to unit types A, B, ..., Z in this particular order.
 *
 * The command parameters pool reconciles the need to keep the average command data structure small while
 * still allowing arbitrarily large commands in semi-rare cases. */
template<typename T, size_t N, size_t S> struct TCommandParamsPool {
public:
	const T* GetPtr(unsigned int i, unsigned int j     ) const { assert(i < pages.size()); return (pages[i].data( ) + j); }
	//    T* GetPtr(unsigned int i, unsigned int j     )       { assert(i < pages.size()); return (pages[i].data( ) + j); }
	      T  Get   (unsigned int i, unsigned int j     ) const { assert(i < pages.size()); return (pages[i].at  (j)    ); }
	      T  Set   (unsigned int i, unsigned int j, T v)       { assert(i < pages.size()); return (pages[i].at  (j) = v); }

	size_t Push(unsigned int i, T v) {
		assert(i < pages.size());
		pages[i].push_back(v);
		return (pages[i].size());
	}

	void ReleasePage(unsigned int i) {
		assert(i < pages.size());
		indcs.push_back(i);
	}

	unsigned int AcquirePage() {
		if (indcs.empty()) {
			const size_t numIndices = indcs.size();

			pages.resize(std::max(N, pages.size() << 1));
			indcs.resize(std::max(N, indcs.size() << 1));

			const auto beg = indcs.begin() + numIndices;
			const auto end = indcs.end();

			// generate new indices
			std::for_each(beg, end, [&](const unsigned int& i) { indcs[&i - &indcs[0]] = &i - &indcs[0]; });
		}

		const unsigned int pageIndex = indcs.back();

		pages[pageIndex].clear();
		pages[pageIndex].reserve(S);

		indcs.pop_back();
		return pageIndex;
	}

private:
	std::vector< std::vector<T> > pages;
	std::vector<unsigned int> indcs;
};

typedef TCommandParamsPool<float, 256, 32> CommandParamsPool;


extern CommandParamsPool cmdParamsPool;

#endif

