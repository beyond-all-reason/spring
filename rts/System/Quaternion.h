/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <cmath>
#include <tuple>

#include "System/float3.h"
#include "System/float4.h"
#include "System/Matrix44f.h"

class CQuaternion
{
public:
	CR_DECLARE_STRUCT(CQuaternion)
public:
	constexpr CQuaternion() : q() {}
	constexpr CQuaternion(const float4& q) : q(q) {}
	constexpr CQuaternion(float real, const float3& imag) : q{ imag, real } {}
	constexpr CQuaternion(float x, float y, float z, float s) : q(x, y, z, s) {}

	CQuaternion(const CQuaternion& q) { *this = q; };
	CQuaternion(CQuaternion&& q) noexcept { *this = std::move(q); }
public:
	static CQuaternion MakeFrom(const float3& euler);
	static CQuaternion MakeFrom(float angle, const float3& axis);
	static CQuaternion MakeFrom(const float3& v1, const float3& v2);
	static CQuaternion MakeFrom(const CMatrix44f& mat);

	static std::tuple<float3, CQuaternion, float3> DecomposeIntoTRS(const CMatrix44f& mat);
public:
	bool Normalized() const { return q.Normalized(); };
	CQuaternion& Normalize();
	constexpr CQuaternion& Conjugate() { q.x = -q.x; q.y = -q.y; q.z = -q.z; return *this; };
	CQuaternion& Inverse();

	std::tuple<float, float3> ToRotation() const;
	CMatrix44f ToRotMatrix() const;
public:
	CQuaternion& operator= (const CQuaternion&) = default;
	CQuaternion& operator= (CQuaternion&&) = default;

	constexpr CQuaternion operator-() const { return CQuaternion(-q); };

	CQuaternion operator*(float a) const { return CQuaternion(q * a); };
	CQuaternion operator/(float a) const { return CQuaternion(q / a); };

	CQuaternion operator+(const CQuaternion& rhs) const {
		return CQuaternion(q + rhs.q);
	}
	CQuaternion operator-(const CQuaternion& rhs) const {
		return CQuaternion(q - rhs.q);
	}

	CQuaternion operator*(const CQuaternion& rhs) const;

	bool operator==(CQuaternion& rhs) const { return q == rhs.q; } //aproximate
	bool operator!=(CQuaternion& rhs) const { return q != rhs.q; } //aproximate
private:
	float dot(const CQuaternion& rhs) const { return q.dot(rhs.q); }
	float SqNorm() const;
public:
	static CQuaternion Lerp (const CQuaternion& q1, const CQuaternion& q2, const float a);
	static CQuaternion SLerp(const CQuaternion& q1, const CQuaternion& q2, const float a);
public:
	float4 q;
};