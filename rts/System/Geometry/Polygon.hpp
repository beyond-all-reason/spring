#pragma once

#include <vector>
#include <optional>
#include <type_traits>
#include <memory_resource>
#include <functional>

#include "System/float4.h"

namespace Geometry {
	class Allocator {
	public:
		Allocator();
		Allocator(size_t allocMemBytes);
		~Allocator();

		void ClearAllocations();

		auto& GetAllocator() { return allocator; }
	private:
		std::vector<std::byte> buffer;
		// std::pmr::monotonic_buffer_resource cannot be copied / moved
		// what else less insane could be done to store it?
		alignas(std::pmr::monotonic_buffer_resource) std::byte pmrMem[sizeof(std::pmr::monotonic_buffer_resource)];
		std::pmr::polymorphic_allocator<std::byte> allocator{ reinterpret_cast<std::pmr::monotonic_buffer_resource*>(&pmrMem[0])};
	};

	class Face {
	public:
		Face()
			: allocRef(std::ref(defaultAllocator))
		{}
		Face(Allocator& allocator_)
			: allocRef(std::ref(allocator_))
		{}
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
		Allocator defaultAllocator;
		std::reference_wrapper<Allocator> allocRef;

		std::optional<float4> plane;
		std::pmr::vector<float3> points{ allocRef.get().GetAllocator() };
	};

	class Polygon {
	public:
		Polygon()
			: allocRef(std::ref(defaultAllocator))
		{}
		Polygon(Allocator& allocator_)
			: allocRef(std::ref(allocator_))
		{}
		Face& AddFace() {
			return faces.emplace_back(allocRef.get());
		}
		template<typename Iterable>
		Face& AddFace(Iterable&& points) {
			auto& face = faces.emplace_back(allocRef.get());
			for (auto&& point : points) {
				face.AddPoint(std::forward<std::remove_cv_t<decltype(point)>>(point));
			}

			return face;
		}
		template<typename ... Item>
		Face& AddFace(Item&& ... item) {
			auto& face = faces.emplace_back(allocRef.get());
			(face.AddPoint(std::forward<Item>(item)), ...);

			return face;
		}
		const auto& GetFaces() const { return faces; }
		Polygon& ClipByInPlace(const Polygon& pc);
		Polygon  ClipBy(const Polygon& pc) { Polygon p = *this; p.ClipByInPlace(pc); return p; }

		std::vector<float3> GetAllLines() const;

		const float3 GetMiddlePos() const;
	private:
		Allocator defaultAllocator;
		std::reference_wrapper<Allocator> allocRef;

		std::pmr::vector<Face> faces{ allocRef.get().GetAllocator() };
	};
}