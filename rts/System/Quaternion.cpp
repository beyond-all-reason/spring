/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#include "Quaternion.h"
#include "System/SpringMath.h"

//contains some code from
// https://github.com/ilmola/gml/blob/master/include/gml/quaternion.hpp
// https://github.com/ilmola/gml/blob/master/include/gml/mat.hpp
// https://github.com/g-truc/glm/blob/master/glm/ext/quaternion_common.inl
// Also nice source https://www.shadertoy.com/view/fdtfWM

#define QUATERNION_EULER_DEBUG

CR_BIND(CQuaternion, )
CR_REG_METADATA(CQuaternion, (
	CR_MEMBER(x),
	CR_MEMBER(y),
	CR_MEMBER(z),
	CR_MEMBER(r)
))

/// <summary>
/// Quaternion from Euler PYR/XYZ angles
/// </summary>
CQuaternion CQuaternion::FromEulerPYR(const float3& angles)
{
	// CMatrix44f::RotateEulerXYZ defines it as (R=R(Z)*R(Y)*R(X))
	// so R(r)*R(y)*R(p)

	const float sp = math::sin(angles[CMatrix44f::ANGLE_P] * 0.5f);
	const float cp = math::cos(angles[CMatrix44f::ANGLE_P] * 0.5f);
	const float sy = math::sin(angles[CMatrix44f::ANGLE_Y] * 0.5f);
	const float cy = math::cos(angles[CMatrix44f::ANGLE_Y] * 0.5f);
	const float sr = math::sin(angles[CMatrix44f::ANGLE_R] * 0.5f);
	const float cr = math::cos(angles[CMatrix44f::ANGLE_R] * 0.5f);

	CQuaternion pyrQ{
		cr * cy * sp + cp * sr * sy,
		cp * cr * sy - cy * sp * sr,
		cp * cy * sr + cr * sp * sy,
		cp * cr * cy - sp * sr * sy
	};
#ifdef QUATERNION_EULER_DEBUG
	static constexpr auto pAxis = float3(1, 0, 0);
	static constexpr auto yAxis = float3(0, 1, 0);
	static constexpr auto rAxis = float3(0, 0, 1);
	auto pyrQ2 = CQuaternion::MakeFrom(angles[CMatrix44f::ANGLE_P], pAxis) * CQuaternion::MakeFrom(angles[CMatrix44f::ANGLE_Y], yAxis) * CQuaternion::MakeFrom(angles[CMatrix44f::ANGLE_R], rAxis);

	CMatrix44f m; m.RotateEulerXYZ(-angles);
	CQuaternion pyrQ3;
	std::tie(std::ignore, pyrQ3, std::ignore) = DecomposeIntoTRS(m);

	assert(pyrQ.equals(pyrQ2));
	assert(pyrQ.equals(pyrQ3));
#endif
	return AssertNormalized(pyrQ);
}

/// <summary>
/// Quaternion from Euler YPR/YXZ angles
/// </summary>
CQuaternion CQuaternion::FromEulerYPR(const float3& angles)
{
	// CMatrix44f::RotateEulerYXZ defines it as (R=R(Z)*R(X)*R(Y))
	// so R(r)*R(p)*R(y)

	const float sp = math::sin(angles[CMatrix44f::ANGLE_P] * 0.5f);
	const float cp = math::cos(angles[CMatrix44f::ANGLE_P] * 0.5f);
	const float sy = math::sin(angles[CMatrix44f::ANGLE_Y] * 0.5f);
	const float cy = math::cos(angles[CMatrix44f::ANGLE_Y] * 0.5f);
	const float sr = math::sin(angles[CMatrix44f::ANGLE_R] * 0.5f);
	const float cr = math::cos(angles[CMatrix44f::ANGLE_R] * 0.5f);

	CQuaternion yprQ{
		cr* cy* sp + cp * sr * sy,
		cp* cr* sy - cy * sp * sr,
		cp* cy* sr - cr * sp * sy,
		cp* cr* cy + sp * sr * sy
	};
#ifdef QUATERNION_EULER_DEBUG
	static constexpr auto pAxis = float3(1, 0, 0);
	static constexpr auto yAxis = float3(0, 1, 0);
	static constexpr auto rAxis = float3(0, 0, 1);
	auto yprQ2 = CQuaternion::MakeFrom(angles[CMatrix44f::ANGLE_Y], yAxis) * CQuaternion::MakeFrom(angles[CMatrix44f::ANGLE_P], pAxis) * CQuaternion::MakeFrom(angles[CMatrix44f::ANGLE_R], rAxis);

	CMatrix44f m; m.RotateEulerYXZ(-angles);
	CQuaternion yprQ3;
	std::tie(std::ignore, yprQ3, std::ignore) = DecomposeIntoTRS(m);

	assert(yprQ.equals(yprQ2));
	assert(yprQ2.equals(yprQ3));
#endif
	return AssertNormalized(yprQ);
}

