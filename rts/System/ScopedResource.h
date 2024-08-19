#pragma once

#include <stdexcept>

namespace spring {
	template<typename D>
	class ScopedNullResource {
	public:
		template<typename C>
		ScopedNullResource(C c, D d_)
			: d{ d_ }
		{
			c();
		}
		~ScopedNullResource() {
			d();
		}
		ScopedNullResource(ScopedNullResource&& rhs) noexcept = delete;
		ScopedNullResource& operator=(ScopedNullResource&& rhs) noexcept = delete;
		ScopedNullResource(const ScopedNullResource& rhs) = delete;
		ScopedNullResource& operator=(const ScopedNullResource& rhs) = delete;
	private:
		D d;
	};

	template<typename R, typename D>
	class ScopedResource {
	public:
		using MyType = ScopedResource<R, D>;
		ScopedResource(R&& r_, D d_)
			: r{ std::forward<R>(r_) }
			, d{ d_ }
			, released{false}
		{}
		ScopedResource(const R& r_, D d_)
			: r{ r_ }
			, d{ d_ }
			, released{ false }
		{}
		~ScopedResource() {
			Reset();
		}

		ScopedResource(ScopedResource&& rhs) noexcept = delete;
		ScopedResource& operator=(ScopedResource&& rhs) noexcept = delete;
		ScopedResource(const ScopedResource& rhs) = delete;
		ScopedResource& operator=(const ScopedResource& rhs) = delete;

		R&& Release() {
			if (released)
				throw std::runtime_error("Scoped Object already released");
			released = true;
			return std::move(r);
		}

		void Reset() {
			if (!released) d(r);
			released = true;
		}

		bool operator==(const MyType& rhs) { return  r == rhs.r; }
		bool operator!=(const MyType& rhs) { return !(r == rhs); }

		const R& operator->() const { return r; };
		      R& operator->()       { return r; };

		operator const R&() const { return r; };
		operator       R&()       { return r; };

		const R& Get() const { return r; };
		      R& Get()       { return r; };

		MyType& operator=(std::nullptr_t) noexcept { Reset(); return *this; }
	private:
		R r;
		D d;

		bool released;
	};
}