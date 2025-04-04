/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CPP11COMPAT_H
#define CPP11COMPAT_H

namespace spring {
// https://en.cppreference.com/w/cpp/algorithm/random_shuffle
template<class RandomIt, class RandomFunc> void random_shuffle(RandomIt first, RandomIt last, RandomFunc&& r)
{
	typename std::iterator_traits<RandomIt>::difference_type i, n;
	n = last - first;
	for (i = n - 1; i > 0; --i) {
		std::swap(first[i], first[r(i + 1)]);
	}
}
}; // namespace spring

#endif // CPP11COMPAT_H
