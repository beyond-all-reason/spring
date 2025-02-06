/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifdef USE_VALGRIND
	#include <valgrind/valgrind.h>
#endif

#include "System/SpringMath.h"
#include "System/Exceptions.h"
#include "System/Sync/FPUCheck.h"
#include "System/Log/ILog.h"
#include "System/AABB.hpp"
#include "Sim/Units/Scripts/CobInstance.h" // for TAANG2RAD (ugh)

#undef far
#undef near

float2 SpringMath::headingToVectorTable[NUM_HEADINGS];

void SpringMath::Init()
{
	good_fpu_init();

	for (int a = 0; a < NUM_HEADINGS; ++a) {
		const float ang = (a - (NUM_HEADINGS / 2)) * math::TWOPI / NUM_HEADINGS;

		headingToVectorTable[a].x = math::sin(ang);
		headingToVectorTable[a].y = math::cos(ang);
	}

	unsigned checksum = 0;
	unsigned uheading[2] = {0, 0};

	// NOLINTNEXTLINE{modernize-loop-construct}
	for (int a = 0; a < NUM_HEADINGS; ++a) {
		memcpy(&uheading[0], &headingToVectorTable[a].x, sizeof(unsigned));
		memcpy(&uheading[1], &headingToVectorTable[a].y, sizeof(unsigned));

		checksum = 33 * checksum + uheading[0];
		checksum *= 33;
		checksum = 33 * checksum + uheading[1];
	}

#ifdef USE_VALGRIND
	if (RUNNING_ON_VALGRIND) {
		// Valgrind doesn't allow us setting the FPU, so syncing is impossible
		LOG_L(L_WARNING, "[%s] valgrind detected, sync-checking disabled!", __func__);
		return;
	}
#endif

#if STREFLOP_ENABLED
	if (checksum == HEADING_CHECKSUM)
		return;

	throw unsupported_error(
		"Invalid headingToVectorTable checksum. Most likely"
		" your streflop library was not compiled with the correct"
		" options, or you are not using streflop at all."
	);
#endif
}



float3 GetVectorFromHAndPExact(const short int heading, const short int pitch)
{
	float3 ret;
	const float h = heading * TAANG2RAD;
	const float p = pitch * TAANG2RAD;
	ret.x = math::sin(h) * math::cos(p);
	ret.y = math::sin(p);
	ret.z = math::cos(h) * math::cos(p);
	return ret;
}

float LinePointDist(const float3 l1, const float3 l2, const float3 p)
{
	const float3 dir = (l2 - l1).SafeNormalize();
	const float3 vec = dir * std::clamp(dir.dot(p - l1), 0.0f, dir.dot(l2 - l1));
	const float3  p2 = p - vec;
	return (p2.distance(l1));
}

/**
 * @brief calculate closest point on linepiece from l1 to l2
 * Note, this clamps the returned point to a position between l1 and l2.
 */
float3 ClosestPointOnLine(const float3 l1, const float3 l2, const float3 p)
{
	const float3 ldir(l2 - l1);
	const float3 pdir( p - l1);

	const float length = ldir.Length();

	if (length < 1e-4f)
		return l1;

	const float pdist = ldir.dot(pdir) / length;
	const float cdist = std::clamp(pdist, 0.0f, length);

	return (l1 + ldir * (cdist / length));
}


bool ClosestPointOnRay(const float3 p0, const float3 ray, const float3 p, float3& px)
{
	const float3 pdir(p - p0);
	const float pdist = ray.dot(pdir);
	if (pdist < 0.0f)
		return false;

	px = p0 + ray * pdist;

	return true;
}


// Credit:
// - https://stackoverflow.com/a/38437831/7351594
// - Practical Geometry Algorithms - Daniel Sunday
// We use the 'Direct Linear Equation' method described in the book above
float3 SolveIntersectingPoint(int zeroCoord, int coord1, int coord2, const float4& plane1, const float4& plane2)
{
	const float a1 = plane1[coord1];
	const float b1 = plane1[coord2];
	const float d1 = -plane1[3];

	const float a2 = plane2[coord1];
	const float b2 = plane2[coord2];
	const float d2 = -plane2[3];

	float3 point;

	point[zeroCoord] = 0;
	point[coord1] = (b2 * d1 - b1 * d2) / (a1 * b2 - a2 * b1);
	point[coord2] = (a1 * d2 - a2 * d1) / (a1 * b2 - a2 * b1);

	return point;
}


