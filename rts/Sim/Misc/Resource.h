/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _RESOURCE_H
#define _RESOURCE_H

#include <string>
#include <iterator>
#include "System/creg/creg_cond.h"

#include <immintrin.h>

struct SResourcePack {
	static constexpr int MAX_RESOURCES = 4;

	union {
		float res[MAX_RESOURCES];
		struct { float metal, energy; };
		struct { float res1, res2, res3, res4; };
	};

public:
	SResourcePack() {
		if constexpr (MAX_RESOURCES == 4) {
			_mm_storeu_ps(&res[0], _mm_setzero_ps());
		}
		else {
			for (int i = 0; i < MAX_RESOURCES; ++i)
				res[i] = 0.0f;
		}
	}
	SResourcePack(float v) {
		for (int i = 0; i < MAX_RESOURCES; ++i)
			res[i] = v;
	}
	SResourcePack(float m, float e) : metal(m), energy(e) {
		for (int i = 2; i < MAX_RESOURCES; ++i)
			res[i] = 0.0f;
	}

	template<typename T = __m128>
	SResourcePack(__m128 packed) {
		static_assert(MAX_RESOURCES == 4);
		_mm_storeu_ps(&res[0], packed);
	}

	CR_DECLARE_STRUCT(SResourcePack)

	bool empty() const {
		if constexpr (MAX_RESOURCES == 4) {
			__m128 v1 = _mm_loadu_ps(&res[0]);
			return (_mm_movemask_ps(_mm_cmpneq_ps(v1, _mm_setzero_ps())) == 0);
		}
		else {
			for (int i = 0; i < MAX_RESOURCES; ++i) {
				if (res[i] != 0.0f) return false;
			}
			return true;
		}
	}

	decltype(std::begin(res)) begin() { return std::begin(res); }
	decltype(std::end(res)) end() { return std::end(res); }

	float& operator[](const size_t i) {
		return res[i];
	}
	const float& operator[](const size_t i) const {
		return res[i];
	}

	bool operator==(const SResourcePack& other) const {
		if constexpr (MAX_RESOURCES == 4) {
			__m128 v1 = _mm_loadu_ps(&res[0]);
			__m128 v2 = _mm_loadu_ps(&other.res[0]);
			return (_mm_movemask_ps(_mm_cmpneq_ps(v1, v2)) == 0);
		}
		else {
			for (int i = 0; i < MAX_RESOURCES; ++i) {
				if (res[i] != other.res[i]) return false;
			}
			return true;
		}
	}
	bool operator<=(const SResourcePack& other) const {
		if constexpr (MAX_RESOURCES == 4) {
			__m128 v1 = _mm_loadu_ps(&res[0]);
			__m128 v2 = _mm_loadu_ps(&other.res[0]);
			return (_mm_movemask_ps(_mm_cmpgt_ps(v1, v2)) == 0);
		}
		else {
			for (int i = 0; i < MAX_RESOURCES; ++i) {
				if (res[i] > other.res[i]) return false;
			}
			return true;
		}
	}
	bool operator>=(const SResourcePack& other) const {
		if constexpr (MAX_RESOURCES == 4) {
			__m128 v1 = _mm_loadu_ps(&res[0]);
			__m128 v2 = _mm_loadu_ps(&other.res[0]);
			return (_mm_movemask_ps(_mm_cmplt_ps(v1, v2)) == 0);
		}
		else {
			for (int i = 0; i < MAX_RESOURCES; ++i) {
				if (res[i] < other.res[i]) return false;
			}
			return true;
		}
	}

	SResourcePack operator+(const SResourcePack& other) const {
		if constexpr (MAX_RESOURCES == 4) {
			__m128 v1 = _mm_loadu_ps(&res[0]);
			__m128 v2 = _mm_loadu_ps(&other.res[0]);
			       v1 = _mm_add_ps(v1, v2);
			return SResourcePack(v1);
		}
		else {
			SResourcePack out = *this;
			for (int i = 0; i < MAX_RESOURCES; ++i)
				out[i] += other.res[i];
			return out;
		}
	}

	SResourcePack operator-(const SResourcePack& other) const {
		if constexpr (MAX_RESOURCES == 4) {
			__m128 v1 = _mm_loadu_ps(&res[0]);
			__m128 v2 = _mm_loadu_ps(&other.res[0]);
			       v1 = _mm_sub_ps(v1, v2);
			return SResourcePack(v1);
		}
		else {
			SResourcePack out = *this;
			for (int i = 0; i < MAX_RESOURCES; ++i)
				out[i] -= other.res[i];
			return out;
		}
	}

	SResourcePack operator*(const SResourcePack& other) const {
		if constexpr (MAX_RESOURCES == 4) {
			__m128 v1 = _mm_loadu_ps(&res[0]);
			__m128 v2 = _mm_loadu_ps(&other.res[0]);
			       v1 = _mm_mul_ps(v1, v2);
			return SResourcePack(v1);
		}
		else {
			SResourcePack out = *this;
			for (int i = 0; i < MAX_RESOURCES; ++i)
				out[i] *= other.res[i];
			return out;
		}
	}

	SResourcePack operator/(const SResourcePack& other) const {
		if constexpr (MAX_RESOURCES == 4) {
			__m128 v1 = _mm_loadu_ps(&res[0]);
			__m128 v2 = _mm_loadu_ps(&other.res[0]);
			       v1 = _mm_div_ps(v1, v2);
			return SResourcePack(v1);
		}
		else {
			SResourcePack out = *this;
			for (int i = 0; i < MAX_RESOURCES; ++i)
				out[i] /= other.res[i];
			return out;
		}
	}

