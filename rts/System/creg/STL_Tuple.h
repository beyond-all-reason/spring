#ifndef CR_STL_TUPLE_H
#define CR_STL_TUPLE_H

#include "creg_cond.h"
#include <tuple>
#include <functional>
#include <sstream>
#include "../TemplateUtils.hpp"

#ifdef USING_CREG

namespace creg
{
	template <typename ... Ts>
	class TupleType : public IType
	{
	public:
		using TT = std::tuple<Ts...>;
		static constexpr auto TTs = std::tuple_size_v<TT>;
		TupleType() : IType(sizeof(TT)) { }
		~TupleType() { }

		void Serialize(ISerializer* s, void* instance)
		{
			TT& t = *(TT*)instance;
			auto IndexDispatcher = spring::make_index_dispatcher<TTs>();
			// same code for IsWriting() and !IsWriting()
			const auto SerializeType = [s](auto&& value) {
				using T = std::decay_t<decltype(value)>;
				DeduceType<T>::Get()->Serialize(s, static_cast<void*>(&value));
			};
			IndexDispatcher([&SerializeType, &t](auto idx) { SerializeType(std::get<idx>(std::forward<TT>(t))); });
		}
		std::string GetName() const {
			std::ostringstream ss;
			ss << "tuple<";
			((ss << DeduceType<Ts>::Get()->GetName() + ","), ...);
			ss.seekp(-1, ss.cur);
			ss << ">";
			return ss.str();
		}
	};

	template<typename ... Ts>
	struct DeduceType<std::tuple<Ts ...> > {
		static std::unique_ptr<IType> Get() {
			return std::unique_ptr<IType>(new TupleType<Ts ...>());
		}
	};

}

#endif // USING_CREG

#endif // CR_STL_TUPLE_H
