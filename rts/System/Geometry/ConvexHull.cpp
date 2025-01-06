#include "ConvexHull.hpp"
#include "System/SpringMath.h"

ConvexHull::Polygon& ConvexHull::Polygon::ClipByInPlace(const Polygon& pc)
{
	static std::vector<Face> newFaces;

    for (const auto& clippingFace : pc.GetFaces()) {
		if (!clippingFace.IsValid())
			continue;

		for (const auto& clippedFace : GetFaces()) {
			if (!clippedFace.IsValid())
				continue;

			std::pair<float3, float3> intersectionLine; // <direction, point>
			if (!IntersectPlanes(clippingFace.GetPlane(), clippedFace.GetPlane(), intersectionLine))
				continue;

			// figure out .w sign
			float4 intersectionLineEquation{ intersectionLine.first, -intersectionLine.first.dot(intersectionLine.second) };

			float3 clippedP0{ std::numeric_limits<float>::infinity() };
			float3 clippedP1{ std::numeric_limits<float>::infinity() };

			// find exact line in the clipped face matching intersectionLine
			const auto& clippedPoints = clippedFace.GetPoints();
			for (size_t i0 = 0; i0 < clippedPoints.size(); ++i0) {
				size_t i1 = (i0 + 1) % clippedPoints.size();
				const auto& p0 = clippedPoints[i0];
				const auto& p1 = clippedPoints[i1];

				float3 p10 = (p1 - p0);
				const auto l = p10.Length();
				if (epscmp(l, 0.0f, float3::cmp_eps()))
					continue;

				p10 /= l;

				float4 lineEquation{ p10, -p10.dot(p0) };

				if (lineEquation != intersectionLineEquation)
					continue;

				if (lineEquation.dot4(p0) > lineEquation.dot4(clippedP0))
					clippedP0 = p0;

				if (lineEquation.dot4(p1) < lineEquation.dot4(clippedP1))
					clippedP0 = p0;
			}
			/*
			const auto& clippingPoints = clippingFace.GetPoints();
			for (size_t i0 = 0; i0 < clippingPoints.size(); ++i0) {
				size_t i1 = (i0 + 1) % clippingPoints.size();
				const auto& p0 = clippingPoints[i0];
				const auto& p1 = clippingPoints[i1];
			}
			*/
		}

    }
    return *this;
}

bool ConvexHull::Face::IsValidFast() const
{
	return points.size() >= 3;
}

bool ConvexHull::Face::IsValid() const
{
	if (!IsValidFast())
		return false;

	for (size_t i0 = 1; i0 < points.size(); ++i0) {
		size_t i1 = (i0 + 1) % points.size();
		size_t i2 = (i0 + 2) % points.size();

		const float3 u = points[i0] - points[i1];
		const float3 v = points[i2] - points[i1];
		if (epscmp(u.SqLength(), 0.0f, float3::cmp_eps()) || epscmp(v.SqLength(), 0.0f, float3::cmp_eps()))
			continue;

		const float3 n = v.cross(u).UnsafeANormalize();
		const float  d = -n.dot(points[1]);

		if (plane != float4{ n, d })
			return false;
	}

	return true;
}

inline void ConvexHull::Face::CondSetPlane()
{
	if (points.size() == 3) {
		const float3 u = points[0] - points[1];
		const float3 v = points[2] - points[1];
		const float3 n = v.cross(u).UnsafeANormalize();
		const float  d = -n.dot(points[1]);
		plane = float4(n, d);
	}
}