// This method helps finding a point on the intersection between two planes.
// Depending on the orientation of the planes, the problem could solve for the
// zero point on either the x, y or z axis
bool IntersectPlanes(const float4& plane1, const float4& plane2, std::pair<float3, float3> &line)
{
	// the cross product gives us the direction of the line at the intersection
	// of the two planes, and gives us an easy way to check if the two planes
	// are parallel - the cross product will have zero magnitude
	line.first = plane1.cross(plane2);

	if (const float magnitude = line.first.Length(); magnitude > float3::nrm_eps()) {
		line.first *= (1.0f / magnitude);
	}
	else {
		return false;
	}

	// now find a point on the intersection. We choose which coordinate
	// to set as zero by seeing which has the largest absolute value in the
	// directional vector
	const float x = fabs(line.first.x);
	const float y = fabs(line.first.y);
	const float z = fabs(line.first.z);

	if (z >= x && z >= y) {
		line.second = SolveIntersectingPoint(2, 0, 1, plane1, plane2); // 'z', 'x', 'y'
	} else if (y >= z && y >= x) {
		line.second = SolveIntersectingPoint(1, 2, 0, plane1, plane2); // 'y', 'z', 'x'
	} else {
		line.second = SolveIntersectingPoint(0, 1, 2, plane1, plane2); // 'x', 'y', 'z'
	}

	return true;
}

// https://math.stackexchange.com/questions/2213165/find-shortest-distance-between-lines-in-3d/2217845#2217845
bool LinesIntersectionPoint(const std::pair<float3, float3>& l1, const std::pair<float3, float3>& l2, float3& px)
{
	const float3 n = l2.first.cross(l1.first);
	const float n2 = n.dot(n);

	if (n2 < float3::nrm_eps())
		return false; // parallel

	const float3 p21 = l2.second - l1.second;

	if (const float d = n.dot(p21) / math::sqrt(n2); math::fabs(d) > float3::cmp_eps())
		return false; // do not intersect

	const float t1 = p21.dot(l2.first.cross(n)) / n2;
	px = l1.second + t1 * l1.first;

	/*
	const float t2 = p21.dot(l1.first.cross(n)) / n2;
	px = l2.second + t2 * l2.first;
	*/

	return true;
}


/**
 * calculates the two intersection points ON the ray
 * as scalar multiples of `dir` starting from `start`;
 * `near` defines the new startpoint and `far` defines
 * the new endpoint.
 *
 * credits:
 * http://ompf.org/ray/ray_box.html
 */
float2 GetMapBoundaryIntersectionPoints(const float3 start, const float3 dir)
{
	const float rcpdirx = (dir.x != 0.0f)? (1.0f / dir.x): 10000.0f;
	const float rcpdirz = (dir.z != 0.0f)? (1.0f / dir.z): 10000.0f;

	const float mapwidth  = float3::maxxpos + 1.0f;
	const float mapheight = float3::maxzpos + 1.0f;

	// x-component
	float xl1 = (    0.0f - start.x) * rcpdirx;
	float xl2 = (mapwidth - start.x) * rcpdirx;
	float xnear = std::min(xl1, xl2);
	float xfar  = std::max(xl1, xl2);

	// z-component
	float zl1 = (     0.0f - start.z) * rcpdirz;
	float zl2 = (mapheight - start.z) * rcpdirz;
	float znear = std::min(zl1, zl2);
	float zfar  = std::max(zl1, zl2);

	// both
	float near = std::max(xnear, znear);
	float far  = std::min(xfar, zfar);

	if (far < 0.0f || far < near) {
		// outside of boundary
		near = -1.0f;
		far = -1.0f;
	}

	return {near, far};
}

bool RayHitsSphere(const float4 sphere, const float3 p0, const float3 ray)
{
	float3 px;
	if (!ClosestPointOnRay(p0, ray, sphere.xyz, px))
		return false;

	return px.distance(sphere.xyz) <= sphere.w;
}

bool RayHitsAABB(const AABB& aabb, const float3& p0, const float3& ray, float3& hitPos)
{
	float3 t0s = (aabb.mins - p0) / ray;
	float3 t1s = (aabb.maxs - p0) / ray;

	float3 tMins = float3::min(t0s, t1s);
	float3 tMaxs = float3::max(t0s, t1s);

	float tMin = std::max({ tMins.x, tMins.y, tMins.z });
	float tMax = std::min({ tMaxs.x, tMaxs.y, tMaxs.z });

	if (tMin >= tMax)
		return false;

	hitPos = p0 + tMax * ray;
	return true;
}

bool RayAndPlaneIntersection(const float3& p0, const float3& p1, const float4& plane, bool directional, float3& px)
{
	const float3 ray = p1 - p0;
	const float denom = plane.dot(ray);

	if (directional && denom > 0.0f)
		return false;

	if (std::fabs(denom) < 1e-4)
		return false;

	const float t = -(plane.dot(p0) + plane.w) / denom;
	if (t < 0.0f/* || t > 1.0f*/) // we only care abut the case when the intersection is behind p0
		return false;

	px = p0 + ray * t;
	return true;
}

bool ClampLineInMap(float3& start, float3& end)
{
	const float3 dir = end - start;
	const float2 ips = GetMapBoundaryIntersectionPoints(start, dir);

	const float near = ips.x;
	const float far  = ips.y;

	if (far < 0.0f) {
		// outside of map!
		start = -OnesVector;
		end   = -OnesVector;
		return true;
	}

	if (far < 1.0f || near > 0.0f) {
		end   = start + dir * std::min(far, 1.0f);
		start = start + dir * std::max(near, 0.0f);

		// precision of near,far are limited, better clamp afterwards
		end.ClampInMap();
		start.ClampInMap();
		return true;
	}

	return false;
}