/// <summary>
/// Quaternion from rotation angle and axis
/// </summary>
CQuaternion CQuaternion::MakeFrom(float angle, const float3& axis)
{
	assert(axis.Normalized());

	const float a = 0.5f * angle;
	return CQuaternion(axis * math::sin(a), math::cos(a)); //Normalized if axis.Normalized()
}

/// <summary>
/// Quaternion to rotate from v1 to v2
/// Expects v1 and v2 to be already normalized
/// </summary>
CQuaternion CQuaternion::MakeFrom(const float3& v1, const float3& v2)
{
	assert(v1.Normalized());
	assert(v2.Normalized());
#if 0
	if unlikely(v1.same(v2)) {
		return CQuaternion(v1, 0.0f).Normalize();
	}
	else if unlikely(v1.same(-v2)) {
		float3 v;
		if (v1.x > -float3::cmp_eps() && v1.x < float3::cmp_eps())       // if x ~= 0
			v = { 1.0f, 0.0f, 0.0f };
		else if (v1.y > -float3::cmp_eps() && v1.y < float3::cmp_eps())  // if y ~= 0
			v = { 0.0f, 1.0f, 0.0f };
		else                                                             // if z ~= 0
			v = { 0.0f, 0.0f, 1.0f };

		return CQuaternion(v, math::HALFPI).Normalize();
	}
	else {
		float3 v = v1.cross(u2);                         // compute rotation axis
		float angle = math::acosf(v1.dot(v2));	         // rotation angle
		return CQuaternion(v, angle * 0.5f).Normalize(); // half angle
	}
#else
	// https://raw.org/proof/quaternion-from-two-vectors/

	const auto dp = v1.dot(v2);
	if unlikely(epscmp(dp, -1.0f, float3::cmp_eps())) {
		// any perpendicular vector to v1/v2 will suffice
		float3 npVec = v1.PickNonParallel();
		const auto cp = v1.cross(npVec).Normalize();
		return CQuaternion(cp, 0.0f);
	}
	else {
		const auto cp = v1.cross(v2);
		return CQuaternion(cp, 1.0f + dp).Normalize();
	}
#endif
}

/// <summary>
///  Quaternion from a rotation matrix
///  Should only be called on R or T * R matrices
/// </summary>
CQuaternion CQuaternion::MakeFrom(const CMatrix44f& mat)
{
	assert(mat.IsRotOrRotTranMatrix());
	const float trace = mat.md[0][0] + mat.md[1][1] + mat.md[2][2];

	if (trace > 0.0f) {
		const float s = 0.5f * InvSqrt(trace + 1.0f);

		return AssertNormalized(CQuaternion(
			s * (mat.md[1][2] - mat.md[2][1]),
			s * (mat.md[2][0] - mat.md[0][2]),
			s * (mat.md[0][1] - mat.md[1][0]),
			0.25f / s
		));
	}
	else if (mat.md[0][0] > mat.md[1][1] && mat.md[0][0] > mat.md[2][2]) {
		const float s = 2.0f * math::sqrt(1.0f + mat.md[0][0] - mat.md[1][1] - mat.md[2][2]);
		const float invs = 1.0f / s;

		return AssertNormalized(CQuaternion(
			0.25f * s,
			(mat.md[1][0] + mat.md[0][1]) * invs,
			(mat.md[2][0] + mat.md[0][2]) * invs,
			(mat.md[1][2] - mat.md[2][1]) * invs
		));
	}
	else if (mat.md[1][1] > mat.md[2][2]) {
		const float s = 2.0f * math::sqrt(1.0f + mat.md[1][1] - mat.md[0][0] - mat.md[2][2]);
		const float invs = 1.0f / s;

		return AssertNormalized(CQuaternion(
			(mat.md[1][0] + mat.md[0][1]) * invs,
			0.25f * s,
			(mat.md[2][1] + mat.md[1][2]) * invs,
			(mat.md[2][0] - mat.md[0][2]) * invs
		));
	}
	else {
		const float s = 2.0f * math::sqrt(1.0f + mat.md[2][2] - mat.md[0][0] - mat.md[1][1]);
		const float invs = 1.0f / s;

		return AssertNormalized(CQuaternion(
			(mat.md[2][0] + mat.md[0][2]) * invs,
			(mat.md[2][1] + mat.md[1][2]) * invs,
			0.25f * s,
			(mat.md[0][1] - mat.md[1][0]) * invs
		));
	}
}

