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
#include "../plugin//SolLuaPlugin.h"
#include "Rml/Backends/RmlUi_Backend.h"


namespace Rml::SolLua
{

	namespace functions
	{
		auto getContext()
		{
			std::function<Rml::Context* (int)> result = [](int idx) { return Rml::GetContext(idx); };
			return result;
		}

		auto getMaxContexts()
		{
			std::function<int ()> result = []() { return Rml::GetNumContexts(); };
			return result;
		}

		auto loadFontFace1(const Rml::String& file)
		{
			return Rml::LoadFontFace(file);
		}

		auto loadFontFace2(const Rml::String& file, bool fallback)
		{
			return Rml::LoadFontFace(file, fallback);
		}

		auto loadFontFace3(const Rml::String& file, bool fallback, Rml::Style::FontWeight weight)
		{
			return Rml::LoadFontFace(file, fallback, weight);
		}

		auto registerEventType4(const Rml::String& type, bool interruptible, bool bubbles, Rml::DefaultActionPhase default_action_phase)
		{
			return Rml::RegisterEventType(type, interruptible, bubbles, default_action_phase);
		}

		auto registerEventType3(const Rml::String& type, bool interruptible, bool bubbles)
		{
			return Rml::RegisterEventType(type, interruptible, bubbles, Rml::DefaultActionPhase::None);
		}
		
		auto removeContext(Rml::Context* context) {
			RmlGui::MarkContextForRemoval(context);
		}

		auto removeContextByName(const Rml::String& name) {
			auto context = Rml::GetContext(name);
			if (context != nullptr) {
				RmlGui::MarkContextForRemoval(context);
			}
		}

		auto setDebugContext(Rml::Context* context) {
			RmlGui::SetDebugContext(context);
		}

		auto setDebugContextByName(const Rml::String& name) {
			auto context = Rml::GetContext(name);
			if (context != nullptr) {
				RmlGui::SetDebugContext(context);
			}
		}
	}

