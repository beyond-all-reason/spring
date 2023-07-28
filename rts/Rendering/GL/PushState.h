/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "State.h"
#include <tuple>
#include <utility>

namespace GL
{

namespace Impl
{
template<class... UniqueAttributeValueTypes> class PushState {
public:
	inline PushState(UniqueAttributeValueTypes... newValues)
	{
		((std::get<UniqueAttributeValueTypes>(savedValues) = std::get<typename UniqueAttributeValueTypes::AttributeType>(State::Attributes)), ...);
		((std::get<typename UniqueAttributeValueTypes::AttributeType>(State::Attributes) = newValues), ...);
	}
	inline void pop()
	{
		if (pushed) {
			((std::get<typename UniqueAttributeValueTypes::AttributeType>(State::Attributes) = std::get<UniqueAttributeValueTypes>(savedValues)), ...);
			pushed = false;
		}
	}
	inline ~PushState()
	{
		pop();
	}

private:
	std::tuple<UniqueAttributeValueTypes...> savedValues;
	bool pushed = true;
};
}

template<class... ArgTypes>
auto PushState(ArgTypes&&... args)
{
	return Impl::PushState<ArgTypes...>(std::forward<ArgTypes>(args)...);
}

}