const CQuaternion& CQuaternion::AssertNormalized(const CQuaternion& q)
{
	assert(q.Normalized());
	return q;
}

/// <summary>
/// Decompose a transformation matrix into translate, rotation (Quaternion), scale components
/// </summary>
std::tuple<float3, CQuaternion, float3>  CQuaternion::DecomposeIntoTRS(const CMatrix44f& mat)
{
	CMatrix44f tmpMat = mat;
	float4& t0 = tmpMat.col[0];
	float4& t1 = tmpMat.col[1];
	float4& t2 = tmpMat.col[2];

	const float4& c0 = mat.col[0];
	const float4& c1 = mat.col[1];
	const float4& c2 = mat.col[2];

	const float d = tmpMat.Det3();
	const float s = Sign(d);

	float3 scaling {s * c0.Length(), c1.Length(), c2.Length()};

	assert(
		!epscmp(scaling[0], 0.0f, float3::cmp_eps()) &&
		!epscmp(scaling[1], 0.0f, float3::cmp_eps()) &&
		!epscmp(scaling[2], 0.0f, float3::cmp_eps())
	);

	t0 /= scaling[0];
	t1 /= scaling[1];
	t2 /= scaling[2];

	assert(tmpMat.IsRotOrRotTranMatrix());

	return std::make_tuple(
		float3(mat.col[3]),             //translate
		CQuaternion::MakeFrom(tmpMat),  //rotate (quat)
		scaling                         //scale
	);
}


bool CQuaternion::Normalized() const
{
	return math::fabs(1.0f - (r * r + x * x + y * y + z * z)) <= float3::cmp_eps();
}

CQuaternion& CQuaternion::Normalize()
{
	const float sqn = SqNorm();
	if unlikely(sqn < float3::nrm_eps())
		return *this;

	*this *= InvSqrt(sqn);

	return *this;
}

/// <summary>
/// Find axis and angle equivalent rotation from a quaternion
/// </summary>
float4 CQuaternion::ToAxisAndAngle() const
{
	return float4(
		float3(x, y, z) * InvSqrt(std::max(0.0f, 1.0f - r * r)),
		2.0f * math::acos(std::clamp(r, -1.0f, 1.0f))
	);
}

/// <summary>
/// Converts a quaternion to rotational matrix
/// </summary>
CMatrix44f CQuaternion::ToRotMatrix() const
{
	const float qxx = x * x;
	const float qyy = y * y;
	const float qzz = z * z;
	const float qxz = x * z;
	const float qxy = x * y;
	const float qyz = y * z;
	const float qrx = r * x;
	const float qry = r * y;
	const float qrz = r * z;

#if 0
	return CMatrix44f(
		1.0f - 2.0f * (qyy + qzz), 2.0f * (qxy - qrz)       , 2.0f * (qxz + qry)       , 0.0f,
		2.0f * (qxy + qrz)       , 1.0f - 2.0f * (qxx + qzz), 2.0f * (qyz - qrx)       , 0.0f,
		2.0f * (qxz - qry)       , 2.0f * (qyz + qrx)       , 1.0f - 2.0f * (qxx + qyy), 0.0f,
		0.0f                     , 0.0f                     , 0.0f                     , 1.0f
	);
#else
	return CMatrix44f(
		1.0f - 2.0f * (qyy + qzz), 2.0f * (qxy + qrz)       , 2.0f * (qxz - qry)       , 0.0f,
		2.0f * (qxy - qrz)       , 1.0f - 2.0f * (qxx + qzz), 2.0f * (qyz + qrx)       , 0.0f,
		2.0f * (qxz + qry)       , 2.0f * (qyz - qrx)       , 1.0f - 2.0f * (qxx + qyy), 0.0f,
		0.0f                     , 0.0f                     , 0.0f                     , 1.0f
	);
#endif
}

