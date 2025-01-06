#pragma once

#include <vector>
#include "System/float4.h"

namespace ConvexHull {
	class Face {
	public:
		void AddPoint(const float3& pnt) {
			points.emplace_back(pnt);
			CondSetPlane();
		}
		void AddPoint(float3&& pnt) {
			points.emplace_back(std::move(pnt));
			CondSetPlane();
		}
		bool IsValidFast() const;
		bool IsValid() const;

		const auto& GetPoints() const { return points; }
		const auto& GetPlane() const { return plane; }
	private:
		void inline CondSetPlane();
	private:
		float4 plane;
		std::vector<float3> points; // last == first
	};

	class Polygon {
	public:
		Face& AddFace() {
			return faces.emplace_back();
		}
		const auto& GetFaces() const { return faces; }
		Polygon& ClipByInPlace(const Polygon& pc);
		Polygon  ClipBy(const Polygon& pc) { Polygon p = *this; p.ClipByInPlace(pc); return p; }
	private:
		std::vector<Face> faces;
	};
}

