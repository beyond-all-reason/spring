#include "Quaternion.h"
/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Quaternion.h"
#include "System/SpringMath.h"


//contains some code from
// https://github.com/ilmola/gml/blob/master/include/gml/quaternion.hpp
// https://github.com/ilmola/gml/blob/master/include/gml/mat.hpp

CR_BIND(CQuaternion, )
CR_REG_METADATA(CQuaternion, CR_MEMBER(q))

/// <summary>
/// Quaternion from Euler angles
/// </summary>
CQuaternion CQuaternion::MakeFrom(const float3& euler)
{
	const float a1 = 0.5f * euler.x;
	const float a2 = 0.5f * euler.y;
	const float a3 = 0.5f * euler.z;

	const float sx = math::sin(a1);
	const float cx = math::cos(a1);
	const float sy = math::sin(a2);
	const float cy = math::cos(a2);
	const float sz = math::sin(a3);
	const float cz = math::cos(a3);

	return CQuaternion(
		cx * cy * cz + sx * sy * sz,
		float3(
			 cy * cz * sx - cx * sy * sz,
			 cx * cz * sy + cy * sx * sz,
			-cz * sx * sy + cx * cy * sz
		)
	);
}

/// <summary>
/// Quaternion from rotation angle and axis
/// </summary>
CQuaternion CQuaternion::MakeFrom(float angle, const float3& axis)
{
	assert(axis.Normalized());

	const float a = 0.5f * angle;
	return CQuaternion(math::cos(a), axis * math::sin(a));
}

/// <summary>
/// Quaternion to rotate from v1 to v2
/// </summary>
CQuaternion CQuaternion::MakeFrom(const float3& v1, const float3& v2)
{
	float dp = v1.dot(v2);
	if (unlikely(v1.same(v2))) {
		return CQuaternion(0.0f, v1);
	}
	else if (unlikely(v1.same(-v2))) {
		float3 v;
		if (v1.x > -float3::cmp_eps() && v1.x < float3::cmp_eps())		// if x ~= 0
			v = { 1.0f, 0.0f, 0.0f };
		else if (v1.y > -float3::cmp_eps() && v1.y < float3::cmp_eps())	// if y ~= 0
			v = { 0.0f, 1.0f, 0.0f };
		else															// if z ~= 0
			v = { 0.0f, 0.0f, 1.0f };

		return CQuaternion(math::HALFPI, v);
	}
	else {
		float3 u1 = v1; u1.Normalize();
		float3 u2 = v1; u2.Normalize();

		float3 v = u1.cross(u2);				// compute rotation axis
		float angle = math::acosf(u1.dot(u2));	// rotation angle
		return CQuaternion(angle * 0.5f, v);	// half angle
	}
}

/// <summary>
///  Quaternion from a rotation matrix
///  Should only be called on R or T * R matrices
/// </summary>
CQuaternion CQuaternion::MakeFrom(const CMatrix44f& mat)
{
	const float trace = mat.md[0][0] + mat.md[1][1] + mat.md[2][2];

	if (trace > 0.0f) {
		const float s = 0.5f * math::isqrt(trace + 1.0f);

		return CQuaternion(
			0.25f / s,
			s * float3(
				(mat.md[1][2] - mat.md[2][1]),
				(mat.md[2][0] - mat.md[0][2]),
				(mat.md[0][1] - mat.md[1][0])
			)
		);
	}
	else if (mat.md[0][0] > mat.md[1][1] && mat.md[0][0] > mat.md[2][2]) {
		const float s = 2.0f * math::sqrt(1.0f + mat.md[0][0] - mat.md[1][1] - mat.md[2][2]);

		return CQuaternion(
			(mat.md[1][2] - mat.md[2][1]) / s,
			float3(
				0.25f * s,
				(mat.md[1][0] + mat.md[0][1]) / s,
				(mat.md[2][0] + mat.md[0][2]) / s
			)
		);
	}
	else if (mat.md[1][1] > mat.md[2][2]) {
		const float s = 2.0f * math::sqrt(1.0f + mat.md[1][1] - mat.md[0][0] - mat.md[2][2]);

		return CQuaternion(
			(mat.md[2][0] - mat.md[0][2]) / s,
			float3(
				(mat.md[1][0] + mat.md[0][1]) / s,
				0.25f * s,
				(mat.md[2][1] + mat.md[1][2]) / s
			)
		);
	}
	else {
		const float s = 2.0f * math::sqrt(1.0f + mat.md[2][2] - mat.md[0][0] - mat.md[1][1]);

		return CQuaternion(
			(mat.md[0][1] - mat.md[1][0]) / s,
			float3(
				(mat.md[2][0] + mat.md[0][2]) / s,
				(mat.md[2][1] + mat.md[1][2]) / s,
				0.25f * s
			)
		);
	}
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

	//triple product == determinant
	const float d = c0.dot(c1.cross(c2));

	const float s = Sign(d);

	float3 scaling {s * c0.Length(), c1.Length(), c2.Length()};


	t0[0] /= scaling[0];
	t1[1] /= scaling[1];
	t2[2] /= scaling[2];

	return std::make_tuple(
		float3(mat.col[3]),				//translate
		CQuaternion::MakeFrom(tmpMat),	//rotate (quat)
		scaling							//scale
	);
}


