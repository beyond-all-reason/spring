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

		#define CONVERSION(T) static_cast<T*(*)(Rml::Element*)>(&functions::convert)
	}

	void bind_convert(sol::state_view& lua)
	{
		auto element = lua.create_named_table("Element");
		auto element_as = element.create_named("As");

		element_as["Document"] = CONVERSION(SolLuaDocument);
		element_as["ElementText"] = CONVERSION(Rml::ElementText);
		// element_as["ElementDataGrid"] = CONVERSION(Rml::ElementDataGrid);
		// element_as["ElementDataGridRow"] = CONVERSION(Rml::ElementDataGridRow);
		// element_as["ElementDataGridCell"] = CONVERSION(Rml::ElementDataGridCell);
		element_as["ElementForm"] = CONVERSION(Rml::ElementForm);
		element_as["ElementFormControl"] = CONVERSION(Rml::ElementFormControl);
		element_as["ElementFormControlInput"] = CONVERSION(Rml::ElementFormControlInput);
		element_as["ElementFormControlSelect"] = CONVERSION(Rml::ElementFormControlSelect);
		// element_as["ElementFormControlDataSelect"] = CONVERSION(Rml::ElementFormControlDataSelect);
		element_as["ElementFormControlTextArea"] = CONVERSION(Rml::ElementFormControlTextArea);
		element_as["ElementTabSet"] = CONVERSION(Rml::ElementTabSet);
	}

} // end namespace Rml::SolLua
