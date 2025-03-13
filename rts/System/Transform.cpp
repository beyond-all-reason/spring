#include "Transform.hpp"

#include <cmath>

#include "System/SpringMath.h"

CR_BIND(Transform, )
CR_REG_METADATA(Transform, (
	CR_MEMBER(r),
	CR_MEMBER(t),
	CR_MEMBER(s)
))

static_assert(sizeof (Transform) == 2 * 4 * sizeof(float));
static_assert(alignof(Transform) == alignof(decltype(Transform::r)));

const Transform& Transform::Zero()
{
	static const Transform zero{
		CQuaternion{ 0, 0, 0, 0 },
		float3{ 0, 0, 0 },
		0
	};
	return zero;
}

void Transform::SetScaleSign(float signSrc)
{
	s = std::copysignf(s, signSrc);
}

bool Transform::IsIdentity() const
{
	static constexpr Transform Identity;
	return this->equals(Identity);
}

Transform Transform::FromMatrix(const CMatrix44f& mat)
{
	Transform tra;
	float3 scale;
	std::tie(tra.t, tra.r, scale) = mat.DecomposeIntoTRS();
	assert(
		epscmp(scale.x, scale.y, std::max(scale.x, scale.y) * float3::cmp_eps()) &&
		epscmp(scale.y, scale.z, std::max(scale.y, scale.z) * float3::cmp_eps()) &&
		epscmp(scale.z, scale.x, std::max(scale.z, scale.x) * float3::cmp_eps())
	);
	// non-uniform scaling is not supported
	tra.s = scale.x;

	return tra;
}

CMatrix44f Transform::ToMatrix() const
{
	// M = T * R * S;
	/*
	(r0 * sx, r1 * sx, r2 * sx, 0)
	(r3 * sy, r4 * sy, r5 * sy, 0)
	(r6 * sz, r7 * sz, r8 * sz, 0)
	(tx     , ty     , tz     , 1)
	*/

	// therefore
	CMatrix44f m = r.ToRotMatrix();
	m.Scale(s);
	m.SetPos(t); // m.Translate() will be wrong here

	return m;
}

Transform Transform::Lerp(const Transform& t0, const Transform& t1, float a)
{
	return Transform{
		CQuaternion::SLerp(t0.r, t1.r, a),
		mix(t0.t, t1.t, a),
		mix(t0.s, t1.s, a)
	};
}

Transform Transform::InvertAffine() const
{
	if (s <= float3::cmp_eps())
		return *this;

	// TODO check correctness
	const auto invR = r.Inverse();
	const auto invS = 1.0f / s;
	return Transform{
		invR,
		invR.Rotate(-t * invS),
		invS,
	};
}

Transform Transform::InvertAffineNormalized() const
{
	if (s <= float3::cmp_eps())
		return *this;

	const auto invR = r.InverseNormalized();
	const auto invS = 1.0f / s;
	return Transform{
		invR,
		invR.Rotate(-t * invS),
		invS,
	};
}

bool Transform::equals(const Transform& tra) const
{
	return
		r.equals(tra.r) &&
		t.equals(tra.t) &&
		epscmp(s, tra.s, float3::cmp_eps());
}

Transform Transform::operator*(const Transform& childTra) const
{
	return Transform{
		r * childTra.r,
		t + r.Rotate(s * childTra.t),
		s * childTra.s
	};
}

float3 Transform::operator*(const float3& v) const
{
	// Scale, Rotate, Translate
	// the same order as CMatrix44f's vTra = T * R * S * v;
	return r.Rotate(v * s) + t;
}

float4 Transform::operator*(const float4& v) const
{
	// roughly the same as above
	// CMatrix44f's vTRA = T * R * S * v in case of float4 follows the following structure:
	// vTra = { tx, ty, tz, 1 } * Rmat * { s, s, s, 1 } * {x, y, z, w} = { s * Rx + tx * w, s * Ry + ty * w, s * Rz + tz * w, w }
	// so do the same here
	return float4{ r.Rotate(float3{ v.xyz } * s) + t * v.w, v.w };
}

void Transform::AssertNaNs() const
{
	r.AssertNaNs();
	t.AssertNaNs();
	assert(!math::isnan(s) && !math::isinf(s));
}