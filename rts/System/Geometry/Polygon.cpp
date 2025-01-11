#include "Polygon.hpp"

#include <numeric>

#include "System/SpringMath.h"
#include "System/AABB.hpp"
#include "System/Matrix44f.h"

Geometry::Allocator::Allocator()
{
	(void) new (pmrMem) std::pmr::monotonic_buffer_resource();
}

Geometry::Allocator::Allocator(size_t allocMemBytes)
{
	buffer.resize(allocMemBytes);
	(void) new (pmrMem) std::pmr::monotonic_buffer_resource(buffer.data(), buffer.size());
}

Geometry::Allocator::~Allocator()
{
	static const auto CallDestructor = []<typename T>(T* ptr) {
		if (ptr == nullptr)
			return;

		ptr->~T();
	};

	CallDestructor(reinterpret_cast<std::pmr::monotonic_buffer_resource*>(&pmrMem[0]));
}

void Geometry::Allocator::ClearAllocations()
{
	(reinterpret_cast<std::pmr::monotonic_buffer_resource*>(&pmrMem[0]))->release();
}

void Geometry::Polygon::MakeFrom(const AABB& aabb)
{
	faces.clear();

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
	const auto corners = aabb.GetCorners();

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

	// Left Face
	AddFace(corners[NTL], corners[NBL], corners[FBL], corners[FTL]);
	// Right Face
	AddFace(corners[NBR], corners[NTR], corners[FTR], corners[FBR]);
	// Bottom Face
	AddFace(corners[NBL], corners[NBR], corners[FBR], corners[FBL]);
	// Top Face
	AddFace(corners[NTR], corners[NTL], corners[FTL], corners[FTR]);
	// Near Face
	AddFace(corners[NTL], corners[NTR], corners[NBR], corners[NBL]);
	// Far Face
	AddFace(corners[FTR], corners[FTL], corners[FBL], corners[FBR]);
}
Geometry::Polygon& Geometry::Polygon::ClipByInPlace(const Polygon& pc)
{
	std::pmr::vector<Face> newFaces(allocRef.get().GetAllocator());
	std::pmr::vector<float3> newPoints(allocRef.get().GetAllocator());

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
			
			Face newFace(allocRef.get());
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

				float3 px;
				if (d0 < 0) {
					const float t = -(clippingFacePlane.dot(p0) + clippingFacePlane.w) / denom;
					px = p0 + p01 * t;
					assert(epscmp(clippingFacePlane.dot(px) + clippingFacePlane.w, 0.0f, 0.1f));
					newFace.AddPoint(px);
					newPoints.emplace_back(px);
				} else {
					newFace.AddPoint(p0);
				}

				if (d1 < 0) {
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
			Face newFace(allocRef.get());
			newFace.SetPlane(clippingFacePlane);
			newFace.AddPoints(std::move(newPoints));
			if (newFace.Sanitize())
				newFaces.emplace_back(std::move(newFace));
		}
		std::swap(faces, newFaces);
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

		const float3 u = points[i0] - points[i1];
		const float3 v = points[i2] - points[i1];
		if (epscmp(u.SqLength(), 0.0f, float3::cmp_eps()) || epscmp(v.SqLength(), 0.0f, float3::cmp_eps()))
			return false;

		const float3 n = v.cross(u).UnsafeANormalize();
		const float  d = -n.dot(points[i1]);

		if (plane != n || !epscmp(plane.w, d, 0.01f))
			return false;
	}

	return true;
}

bool Geometry::Face::Sanitize()
{
	if (!IsValidFast())
		return false;

	const float3 midPoint = std::reduce(points.begin(), points.end()) / points.size();

	const auto& normal = GetPlane();
	const float3 ref = (*points.begin() - midPoint);

	const auto SortPred = [&ref, &midPoint, &normal](const auto& lhs, const auto& rhs) {
		const auto lhsVec = (lhs - midPoint);
		const auto rhsVec = (rhs - midPoint);

		const auto lDet = normal.dot(ref.cross(lhsVec));
		const auto lDot = ref.dot(lhsVec);
		auto lAngle = math::atan2f(lDet, lDot);

		const auto rDet = normal.dot(ref.cross(rhsVec));
		const auto rDot = ref.dot(rhsVec);
		auto rAngle = math::atan2f(rDet, rDot);

		// TODO: figure out the way to compare angles without atan2
		return lAngle < rAngle;
	};
	const auto UniqPred = [](const auto& lhs, const auto& rhs) {
		return (rhs - lhs).SqLength() <= 1.0f;
	};

	// put vertices in radial order
	std::sort(points.begin(), points.end(), SortPred);

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
		const float3 u = points[0] - points[1];
		const float3 v = points[2] - points[1];
		const float3 n = v.cross(u).UnsafeANormalize();
		const float  d = -n.dot(points[1]);
		plane = float4(n, d);
	}
}