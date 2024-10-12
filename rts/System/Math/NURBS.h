#include "System/float3.h"
#include "System/float4.h"
#include <vector>

namespace NURBS
{
	std::vector<float> Basis_ITS0(int k, int p, const std::vector<float>& knots, float u);

	float3 GetPoint0(std::vector<float>& N, int k, const std::vector<float4>& P, int p);

	int findSpan(int n, int degree, const std::vector<float>& knots, float t);

	float3 SolveNURBS(int degree, const std::vector<float4>& controlPoints, const std::vector<float>& knots,
	                  float t);
	std::vector<float3> SolveNURBSCurve(int degree, const std::vector<float4>& controlPoints,
	                                    const std::vector<float>& knots, float segments);

	float minU(int degree, const std::vector<float4>& controlPoints, const std::vector<float>& knots);
	float maxU(int degree, const std::vector<float4>& controlPoints, const std::vector<float>& knots);

}  // namespace NURBS