/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "System/Transform.hpp"

namespace gltfmodel {

	enum class SourceConvention {
		Default,
		S3OCompatible,
	};

	constexpr SourceConvention GetSourceConvention(bool s3oCompatible)
	{
		return s3oCompatible ? SourceConvention::S3OCompatible : SourceConvention::Default;
	}

	// Recoil's GLTF pipeline keeps Blender's Z-up coordinates in the file.
	// Convert those coordinates into the Y-up engine model frame. The two
	// conventions differ only in the source model's forward direction.
	constexpr float3 ToEngineSpace(const float3& v, SourceConvention convention)
	{
		if (convention == SourceConvention::S3OCompatible)
			return float3{-v.x, v.z, v.y};

		return float3{v.x, v.z, -v.y};
	}

	// For a proper basis rotation qBasis, q' = qBasis * q * inverse(qBasis).
	// Quaternion conjugation rotates the imaginary part and preserves the real
	// part, so the exact signed swizzle avoids needless floating-point noise.
	constexpr CQuaternion ToEngineSpace(const CQuaternion& q, SourceConvention convention)
	{
		const float3 imaginary = ToEngineSpace(float3{q.x, q.y, q.z}, convention);
		return CQuaternion{imaginary, q.r};
	}

	constexpr Transform ToEngineSpace(const Transform& transform, SourceConvention convention)
	{
		return Transform{
			ToEngineSpace(transform.r, convention),
			ToEngineSpace(transform.t, convention),
			transform.s,
		};
	}
}
