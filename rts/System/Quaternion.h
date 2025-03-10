/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */
#pragma once

#include <cmath>
#include <tuple>

#include "System/float3.h"
#include "System/float4.h"
#include "System/Matrix44f.h"

struct Transform;

class alignas(16) CQuaternion
{
public:
	CR_DECLARE_STRUCT(CQuaternion)
public:
	explicit constexpr CQuaternion()
		: x(0.0f)
		, y(0.0f)
		, z(0.0f)
		, r(1.0f)
	{}
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
	constexpr CQuaternion(const CQuaternion& q) = default;
	constexpr CQuaternion(CQuaternion&& q) noexcept = default;
public:
	// Euler angles, also known as RotateEulerXYZ, produces same rotation as CMatrix44f::RotateEulerYXZ()
	static CQuaternion FromEulerPYRNeg(const float3& angles) { return FromEulerPYR(-angles); };
	// Euler angles, also known as RotateEulerXYZ
	static CQuaternion FromEulerPYR(const float3& angles);

	// Euler angles, also known as RotateEulerYXZ, produces same rotation as CMatrix44f::RotateEulerXYZ()
	static CQuaternion FromEulerYPRNeg(const float3& angles) { return FromEulerYPR(-angles); };
	// Euler angles, also known as RotateEulerYXZ
	static CQuaternion FromEulerYPR(const float3& angles);

	// To Euler angles, YPR/YXZ order
	float3 ToEulerYPR() const;

	// To Euler angles, PYR/XYZ order
	float3 ToEulerPYR() const;

	static CQuaternion MakeFrom(float angle, const float3& axis);
	static CQuaternion MakeFrom(const float3& v1, const float3& v2);
	static CQuaternion MakeFrom(const float3& newFwdDir);
	static CQuaternion MakeFrom(const CMatrix44f& mat);

	static const CQuaternion& AssertNormalized(const CQuaternion& q);
public:
	bool Normalized() const;
	CQuaternion& Normalize();
	CQuaternion& ANormalize();
	constexpr CQuaternion& Conjugate() { x = -x; y = -y; z = -z; return *this; }
	CQuaternion  Inverse() const;
	CQuaternion& InverseInPlace();
	CQuaternion  InverseNormalized() const;
	CQuaternion& InverseInPlaceNormalized();

	float4 ToAxisAndAngle() const;
	CMatrix44f ToRotMatrix() const;

	float3 Rotate(const float3& v) const;
	float4 Rotate(const float4& v) const;

	bool equals(const CQuaternion& rhs) const;
public:
	constexpr CQuaternion& operator= (const CQuaternion&) = default;
	constexpr CQuaternion& operator= (CQuaternion&&) noexcept = default;

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

	float3 operator*(const float3& arg) const {
		return Rotate(arg);
	}
	float4 operator*(const float4& arg) const {
		return Rotate(arg);
	}

	CQuaternion operator*(const CQuaternion& rhs) const;
	CQuaternion& operator*=(float f);
	CQuaternion& operator/=(float f);

	bool operator==(const CQuaternion& rhs) const { return  equals(rhs); } //aproximate
	bool operator!=(const CQuaternion& rhs) const { return !equals(rhs); } //aproximate

	void AssertNaNs() const;
private:
	constexpr float SqNorm() const { return (x * x + y * y + z * z + r * r); }
	static float InvSqrt(float f);
public:
	static CQuaternion Lerp (const CQuaternion& q1, const CQuaternion& q2, const float a);
	static CQuaternion SLerp(const CQuaternion& q1, const CQuaternion& q2, const float a);
public:
	float x, y, z, r;
};