#include "Polygon.hpp"

#include <numeric>

#include "System/SpringMath.h"
#include "System/AABB.hpp"
#include "System/Matrix44f.h"

namespace {
	enum {
		NBL = 0,
		FBL = 1,
		NBR = 2,
		FBR = 3,
		NTL = 4,
		FTL = 5,
		NTR = 6,
		FTR = 7,
	};
}

void Geometry::Polygon::MakeFrom(const std::array<float3, 8>& points)
{
	faces.clear();

	// in alignment with AABB.GetCorners() ordering, i.e.

	/*
		// bottom
		float3{ mins.x, mins.y, mins.z },   //NBL
		float3{ mins.x, mins.y, maxs.z },   //FBL
		float3{ maxs.x, mins.y, mins.z },   //NBR
		float3{ maxs.x, mins.y, maxs.z },   //FBR
		// top
		float3{ mins.x, maxs.y, mins.z },   //NTL
		float3{ mins.x, maxs.y, maxs.z },   //FTL
		float3{ maxs.x, maxs.y, mins.z },   //NTR
		float3{ maxs.x, maxs.y, maxs.z }    //FTR
	*/

	// Left Face
	AddFace(points[NTL], points[NBL], points[FBL], points[FTL]);
	// Right Face
	AddFace(points[NBR], points[NTR], points[FTR], points[FBR]);
	// Bottom Face
	AddFace(points[NBL], points[NBR], points[FBR], points[FBL]);
	// Top Face
	AddFace(points[NTR], points[NTL], points[FTL], points[FTR]);
	// Near Face
	AddFace(points[NTL], points[NTR], points[NBR], points[NBL]);
	// Far Face
	AddFace(points[FTR], points[FTL], points[FBL], points[FBR]);
}

void Geometry::Polygon::FlipFacesDirection()
{
	for (auto& face : faces) {
		face.FlipDirection();
	}
}

Geometry::Polygon& Geometry::Polygon::ClipByInPlace(const Polygon& pc)
{
	std::vector<Face> newFaces;
	std::vector<float3> newPoints;

    for (const auto& clippingFace : pc.GetFaces()) {
		if (!clippingFace.HasPlane())
			continue;

		const auto& clippingFacePlane = clippingFace.GetPlane();
		newPoints.clear(); // points to form a new plane

		for (const auto& clippedFace : GetFaces()) {
			if (!clippedFace.IsValid())
				continue;

			const auto& clippedFacePlane = clippedFace.GetPlane();
			if (epscmp(std::fabs(clippingFacePlane.dot(clippedFacePlane)), 1.0f, 1e-3f)) {
				// almost parallel or anti-parallel, skip
				continue;
			}
			
			Face newFace;
			newFace.SetPlane(clippedFace.GetPlane());

			const auto& clippedPoints = clippedFace.GetPoints();
			for (size_t i0 = 0; i0 < clippedPoints.size(); ++i0) {
				size_t i1 = (i0 + 1) % clippedPoints.size();
				const auto& p0 = clippedPoints[i0];
				const auto& p1 = clippedPoints[i1];

				const float d0 = clippingFacePlane.dot(p0) + clippingFacePlane.w;
				const float d1 = clippingFacePlane.dot(p1) + clippingFacePlane.w;
				if (d0 < 0 && d1 < 0)
					continue;

				// Note: RayAndPlaneIntersection is unreliable when p0 or p1 lays on the plane
				// due to precision errors, so copy the code here, but remove sanity checks
				const float3 p01 = p1 - p0;
				const float denom = clippingFacePlane.dot(p01);
				const bool nonZeroDenom = math::fabs(denom) > float3::cmp_eps();

				float3 px;
				if (d0 < 0 && nonZeroDenom) {
					const float t = -(clippingFacePlane.dot(p0) + clippingFacePlane.w) / denom;
					px = p0 + p01 * t;
					assert(epscmp(clippingFacePlane.dot(px) + clippingFacePlane.w, 0.0f, 0.1f));
					newFace.AddPoint(px);
					newPoints.emplace_back(px);
				} else {
					newFace.AddPoint(p0);
				}

				if (d1 < 0 && nonZeroDenom) {
					const float t =  (clippingFacePlane.dot(p1) + clippingFacePlane.w) / denom;
					px = p1 - p01 * t;
					assert(epscmp(clippingFacePlane.dot(px) + clippingFacePlane.w, 0.0f, 0.1f));
					newFace.AddPoint(px);
					newPoints.emplace_back(px);
				} else {
					newFace.AddPoint(p1);
				}
			}

			if (newFace.Sanitize())
				newFaces.emplace_back(std::move(newFace));
		}

		if (newPoints.size() >= 3) {
			Face newFace;
			newFace.SetPlane(clippingFacePlane);
			newFace.AddPoints(std::move(newPoints));
			if (newFace.Sanitize())
				newFaces.emplace_back(std::move(newFace));
		}
		faces = std::move(newFaces);
		newFaces.clear();
    }

    return *this;
}

