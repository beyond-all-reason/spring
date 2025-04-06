#pragma once

#include <stdexcept>

namespace spring {
	template<typename D>
	class ScopedNullResource {
	public:
		template<typename C>
		ScopedNullResource(C&& c, D d_)
			:d{ std::move(d_) }
		{
			c();
		}
		~ScopedNullResource() {
			d();
		}
	private:
		D d;
	};

	template<typename R, typename D>
	class ScopedResource {
	public:
		using MyType = ScopedResource<R, D>;
		ScopedResource(R&& r_, D&& d_)
			: r{ std::forward<R>(r_) }
			, d{ std::forward<D>(d_) }
			, released{false}
		{}
		ScopedResource(const R& r_, D&& d_)
			: r{ r_ }
			, d{ std::forward<D>(d_) }
			, released{ false }
		{}
		~ScopedResource() {
			Reset();
		}

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
		MyType& operator=(const MyType&) = delete;
	private:
		R r;
		D d;

		bool released;
	};
}