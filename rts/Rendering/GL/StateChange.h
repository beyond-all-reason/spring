/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "State.h"

namespace GL
{

namespace Impl
{
template<class... ValuesUniqueTypes> class StateChange {
public:
	inline StateChange(ValuesUniqueTypes... newValues)
	{
		((std::get<ValuesUniqueTypes>(savedValues) = std::get<typename ValuesUniqueTypes::AttributeType>(State::Attributes)), ...);
		((std::get<typename ValuesUniqueTypes::AttributeType>(State::Attributes) = newValues), ...);
	}
	inline void pop()
	{
		if (pushed) {
			((std::get<typename ValuesUniqueTypes::AttributeType>(State::Attributes) = std::get<ValuesUniqueTypes>(savedValues)), ...);
			pushed = false;
		}
	}
	inline ~StateChange()
	{
		pop();
	}

private:
	std::tuple<ValuesUniqueTypes...> savedValues;
	bool pushed = true;
};
}

template<class... ArgTypes>
auto StateChange(ArgTypes&&... args)
{
	return Impl::StateChange<ArgTypes...>(args...);
}

namespace State { using GL::StateChange; }

}