#include "System/float3.h"
#include "System/float4.h"
#include <vector>

std::vector<float> Basis_ITS0(int k, int p, const std::vector<float>& knots, float u) ;

float3 GetPoint0(std::vector<float>& N, int k, const std::vector<float4>& P, int p) ;

int findSpan(int n, int degree, const std::vector<float>& knots, float t) ;

float3 SolveNURBS(int degree, std::vector<float4> controlPoints, std::vector<float> knots, float t) ;

float minU(int degree, std::vector<float4> controlPoints, std::vector<float> knots);
float maxU(int degree, std::vector<float4> controlPoints, std::vector<float> knots);