	SResourcePack operator+(float value) const {
		if constexpr (MAX_RESOURCES == 4) {
			__m128 v1 = _mm_loadu_ps(&res[0]);
			       v1 = _mm_add_ps(v1, _mm_set1_ps(value));
			return SResourcePack(v1);
		}
		else {
			SResourcePack out = *this;
			for (int i = 0; i < MAX_RESOURCES; ++i)
				out[i] += value;
			return out;
		}	
	}

	SResourcePack operator*(float scale) const {
		if constexpr (MAX_RESOURCES == 4) {
			__m128 v1 = _mm_loadu_ps(&res[0]);
			       v1 = _mm_mul_ps(v1, _mm_set1_ps(scale));
			return SResourcePack(v1);
		}
		else {
			SResourcePack out = *this;
			for (int i = 0; i < MAX_RESOURCES; ++i)
				out[i] *= scale;
			return out;
		}
	}

	SResourcePack operator-() const {
		if constexpr (MAX_RESOURCES == 4) {
			__m128 v1 = _mm_loadu_ps(&res[0]);
			       v1 = _mm_sub_ps(_mm_setzero_ps(), v1);
			return SResourcePack(v1);
		}
		else {
			SResourcePack out = *this;
			for (int i = 0; i < MAX_RESOURCES; ++i)
				out[i] = -out[i];
			return out;
		}
	}

	SResourcePack& operator+=(const SResourcePack& other) {
		if constexpr (MAX_RESOURCES == 4) {
			__m128 v1 = _mm_loadu_ps(&res[0]);
			__m128 v2 = _mm_loadu_ps(&other.res[0]);
			       v1 = _mm_add_ps(v1, v2);
			_mm_storeu_ps(&res[0], v1);
		}
		else {
			for (int i = 0; i < MAX_RESOURCES; ++i)
				res[i] += other.res[i];
		}
		return *this;
	}
	SResourcePack& operator-=(const SResourcePack& other) {
		if constexpr (MAX_RESOURCES == 4) {
			__m128 v1 = _mm_loadu_ps(&res[0]);
			__m128 v2 = _mm_loadu_ps(&other.res[0]);
			       v1 = _mm_sub_ps(v1, v2);
			_mm_storeu_ps(&res[0], v1);
		}
		else {
			for (int i = 0; i < MAX_RESOURCES; ++i)
				res[i] -= other.res[i];
		}
		return *this;
	}

	SResourcePack& operator*=(float scale) {
		if constexpr (MAX_RESOURCES == 4) {
			__m128 v1 = _mm_loadu_ps(&res[0]);
			       v1 = _mm_mul_ps(v1, _mm_set1_ps(scale));
			_mm_storeu_ps(&res[0], v1);
		}
		else {
			for (int i = 0; i < MAX_RESOURCES; ++i)
				res[i] *= scale;
		}
		return *this;
	}

	SResourcePack& RemoveNegativeValues() {
		//static __m128 all1s = _mm_set_ps1(1.0f);

		if constexpr (MAX_RESOURCES == 4) {
			__m128 v1 = _mm_loadu_ps(&res[0]);
			// with _mm_cmpgt_ps(x, [0.f, 0.f, 0.f, 0.f]):
			// packed values > 0 == FFFFFFFF, so we 'and' it against (1.0f) to convert
			// the unusable FFFFFFFF to 1.0f, which we can use as a multiplier to keep
			// values we want to preserve. Values multiplied against 0.0f will be reset.
			__m128 v2 = _mm_and_ps(_mm_cmpgt_ps(v1, _mm_setzero_ps()), _mm_set_ps1(1.0f));
				   v1 = _mm_mul_ps(v1, v2);
			_mm_storeu_ps(&res[0], v1);
		}
		else {
			for (int i = 0; i<SResourcePack::MAX_RESOURCES; ++i)
				res[i] *= (res[i] > 0.f);
		}
		return *this;
	}

	static SResourcePack min(const SResourcePack& lv, const SResourcePack& rv) {
		if constexpr (MAX_RESOURCES == 4) {
			__m128 v1 = _mm_loadu_ps(&lv.res[0]);
			__m128 v2 = _mm_loadu_ps(&rv.res[0]);
			       v1 = _mm_min_ps(v1, v2);
			return SResourcePack(v1);
		}
		else {
			SResourcePack out = lv;
			for (int i = 0; i < MAX_RESOURCES; ++i)
				out[i] = std::min(out.res[i], rv.res[i]);
			return out;
		}
	}
};


struct SResourceOrder {
	SResourcePack use;
	SResourcePack add;

	//! allow to split a resource when storage is empty/full?
	bool quantum;

	//! allow excessing when storages are full (only matters when quantum := true)
	bool overflow;

	//! handle resources separate, i.e. when metal storage is full still allow energy one to be filled?
	bool separate;

public:
	SResourceOrder() : quantum(false), overflow(false), separate(false) {}
	CR_DECLARE_STRUCT(SResourceOrder)
};


class CResourceDescription
{
public:
	CR_DECLARE_STRUCT(CResourceDescription)

	CResourceDescription();
	~CResourceDescription();

	/// The name of this resource, eg. "Energy" or "Metal"
	std::string name;
	std::string description;

	/// The optimum value for this resource, eg. 0 for "Waste" or FLT_MAX for "Metal"
	float optimum;

	/// The default extractor radius for the resource map, 0.0f if non applicable
	float extractorRadius;

	/// What value 255 in the resource map is worth
	float maxWorth;
};


#endif // _RESOURCE_H
