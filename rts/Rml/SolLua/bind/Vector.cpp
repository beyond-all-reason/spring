#include "bind.h"


namespace Rml::SolLua
{

	void bind_vector(sol::state_view& lua)
	{
		lua.new_usertype<Rml::Vector2i>("Vector2i", sol::constructors<Rml::Vector2i(), Rml::Vector2i(int, int)>(),
			// O
			sol::meta_function::addition, &Rml::Vector2i::operator+,
			sol::meta_function::subtraction, sol::resolve<Rml::Vector2i(Vector2i) const>(&Rml::Vector2i::operator-),
			sol::meta_function::multiplication, sol::overload(
				sol::resolve<Rml::Vector2i(int) const>(&Rml::Vector2i::operator*),
				sol::resolve<Rml::Vector2i(Rml::Vector2i) const>(&Rml::Vector2i::operator*)
			),
			sol::meta_function::division, sol::overload(
				sol::resolve<Rml::Vector2i(int) const>(&Rml::Vector2i::operator/),
				sol::resolve<Rml::Vector2i(Rml::Vector2i) const>(&Rml::Vector2i::operator/)
			),
			sol::meta_function::unary_minus, sol::resolve<Rml::Vector2i() const>(&Rml::Vector2i::operator-),

			// G+S
			"x", &Rml::Vector2i::x,
			"y", &Rml::Vector2i::y,

			// G
			"magnitude", &Rml::Vector2i::Magnitude
		);

		lua.new_usertype<Rml::Vector2f>("Vector2f", sol::constructors<Rml::Vector2f(), Rml::Vector2f(float, float)>(),
			// O
			sol::meta_function::addition, &Rml::Vector2f::operator+,
			sol::meta_function::subtraction, sol::resolve<Rml::Vector2f(Vector2f) const>(&Rml::Vector2f::operator-),
			sol::meta_function::multiplication, sol::overload(
				sol::resolve<Rml::Vector2f(float) const>(&Rml::Vector2f::operator*),
				sol::resolve<Rml::Vector2f(Rml::Vector2f) const>(&Rml::Vector2f::operator*)
			),
			sol::meta_function::division, sol::overload(
				sol::resolve<Rml::Vector2f(float) const>(&Rml::Vector2f::operator/),
				sol::resolve<Rml::Vector2f(Rml::Vector2f) const>(&Rml::Vector2f::operator/)
			),
			sol::meta_function::unary_minus, sol::resolve<Rml::Vector2f() const>(&Rml::Vector2f::operator-),

			// G+S
			"x", &Rml::Vector2f::x,
			"y", &Rml::Vector2f::y,

			// G
			"magnitude", &Rml::Vector2f::Magnitude
		);
	}

} // end namespace Rml::SolLua
