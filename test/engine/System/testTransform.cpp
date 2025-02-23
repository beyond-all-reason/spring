/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <array>

#include "System/MathConstants.h"
#include "System/Matrix44f.h"
#include "System/Transform.hpp"
#include "System/Log/ILog.h"

#include <stdlib.h>
#include <time.h>

static inline float randf()
{
	return rand() / float(RAND_MAX);
}

static inline float srandf()
{
	return 2.0f * (randf() - 0.5f);
}

#define CATCH_CONFIG_MAIN
#include "lib/catch.hpp"

namespace std {
	ostream& operator<<(ostream& s, float3 const& f) {
		s << "x:" << f.x << ", y:" << f.y << " z:" << f.z;
		return s;
	}
}

TEST_CASE("VertexTransformation")
{
	for (size_t i = 0; i < 10000; ++i) {
		const float3 ypr{
			srandf() * math::PI,
			srandf() * math::PI,
			srandf() * math::PI
		};

		const float3 t{
			srandf() * 10000.0f,
			srandf() * 10000.0f,
			srandf() * 10000.0f
		};

		const float s = 0.5f + 5.0f * randf();
		CMatrix44f m;

		m.Scale(s);
		m.RotateEulerYXZ(ypr);
		m.Translate(t);

		Transform tra = Transform::FromMatrix(m);

		const float3 v{ srandf() * 100.0f, srandf() * 100.0f, srandf() * 100.0f };
		auto vMat = m * v;
		auto vTra = tra * v;

		auto vMatN = vMat; vMatN.Normalize();
		auto vTraN = vTra; vTraN.Normalize();

		CHECK(math::fabs(1.0f - vMatN.dot(vTraN)) < float3::cmp_eps());
	}
}

TEST_CASE("FromToMatrix")
{
	for (size_t i = 0; i < 10000; ++i) {
		const float3 ypr {
			srandf() * math::PI,
			srandf() * math::PI,
			srandf() * math::PI
		};

		const float3 t {
			srandf() * 10000.0f,
			srandf() * 10000.0f,
			srandf() * 10000.0f
		};

		const float s = 0.5f + 5.0f * randf();
		CMatrix44f m;
		
		m.Scale(s);
		m.RotateEulerYXZ(ypr);
		m.Translate(t);
		
		auto tra = Transform::FromMatrix(m);
		auto m2 = tra.ToMatrix();

		CHECK(m.equals(m2));
	}
}

TEST_CASE("TransformChaining")
{
	for (size_t i = 0; i < 10000; ++i) {
		auto t0 = Transform{
			CQuaternion{ srandf(), srandf(), srandf(), srandf() }.ANormalize(),
			float3{ srandf() * 10000.0f, srandf() * 10000.0f, srandf() * 10000.0f },
			0.5f + 5.0f * randf()
		};
		auto t1 = Transform{
			CQuaternion{ srandf(), srandf(), srandf(), srandf() }.ANormalize(),
			float3{ srandf() * 10000.0f, srandf() * 10000.0f, srandf() * 10000.0f },
			0.5f + 5.0f * randf()
		};
		auto t2 = Transform{
			CQuaternion{ srandf(), srandf(), srandf(), srandf() }.ANormalize(),
			float3{ srandf() * 10000.0f, srandf() * 10000.0f, srandf() * 10000.0f },
			0.5f + 5.0f * randf()
		};

		auto mx = t2.ToMatrix() * t1.ToMatrix() * t0.ToMatrix();
		auto tx = t2 * t1 * t0;

		const float3 v{ srandf() * 100.0f, srandf() * 100.0f, srandf() * 100.0f };
		auto vMat = mx * v;
		auto vTra = tx * v;

		auto vMatN = vMat; vMatN.Normalize();
		auto vTraN = vTra; vTraN.Normalize();

		CHECK(math::fabs(1.0f - vMatN.dot(vTraN)) < float3::cmp_eps());
	}
}

TEST_CASE("AffineInversion")
{
	for (size_t i = 0; i < 10000; ++i) {
		const float3 ypr{
			srandf() * math::PI,
			srandf() * math::PI,
			srandf() * math::PI
		};

		const float3 t{
			srandf() * 10000.0f,
			srandf() * 10000.0f,
			srandf() * 10000.0f
		};

		const float s = 0.5f + 5.0f * randf();
		CMatrix44f m;

		m.Scale(s);
		m.RotateEulerYXZ(ypr);
		m.Translate(t);

		// InvertAffine doesn't handle scale
		auto matInv = m.Invert();

		auto tra = Transform::FromMatrix(m);
		auto traInv = tra.InvertAffine();

		const float3 v{ srandf() * 100.0f, srandf() * 100.0f, srandf() * 100.0f };
		auto vMat = matInv * v;
		auto vTra = traInv * v;

		auto vMatN = vMat; vMatN.Normalize();
		auto vTraN = vTra; vTraN.Normalize();

		CHECK(math::fabs(1.0f - vMatN.dot(vTraN)) < float3::cmp_eps());
	}
}

