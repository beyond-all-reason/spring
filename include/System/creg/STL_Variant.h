#ifndef CR_STL_VARIANT_H
#define CR_STL_VARIANT_H

#include "creg_cond.h"
#include <variant>
#include <functional>
#include <sstream>

#ifdef USING_CREG

namespace {
	// Calls your func with Variant element.
	// Updated https://stackoverflow.com/a/58674921/9819318
	template <typename Func, typename Variant, size_t N = 0>
	void runtime_get(Func&& func, Variant& var, size_t idx) {
		if (N == idx) {
			std::invoke(func, std::get<N>(var));
			return;
		}

		if constexpr (N + 1 < std::variant_size_v<Variant>) {
			return runtime_get<Func, Variant, N + 1>(func, var, idx);
		}
	}
	template <typename Func, typename Variant, size_t N = 0>
	void runtime_set(Func&& func, Variant& var, size_t idx) {
		if (N == idx) {
			using T = std::variant_alternative_t<N, Variant>;
			T input;
			std::invoke(func, input);
			var.template emplace <N> (std::move(input));
			return;
		}

		if constexpr (N + 1 < std::variant_size_v<Variant>) {
			return runtime_set<Func, Variant, N + 1>(func, var, idx);
		}
	}
}

namespace creg
{
	template <typename ... Ts>
	class VariantType : public IType
	{
	public:
		using VT = std::variant<Ts...>;
		VariantType() : IType(sizeof(VT)) { }
		~VariantType() { }

		void Serialize(ISerializer* s, void* instance)
		{
			VT& p = *(VT*)instance;
			const auto SerializeType = [s](auto&& input) {
				using T = std::decay_t<decltype(input)>;
				DeduceType<T>::Get()->Serialize(s, static_cast<void*>(&input));
			};
			if (s->IsWriting()) {
				auto index = p.index();
				s->SerializeInt(&index, sizeof(index));
				runtime_get(SerializeType, p, index);
			} else {
				std::size_t index;
				s->SerializeInt(&index, sizeof(index));
				runtime_set(SerializeType, p, index);
			}
		}
		std::string GetName() const {
			std::ostringstream ss;
			ss << "variant<";
			((ss << DeduceType<Ts>::Get()->GetName() + ","), ...);
			ss.seekp(-1, ss.cur);
			ss << ">";
			return ss.str();
		}
	};

	template<typename ... Ts>
	struct DeduceType<std::variant<Ts ...> > {
		static std::unique_ptr<IType> Get() {
			return std::unique_ptr<IType>(new VariantType<Ts ...>());
		}
	};

}

#endif // USING_CREG

#endif // CR_STL_VARIANT_H
