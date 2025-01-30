#pragma once

#include <vector>
#include <optional>
#include <type_traits>
#include <functional>

#include "System/float4.h"
#include "System/AABB.hpp"

class CMatrix44f;

namespace Geometry {
	class Face {
	public:
		Face() = default;
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

		void FlipDirection();

		bool Sanitize();
	private:
		void CondSetPlane();
	private:
		std::optional<float4> plane;
		std::vector<float4> points;
	};

	class Polygon {
	public:
		Polygon() = default;

		Face& AddFace() {
			return faces.emplace_back();
		}
		void MakeFrom(const std::array<float3, 8>& points);
		void MakeFrom(const AABB& aabb) { const auto corners = aabb.GetCorners(); MakeFrom(corners); }
		void MakeFrom(const AABB& aabb, const CMatrix44f& mat) { const auto corners = aabb.GetCorners(mat); MakeFrom(corners); }
		template<typename Iterable>
		Face& AddFace(Iterable&& points) {
			auto& face = faces.emplace_back();
			for (auto&& point : points) {
				face.AddPoint(std::forward<std::remove_cv_t<decltype(point)>>(point));
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

		void FlipFacesDirection();

		Polygon& ClipByInPlace(const Polygon& pc);
		Polygon  ClipBy(const Polygon& pc) const { Polygon p = *this; p.ClipByInPlace(pc); return p; }

		std::vector<float3> GetAllLines() const;
		AABB GetAABB() const;
		AABB GetAABB(const CMatrix44f& mat) const;

		const float3 GetMiddlePos() const;
	private:
		std::vector<Face> faces;
	};
}