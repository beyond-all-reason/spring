/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <array>
#include "lib/streflop/streflop_cond.h"
#include "System/BranchPrediction.h"
#include "System/creg/creg_cond.h"
#include "System/FastMath.h"

template<typename t> struct type2 {
	CR_DECLARE_STRUCT(type2)

	constexpr type2(): x(t(0)), y(t(0)) {}
	constexpr type2(const t nx, const t ny) : x(nx), y(ny) {}
	template<typename T2> constexpr type2(const T2 v) : x(v.x), y(v.y) {}

	bool operator == (const type2<t>& v) const { return (x == v.x) && (y == v.y); }
	bool operator != (const type2<t>& v) const { return (x != v.x) || (y != v.y); }
	bool operator  < (const type2<t>& f) const { return (y == f.y) ? (x < f.x) : (y < f.y); }

	type2<t> operator - () const { return (type2<t>(-x, -y)); }
	type2<t> operator + (const type2<t>& v) const { return (type2<t>(x + v.x, y + v.y)); }
	type2<t> operator + (const t& i) const        { return (type2<t>(x + i  , y + i  )); }
	type2<t> operator - (const type2<t>& v) const { return (type2<t>(x - v.x, y - v.y)); }
	type2<t> operator - (const t& i) const        { return (type2<t>(x - i  , y - i  )); }
	type2<t> operator / (const type2<t>& v) const { return (type2<t>(x / v.x, y / v.y)); }
	type2<t> operator / (const t& i) const        { return (type2<t>(x / i  , y / i  )); }
	type2<t> operator * (const type2<t>& v) const { return (type2<t>(x * v.x, y * v.y)); }
	type2<t> operator * (const t& i) const        { return (type2<t>(x * i  , y * i  )); }

	type2<t>& operator += (const t& i) { x += i; y += i; return *this; }
	type2<t>& operator += (const type2<t>& v) { x += v.x; y += v.y; return *this; }
	type2<t>& operator -= (const t& i) { x -= i; y -= i; return *this; }
	type2<t>& operator -= (const type2<t>& v) { x -= v.x; y -= v.y; return *this; }
	type2<t>& operator *= (const t& i) { x *= i; y *= i; return *this; }
	type2<t>& operator *= (const type2<t>& v) { x *= v.x; y *= v.y; return *this; }
	type2<t>& operator /= (const t& i) { x /= i; y /= i; return *this; }
	type2<t>& operator /= (const type2<t>& v) { x /= v.x; y /= v.y; return *this; }

	t Norm() const {
		return t(math::sqrt(x*x + y*y));
	}

	float Dot(const type2<t>& f) const {
		return x*f.x + y*f.y;
	}

	t Distance(const type2<t>& f) const {
		const t dx = x - f.x;
		const t dy = y - f.y;
		return t(math::sqrt(dx*dx + dy*dy));
	}

	t DistanceSq(const type2<t>& f) const {
		const t dx = x - f.x;
		const t dy = y - f.y;
		return t(dx*dx + dy*dy);
	}

	type2<t>& SafeNormalize() {
		static constexpr float NRM_EMS = 1e-12f;

		const float sql = x*x + y*y;

		if likely(sql > NRM_EMS) {
			float isqrt = math::isqrt(sql);
			x = t(x * isqrt);
			y = t(y * isqrt);
		}

		return *this;
	}

	union {
		struct {
			t x;
			t y;
		};
		std::array<t, 2> xy;
	};
};

template<typename t> struct itype2 : public type2<t> {
	CR_DECLARE_STRUCT(itype2)

	constexpr itype2() {}
	constexpr itype2(const t nx, const t ny) : type2<t>(nx, ny) {}
	constexpr itype2(const type2<int>& v) : type2<t>(v.x, v.y) {}

	bool operator == (const type2<int>& v) const { return (type2<t>::x == v.x) && (type2<t>::y == v.y); }
	bool operator != (const type2<int>& v) const { return (type2<t>::x != v.x) || (type2<t>::y != v.y); }
	bool operator  < (const type2<int>& f) const { return (type2<t>::y == f.y) ? (type2<t>::x < f.x) : (type2<t>::y < f.y); }

	type2<int> operator + (const type2<int>& v) const { return (type2<int>(type2<t>::x + v.x, type2<t>::y + v.y)); }
	type2<int> operator - (const type2<int>& v) const { return (type2<int>(type2<t>::x - v.x, type2<t>::y - v.y)); }
	type2<int> operator / (const type2<int>& v) const { return (type2<int>(type2<t>::x / v.x, type2<t>::y / v.y)); }
	type2<int> operator / (const int& i) const        { return (type2<int>(type2<t>::x / i  , type2<t>::y / i  )); }
	type2<int> operator * (const type2<int>& v) const { return (type2<int>(type2<t>::x * v.x, type2<t>::y * v.y)); }
	type2<int> operator * (const int& i) const        { return (type2<int>(type2<t>::x * i  , type2<t>::y * i  )); }

	operator type2<int> () const { return (type2<int>(type2<t>::x, type2<t>::y)); }
};

using int2 = type2<int32_t>;
using uint2 = type2<uint32_t>;
using float2 = type2<float>;
using short2 = type2<int16_t>;
using ushort2 = type2<uint16_t>;