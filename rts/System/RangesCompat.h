/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef RANGES_COMPAT_H
#define RANGES_COMPAT_H

#include <version>
#include <iterator>
#include <ranges>
#include <utility>

namespace spring::views {

#ifdef __cpp_lib_ranges_enumerate

using std::views::enumerate;

#else

template<typename Rng> auto enumerate(Rng& rng)
{
	struct Iterator {
		decltype(std::begin(rng)) it;
		std::ptrdiff_t idx;

		// the element half stays a reference, copying it would turn a
		// mutating loop into a no-op
		auto operator * () const { return std::pair<std::ptrdiff_t, decltype(*it)>(idx, *it); }
		Iterator& operator ++ () { ++it; ++idx; return *this; }
		bool operator != (const Iterator& o) const { return it != o.it; }
	};

	struct View {
		Rng& rng;
		Iterator begin() const { return {std::begin(rng), 0}; }
		Iterator end  () const { return {std::end  (rng), 0}; }
	};

	return View{rng};
}

// the view holds a reference, so a temporary would dangle
template<typename Rng> void enumerate(Rng&&) = delete;

#endif

}

#endif // RANGES_COMPAT_H
