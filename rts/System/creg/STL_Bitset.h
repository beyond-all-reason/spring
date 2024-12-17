#pragma once

#include "creg_cond.h"
#include <bitset>
#include <bit>
#include <array>
#include <cstdint>
#include "../TemplateUtils.hpp"

#ifdef USING_CREG

namespace creg
{
	template<size_t N>
	class BitsetType : public IType
	{
	public:
		using T = std::bitset<N>;
		BitsetType() : IType(sizeof(T)) { }
		~BitsetType() { }

		void Serialize(ISerializer* s, void* instance)
		{
			T& ct = *(T*)instance;

			std::array<uint8_t, sizeof(T)> bytesRep = {0};

			if (s->IsWriting())
				bytesRep = std::bit_cast<decltype(bytesRep)>(ct);

			for (auto& b : bytesRep) {
				s->SerializeInt(b, sizeof(b));
			}

			if (!s->IsWriting())
				ct = std::bit_cast<T>(bytesRep);
		}
		std::string GetName() const {
			return std::string("bitset<" + std::to_string(N) + ">");
		}
	};

	template<size_t N>
	struct DeduceType<std::bitset<N> > {
		static std::unique_ptr<IType> Get() {
			return std::unique_ptr<IType>(new BitsetType<N>());
		}
	};
}

#endif // USING_CREG