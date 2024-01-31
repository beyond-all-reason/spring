#include "bind.h"

#include "../plugin/SolLuaDocument.h"


namespace Rml::SolLua
{
	namespace functions
	{
		template <typename T>
		T* convert(Rml::Element* element)
		{
			auto test = dynamic_cast<T*>(element);
			if (test == nullptr)
				return nullptr;

			return test;
		}
	}

	void bind_convert(sol::table& namespace_table)
	{
		#define CONVERSION(T) (static_cast<T*(*)(Rml::Element*)>(&functions::convert))
		namespace_table["Element"]["As"] = namespace_table.create_with(
			"Document", CONVERSION(SolLuaDocument),
			"ElementText", CONVERSION(Rml::ElementText),
			"ElementForm", CONVERSION(Rml::ElementForm),
			"ElementFormControl", CONVERSION(Rml::ElementFormControl),
			"ElementFormControlInput", CONVERSION(Rml::ElementFormControlInput),
			"ElementFormControlSelect", CONVERSION(Rml::ElementFormControlSelect),
			"ElementFormControlTextArea", CONVERSION(Rml::ElementFormControlTextArea),
			"ElementTabSet", CONVERSION(Rml::ElementTabSet)
		);
		#undef CONVERSION
	}

} // end namespace Rml::SolLua
