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

#include "SolLuaInstancer.h"

#include "SolLuaDocument.h"
#include "SolLuaEventListener.h"


namespace Rml::SolLua
{

	ElementPtr SolLuaDocumentElementInstancer::InstanceElement(Element* parent, const String& tag, const XMLAttributes& attributes)
	{
		return ElementPtr(new SolLuaDocument(m_state, tag, m_lua_env_identifier));
	}

	void SolLuaDocumentElementInstancer::ReleaseElement(Element* element)
	{
		delete element;
	}


	EventListener* SolLuaEventListenerInstancer::InstanceEventListener(const String& value, Element* element)
	{
		return new SolLuaEventListener(m_state, value, element);
	}

} // end namespace Rml::SolLua
