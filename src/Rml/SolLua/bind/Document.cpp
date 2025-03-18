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

#include "../plugin/SolLuaDocument.h"


namespace Rml::SolLua
{

	namespace document
	{
		auto show(SolLuaDocument& self)
		{
			self.Show();
		}

		auto showModal(SolLuaDocument& self, Rml::ModalFlag modal)
		{
			self.Show(modal);
		}

		auto showModalFocus(SolLuaDocument& self, Rml::ModalFlag modal, Rml::FocusFlag focus)
		{
			self.Show(modal, focus);
		}

		auto reloadStyleSheet(SolLuaDocument& self)
		{
			self.ReloadStyleSheet();
		}

		auto reloadStyleSheetAndLoad(SolLuaDocument& self, bool load)
		{
			reloadStyleSheet(self);
			if (load)
			{
				// Dispatch the load event so we can re-bind any scripts that got wiped out.
				self.DispatchEvent(EventId::Load, Dictionary());
			}
		}

		auto loadInlineScript3(SolLuaDocument& self, const Rml::String& content, const Rml::String& source_path, int source_line)
		{
			self.LoadInlineScript(content, source_path, source_line);
		}

		auto loadInlineScript2(SolLuaDocument& self, const Rml::String& content, const Rml::String& source_path)
		{
			loadInlineScript3(self, content, source_path, 0);
		}

		auto loadInlineScript1(SolLuaDocument& self, const Rml::String& content)
		{
			loadInlineScript3(self, content, self.GetSourceURL(), 0);
		}

		auto appendToStyleSheet(SolLuaDocument& self, const Rml::String& content)
		{
			auto styleSheet = Rml::Factory::InstanceStyleSheetString(content);
			auto combined = styleSheet->CombineStyleSheetContainer(*self.GetStyleSheetContainer());
			self.SetStyleSheetContainer(std::move(combined));
		}

		auto getWidget(SolLuaDocument& self)
		{
			return self.GetLuaEnvironment()["widget"];
		}
	}

	void bind_document(sol::table& namespace_table)
	{
		namespace_table.new_enum<Rml::ModalFlag>("RmlModalFlag",
			{
				{ "None", Rml::ModalFlag::None },
				{ "Modal", Rml::ModalFlag::Modal },
				{ "Keep", Rml::ModalFlag::Keep }
			}
		);

		namespace_table.new_enum<Rml::FocusFlag>("RmlFocusFlag",
			{
				{ "None", Rml::FocusFlag::None },
				{ "Document", Rml::FocusFlag::Document },
				{ "Keep", Rml::FocusFlag::Keep },
				{ "Auto", Rml::FocusFlag::Auto }
			}
		);

		namespace_table.new_usertype<SolLuaDocument>("Document", sol::no_constructor,
			// M
			"PullToFront", &SolLuaDocument::PullToFront,
			"PushToBack", &SolLuaDocument::PushToBack,
			"Show", sol::overload(&document::show, &document::showModal, &document::showModalFocus),
			"Hide", &SolLuaDocument::Hide,
			"Close", &SolLuaDocument::Close,
			"CreateElement", &SolLuaDocument::CreateElement,
			"CreateTextNode", &SolLuaDocument::CreateTextNode,
			//--
			"ReloadStyleSheet", sol::overload(&document::reloadStyleSheet, &document::reloadStyleSheetAndLoad),
			"LoadInlineScript", sol::overload(&document::loadInlineScript1, &document::loadInlineScript2, &document::loadInlineScript3),
			"LoadExternalScript", &SolLuaDocument::LoadExternalScript,
			"UpdateDocument", &SolLuaDocument::UpdateDocument,
			"AppendToStyleSheet", &document::appendToStyleSheet,

			// G+S
			"title", sol::property(&SolLuaDocument::GetTitle, &SolLuaDocument::SetTitle),

			// G
			"context", sol::readonly_property(&SolLuaDocument::GetContext),
			//--
			"url", sol::readonly_property(&SolLuaDocument::GetSourceURL),
			"modal", sol::readonly_property(&SolLuaDocument::IsModal),
			"widget", sol::readonly_property(&document::getWidget),

			// B
			sol::base_classes, sol::bases<Rml::Element>()
		);
	}

} // end namespace Rml::SolLua
