#include "Transform.hpp"

CR_BIND(Transform, )
CR_REG_METADATA(Transform, (
	CR_MEMBER(r),
	CR_MEMBER(t),
	CR_MEMBER(s)
))

static_assert(sizeof(Transform) == 3 * 4 * sizeof(float));
static_assert(alignof(Transform) == alignof(decltype(Transform::r)));

void Transform::FromMatrix(const CMatrix44f& mat)
{
	std::tie(t, r, s) = CQuaternion::DecomposeIntoTRS(mat);
#ifdef _DEBUG
	const float3 v{ 100, 200, 300 };
	auto vMat = mat * v;
	auto vTra = r.Rotate(v * s) + t;

	auto vMatN = vMat; vMatN.Normalize();
	auto vTraN = vTra; vTraN.Normalize();

	assert(math::fabs(1.0f - vMatN.dot(vTraN)) < 0.05f);
#endif
}

CMatrix44f Transform::ToMatrix() const
{
	// TODO check the sequence
#if 0
	CMatrix44f m = r.ToRotMatrix();
	m.Translate(t);
	m.Scale(s);
#else
	CMatrix44f m = r.ToRotMatrix();
	m.Scale(s);
	m.Translate(t);
#endif
#ifdef _DEBUG
	//auto [t_, r_, s_] = CQuaternion::DecomposeIntoTRS(m);

	const float3 v{ 100, 200, 300 };
	auto vMat = m * v;
	auto vTra = r.Rotate(v * s) + t;

	auto vMatN = vMat; vMatN.Normalize();
	auto vTraN = vTra; vTraN.Normalize();

	assert(math::fabs(1.0f - vMatN.dot(vTraN)) < 0.05f);
#endif
	return m;
}

Transform Transform::InvertAffine() const
{
	return Transform{
		r.Inverse(),
		float4{ -t.x, -t.y, -t.z, t.w },
		float4{ 1.0f / s.x, 1.0f / s.y, 1.0f / s.z, 0.0f }
	};
}

Transform Transform::operator*(const Transform& tra) const
{
	// TODO check correctness
	return Transform{
		r * tra.r,
		t + r.Rotate(tra.t),
		s * tra.s
	};
}