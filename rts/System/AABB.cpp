#include "AABB.hpp"
#include "System/SpringMath.h"

float3 AABB::ClampInto(const float3& pnt) const
{
    return float3{
        std::clamp(pnt.x, mins.x, maxs.x),
        std::clamp(pnt.y, mins.y, maxs.x),
        std::clamp(pnt.z, mins.z, maxs.z)
    };
}

float3 AABB::GetVertexP(const float3& normal) const
{
    return mix(mins, maxs, float3{ static_cast<float>(normal.x >= 0), static_cast<float>(normal.y >= 0), static_cast<float>(normal.z >= 0) });
}

float3 AABB::GetVertexN(const float3& normal) const
{
    return mix(maxs, mins, float3{ static_cast<float>(normal.x >= 0), static_cast<float>(normal.y >= 0), static_cast<float>(normal.z >= 0) });
}