std::vector<float3> Geometry::Polygon::GetAllLines() const
{
	std::vector<float3> allLines;
	for (const auto& face : GetFaces()) {
		const auto& points = face.GetPoints();
		for (const auto& point : points) {
			allLines.emplace_back(point);
		}
		allLines.emplace_back(points.front()); // close the loop
	}

	return allLines;
}

AABB Geometry::Polygon::GetAABB() const
{
	AABB aabb;
	for (const auto& face : GetFaces()) {
		for (const auto& point : face.GetPoints()) {
			aabb.AddPoint(point);
		}
	}

	return aabb;
}

AABB Geometry::Polygon::GetAABB(const CMatrix44f& mat) const
{
	AABB aabb;
	for (const auto& face : GetFaces()) {
		for (const auto& point : face.GetPoints()) {
			aabb.AddPoint(mat * point);
		}
	}

	return aabb;
}

const float3 Geometry::Polygon::GetMiddlePos() const
{
	size_t count = 0;
	float3 midPos { 0.0f };
	for (const auto& face : GetFaces()) {
		const auto& points = face.GetPoints();
		midPos += std::reduce(points.begin(), points.end());
		count += points.size();
	}

	return midPos / count;
}

bool Geometry::Face::IsValidFast() const
{
	return points.size() >= 3 && HasPlane();
}

bool Geometry::Face::IsValid() const
{
	if (!IsValidFast())
		return false;

	const auto& plane = GetPlane();

	for (size_t i0 = 0; i0 < points.size() - 1; ++i0) {
		size_t i1 = (i0 + 1) % points.size();
		size_t i2 = (i0 + 2) % points.size();

#if 1
		auto u = (points[i0] - points[i1]); const float uLen = u.LengthNormalize();
		auto v = (points[i2] - points[i1]); const float vLen = v.LengthNormalize();
		if (epscmp(uLen, 0.0f, float3::cmp_eps()) || epscmp(vLen, 0.0f, float3::cmp_eps()))
			return false;

		if (epscmp(math::fabs(u.dot(v)), 1.0f, 1e-3f))
			return false;
#else
		const auto u = (points[i0] - points[i1]); const float uLen2 = u.SqLength();
		const auto v = (points[i2] - points[i1]); const float vLen2 = v.SqLength();
		if (epscmp(uLen2, 0.0f, float3::cmp_eps()) || epscmp(vLen2, 0.0f, float3::cmp_eps()))
			return false;
#endif

		const float3 n = v.cross(u).UnsafeANormalize();
		const float  d = -n.dot(points[i1]);

		if (plane != n || !epscmp(plane.w, d, 0.01f))
			return false;
	}

	return true;
}

void Geometry::Face::FlipDirection()
{
	if (!HasPlane())
		return;

	plane = float4{ -plane->x, -plane->y, -plane->z, -plane->w };
	Sanitize();
}

bool Geometry::Face::Sanitize()
{
	if (!IsValidFast())
		return false;

	const auto midPoint = std::reduce(points.begin(), points.end()) / points.size();

	const auto& normal = GetPlane();
	const auto ref = static_cast<float3>(*points.begin() - midPoint);

	// use point.w to store the angle
	for (size_t i = 0; i < points.size(); ++i) {
		const auto vec = static_cast<float3>(points[i]) - midPoint;

		const auto det = normal.dot(ref.cross(vec));
		const auto dot = ref.dot(vec);
		points[i].w = math::atan2f(det, dot);
	}

	static const auto SortPred = [](const auto& lhs, const auto& rhs) {
		return lhs.w < rhs.w;
	};

	static const auto UniqPred = [](const auto& lhs, const auto& rhs) {
		return (rhs - lhs).SqLength() <= 1.0f;
	};

	// put vertices in radial order
	std::sort(points.begin(), points.end(), SortPred);

	// restore point.w for matrix multiplication operations
	for (auto& point : points)
		point.w = 1.0f;

	// and dedup them
	points.erase(std::unique(points.begin(), points.end(), UniqPred), points.end());


	// remove points on the straight line
	for (size_t i0 = 0; i0 < points.size() - 1; /*NOOP*/) {
		size_t i1 = (i0 + 1) % points.size();
		size_t i2 = (i0 + 2) % points.size();

		const float3 u = (points[i0] - points[i1]).Normalize();
		const float3 v = (points[i2] - points[i1]).Normalize();
		if (epscmp(u.dot(v), 1.0f, 1e-3f)) {
			points.erase(points.begin() + i1); // remove point in the middle			
		} else {
			++i0;
		}
	}

	// and dedup again
	points.erase(std::unique(points.begin(), points.end(), UniqPred), points.end());

	if (!IsValidFast())
		return false;

	return IsValid();
}

void Geometry::Face::CondSetPlane()
{
	if (points.size() == 3 && !plane.has_value()) {
		const auto u = points[0] - points[1];
		const auto v = points[2] - points[1];
		const float3 n = v.cross(u).UnsafeANormalize();
		const float  d = -n.dot(points[1]);
		plane = float4(n, d);
	}
}