bool ClampRayInMap(const float3 start, float3& end)
{
	const float3 dir = end - start;
	const float2 ips = GetMapBoundaryIntersectionPoints(start, dir);

	const float near = ips.x;
	const float far  = ips.y;

	if (far < 0.0f) {
		end = start;
		return true;
	}

	if (far < 1.0f || near > 0.0f) {
		end = (start + dir * std::min(far, 1.0f)).cClampInMap();
		return true;
	}

	return false;
}

void ClipRayByPlanes(const float3& p0, float3& p, const std::initializer_list<float4>& clipPlanes)
{
	float3 minPx = p;
	float dMin = p.SqDistance(p0);
	for (const auto& clipPlane : clipPlanes) {
		float3 px;
		if (RayAndPlaneIntersection(p0, p, clipPlane, true, px)) {
			const float dx = px.SqDistance(p0);
			if (dx < dMin) {
				minPx = px;
				dMin = dx;
			}
		}
	}
	p = minPx;
}

float3 GetTriangleBarycentric(const float3& p0, const float3& p1, const float3& p2, const float3& p)
{
	const float3 v0 = p2 - p0;
	const float3 v1 = p1 - p0;
	const float3 v2 = p - p0;

	const float dot00 = v0.dot(v0);
	const float dot01 = v0.dot(v1);
	const float dot02 = v0.dot(v2);
	const float dot11 = v1.dot(v1);
	const float dot12 = v1.dot(v2);

	const float invDenom = 1.0 / (dot00 * dot11 - dot01 * dot01);

	const float s = (dot11 * dot02 - dot01 * dot12) * invDenom;
	const float t = (dot00 * dot12 - dot01 * dot02) * invDenom;
	const float q = 1.0 - s - t;
	return float3(s, t, q);
}

bool PointInsideTriangle(const float3& v0, const float3& v1, const float3& v2, const float3& vx)
{
#if 0
	float s1 = std::copysignf(1.0f, v2.dot(v0.cross(v1)) );
	float s2 = std::copysignf(1.0f, v2.dot(vx.cross(v1)) );
	float s3 = std::copysignf(1.0f, v2.dot(v0.cross(vx)) );
	float s4 = std::copysignf(1.0f, vx.dot(v0.cross(v1)) );

	return s2 == s1 && s3 == s1 && s4 == s1;
#else
	const float3 bary = GetTriangleBarycentric(v0, v1, v2, vx);
	return bary.x >= 0.0f && bary.y >= 0.0f && bary.z >= 0.0f;
#endif
}

bool PointInsideQuadrilateral(const float3& p0, const float3& p1, const float3& p2, const float3& p3, const float3& px)
{
	return PointInsideTriangle(p0, p1, p2, px) || PointInsideTriangle(p2, p3, p0, px);
}


float linearstep(const float edge0, const float edge1, const float value)
{
	const float v = std::clamp(value, edge0, edge1);
	const float x = (v - edge0) / (edge1 - edge0);
	const float t = std::clamp(x, 0.0f, 1.0f);
	return t;
}

float smoothstep(const float edge0, const float edge1, const float value)
{
	const float v = std::clamp(value, edge0, edge1);
	const float x = (v - edge0) / (edge1 - edge0);
	const float t = std::clamp(x, 0.0f, 1.0f);
	return (t * t * (3.0f - 2.0f * t));
}

float3 smoothstep(const float edge0, const float edge1, float3 vec)
{
	vec.x = smoothstep(edge0, edge1, vec.x);
	vec.y = smoothstep(edge0, edge1, vec.y);
	vec.z = smoothstep(edge0, edge1, vec.z);
	return vec;
}


float3 hs2rgb(float h, float s)
{
	// FIXME? ignores saturation completely
	s = 1.0f;

	const float invSat = 1.0f - s;

	if (h > 0.5f) { h += 0.1f; }
	if (h > 1.0f) { h -= 1.0f; }

	float3 col(invSat / 2.0f, invSat / 2.0f, invSat / 2.0f);

	if (h < (1.0f / 6.0f)) {
		col.x += s;
		col.y += s * (h * 6.0f);
	} else if (h < (1.0f / 3.0f)) {
		col.y += s;
		col.x += s * ((1.0f / 3.0f - h) * 6.0f);
	} else if (h < (1.0f / 2.0f)) {
		col.y += s;
		col.z += s * ((h - (1.0f / 3.0f)) * 6.0f);
	} else if (h < (2.0f / 3.0f)) {
		col.z += s;
		col.y += s * ((2.0f / 3.0f - h) * 6.0f);
	} else if (h < (5.0f / 6.0f)) {
		col.z += s;
		col.x += s * ((h - (2.0f / 3.0f)) * 6.0f);
	} else {
		col.x += s;
		col.z += s * ((3.0f / 3.0f - h) * 6.0f);
	}

	return col;
}
