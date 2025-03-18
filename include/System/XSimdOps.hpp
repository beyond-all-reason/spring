#pragma once

#include "xsimd/xsimd.hpp"

// Binary SIMD operators
struct MinOp
{
	auto operator()(float x, float y) { return std::min(x, y); }
	template<typename SimdType>
	auto operator()(SimdType&& x, SimdType&& y) { return xsimd::min(std::forward<SimdType>(x), std::forward<SimdType>(y)); }
};
struct MaxOp
{
	auto operator()(float x, float y) { return std::max(x, y); }
	template<typename SimdType>
	auto operator()(SimdType&& x, SimdType&& y) { return xsimd::max(std::forward<SimdType>(x), std::forward<SimdType>(y)); }
};
struct PlusOp
{
	template <class X, class Y>
	auto operator()(X&& x, Y&& y) -> decltype(x + y) { return x + y; }
};