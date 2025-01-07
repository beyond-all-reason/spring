#pragma once

#include <vector>
#include <optional>
#include <type_traits>

#include "System/float4.h"

namespace ConvexHull {
	class Face {
	public:
		Face& AddPoint(const float3& pnt) {
			points.emplace_back(pnt);
			CondSetPlane();

			return *this;
		}
		Face& AddPoint(float3&& pnt) {
			points.emplace_back(std::move(pnt));
			CondSetPlane();

			return *this;
		}
		template<typename Iterable>
		Face& AddPoints(Iterable&& iterable) {
			for (auto&& item : iterable) {
				AddPoint(std::forward<std::remove_cv_t<decltype(item)>>(item));
			}

			return *this;
		}
		template<typename ... Item>
		Face& AddPoints(Item&& ... item) {
			(AddPoint(std::forward<Item>(item)), ...);

			return *this;
		}

		bool HasPlane() const { return plane.has_value(); }
		bool IsValidFast() const;
		bool IsValid() const;

		const auto& GetPoints() const { return points; }
		const auto& GetPlane() const { return plane.value(); }
		Face& SetPlane(const float4& pl) {
			plane = pl;
			return *this;
		}

		bool Sanitize();
	private:
		void CondSetPlane();
	private:
		std::optional<float4> plane;
		std::vector<float3> points; // last == first
	};

	class Polygon {
	public:
		Face& AddFace() {
			return faces.emplace_back();
		}
		template<typename Iterable>
		Face& AddFace(Iterable&& iterable) {
			auto& face = faces.emplace_back();
			for (auto&& item : iterable) {
				face.AddPoint(std::forward<std::remove_cv_t<decltype(item)>>(item));
			}

			return face;
		}
		template<typename ... Item>
		Face& AddFace(Item&& ... item) {
			auto& face = faces.emplace_back();
			(face.AddPoint(std::forward<Item>(item)), ...);

			return face;
		}
		const auto& GetFaces() const { return faces; }
		Polygon& ClipByInPlace(const Polygon& pc);
		Polygon  ClipBy(const Polygon& pc) { Polygon p = *this; p.ClipByInPlace(pc); return p; }

		std::vector<std::pair<float3, float3>> GetAllLines() const;

		const float3 GetMiddlePos() const;
	private:
		std::vector<Face> faces;
	};
}

