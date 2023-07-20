#ifndef CR_STL_VARIANT_H
#define CR_STL_VARIANT_H

#include "creg_cond.h"
#include <variant>

#ifdef USING_CREG

namespace creg
{
	template <typename T0, typename T1, typename T2>
	class Variant3Type : public IType
	{
	public:
		using VT = std::variant<T0,T1,T2>;
		Variant3Type() : IType(sizeof(VT)) { }
		~Variant3Type() { }

		void Serialize(ISerializer* s, void* instance)
		{
			VT& p = *(VT*)instance;
			if (s->IsWriting()) {
				int index = p.index();
				s->SerializeInt(&index, sizeof(int));
				switch (index) {
					case 0: DeduceType<T0>::Get()->Serialize(s, (void*) &(std::get<0>(p))); break;
					case 1: DeduceType<T1>::Get()->Serialize(s, (void*) &(std::get<1>(p))); break;
					case 2: DeduceType<T2>::Get()->Serialize(s, (void*) &(std::get<2>(p))); break;
				}
			} else {
				int index;
				s->SerializeInt(&index, sizeof(int));
				switch (index) {
					case 0: {
						T0 x;
						DeduceType<T0>::Get()->Serialize(s, (void*) &x);
						p = std::move(x); // neither `p.emplace <T0>` nor `<0>` worked; still, should be safe
					} break;
					case 1: {
						T1 x;
						DeduceType<T1>::Get()->Serialize(s, (void*) &x);
						p = std::move(x);
					} break;
					case 2: {
						T2 x;
						DeduceType<T2>::Get()->Serialize(s, (void*) &x);
						p = std::move(x);
					} break;
				}
			}
		}
		std::string GetName() const { return "variant<"
			+ DeduceType<T0>::Get()->GetName() + ","
			+ DeduceType<T1>::Get()->GetName() + ","
			+ DeduceType<T2>::Get()->GetName() + ">";
		}
	};

	/* FIXME: ideally this would support arbitrary variants, but that involves some
	 * recursive variadic template bullshit. Three just happened to be the first use case. */
	template<typename T0, typename T1, typename T2>
	struct DeduceType<std::variant<T0,T1,T2> > {
		static std::unique_ptr<IType> Get() {
			return std::unique_ptr<IType>(new Variant3Type<T0,T1,T2>());
		}
	};
}

#endif // USING_CREG

#endif // CR_STL_VARIANT_H