TEST_CASE("QuaternionRotationEquivalenceYPR")
{
	for (size_t i = 0; i < 10000; ++i) {
		const float3 angles{
			srandf() * math::PI,
			srandf() * math::PI,
			srandf() * math::PI
		};
		
		CQuaternion yprQ = CQuaternion::FromEulerYPR(angles);

		static constexpr auto pAxis = float3(1, 0, 0);
		static constexpr auto yAxis = float3(0, 1, 0);
		static constexpr auto rAxis = float3(0, 0, 1);
		auto yprQ2 = CQuaternion::MakeFrom(angles[CMatrix44f::ANGLE_Y], yAxis) * CQuaternion::MakeFrom(angles[CMatrix44f::ANGLE_P], pAxis) * CQuaternion::MakeFrom(angles[CMatrix44f::ANGLE_R], rAxis);

		CMatrix44f m; m.RotateEulerYXZ(-angles);
		CQuaternion yprQ3;
		std::tie(std::ignore, yprQ3, std::ignore) = m.DecomposeIntoTRS();

		CHECK(yprQ.equals(yprQ2));
		CHECK(yprQ2.equals(yprQ3));
	}
}

TEST_CASE("QuaternionRotationEquivalencePYR")
{
	for (size_t i = 0; i < 10000; ++i) {
		const float3 angles{
			srandf() * math::PI,
			srandf() * math::PI,
			srandf() * math::PI
		};

		CQuaternion pyrQ = CQuaternion::FromEulerPYR(angles);

		static constexpr auto pAxis = float3(1, 0, 0);
		static constexpr auto yAxis = float3(0, 1, 0);
		static constexpr auto rAxis = float3(0, 0, 1);
		auto pyrQ2 = CQuaternion::MakeFrom(angles[CMatrix44f::ANGLE_P], pAxis) * CQuaternion::MakeFrom(angles[CMatrix44f::ANGLE_Y], yAxis) * CQuaternion::MakeFrom(angles[CMatrix44f::ANGLE_R], rAxis);

		CMatrix44f m; m.RotateEulerXYZ(-angles);
		CQuaternion pyrQ3;
		std::tie(std::ignore, pyrQ3, std::ignore) = m.DecomposeIntoTRS();

		CHECK(pyrQ.equals(pyrQ2));
		CHECK(pyrQ.equals(pyrQ3));
	}
}

TEST_CASE("EulerPYR")
{
	for (size_t i = 0; i < 10000; ++i) {
		const float3 angles {
			srandf() * math::PI,
			srandf() * math::PI,
			srandf() * math::PI
		};

		CQuaternion qPYR1 = CQuaternion::FromEulerPYR(angles);
		auto anglesPYR = qPYR1.ToEulerPYR();
		CQuaternion qPYR2 = CQuaternion::FromEulerPYR(anglesPYR);

		const float3 v{ srandf() * 100.0f, srandf() * 100.0f, srandf() * 100.0f };
		auto v1 = qPYR1 * v;
		auto v2 = qPYR2 * v;

		auto v1N = v1; v1N.Normalize();
		auto v2N = v2; v2N.Normalize();

		CHECK(math::fabs(1.0f - v1N.dot(v2N)) < float3::cmp_eps());
	}
}

TEST_CASE("EulerYPR")
{
	for (size_t i = 0; i < 10000; ++i) {
		const float3 angles{
			srandf() * math::PI,
			srandf() * math::PI,
			srandf() * math::PI
		};

		CQuaternion qYPR1 = CQuaternion::FromEulerYPR(angles);
		auto anglesYPR = qYPR1.ToEulerYPR();
		CQuaternion qYPR2 = CQuaternion::FromEulerYPR(anglesYPR);

		const float3 v{ srandf() * 100.0f, srandf() * 100.0f, srandf() * 100.0f };
		auto v1 = qYPR1 * v;
		auto v2 = qYPR2 * v;

		auto v1N = v1; v1N.Normalize();
		auto v2N = v2; v2N.Normalize();

		CHECK(math::fabs(1.0f - v1N.dot(v2N)) < float3::cmp_eps());
	}
}