CQuaternion& CQuaternion::Normalize()
{
	const float sqn = SqNorm();
	if (unlikely(sqn < float3::nrm_eps()))
		return *this;

	q *= math::isqrt(sqn);

	return *this;
}

/// <summary>
/// Find angle and axis from a quaternion
/// </summary>
std::tuple<float, float3> CQuaternion::ToRotation() const
{
	return std::make_tuple(
		2.0f * math::acos(std::clamp(q.w, -1.0f, 1.0f)),
		static_cast<float3>(q) * math::isqrt(std::max(0.0f, 1.0f - q.w * q.w))
	);
}

/// <summary>
/// Converts a quternion to rotational matrix
/// </summary>
CMatrix44f CQuaternion::ToRotMatrix() const
{
	const float qxx = q.x * q.x;
	const float qyy = q.y * q.y;
	const float qzz = q.z * q.z;
	const float qxz = q.x * q.z;
	const float qxy = q.x * q.y;
	const float qyz = q.y * q.z;
	const float qwx = q.w * q.x;
	const float qwy = q.w * q.y;
	const float qwz = q.w * q.z;

	return CMatrix44f(
		1.0f - 2.0f * (qyy + qzz), 2.0f * (qxy - qwz)       , 2.0f * (qxz + qwy)       , 0.0f,
		2.0f * (qxy + qwz)       , 1.0f - 2.0f * (qxx + qzz), 2.0f * (qyz - qwx)       , 0.0f,
		2.0f * (qxz - qwy)       , 2.0f * (qyz + qwx)       , 1.0f - 2.0f * (qxx + qyy), 0.0f,
		0.0f                     , 0.0f                     , 0.0f                     , 1.0f
	);
}

CQuaternion& CQuaternion::Inverse()
{
	const float sqn = SqNorm();
	if (unlikely(sqn < float3::nrm_eps()))
		return *this;

	Conjugate() / SqNorm();
	return *this;
};

CQuaternion CQuaternion::operator*(const CQuaternion& rhs) const
{
	const float3& limag =     q;
	const float3& rimag = rhs.q;

	return CQuaternion(
		q.w * rhs.q.w - limag.dot(rimag),
		q.w * rimag + rhs.q.w * limag + limag.cross(rimag)
	);
}

float CQuaternion::SqNorm() const {
	return q.SqLength();
}

CQuaternion CQuaternion::Lerp(const CQuaternion& q1, const CQuaternion& q2, const float a) {
	return q1 * (1.0f - a) + (q2 * a);
}

CQuaternion CQuaternion::SLerp(const CQuaternion& q1, const CQuaternion& q2_, const float a) {
	float cosTheta = q1.dot(q2_);

	const float s = Sign(cosTheta);

	CQuaternion q2 = q2_ * s;
	cosTheta *= s;

	if (unlikely(cosTheta > 1.0f - float3::cmp_eps())) {
		// Linear interpolation
		return CQuaternion(
			mix(q1.q, q2.q, a)
		);
	} else {
		// Essential Mathematics, page 467
		const float angle = math::acos(cosTheta);
		return CQuaternion(
			(math::sin((1.0f - a) * angle) * q1.q + sin(a * angle) * q2.q) / sin(angle)
		);
	}
}