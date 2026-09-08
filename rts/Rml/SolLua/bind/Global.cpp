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

		 auto loadFontFace4(const Rml::String& file, bool fallback, Rml::Style::FontWeight weight, int face_index)
		 {
			 return Rml::LoadFontFace(file, fallback, weight, face_index);
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
		/***
		 * Represents an owned element.
		 *
		 * This type is mainly used to modify the DOM tree by passing the object into other elements. For example `RmlUi.Element:AppendChild()`.
		 * A current limitation in the Lua plugin is that Element member properties and functions cannot be used directly on this type.
		 * @class RmlUi.ElementPtr
		 */

		/***
		 * @alias RmlUi.MouseButton
		 * | 0 # Left button
		 * | 1 # Right button
		 * | 2 # Middle button
		 */

		/***
		 * Contains a list of all child elements.
		 * @alias RmlUi.ElementChildNodesProxy RmlUi.Element[]
		 */

		/***
		 * Contains all the attributes of an element: The stuff in the opening tag i.e. `<span class="my-class">`
		 * @alias RmlUi.ElementAttributesProxy {[string]: string|number|boolean}
		 */

		/***
		 * Gets the rcss styles associated with an element. As far as I can tell, the values will always be a string.
		 * @alias RmlUi.ElementStyleProxy { [string]: string }
		 */

		 auto translationTable = &slp->translationTable;
		 namespace_table.set(
			 /***
			  * Create a new context.
			  *
			  * @function RmlUi.CreateContext
			  *
			  * @param name string
			  * @return RmlUi.Context? nil if failed.
			  */
			 "CreateContext", [slp](const Rml::String& name) {
				 // context will be resized right away by other code
				 // send {0, 0} in to avoid triggering a pointless resize event in the Rml code
				 auto context = RmlGui::GetOrCreateContext(name);
				 if (context != nullptr) {
					 slp->AddContextTracking(context);
				 }
				 return context;
			 },
			 /***
			  * Remove a context.
			  *
			  * @function RmlUi.RemoveContext
			  *
			  * @param context string|RmlUi.Context
			  */
			 "RemoveContext", sol::overload(
				 &functions::removeContext,
				 &functions::removeContextByName
			 ),
			 /***
			  * Load a font face.
			  *
			  * @function RmlUi.LoadFontFace
			  *
			  * @param file_path string
			  * @param fallback boolean?
			  * @param weight RmlUi.font_weight?
			  * @param face_index number? The index of the font face within a font collection.
			  * @return boolean success
			  */
			 "LoadFontFace", sol::overload(
				 &functions::loadFontFace1,
				 &functions::loadFontFace2,
				 &functions::loadFontFace3,
				 &functions::loadFontFace4
			 ),
			 //"RegisterTag",
			 /***
			  * Get a context by name.
			  *
			  * @function RmlUi.GetContext
			  *
			  * @param name string
			  * @return RmlUi.Context? nil if failed.
			  */
			 "GetContext", sol::resolve<Rml::Context* (const Rml::String&)>(&RmlGui::GetContext),

			 /***
			  * @alias RmlUi.EventID
			  * | 0 # Invalid
			  * | 1 # Mousedown
			  * | 2 # Mousescroll
			  * | 3 # Mouseover
			  * | 4 # Mouseout
			  * | 5 # Focus
			  * | 6 # Blur
			  * | 7 # Keydown
			  * | 8 # Keyup
			  * | 9 # Textinput
			  * | 10 # Mouseup
			  * | 11 # Click
			  * | 12 # Dblclick
			  * | 13 # Load
			  * | 14 # Unload
			  * | 15 # Show
			  * | 16 # Hide
			  * | 17 # Mousemove
			  * | 18 # Dragmove
			  * | 19 # Drag
			  * | 20 # Dragstart
			  * | 21 # Dragover
			  * | 22 # Dragdrop
			  * | 23 # Dragout
			  * | 24 # Dragend
			  * | 25 # Handledrag
			  * | 26 # Resize
			  * | 27 # Scroll
			  * | 28 # Animationend
			  * | 29 # Transitionend
			  * | 30 # Change
			  * | 31 # Submit
			  * | 32 # Tabchange
			  * | 33 # NumDefinedIDs
			  * | integer # Custom ID
			  */

			 /***
			  * Register a new event type.
			  *
			  * @function RmlUi.RegiserEventType
			  *
			  * @param event_type string
			  * @param interruptible boolean?
			  * @param bubbles boolean?
			  * @param default_phase RmlUi.default_action_phase?
			  * @return RmlUi.EventID
			  */
			 "RegisterEventType", sol::overload(&functions::registerEventType4, &functions::registerEventType3),
			 /***
			  * Add a translation string.
			  *
			  * @function RmlUi.AddTranslationString
			  *
			  * @param key string
			  * @param translation string
			  * @return boolean success
			  */
			 "AddTranslationString", [translationTable](const Rml::String& key, const Rml::String& translation, sol::this_state s) {
				 return translationTable->addTranslation(key, translation);
			 },
			 /***
			  * Clear registered translations.
			  *
			  * @function RmlUi.ClearTranslations
			  */
			 "ClearTranslations", [translationTable](sol::this_state s) {
				 return translationTable->clear();
			 },
			 /***
			  * Converts the css names for cursors to the Recoil Engine names for cursors like `RmlUi.SetMouseCursorAlias("pointer", 'Move')`.
			  * Web devs tend to want to use specific words for pointer types.
			  *
			  * @function RmlUi.SetMouseCursorAlias
			  *
			  * @param rml_name string name used in rml script
			  * @param recoil_name string name used in recoil
			  */
			 "SetMouseCursorAlias", &RmlGui::SetMouseCursorAlias,
			 /***
			  * Set which context the debug inspector is meant to inspect.
			  *
			  * @function RmlUi.SetDebugContext
			  *
			  * @param context string | RmlUi.Context
			  */
			 "SetDebugContext", sol::overload(&functions::setDebugContext, &functions::setDebugContextByName),

			 // G
			 /*** @field RmlUi.contexts RmlUi.Context[] */
			 "contexts", sol::readonly_property(&getIndexedTable<Rml::Context, &functions::getContext, &functions::getMaxContexts>),
			 //--
			 /*** @field RmlUi.version string RmlUi version */
			 "version", sol::readonly_property(&Rml::GetVersion)
		 );
		namespace_table.set_function(
			/***
			* Clear the list of paths requested for a document for the purpose of GetDocumentPathRequests.
			* Does not clear anything else related to those requests themselves.
			*
			* @function RmlUi.ClearDocumentPathRequests
			*
			* @param document_path string path of document
			*/
			"ClearDocumentPathRequests", [slp](const std::string& document_path) {
				slp->systemInterface->ClearDocumentPathRequests(document_path);
			}
		);
		namespace_table.set_function(
			/***
			* Retrieve the list of paths requested by the loaded RML document since the most recent RmlUi.ClearDocumentPathRequests call.
			*
			* @function RmlUi.GetDocumentPathRequests
			*
			* @param document_path string path of document
			* @return string[] array of paths requested by the document
			*/
			"GetDocumentPathRequests", [slp](const std::string& document_path) {
				return sol::as_table(slp->systemInterface->GetDocumentPathRequests(document_path));
		 	}
		);
		/***
		 * @enum RmlUi.key_identifier
		 * @field ["0"] integer
		 * @field ["1"] integer
		 * @field ["2"] integer
		 * @field ["3"] integer
		 * @field ["4"] integer
		 * @field ["5"] integer
		 * @field ["6"] integer
		 * @field ["7"] integer
		 * @field ["8"] integer
		 * @field ["9"] integer
		 */
		 namespace_table.set("key_identifier", sol::readonly_property([](sol::this_state l) {
			sol::state_view lua(l);
			sol::table t = lua.create_table();

			#define KEY_ENUM(N) t[#N] = Rml::Input::KI_##N
			/*** @field RmlUi.key_identifier.UNKNOWN integer */
			KEY_ENUM(UNKNOWN);
			/*** @field RmlUi.key_identifier.SPACE integer */
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
			/*** @field RmlUi.key_identifier.A integer */
			KEY_ENUM(A);
			/*** @field RmlUi.key_identifier.B integer */
			KEY_ENUM(B);
			/*** @field RmlUi.key_identifier.C integer */
			KEY_ENUM(C);
			/*** @field RmlUi.key_identifier.D integer */
			KEY_ENUM(D);
			/*** @field RmlUi.key_identifier.E integer */
			KEY_ENUM(E);
			/*** @field RmlUi.key_identifier.F integer */
			KEY_ENUM(F);
			/*** @field RmlUi.key_identifier.G integer */
			KEY_ENUM(G);
			/*** @field RmlUi.key_identifier.H integer */
			KEY_ENUM(H);
			/*** @field RmlUi.key_identifier.I integer */
			KEY_ENUM(I);
			/*** @field RmlUi.key_identifier.J integer */
			KEY_ENUM(J);
			/*** @field RmlUi.key_identifier.K integer */
			KEY_ENUM(K);
			/*** @field RmlUi.key_identifier.L integer */
			KEY_ENUM(L);
			/*** @field RmlUi.key_identifier.M integer */
			KEY_ENUM(M);
			/*** @field RmlUi.key_identifier.N integer */
			KEY_ENUM(N);
			/*** @field RmlUi.key_identifier.O integer */
			KEY_ENUM(O);
			/*** @field RmlUi.key_identifier.P integer */
			KEY_ENUM(P);
			/*** @field RmlUi.key_identifier.Q integer */
			KEY_ENUM(Q);
			/*** @field RmlUi.key_identifier.R integer */
			KEY_ENUM(R);
			/*** @field RmlUi.key_identifier.S integer */
			KEY_ENUM(S);
			/*** @field RmlUi.key_identifier.T integer */
			KEY_ENUM(T);
			/*** @field RmlUi.key_identifier.U integer */
			KEY_ENUM(U);
			/*** @field RmlUi.key_identifier.V integer */
			KEY_ENUM(V);
			/*** @field RmlUi.key_identifier.W integer */
			KEY_ENUM(W);
			/*** @field RmlUi.key_identifier.X integer */
			KEY_ENUM(X);
			/*** @field RmlUi.key_identifier.Y integer */
			KEY_ENUM(Y);
			/*** @field RmlUi.key_identifier.Z integer */
			KEY_ENUM(Z);
			/*** @field RmlUi.key_identifier.OEM_1 integer */
			KEY_ENUM(OEM_1);
			/*** @field RmlUi.key_identifier.OEM_PLUS integer */
			KEY_ENUM(OEM_PLUS);
			/*** @field RmlUi.key_identifier.OEM_COMMA integer */
			KEY_ENUM(OEM_COMMA);
			/*** @field RmlUi.key_identifier.OEM_MINUS integer */
			KEY_ENUM(OEM_MINUS);
			/*** @field RmlUi.key_identifier.OEM_PERIOD integer */
			KEY_ENUM(OEM_PERIOD);
			/*** @field RmlUi.key_identifier.OEM_2 integer */
			KEY_ENUM(OEM_2);
			/*** @field RmlUi.key_identifier.OEM_3 integer */
			KEY_ENUM(OEM_3);
			/*** @field RmlUi.key_identifier.OEM_4 integer */
			KEY_ENUM(OEM_4);
			/*** @field RmlUi.key_identifier.OEM_5 integer */
			KEY_ENUM(OEM_5);
			/*** @field RmlUi.key_identifier.OEM_6 integer */
			KEY_ENUM(OEM_6);
			/*** @field RmlUi.key_identifier.OEM_7 integer */
			KEY_ENUM(OEM_7);
			/*** @field RmlUi.key_identifier.OEM_8 integer */
			KEY_ENUM(OEM_8);
			/*** @field RmlUi.key_identifier.OEM_102 integer */
			KEY_ENUM(OEM_102);
			/*** @field RmlUi.key_identifier.NUMPAD0 integer */
			KEY_ENUM(NUMPAD0);
			/*** @field RmlUi.key_identifier.NUMPAD1 integer */
			KEY_ENUM(NUMPAD1);
			/*** @field RmlUi.key_identifier.NUMPAD2 integer */
			KEY_ENUM(NUMPAD2);
			/*** @field RmlUi.key_identifier.NUMPAD3 integer */
			KEY_ENUM(NUMPAD3);
			/*** @field RmlUi.key_identifier.NUMPAD4 integer */
			KEY_ENUM(NUMPAD4);
			/*** @field RmlUi.key_identifier.NUMPAD5 integer */
			KEY_ENUM(NUMPAD5);
			/*** @field RmlUi.key_identifier.NUMPAD6 integer */
			KEY_ENUM(NUMPAD6);
			/*** @field RmlUi.key_identifier.NUMPAD7 integer */
			KEY_ENUM(NUMPAD7);
			/*** @field RmlUi.key_identifier.NUMPAD8 integer */
			KEY_ENUM(NUMPAD8);
			/*** @field RmlUi.key_identifier.NUMPAD9 integer */
			KEY_ENUM(NUMPAD9);
			/*** @field RmlUi.key_identifier.NUMPADENTER integer */
			KEY_ENUM(NUMPADENTER);
			/*** @field RmlUi.key_identifier.MULTIPLY integer */
			KEY_ENUM(MULTIPLY);
			/*** @field RmlUi.key_identifier.ADD integer */
			KEY_ENUM(ADD);
			/*** @field RmlUi.key_identifier.SEPARATOR integer */
			KEY_ENUM(SEPARATOR);
			/*** @field RmlUi.key_identifier.SUBTRACT integer */
			KEY_ENUM(SUBTRACT);
			/*** @field RmlUi.key_identifier.DECIMAL integer */
			KEY_ENUM(DECIMAL);
			/*** @field RmlUi.key_identifier.DIVIDE integer */
			KEY_ENUM(DIVIDE);
			/*** @field RmlUi.key_identifier.OEM_NEC_EQUAL integer */
			KEY_ENUM(OEM_NEC_EQUAL);
			/*** @field RmlUi.key_identifier.BACK integer */
			KEY_ENUM(BACK);
			/*** @field RmlUi.key_identifier.TAB integer */
			KEY_ENUM(TAB);
			/*** @field RmlUi.key_identifier.CLEAR integer */
			KEY_ENUM(CLEAR);
			/*** @field RmlUi.key_identifier.RETURN integer */
			KEY_ENUM(RETURN);
			/*** @field RmlUi.key_identifier.PAUSE integer */
			KEY_ENUM(PAUSE);
			/*** @field RmlUi.key_identifier.CAPITAL integer */
			KEY_ENUM(CAPITAL);
			/*** @field RmlUi.key_identifier.KANA integer */
			KEY_ENUM(KANA);
			/*** @field RmlUi.key_identifier.HANGUL integer */
			KEY_ENUM(HANGUL);
			/*** @field RmlUi.key_identifier.JUNJA integer */
			KEY_ENUM(JUNJA);
			/*** @field RmlUi.key_identifier.FINAL integer */
			KEY_ENUM(FINAL);
			/*** @field RmlUi.key_identifier.HANJA integer */
			KEY_ENUM(HANJA);
			/*** @field RmlUi.key_identifier.KANJI integer */
			KEY_ENUM(KANJI);
			/*** @field RmlUi.key_identifier.ESCAPE integer */
			KEY_ENUM(ESCAPE);
			/*** @field RmlUi.key_identifier.CONVERT integer */
			KEY_ENUM(CONVERT);
			/*** @field RmlUi.key_identifier.NONCONVERT integer */
			KEY_ENUM(NONCONVERT);
			/*** @field RmlUi.key_identifier.ACCEPT integer */
			KEY_ENUM(ACCEPT);
			/*** @field RmlUi.key_identifier.MODECHANGE integer */
			KEY_ENUM(MODECHANGE);
			/*** @field RmlUi.key_identifier.PRIOR integer */
			KEY_ENUM(PRIOR);
			/*** @field RmlUi.key_identifier.NEXT integer */
			KEY_ENUM(NEXT);
			/*** @field RmlUi.key_identifier.END integer */
			KEY_ENUM(END);
			/*** @field RmlUi.key_identifier.HOME integer */
			KEY_ENUM(HOME);
			/*** @field RmlUi.key_identifier.LEFT integer */
			KEY_ENUM(LEFT);
			/*** @field RmlUi.key_identifier.UP integer */
			KEY_ENUM(UP);
			/*** @field RmlUi.key_identifier.RIGHT integer */
			KEY_ENUM(RIGHT);
			/*** @field RmlUi.key_identifier.DOWN integer */
			KEY_ENUM(DOWN);
			/*** @field RmlUi.key_identifier.SELECT integer */
			KEY_ENUM(SELECT);
			/*** @field RmlUi.key_identifier.PRINT integer */
			KEY_ENUM(PRINT);
			/*** @field RmlUi.key_identifier.EXECUTE integer */
			KEY_ENUM(EXECUTE);
			/*** @field RmlUi.key_identifier.SNAPSHOT integer */
			KEY_ENUM(SNAPSHOT);
			/*** @field RmlUi.key_identifier.INSERT integer */
			KEY_ENUM(INSERT);
			/*** @field RmlUi.key_identifier.DELETE integer */
			KEY_ENUM(DELETE);
			/*** @field RmlUi.key_identifier.HELP integer */
			KEY_ENUM(HELP);
			/*** @field RmlUi.key_identifier.LWIN integer */
			KEY_ENUM(LWIN);
			/*** @field RmlUi.key_identifier.RWIN integer */
			KEY_ENUM(RWIN);
			/*** @field RmlUi.key_identifier.APPS integer */
			KEY_ENUM(APPS);
			/*** @field RmlUi.key_identifier.POWER integer */
			KEY_ENUM(POWER);
			/*** @field RmlUi.key_identifier.SLEEP integer */
			KEY_ENUM(SLEEP);
			/*** @field RmlUi.key_identifier.WAKE integer */
			KEY_ENUM(WAKE);
			/*** @field RmlUi.key_identifier.F1 integer */
			KEY_ENUM(F1);
			/*** @field RmlUi.key_identifier.F2 integer */
			KEY_ENUM(F2);
			/*** @field RmlUi.key_identifier.F3 integer */
			KEY_ENUM(F3);
			/*** @field RmlUi.key_identifier.F4 integer */
			KEY_ENUM(F4);
			/*** @field RmlUi.key_identifier.F5 integer */
			KEY_ENUM(F5);
			/*** @field RmlUi.key_identifier.F6 integer */
			KEY_ENUM(F6);
			/*** @field RmlUi.key_identifier.F7 integer */
			KEY_ENUM(F7);
			/*** @field RmlUi.key_identifier.F8 integer */
			KEY_ENUM(F8);
			/*** @field RmlUi.key_identifier.F9 integer */
			KEY_ENUM(F9);
			/*** @field RmlUi.key_identifier.F10 integer */
			KEY_ENUM(F10);
			/*** @field RmlUi.key_identifier.F11 integer */
			KEY_ENUM(F11);
			/*** @field RmlUi.key_identifier.F12 integer */
			KEY_ENUM(F12);
			/*** @field RmlUi.key_identifier.F13 integer */
			KEY_ENUM(F13);
			/*** @field RmlUi.key_identifier.F14 integer */
			KEY_ENUM(F14);
			/*** @field RmlUi.key_identifier.F15 integer */
			KEY_ENUM(F15);
			/*** @field RmlUi.key_identifier.F16 integer */
			KEY_ENUM(F16);
			/*** @field RmlUi.key_identifier.F17 integer */
			KEY_ENUM(F17);
			/*** @field RmlUi.key_identifier.F18 integer */
			KEY_ENUM(F18);
			/*** @field RmlUi.key_identifier.F19 integer */
			KEY_ENUM(F19);
			/*** @field RmlUi.key_identifier.F20 integer */
			KEY_ENUM(F20);
			/*** @field RmlUi.key_identifier.F21 integer */
			KEY_ENUM(F21);
			/*** @field RmlUi.key_identifier.F22 integer */
			KEY_ENUM(F22);
			/*** @field RmlUi.key_identifier.F23 integer */
			KEY_ENUM(F23);
			/*** @field RmlUi.key_identifier.F24 integer */
			KEY_ENUM(F24);
			/*** @field RmlUi.key_identifier.NUMLOCK integer */
			KEY_ENUM(NUMLOCK);
			/*** @field RmlUi.key_identifier.SCROLL integer */
			KEY_ENUM(SCROLL);
			/*** @field RmlUi.key_identifier.OEM_FJ_JISHO integer */
			KEY_ENUM(OEM_FJ_JISHO);
			/*** @field RmlUi.key_identifier.OEM_FJ_MASSHOU integer */
			KEY_ENUM(OEM_FJ_MASSHOU);
			/*** @field RmlUi.key_identifier.OEM_FJ_TOUROKU integer */
			KEY_ENUM(OEM_FJ_TOUROKU);
			/*** @field RmlUi.key_identifier.OEM_FJ_LOYA integer */
			KEY_ENUM(OEM_FJ_LOYA);
			/*** @field RmlUi.key_identifier.OEM_FJ_ROYA integer */
			KEY_ENUM(OEM_FJ_ROYA);
			/*** @field RmlUi.key_identifier.LSHIFT integer */
			KEY_ENUM(LSHIFT);
			/*** @field RmlUi.key_identifier.RSHIFT integer */
			KEY_ENUM(RSHIFT);
			/*** @field RmlUi.key_identifier.LCONTROL integer */
			KEY_ENUM(LCONTROL);
			/*** @field RmlUi.key_identifier.RCONTROL integer */
			KEY_ENUM(RCONTROL);
			/*** @field RmlUi.key_identifier.LMENU integer */
			KEY_ENUM(LMENU);
			/*** @field RmlUi.key_identifier.RMENU integer */
			KEY_ENUM(RMENU);
			/*** @field RmlUi.key_identifier.BROWSER_BACK integer */
			KEY_ENUM(BROWSER_BACK);
			/*** @field RmlUi.key_identifier.BROWSER_FORWARD integer */
			KEY_ENUM(BROWSER_FORWARD);
			/*** @field RmlUi.key_identifier.BROWSER_REFRESH integer */
			KEY_ENUM(BROWSER_REFRESH);
			/*** @field RmlUi.key_identifier.BROWSER_STOP integer */
			KEY_ENUM(BROWSER_STOP);
			/*** @field RmlUi.key_identifier.BROWSER_SEARCH integer */
			KEY_ENUM(BROWSER_SEARCH);
			/*** @field RmlUi.key_identifier.BROWSER_FAVORITES integer */
			KEY_ENUM(BROWSER_FAVORITES);
			/*** @field RmlUi.key_identifier.BROWSER_HOME integer */
			KEY_ENUM(BROWSER_HOME);
			/*** @field RmlUi.key_identifier.VOLUME_MUTE integer */
			KEY_ENUM(VOLUME_MUTE);
			/*** @field RmlUi.key_identifier.VOLUME_DOWN integer */
			KEY_ENUM(VOLUME_DOWN);
			/*** @field RmlUi.key_identifier.VOLUME_UP integer */
			KEY_ENUM(VOLUME_UP);
			/*** @field RmlUi.key_identifier.MEDIA_NEXT_TRACK integer */
			KEY_ENUM(MEDIA_NEXT_TRACK);
			/*** @field RmlUi.key_identifier.MEDIA_PREV_TRACK integer */
			KEY_ENUM(MEDIA_PREV_TRACK);
			/*** @field RmlUi.key_identifier.MEDIA_STOP integer */
			KEY_ENUM(MEDIA_STOP);
			/*** @field RmlUi.key_identifier.MEDIA_PLAY_PAUSE integer */
			KEY_ENUM(MEDIA_PLAY_PAUSE);
			/*** @field RmlUi.key_identifier.LAUNCH_MAIL integer */
			KEY_ENUM(LAUNCH_MAIL);
			/*** @field RmlUi.key_identifier.LAUNCH_MEDIA_SELECT integer */
			KEY_ENUM(LAUNCH_MEDIA_SELECT);
			/*** @field RmlUi.key_identifier.LAUNCH_APP1 integer */
			KEY_ENUM(LAUNCH_APP1);
			/*** @field RmlUi.key_identifier.LAUNCH_APP2 integer */
			KEY_ENUM(LAUNCH_APP2);
			/*** @field RmlUi.key_identifier.OEM_AX integer */
			KEY_ENUM(OEM_AX);
			/*** @field RmlUi.key_identifier.ICO_HELP integer */
			KEY_ENUM(ICO_HELP);
			/*** @field RmlUi.key_identifier.ICO_00 integer */
			KEY_ENUM(ICO_00);
			/*** @field RmlUi.key_identifier.PROCESSKEY integer */
			KEY_ENUM(PROCESSKEY);
			/*** @field RmlUi.key_identifier.ICO_CLEAR integer */
			KEY_ENUM(ICO_CLEAR);
			/*** @field RmlUi.key_identifier.ATTN integer */
			KEY_ENUM(ATTN);
			/*** @field RmlUi.key_identifier.CRSEL integer */
			KEY_ENUM(CRSEL);
			/*** @field RmlUi.key_identifier.EXSEL integer */
			KEY_ENUM(EXSEL);
			/*** @field RmlUi.key_identifier.EREOF integer */
			KEY_ENUM(EREOF);
			/*** @field RmlUi.key_identifier.PLAY integer */
			KEY_ENUM(PLAY);
			/*** @field RmlUi.key_identifier.ZOOM integer */
			KEY_ENUM(ZOOM);
			/*** @field RmlUi.key_identifier.PA1 integer */
			KEY_ENUM(PA1);
			/*** @field RmlUi.key_identifier.OEM_CLEAR integer */
			KEY_ENUM(OEM_CLEAR);
			#undef KEY_ENUM

			 return t;
		 }));
		 /***
		  * @enum RmlUi.key_modifier
		  */
		 namespace_table.set("key_modifier", sol::readonly_property([](sol::this_state l) {
			 sol::state_view lua(l);
			 return lua.create_table_with(
				/*** @field RmlUi.key_modifier.CTRL integer */
				"CTRL", Rml::Input::KM_CTRL,
				/*** @field RmlUi.key_modifier.SHIFT integer */
				"SHIFT", Rml::Input::KM_SHIFT,
				/*** @field RmlUi.key_modifier.ALT integer */
				"ALT", Rml::Input::KM_ALT,
				/*** @field RmlUi.key_modifier.META integer */
				"META", Rml::Input::KM_META,
				/*** @field RmlUi.key_modifier.CAPSLOCK integer */
				"CAPSLOCK", Rml::Input::KM_CAPSLOCK,
				/*** @field RmlUi.key_modifier.NUMLOCK integer */
				"NUMLOCK", Rml::Input::KM_NUMLOCK,
				/*** @field RmlUi.key_modifier.SCROLLLOCK integer */
				"SCROLLLOCK", Rml::Input::KM_SCROLLLOCK
			 );
		 }));
		 /***
		  * @enum RmlUi.font_weight
		  */
		 namespace_table.set("font_weight", sol::readonly_property([](sol::this_state l) {
			 sol::state_view lua(l);
			 return lua.create_table_with(
				/*** @field RmlUi.font_weight.Auto integer */
				"Auto", Rml::Style::FontWeight::Auto,
				/*** @field RmlUi.font_weight.Normal integer */
				"Normal", Rml::Style::FontWeight::Normal,
				/*** @field RmlUi.font_weight.Bold integer */
				"Bold", Rml::Style::FontWeight::Bold
			 );
		 }));
		 /***
		  * @enum RmlUi.default_action_phase
		  */
		 namespace_table.set("default_action_phase", sol::readonly_property([](sol::this_state l) {
			 sol::state_view lua(l);
			 return lua.create_table_with(
				/*** @field RmlUi.default_action_phase.Auto integer */
				"None", Rml::DefaultActionPhase::None,
				/*** @field RmlUi.default_action_phase.Target integer */
				"Target", Rml::DefaultActionPhase::Target,
				/*** @field RmlUi.default_action_phase.TargetAndBubble integer */
				"TargetAndBubble", Rml::DefaultActionPhase::TargetAndBubble
			 );
		 }));
	 }

 } // end namespace Rml::SolLua
