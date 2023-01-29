/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/*
 * creg - Code compoment registration system
 * Implementations of IType for specific types
 */

#ifndef CR_VARIABLE_TYPES_H
#define CR_VARIABLE_TYPES_H

#include "creg_cond.h"

namespace creg
{
	class ObjectInstanceType : public IType
	{
	public:
		ObjectInstanceType(Class* objc, size_t size) : IType(size), objectClass(objc) {}
		~ObjectInstanceType() {}
		void Serialize(ISerializer* s, void* instance);
		std::string GetName() const;

		Class* objectClass;
	};

	class StringType : public DynamicArrayType<std::string>
	{
	public:
		StringType()
			: DynamicArrayType<std::string>(GCC_STRING_SIZE)
		{
			#ifndef _MSC_VER
			static_assert(sizeof(std::string) == GCC_STRING_SIZE);
			#endif
		}
		std::string GetName() const;
		static constexpr size_t GCC_STRING_SIZE = 32;
	};

}

#endif