float3 CQuaternion::Rotate(const float3& v) const
{
	assert(Normalized());
#if 0
	const auto vRotQ = (*this) * CQuaternion(v, 0.0f) * this->Inverse();
	return float3{ vRotQ.x, vRotQ.y, vRotQ.z };
#else
	const float3 u = float3{ x, y, z };
	return 2.0f * u.dot(v) * u
		+ (r * r - u.dot(u)) * v
		+ 2.0f * r * u.cross(v);
#endif
}

float4 CQuaternion::Rotate(const float4& v) const
{
	return float4{ Rotate(static_cast<float3>(v)), v.w };
}

bool CQuaternion::equals(const CQuaternion& rhs) const
{
	return
		(
			epscmp(x,  rhs.x, float3::cmp_eps()) &&
			epscmp(y,  rhs.y, float3::cmp_eps()) &&
			epscmp(z,  rhs.z, float3::cmp_eps()) &&
			epscmp(r,  rhs.r, float3::cmp_eps())
		) || (
			epscmp(x, -rhs.x, float3::cmp_eps()) &&
			epscmp(y, -rhs.y, float3::cmp_eps()) &&
			epscmp(z, -rhs.z, float3::cmp_eps()) &&
			epscmp(r, -rhs.r, float3::cmp_eps())
		);
}

CQuaternion CQuaternion::Inverse() const
{
	CQuaternion inv = *this;
	inv.InverseInPlace();
	return inv;
}

CQuaternion& CQuaternion::InverseInPlace()
{
	const float sqn = SqNorm();
	if unlikely(sqn < float3::nrm_eps())
		return *this;

	*this = Conjugate() / sqn; // aparently not math::sqrt(sqn)
	return *this;
};

CQuaternion CQuaternion::operator*(const CQuaternion& rhs) const
{
	// *this or rhs can be a vertex from CQuaternion::Rotate(), can't assume either of them is normalized

	std::array<float, 3> crossProduct = {
		(y * rhs.z) - (z * rhs.y),
		(z * rhs.x) - (x * rhs.z),
		(x * rhs.y) - (y * rhs.x)
	};

	return CQuaternion(
		r * rhs.x + rhs.r * x + crossProduct[0],
		r * rhs.y + rhs.r * y + crossProduct[1],
		r * rhs.z + rhs.r * z + crossProduct[2],
		r * rhs.r - (x * rhs.x + y * rhs.y + z * rhs.z)
	);
}

CQuaternion& CQuaternion::operator*=(float f)
{
	x *= f;
	y *= f;
	z *= f;
	r *= f;

	return *this;
}

float CQuaternion::SqNorm() const {
	return (x * x + y * y + z * z + r * r);
}

float CQuaternion::InvSqrt(float f)
{
#if 0
	return math::isqrt(f);
#else
	return 1.0f / math::sqrt(f);
#endif
}

CQuaternion CQuaternion::Lerp(const CQuaternion& q1, const CQuaternion& q2, const float a) {
	assert(q1.Normalized());
	assert(q2.Normalized());
	return (q1 * (1.0f - a) + (q2 * a)).Normalize();
}

CQuaternion CQuaternion::SLerp(const CQuaternion& q1, const CQuaternion& q2_, const float a) {
	assert( q1.Normalized());
	assert(q2_.Normalized());

	if (a == 0.0f)
		return q1;
	else if (a == 1.0f)
		return q2_;

	// dot product
	float cosTheta = (q1.x * q2_.x + q1.y * q2_.y + q1.z * q2_.z);

	const float s = Sign(cosTheta);

	CQuaternion q2 = q2_ * s;
	cosTheta *= s;

	if unlikely(cosTheta > 1.0f - float3::cmp_eps()) {
		// Linear interpolation
		return Lerp(q1, q2, a);
	} else {
		// Essential Mathematics, page 467
		const float angle = math::acos(cosTheta);
		const float s1 = math::sin((1.0f - a) * angle);
		const float s2 = math::sin((       a) * angle);

		const float invsin = 1.0f / math::sin(angle);
		return AssertNormalized(CQuaternion(
			(s1 * q1.x + s2 * q2.x) * invsin,
			(s1 * q1.y + s2 * q2.y) * invsin,
			(s1 * q1.z + s2 * q2.z) * invsin,
			(s1 * q1.r + s2 * q2.r) * invsin
		));
	}
}