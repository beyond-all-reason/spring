#include "AABB.hpp"
#include "System/SpringMath.h"

float3 AABB::GetVertexP(const float3& normal) const
{
    return mix(mins, maxs, float3{ static_cast<float>(normal.x >= 0), static_cast<float>(normal.y >= 0), static_cast<float>(normal.z >= 0) });
}

float3 AABB::GetVertexN(const float3& normal) const
{
    return mix(maxs, mins, float3{ static_cast<float>(normal.x >= 0), static_cast<float>(normal.y >= 0), static_cast<float>(normal.z >= 0) });
}
