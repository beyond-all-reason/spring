/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <cmath>
#include <tuple>

#include "System/float3.h"
#include "System/float4.h"
#include "System/Matrix44f.h"

class alignas(16) CQuaternion
{
public:
	CR_DECLARE_STRUCT(CQuaternion)
public:
	constexpr CQuaternion() = default;
	constexpr CQuaternion(const float4& q)
		: x(q.x)
		, y(q.y)
		, z(q.z)
		, r(q.w)
	{}
	constexpr CQuaternion(const float3& imag, float real)
		: x(imag.x)
		, y(imag.y)
		, z(imag.z)
		, r(real)
	{}
	constexpr CQuaternion(float xi, float yi, float zi, float real)
		: x(xi)
		, y(yi)
		, z(zi)
		, r(real)
	{}
	CQuaternion(const CQuaternion& q) { *this = q; };
	CQuaternion(CQuaternion&& q) noexcept { *this = std::move(q); }
public:
	static CQuaternion MakeFrom(const float3& euler);
	static CQuaternion MakeFrom(float angle, const float3& axis);
	static CQuaternion MakeFrom(const float3& v1, const float3& v2);
	static CQuaternion MakeFrom(const CMatrix44f& mat);

	static std::tuple<float3, CQuaternion, float3> DecomposeIntoTRS(const CMatrix44f& mat);
public:
	bool Normalized() const;
	CQuaternion& Normalize();
	constexpr CQuaternion& Conjugate() { x = -x; y = -y; z = -z; return *this; }
	CQuaternion& Inverse();

	float4 ToAxisAndAngle() const;
	CMatrix44f ToRotMatrix() const;
public:
	CQuaternion& operator= (const CQuaternion&) = default;
	CQuaternion& operator= (CQuaternion&&) = default;

	constexpr CQuaternion operator-() const { return CQuaternion(-x, -y, -z, -r); }

	CQuaternion operator*(float a) const {
		return CQuaternion(x * a, y * a, z * a, r * a);
	}
	CQuaternion operator/(float a) const {
		const float ainv = 1.0f / a;
		return CQuaternion(x * ainv, y * ainv, z * ainv, r * ainv);
	}

	CQuaternion operator+(const CQuaternion& rhs) const {
		return CQuaternion(x + rhs.x, y + rhs.y, z + rhs.z, r + rhs.r);
	}
	CQuaternion operator-(const CQuaternion& rhs) const {
		return CQuaternion(x - rhs.x, y - rhs.y, z - rhs.z, r - rhs.r);
	}

	CQuaternion operator*(const CQuaternion& rhs) const;
	CQuaternion& operator*=(float f);

	bool operator==(CQuaternion& rhs) const; //aproximate
	bool operator!=(CQuaternion& rhs) const { return !operator==(rhs); } //aproximate
private:
	float SqNorm() const;
public:
	static CQuaternion Lerp (const CQuaternion& q1, const CQuaternion& q2, const float a);
	static CQuaternion SLerp(const CQuaternion& q1, const CQuaternion& q2, const float a);
public:
	float x, y, z, r;
};