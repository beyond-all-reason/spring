/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/*
 * This source file is derived from the source code of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef RMLUI_SYSTEMINTERFACE_H
#define RMLUI_SYSTEMINTERFACE_H


#include <RmlUi/Core/Input.h>
#include <RmlUi/Core/SystemInterface.h>
#include <RmlUi/Core/Types.h>
#include <SDL.h>

#include "Rml/SolLua/TranslationTable.h"

class RmlSystemInterface : public Rml::SystemInterface
{
public:
	RmlSystemInterface();
	~RmlSystemInterface();

	// -- Inherited from Rml::SystemInterface  --
	/**
	 * @brief Call back for any raw text (CDATA in the XML spec) inside the *.rml files.
	 * @param translatedOut
	 * @param input
	 * @return
	 */
	int TranslateString(Rml::String& translatedOut, const Rml::String& input) override;
	bool LogMessage(Rml::Log::Type type, const Rml::String& message) override;

	double GetElapsedTime() override;

	void SetMouseCursor(const Rml::String& cursor_name) override;

	void SetClipboardText(const Rml::String& text) override;
	void GetClipboardText(Rml::String& text) override;

	// New hooks for Recoil engine
	const Rml::String& GetMouseCursor();
	void SetTranslationTable(TranslationTable* tt);

	/**
	 * <p>
	 * Map in which to set alias such that when given a string <u>from</u> RmlUi via SetMouseCursor(),
	 * it is passed <u>to</u> the Recoil engine as a different string when GetMouseCursor() is called.
	 * </p><p>
	 * If a string passed via SetMouseCursor() in not in the map,
	 * that string is passed to the rest of the engine code unaltered. (Nothing happens)
	 * </p><p>
	 * This is to allow for the use of standard CSS cursor names in RCSS styles
	 * instead of having to look up what cursors are registered with the engine and using that.
	 * </p>
	 * @example Use from Lua: @code RmlUi.SetMouseCursorAlias("default", "cursornormal") @endcode
	 * @link CSS cursor list:<br/> https://developer.mozilla.org/en-US/docs/Web/CSS/cursor
	 * @link RmlUi documentation:<br/> https://mikke89.github.io/RmlUiDoc/pages/rcss/user_interface.html#cursor
	 */
	std::unordered_map<std::string, std::string> mouseCursorAliases;

private:
	TranslationTable* translationTable = nullptr;
	Rml::String mouseCursor;
};

// not named "Rml" or "RmlSDL" to avoid name collision with the original Rml lib files
namespace RmlSDLRecoil
{

	// Applies input on the context based on the given SDL event.
	// @return True if the event is still propagating, false if it was handled by the context.
	bool InputEventHandler(Rml::Context* context, const SDL_Event& ev);
	bool EventKeyDown(Rml::Context* context, Rml::Input::KeyIdentifier key);
	bool EventKeyUp(Rml::Context* context, Rml::Input::KeyIdentifier key);
	bool EventTextInput(Rml::Context* context, const std::string& text);
	bool EventMouseMove(Rml::Context* context, Sint32 x, Sint32 y);
	bool EventMousePress(Rml::Context* context, Sint32 x, Sint32 y, Sint32 button);
	bool EventMouseRelease(Rml::Context* context, Sint32 x, Sint32 y, Sint32 button);
	bool EventMouseWheel(Rml::Context* context, float delta);

	// Converts the SDL key to RmlUi key.
	Rml::Input::KeyIdentifier ConvertKey(int sdl_key);

	// Converts the SDL mouse button to RmlUi mouse button.
	int ConvertMouseButton(int sdl_mouse_button);

	// Returns the active RmlUi key modifier state.
	int GetKeyModifierState();

}  // namespace RmlSDLRecoil
#endif  // RMLUI_SYSTEMINTERFACE_H
