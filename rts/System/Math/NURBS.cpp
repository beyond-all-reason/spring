#include "NURBS.h"

namespace NURBS
{
	// NURBS inverted triangle scheme: Time-Efficient NURBS Curve Evaluation Algorithms
	std::vector<float> Basis_ITS0(int k, int p, const std::vector<float>& knots, float u)
	{
		std::vector<float> N(p + 1);
		std::vector<float> L(p + 1);
		std::vector<float> R(p + 1);

		N[0] = 1.0;

		for (int j = 1; j <= p; j++) {
			float saved = 0.0;
			L[j] = u - knots[k + 1 - j];
			R[j] = knots[k + j] - u;

			for (int r = 0; r < j; r++) {
				float tmp = N[r] / (R[r + 1] + L[j - r]);
				N[r] = saved + R[r + 1] * tmp;
				saved = L[j - r] * tmp;
			}

			N[j] = saved;
		}

		return N;
	}

	float3 GetPoint0(std::vector<float>& N, int k, const std::vector<float4>& P, int p)
	{
		float Nsum = 0.0;
		float3 Cu = {0.0, 0.0, 0.0};

		for (int i = 0; i <= p; i++) {
			N[i] *= P[k - p + i].w;
			Nsum += N[i];
			Cu += N[i] * P[k - p + i];
		}

		if (Nsum != 0.0) {
			Cu /= Nsum;
		}

		return Cu;
	}

	int findSpan(int n, int degree, const std::vector<float>& knots, float t)
	{
		// todo: binary search
		for (int i = degree; i < n + 1; i++) {
			if (knots[i] > t)
				return i - 1;
		}
		return n;
	}

	bool isValidNURBS(int degree, std::vector<float4>& controlPoints, std::vector<float>& knots,
	                  float t)
	{
		float last = knots[0];
		int mult = 1;

		if (controlPoints.size() <= degree || t < 0.0 || t > 1.0 ||
		    knots.size() != controlPoints.size() + degree + 1) {
			return false;
		}

		// check multiplicity
		for (int i = 1; i < knots.size(); i++) {
			if (knots[i] < last || knots[i] < 0.0 || knots[i] > 1.0) {
				return false;
			}
			if (knots[i] == last) {
				mult++;
			} else {
				mult = 1;
			}
			if (mult > degree && i > degree + 1 && i < knots.size() - degree - 1) {
				return false;
			}
			last = knots[i];
		}
		return true;
	}

	float3 SolveNURBS(int degree, std::vector<float4>& controlPoints, std::vector<float>& knots,
	                  float t)
	{
		if (!isValidNURBS(degree, controlPoints, knots, t)) {
			return float3{0, 0, 0};
		}

		const int k = findSpan(controlPoints.size() - 1, degree, knots, t);
		if (k < 0) {
			return float3{0, 0, 0};
		}
		std::vector<float> N = Basis_ITS0(k, degree, knots, t);
		return GetPoint0(N, k, controlPoints, degree);
	}

	std::vector<float3> SolveNURBSCurve(int degree, std::vector<float4>& controlPoints,
	                                    std::vector<float>& knots, float segments)
	{
		float umin = minU(degree, controlPoints, knots);
		float umax = maxU(degree, controlPoints, knots);
		float increment = (umax - umin) / segments;
		std::vector<float3> points{};

		if (!isValidNURBS(degree, controlPoints, knots, umin)) {
			return points;
		}

		for (float x = umin; x <= umax; x += increment) {

			const int k = findSpan(controlPoints.size() - 1, degree, knots, x);
			if (k < 0) {
				return points;
			}
			std::vector<float> N = Basis_ITS0(k, degree, knots, x);
			points.push_back(GetPoint0(N, k, controlPoints, degree));
		}
		return points;
	}

	float minU(int degree, std::vector<float4>& controlPoints, std::vector<float>& knots)
	{
		float last = knots[0];
		int mult = 1;
		for (int i = 1; i < degree; i++) {
			if (knots[i] == last) {
				mult++;
			} else {
				return knots[degree]; // +1?
			}
			if (mult > degree) {
				return 0;
			}
		}
		return 0;
	}
	float maxU(int degree, std::vector<float4>& controlPoints, std::vector<float>& knots)
	{
		int klen = knots.size();
		float last = knots[klen - 1];
		int mult = 1;
		for (int i = 1; i < degree; i++) {
			if (knots[klen - 1 - i] == last) {
				mult++;
			} else {
				return knots[klen - degree - 1];
			}
			if (mult > degree) {
				return 1;
			}
		}
		return 1;
	}

}  // namespace NURBS