/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef AABB_H
#define AABB_H

#include "System/Sync/Logger.h"
#include "System/Log/ILog.h"
#include "System/float3.h"
#include "System/type2.h"
#include "System/Matrix44f.h"

struct AABB {
public:
	static bool RangeOverlap(const float2& a, const float2& b) {
		uint8_t n = 0;

		n += (b.x >= a.x && b.x <= a.y);
		n += (b.y >= a.x && b.y <= a.y);
		n += (a.x >= b.x && a.x <= b.y);
		n += (a.y >= b.x && a.y <= b.y);

		return (n > 0);
	}

	bool Intersects(const AABB& b) const {
		uint8_t n = 0;

		n += RangeOverlap({mins.x, maxs.x}, {b.mins.x, b.maxs.x});
		n += RangeOverlap({mins.y, maxs.y}, {b.mins.y, b.maxs.y});
		n += RangeOverlap({mins.z, maxs.z}, {b.mins.z, b.maxs.z});

		return (n == 3);
	}

	bool Intersects(const float3& center, const float radiusSq) const {
		//point on rectangle closest to circle 
		//(snaps the point to the rectangle, pretty much, 
		//if the circle center is inside the rectangle there isn't snapping, 
		//but this is fine since it will detect a collision as a result)
		float3 pt = center;

		LOG_L(L_ERROR, "1. center.x = %f, center.z = %f, mins.x = %f, mins.y = %f, maxs.x = %f, maxs.y = %f", center.x, center.z, mins.x, mins.y, maxs.x, maxs.y);

		if(pt.x > maxs.x) pt.x = maxs.x;
		if(pt.x < mins.x) pt.x = mins.x;
		if(pt.z > maxs.y) pt.z = maxs.y;
		if(pt.z < mins.y) pt.z = mins.y;

		LOG_L(L_ERROR, "2. pt.x = %f, pt.z = %f, mins.x = %f, mins.y = %f, maxs.x = %f, maxs.y = %f", pt.x, pt.z, mins.x, mins.y, maxs.x, maxs.y);
		LOG_L(L_ERROR, "3. dist = %f, radiusSq = %f, diff = %f, intersect = %i", pt.SqDistance2D(center), radiusSq, radiusSq - pt.SqDistance2D(center), (pt.SqDistance2D(center) < radiusSq));

		return pt.SqDistance2D(center) < radiusSq;
	}

	bool Contains(const float3& p) const {
		uint8_t n = 0;

		n += (p.x >= mins.x && p.x <= maxs.x);
		n += (p.y >= mins.y && p.y <= maxs.y);
		n += (p.z >= mins.z && p.z <= maxs.z);

		return (n == 3);
	};

	void CalcCorners(float3 verts[8]) const { CalcCorners(CMatrix44f::Identity(), verts); }
	void CalcCorners(const CMatrix44f& mat, float3 verts[8]) const {
		// bottom
		verts[0] = mat * float3{mins.x, mins.y, mins.z};
		verts[1] = mat * float3{mins.x, mins.y, maxs.z};
		verts[2] = mat * float3{maxs.x, mins.y, mins.z};
		verts[3] = mat * float3{maxs.x, mins.y, maxs.z};
		// top
		verts[4] = mat * float3{mins.x, maxs.y, mins.z};
		verts[5] = mat * float3{mins.x, maxs.y, maxs.z};
		verts[6] = mat * float3{maxs.x, maxs.y, mins.z};
		verts[7] = mat * float3{maxs.x, maxs.y, maxs.z};
	}

	float3 CalcCenter(const CMatrix44f& mat) const { return (mat * CalcCenter()); }
	float3 CalcCenter() const { return ((maxs + mins) * 0.5f); }
	float3 CalcScales() const { return ((maxs - mins) * 0.5f); }

	float CalcRadiusSq() const { return (CalcScales().SqLength()); }
	float CalcRadius() const { return (CalcScales().Length()); }

public:
	float3 mins;
	float3 maxs;
};

#endif