	void bind_global(sol::table& namespace_table, SolLuaPlugin* slp)
	{
		auto translationTable = &slp->translationTable;
		namespace_table.set(
			// M
			"CreateContext", [slp](const Rml::String& name) {
				// context will be resized right away by other code
				// send {0, 0} in to avoid triggering a pointless resize event in the Rml code
				auto context = RmlGui::GetOrCreateContext(name);
				if (context != nullptr) {
					slp->AddContextTracking(context);
				}
				return context;
			},
			"RemoveContext", sol::overload(
				&functions::removeContext,
				&functions::removeContextByName
			),
			"LoadFontFace", sol::overload(
				&functions::loadFontFace1,
				&functions::loadFontFace2,
				&functions::loadFontFace3
			),
			//"RegisterTag",
			//--
			"GetContext", sol::resolve<Rml::Context* (const Rml::String&)>(&RmlGui::GetContext),
			"RegisterEventType", sol::overload(&functions::registerEventType4, &functions::registerEventType3),
			"AddTranslationString", [translationTable](const Rml::String& key, const Rml::String& translation, sol::this_state s) {
				return translationTable->addTranslation(key, translation);
			},
			"ClearTranslations", [translationTable](sol::this_state s) {
				return translationTable->clear();
			},
			"SetMouseCursorAlias", &RmlGui::SetMouseCursorAlias,
			"SetDebugContext", sol::overload(&functions::setDebugContext, &functions::setDebugContextByName),

			// G
			"contexts", sol::readonly_property(&getIndexedTable<Rml::Context, &functions::getContext, &functions::getMaxContexts>),
			//--
			"version", sol::readonly_property(&Rml::GetVersion)
		);

		namespace_table.set("key_identifier", sol::readonly_property([](sol::this_state l) {
			sol::state_view lua(l);
			sol::table t = lua.create_table();

			#define KEY_ENUM(N) t[#N] = Rml::Input::KI_##N
			KEY_ENUM(UNKNOWN);
			KEY_ENUM(SPACE);
			KEY_ENUM(0);
			KEY_ENUM(1);
			KEY_ENUM(2);
			KEY_ENUM(3);
			KEY_ENUM(4);
			KEY_ENUM(5);
			KEY_ENUM(6);
			KEY_ENUM(7);
			KEY_ENUM(8);
			KEY_ENUM(9);
			KEY_ENUM(A);
			KEY_ENUM(B);
			KEY_ENUM(C);
			KEY_ENUM(D);
			KEY_ENUM(E);
			KEY_ENUM(F);
			KEY_ENUM(G);
			KEY_ENUM(H);
			KEY_ENUM(I);
			KEY_ENUM(J);
			KEY_ENUM(K);
			KEY_ENUM(L);
			KEY_ENUM(M);
			KEY_ENUM(N);
			KEY_ENUM(O);
			KEY_ENUM(P);
			KEY_ENUM(Q);
			KEY_ENUM(R);
			KEY_ENUM(S);
			KEY_ENUM(T);
			KEY_ENUM(U);
			KEY_ENUM(V);
			KEY_ENUM(W);
			KEY_ENUM(X);
			KEY_ENUM(Y);
			KEY_ENUM(Z);
			KEY_ENUM(OEM_1);
			KEY_ENUM(OEM_PLUS);
			KEY_ENUM(OEM_COMMA);
			KEY_ENUM(OEM_MINUS);
			KEY_ENUM(OEM_PERIOD);
			KEY_ENUM(OEM_2);
			KEY_ENUM(OEM_3);
			KEY_ENUM(OEM_4);
			KEY_ENUM(OEM_5);
			KEY_ENUM(OEM_6);
			KEY_ENUM(OEM_7);
			KEY_ENUM(OEM_8);
			KEY_ENUM(OEM_102);
			KEY_ENUM(NUMPAD0);
			KEY_ENUM(NUMPAD1);
			KEY_ENUM(NUMPAD2);
			KEY_ENUM(NUMPAD3);
			KEY_ENUM(NUMPAD4);
			KEY_ENUM(NUMPAD5);
			KEY_ENUM(NUMPAD6);
			KEY_ENUM(NUMPAD7);
			KEY_ENUM(NUMPAD8);
			KEY_ENUM(NUMPAD9);
			KEY_ENUM(NUMPADENTER);
			KEY_ENUM(MULTIPLY);
			KEY_ENUM(ADD);
			KEY_ENUM(SEPARATOR);
			KEY_ENUM(SUBTRACT);
			KEY_ENUM(DECIMAL);
			KEY_ENUM(DIVIDE);
			KEY_ENUM(OEM_NEC_EQUAL);
			KEY_ENUM(BACK);
			KEY_ENUM(TAB);
			KEY_ENUM(CLEAR);
			KEY_ENUM(RETURN);
			KEY_ENUM(PAUSE);
			KEY_ENUM(CAPITAL);
			KEY_ENUM(KANA);
			KEY_ENUM(HANGUL);
			KEY_ENUM(JUNJA);
			KEY_ENUM(FINAL);
			KEY_ENUM(HANJA);
			KEY_ENUM(KANJI);
			KEY_ENUM(ESCAPE);
			KEY_ENUM(CONVERT);
			KEY_ENUM(NONCONVERT);
			KEY_ENUM(ACCEPT);
			KEY_ENUM(MODECHANGE);
			KEY_ENUM(PRIOR);
			KEY_ENUM(NEXT);
			KEY_ENUM(END);
			KEY_ENUM(HOME);
			KEY_ENUM(LEFT);
			KEY_ENUM(UP);
			KEY_ENUM(RIGHT);
			KEY_ENUM(DOWN);
			KEY_ENUM(SELECT);
			KEY_ENUM(PRINT);
			KEY_ENUM(EXECUTE);
			KEY_ENUM(SNAPSHOT);
			KEY_ENUM(INSERT);
			KEY_ENUM(DELETE);
			KEY_ENUM(HELP);
			KEY_ENUM(LWIN);
			KEY_ENUM(RWIN);
			KEY_ENUM(APPS);
			KEY_ENUM(POWER);
			KEY_ENUM(SLEEP);
			KEY_ENUM(WAKE);
			KEY_ENUM(F1);
			KEY_ENUM(F2);
			KEY_ENUM(F3);
			KEY_ENUM(F4);
			KEY_ENUM(F5);
			KEY_ENUM(F6);
			KEY_ENUM(F7);
			KEY_ENUM(F8);
			KEY_ENUM(F9);
			KEY_ENUM(F10);
			KEY_ENUM(F11);
			KEY_ENUM(F12);
			KEY_ENUM(F13);
			KEY_ENUM(F14);
			KEY_ENUM(F15);
			KEY_ENUM(F16);
			KEY_ENUM(F17);
			KEY_ENUM(F18);
			KEY_ENUM(F19);
			KEY_ENUM(F20);
			KEY_ENUM(F21);
			KEY_ENUM(F22);
			KEY_ENUM(F23);
			KEY_ENUM(F24);
			KEY_ENUM(NUMLOCK);
			KEY_ENUM(SCROLL);
			KEY_ENUM(OEM_FJ_JISHO);
			KEY_ENUM(OEM_FJ_MASSHOU);
			KEY_ENUM(OEM_FJ_TOUROKU);
			KEY_ENUM(OEM_FJ_LOYA);
			KEY_ENUM(OEM_FJ_ROYA);
			KEY_ENUM(LSHIFT);
			KEY_ENUM(RSHIFT);
			KEY_ENUM(LCONTROL);
			KEY_ENUM(RCONTROL);
			KEY_ENUM(LMENU);
			KEY_ENUM(RMENU);
			KEY_ENUM(BROWSER_BACK);
			KEY_ENUM(BROWSER_FORWARD);
			KEY_ENUM(BROWSER_REFRESH);
			KEY_ENUM(BROWSER_STOP);
			KEY_ENUM(BROWSER_SEARCH);
			KEY_ENUM(BROWSER_FAVORITES);
			KEY_ENUM(BROWSER_HOME);
			KEY_ENUM(VOLUME_MUTE);
			KEY_ENUM(VOLUME_DOWN);
			KEY_ENUM(VOLUME_UP);
			KEY_ENUM(MEDIA_NEXT_TRACK);
			KEY_ENUM(MEDIA_PREV_TRACK);
			KEY_ENUM(MEDIA_STOP);
			KEY_ENUM(MEDIA_PLAY_PAUSE);
			KEY_ENUM(LAUNCH_MAIL);
			KEY_ENUM(LAUNCH_MEDIA_SELECT);
			KEY_ENUM(LAUNCH_APP1);
			KEY_ENUM(LAUNCH_APP2);
			KEY_ENUM(OEM_AX);
			KEY_ENUM(ICO_HELP);
			KEY_ENUM(ICO_00);
			KEY_ENUM(PROCESSKEY);
			KEY_ENUM(ICO_CLEAR);
			KEY_ENUM(ATTN);
			KEY_ENUM(CRSEL);
			KEY_ENUM(EXSEL);
			KEY_ENUM(EREOF);
			KEY_ENUM(PLAY);
			KEY_ENUM(ZOOM);
			KEY_ENUM(PA1);
			KEY_ENUM(OEM_CLEAR);
			#undef KEY_ENUM

			return t;
		}));

		namespace_table.set("key_modifier", sol::readonly_property([](sol::this_state l) {
			sol::state_view lua(l);
			return lua.create_table_with(
				 "CTRL", Rml::Input::KM_CTRL,
				 "SHIFT", Rml::Input::KM_SHIFT,
				 "ALT", Rml::Input::KM_ALT,
				 "META", Rml::Input::KM_META,
				 "CAPSLOCK", Rml::Input::KM_CAPSLOCK,
				 "NUMLOCK", Rml::Input::KM_NUMLOCK,
				 "SCROLLLOCK", Rml::Input::KM_SCROLLLOCK
			);
		}));

		namespace_table.set("font_weight", sol::readonly_property([](sol::this_state l) {
			sol::state_view lua(l);
			return lua.create_table_with(
				"Auto", Rml::Style::FontWeight::Auto,
				"Normal", Rml::Style::FontWeight::Normal,
				"Bold", Rml::Style::FontWeight::Bold
			);
		}));

		namespace_table.set("default_action_phase", sol::readonly_property([](sol::this_state l) {
			sol::state_view lua(l);
			return lua.create_table_with(
				"None", Rml::DefaultActionPhase::None,
				"Target", Rml::DefaultActionPhase::Target,
				"TargetAndBubble", Rml::DefaultActionPhase::TargetAndBubble
			);
		}));
	}

} // end namespace Rml::SolLua
