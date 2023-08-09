/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "State.h"
#include "System/TemplateUtils.hpp"
#include <tuple>
#include <type_traits>
#include <utility>

namespace GL
{

namespace Impl
{
template<class... UniqueAttributeValueTypes> class SubState {
private:
	using ValuesType = std::tuple<UniqueAttributeValueTypes...>;

public:
	inline SubState(UniqueAttributeValueTypes... newValues)
	{
		((std::get<UniqueAttributeValueTypes>(savedValues) = std::get<typename UniqueAttributeValueTypes::AttributeType>(State::Attributes)), ...);
		((std::get<typename UniqueAttributeValueTypes::AttributeType>(State::Attributes) = newValues), ...);
	}

	template<class UniqueAttributeValueType, std::enable_if_t<spring::tuple_contains_type_v<ValuesType, UniqueAttributeValueType>, bool> = true>
	inline const SubState& operator<<(UniqueAttributeValueType newValue) const
	{
		if (pushed) {
			std::get<typename UniqueAttributeValueType::AttributeType>(State::Attributes) = newValue;
		}
		return *this;
	}

	inline void pop()
	{
		if (pushed) {
			((std::get<typename UniqueAttributeValueTypes::AttributeType>(State::Attributes) = std::get<UniqueAttributeValueTypes>(savedValues)), ...);
			pushed = false;
		}
	}

	inline ~SubState()
	{
		pop();
	}

private:
	ValuesType savedValues;
	bool pushed = true;
};
}

template<class... ArgTypes>
auto SubState(ArgTypes&&... args)
{
	return Impl::SubState<ArgTypes...>(std::forward<ArgTypes>(args)...);
}

}