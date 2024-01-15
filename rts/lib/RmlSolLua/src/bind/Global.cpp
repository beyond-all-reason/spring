#include "bind.h"
#include "RmlSolLua/SolLuaPlugin.h"


namespace Rml::SolLua
{

	namespace functions
	{
		auto createContext(const Rml::String& name)
		{
			// Janky, but less jank than before at least
			// context will be resized right away, use 1080p dimensions
			return Rml::CreateContext(name, Rml::Vector2i(1920, 1080));
		}

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
	}

	#define _ENUM(N) t[#N] = Rml::Input::KI_##N

	void bind_global(sol::state_view& lua, SolLuaPlugin* slp)
	{

		struct rmlui {};
		auto translationTable = &slp->translationTable;

		auto g = lua.new_usertype<rmlui>("rmlui",
			// M
			"CreateContext", functions::createContext,
			"RemoveContext", sol::resolve<bool (const Rml::String&)>(&Rml::RemoveContext),
			"LoadFontFace", sol::overload(
				&functions::loadFontFace1,
				&functions::loadFontFace2,
				&functions::loadFontFace3
			),
			//"RegisterTag",
			//--
			"GetContext", sol::resolve<Rml::Context* (const Rml::String&)>(&Rml::GetContext),
			"RegisterEventType", sol::overload(&functions::registerEventType4, &functions::registerEventType3),
			"AddTranslationString", [translationTable](const Rml::String& key, const Rml::String& translation, sol::this_state s) {
				return translationTable->addTranslation(key, translation);
			},
			"ClearTranslations", [translationTable](sol::this_state s) {
				return translationTable->clear();
			},

			// G
			"contexts", sol::readonly_property(&getIndexedTable<Rml::Context, &functions::getContext, &functions::getMaxContexts>),
			//--
			"version", sol::readonly_property(&Rml::GetVersion)
		);
		g.set("key_identifier", sol::readonly_property([](sol::this_state l) {
			sol::state_view lua(l);
			sol::table t = lua.create_table();
			_ENUM(UNKNOWN);
			_ENUM(SPACE);
			_ENUM(0);
			_ENUM(1);
			_ENUM(2);
			_ENUM(3);
			_ENUM(4);
			_ENUM(5);
			_ENUM(6);
			_ENUM(7);
			_ENUM(8);
			_ENUM(9);
			_ENUM(A);
			_ENUM(B);
			_ENUM(C);
			_ENUM(D);
			_ENUM(E);
			_ENUM(F);
			_ENUM(G);
			_ENUM(H);
			_ENUM(I);
			_ENUM(J);
			_ENUM(K);
			_ENUM(L);
			_ENUM(M);
			_ENUM(N);
			_ENUM(O);
			_ENUM(P);
			_ENUM(Q);
			_ENUM(R);
			_ENUM(S);
			_ENUM(T);
			_ENUM(U);
			_ENUM(V);
			_ENUM(W);
			_ENUM(X);
			_ENUM(Y);
			_ENUM(Z);
			_ENUM(OEM_1);
			_ENUM(OEM_PLUS);
			_ENUM(OEM_COMMA);
			_ENUM(OEM_MINUS);
			_ENUM(OEM_PERIOD);
			_ENUM(OEM_2);
			_ENUM(OEM_3);
			_ENUM(OEM_4);
			_ENUM(OEM_5);
			_ENUM(OEM_6);
			_ENUM(OEM_7);
			_ENUM(OEM_8);
			_ENUM(OEM_102);
			_ENUM(NUMPAD0);
			_ENUM(NUMPAD1);
			_ENUM(NUMPAD2);
			_ENUM(NUMPAD3);
			_ENUM(NUMPAD4);
			_ENUM(NUMPAD5);
			_ENUM(NUMPAD6);
			_ENUM(NUMPAD7);
			_ENUM(NUMPAD8);
			_ENUM(NUMPAD9);
			_ENUM(NUMPADENTER);
			_ENUM(MULTIPLY);
			_ENUM(ADD);
			_ENUM(SEPARATOR);
			_ENUM(SUBTRACT);
			_ENUM(DECIMAL);
			_ENUM(DIVIDE);
			_ENUM(OEM_NEC_EQUAL);
			_ENUM(BACK);
			_ENUM(TAB);
			_ENUM(CLEAR);
			_ENUM(RETURN);
			_ENUM(PAUSE);
			_ENUM(CAPITAL);
			_ENUM(KANA);
			_ENUM(HANGUL);
			_ENUM(JUNJA);
			_ENUM(FINAL);
			_ENUM(HANJA);
			_ENUM(KANJI);
			_ENUM(ESCAPE);
			_ENUM(CONVERT);
			_ENUM(NONCONVERT);
			_ENUM(ACCEPT);
			_ENUM(MODECHANGE);
			_ENUM(PRIOR);
			_ENUM(NEXT);
			_ENUM(END);
			_ENUM(HOME);
			_ENUM(LEFT);
			_ENUM(UP);
			_ENUM(RIGHT);
			_ENUM(DOWN);
			_ENUM(SELECT);
			_ENUM(PRINT);
			_ENUM(EXECUTE);
			_ENUM(SNAPSHOT);
			_ENUM(INSERT);
			_ENUM(DELETE);
			_ENUM(HELP);
			_ENUM(LWIN);
			_ENUM(RWIN);
			_ENUM(APPS);
			_ENUM(POWER);
			_ENUM(SLEEP);
			_ENUM(WAKE);
			_ENUM(F1);
			_ENUM(F2);
			_ENUM(F3);
			_ENUM(F4);
			_ENUM(F5);
			_ENUM(F6);
			_ENUM(F7);
			_ENUM(F8);
			_ENUM(F9);
			_ENUM(F10);
			_ENUM(F11);
			_ENUM(F12);
			_ENUM(F13);
			_ENUM(F14);
			_ENUM(F15);
			_ENUM(F16);
			_ENUM(F17);
			_ENUM(F18);
			_ENUM(F19);
			_ENUM(F20);
			_ENUM(F21);
			_ENUM(F22);
			_ENUM(F23);
			_ENUM(F24);
			_ENUM(NUMLOCK);
			_ENUM(SCROLL);
			_ENUM(OEM_FJ_JISHO);
			_ENUM(OEM_FJ_MASSHOU);
			_ENUM(OEM_FJ_TOUROKU);
			_ENUM(OEM_FJ_LOYA);
			_ENUM(OEM_FJ_ROYA);
			_ENUM(LSHIFT);
			_ENUM(RSHIFT);
			_ENUM(LCONTROL);
			_ENUM(RCONTROL);
			_ENUM(LMENU);
			_ENUM(RMENU);
			_ENUM(BROWSER_BACK);
			_ENUM(BROWSER_FORWARD);
			_ENUM(BROWSER_REFRESH);
			_ENUM(BROWSER_STOP);
			_ENUM(BROWSER_SEARCH);
			_ENUM(BROWSER_FAVORITES);
			_ENUM(BROWSER_HOME);
			_ENUM(VOLUME_MUTE);
			_ENUM(VOLUME_DOWN);
			_ENUM(VOLUME_UP);
			_ENUM(MEDIA_NEXT_TRACK);
			_ENUM(MEDIA_PREV_TRACK);
			_ENUM(MEDIA_STOP);
			_ENUM(MEDIA_PLAY_PAUSE);
			_ENUM(LAUNCH_MAIL);
			_ENUM(LAUNCH_MEDIA_SELECT);
			_ENUM(LAUNCH_APP1);
			_ENUM(LAUNCH_APP2);
			_ENUM(OEM_AX);
			_ENUM(ICO_HELP);
			_ENUM(ICO_00);
			_ENUM(PROCESSKEY);
			_ENUM(ICO_CLEAR);
			_ENUM(ATTN);
			_ENUM(CRSEL);
			_ENUM(EXSEL);
			_ENUM(EREOF);
			_ENUM(PLAY);
			_ENUM(ZOOM);
			_ENUM(PA1);
			_ENUM(OEM_CLEAR);
			return t;
		}));

		g.set("key_modifier", sol::readonly_property([](sol::this_state l) {
			sol::state_view lua(l);
			return sol::table::create_with(lua.lua_state(),
				 "CTRL", Rml::Input::KM_CTRL ,
				 "SHIFT", Rml::Input::KM_SHIFT ,
				 "ALT", Rml::Input::KM_ALT ,
				 "META", Rml::Input::KM_META ,
				 "CAPSLOCK", Rml::Input::KM_CAPSLOCK ,
				 "NUMLOCK", Rml::Input::KM_NUMLOCK ,
				 "SCROLLLOCK", Rml::Input::KM_SCROLLLOCK
			);
		}));

		g.set("font_weight", sol::readonly_property([](sol::this_state l) {
			sol::state_view lua(l);
			return sol::table::create_with(lua.lua_state(),
				"Auto", Rml::Style::FontWeight::Auto,
				"Normal", Rml::Style::FontWeight::Normal,
				"Bold", Rml::Style::FontWeight::Bold
			);
		}));

		g.set("default_action_phase", sol::readonly_property([](sol::this_state l) {
			sol::state_view lua(l);
			return sol::table::create_with(lua.lua_state(),
				"None", Rml::DefaultActionPhase::None,
				"Target", Rml::DefaultActionPhase::Target,
				"TargetAndBubble", Rml::DefaultActionPhase::TargetAndBubble
			);
		}));
	}

	#undef _ENUM

} // end namespace Rml::SolLua
