/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/*
 * This source file is derived from the code 
 * at https://github.com/LoneBoco/RmlSolLua
 * which is under the following license:
 * 
 * MIT License
 *
 * Copyright (c) 2022 John Norman
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "bind.h"


namespace Rml::SolLua
{

	using ColourbTuple = std::tuple<Rml::byte, Rml::byte, Rml::byte, Rml::byte>;
	using ColourfTuple = std::tuple<float, float, float, float>;

	template <typename T, int A>
	std::tuple<T,T,T,T> getRGBA(Rml::Colour<T, A>& self)
	{
		return std::tuple<T,T,T,T>(self.red, self.green, self.blue, self.alpha);
	}

	template <typename T, int A>
	void setRGBA(Rml::Colour<T, A>& self, std::tuple<T,T,T,T> color)
	{
		sol::tie(self.red, self.green, self.blue, self.alpha) = color;
	}

	void bind_color(sol::table& bindings)
	{
		bindings.new_usertype<Rml::Colourb>("Colourb", sol::constructors<Rml::Colourb(), Rml::Colourb(Rml::byte, Rml::byte, Rml::byte), Rml::Colourb(Rml::byte, Rml::byte, Rml::byte, Rml::byte)>(),
			// O
			sol::meta_function::addition, &Rml::Colourb::operator+,
			sol::meta_function::subtraction, &Rml::Colourb::operator-,
			sol::meta_function::multiplication, &Rml::Colourb::operator*,
			sol::meta_function::division, &Rml::Colourb::operator/,
			sol::meta_function::equal_to, &Rml::Colourb::operator==,

			// G+S
			"red", &Rml::Colourb::red,
			"green", &Rml::Colourb::green,
			"blue", &Rml::Colourb::blue,
			"alpha", &Rml::Colourb::alpha,
			"rgba", sol::property(static_cast<ColourbTuple(*)(Rml::Colourb&)>(&getRGBA), static_cast<void(*)(Rml::Colourb&, ColourbTuple)>(&setRGBA))
		);

		bindings.new_usertype<Rml::Colourf>("Colourf", sol::constructors<Rml::Colourf(), Rml::Colourf(float, float, float), Rml::Colourf(float, float, float, float)>(),
			// O
			sol::meta_function::addition, &Rml::Colourf::operator+,
			sol::meta_function::subtraction, &Rml::Colourf::operator-,
			sol::meta_function::multiplication, &Rml::Colourf::operator*,
			sol::meta_function::division, &Rml::Colourf::operator/,
			sol::meta_function::equal_to, &Rml::Colourf::operator==,

			// G+S
			"red", &Rml::Colourf::red,
			"green", &Rml::Colourf::green,
			"blue", &Rml::Colourf::blue,
			"alpha", &Rml::Colourf::alpha,
			"rgba", sol::property(static_cast<ColourfTuple(*)(Rml::Colourf&)>(&getRGBA), static_cast<void(*)(Rml::Colourf&, ColourfTuple)>(&setRGBA))
		);
	}

} // end namespace Rml::SolLua
