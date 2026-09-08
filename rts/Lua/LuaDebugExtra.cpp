/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#include "LuaDebugExtra.h"

#include "LuaInclude.h"
#include "LuaUtils.h"

#include "Game/GameController.h"
#include "Game/UI/KeyBindings.h"
#include "Game/UI/KeyCodes.h"
#include "Game/UI/ScanCodes.h"
#include "Game/UI/MouseHandler.h"
#include "Rendering/GlobalRendering.h"
#include "System/Input/KeyInput.h"
#include "System/Input/MouseInput.h"
#include "System/Platform/SDL1_keysym.h"

#include <set>

#include <SDL_keyboard.h>
#include <SDL_keycode.h>
#include <SDL_mouse.h>


/******************************************************************************
 * debug input emulation
 *
 * Callouts that feed input to the engine as if it came from real hardware,
 * for headless regression tests. Emulated presses are held in a separate store
 * and OR'd into the real input state; an event fires only when the combined
 * (physical-or-emulated) state actually changes, so Lua never sees two Presses
 * or two Releases in a row.
 *
 * No engine-side access gate: the property that must not regress (no doubled
 * events) is structural, not access-controlled. A game that wants to restrict
 * these nils them out.
 *
 * @see rts/Lua/LuaDebugExtra.cpp
******************************************************************************/

bool LuaDebugExtra::PushEntries(lua_State* L)
{
	LuaPushNamedCFunc(L, "emulateKeyPress",     EmulateKeyPress);
	LuaPushNamedCFunc(L, "emulateKeyRelease",   EmulateKeyRelease);
	LuaPushNamedCFunc(L, "emulateMousePress",   EmulateMousePress);
	LuaPushNamedCFunc(L, "emulateMouseRelease", EmulateMouseRelease);
	LuaPushNamedCFunc(L, "emulateMouseMove",    EmulateMouseMove);
	LuaPushNamedCFunc(L, "emulateMouseWheel",   EmulateMouseWheel);
	LuaPushNamedCFunc(L, "clearEmulatedInput",  ClearEmulatedInputLua);

	return true;
}


// shared body for emulateKeyPress/emulateKeyRelease; only the store update and
// the edge-fire differ, keyed on `pressed`
static int emulateKey(lua_State* L, bool pressed)
{
	if (activeController == nullptr)
		return 0;

	// Lua passes SDL1.2 keysyms; the held-state side (keyVec/IsKeyPressed) works in
	// raw SDL2, while the event side wants the normalized code like a real KEYDOWN
	const int rawKey = SDL12_keysyms(luaL_checkint(L, 1));

	// reject a junk keycode (unmapped -> SDLK_UNKNOWN). We deliberately do NOT
	// reject on an unknown scancode: headless has no keyboard layout, so
	// SDL_GetScancodeFromKey returns SDL_SCANCODE_UNKNOWN even for valid keys
	if (rawKey == SDLK_UNKNOWN)
		return 0;

	const SDL_Scancode sc = SDL_GetScancodeFromKey((SDL_Keycode)rawKey);
	const int eventKey = CKeyCodes::GetNormalizedSymbol(rawKey);
	const int scanCode = CScanCodes::GetNormalizedSymbol(sc);

	int numKeys = 0;
	const uint8_t* kbState = SDL_GetKeyboardState(&numKeys);
	const bool physicalDown = ((int)sc < numKeys && kbState[sc] != 0);

	// effective (physical-or-emulated) state before this call
	const bool wasDown = physicalDown || KeyInput::IsKeyEmulated(rawKey);

	KeyInput::SetKeyEmulated(rawKey, pressed);
	KeyInput::Update(keyBindings.GetFakeMetaKey());

	if (pressed) {
		// fire only on a false->true edge
		if (!wasDown)
			activeController->KeyPressed(eventKey, scanCode, false);
	} else {
		// effective after = physical; fire only on a true->false edge
		if (wasDown && !physicalDown)
			activeController->KeyReleased(eventKey, scanCode);
	}

	return 0;
}


/*** Emulate a keyboard key being pressed and held.
 *
 * Fires the KeyPress event and holds the key down (merged with real hardware
 * state) until released or cleared. The accompanying scancode is derived from
 * the keycode using the currently active system keyboard layout.
 *
 * @function debug.emulateKeyPress
 * @param keycode integer
 * @return nil
 */
int LuaDebugExtra::EmulateKeyPress(lua_State* L) { return emulateKey(L, true); }


/*** Emulate a held keyboard key being released.
 *
 * @function debug.emulateKeyRelease
 * @param keycode integer
 * @return nil
 */
int LuaDebugExtra::EmulateKeyRelease(lua_State* L) { return emulateKey(L, false); }


/*** Emulate a mouse button being pressed and held.
 *
 * @function debug.emulateMousePress
 * @param button integer
 * @return nil
 */
int LuaDebugExtra::EmulateMousePress(lua_State* L)
{
	if (mouse == nullptr)
		return 0;

	const int button = luaL_checkint(L, 1);

	if (button < 1 || button > NUM_BUTTONS)
		return 0;

	mouse->SetButtonEmulated(button, true);
	return 0;
}

int LuaDebugExtra::EmulateMouseWheel(lua_State* L)
{
	if (mouse == nullptr)
		return 0;

	// momentary tick, no persistent state to track; fire directly like a real wheel event
	mouse->MouseWheel((float)luaL_checknumber(L, 1));
	return 0;
}


/*** Emulate a held mouse button being released.
 *
 * @function debug.emulateMouseRelease
 * @param button integer
 * @return nil
 */
int LuaDebugExtra::EmulateMouseRelease(lua_State* L)
{
	if (mouse == nullptr)
		return 0;

	const int button = luaL_checkint(L, 1);

	if (button < 1 || button > NUM_BUTTONS)
		return 0;

	mouse->SetButtonEmulated(button, false);
	return 0;
}


/*** Emulate the cursor moving to a screen position.
 *
 * Fires a MouseMove through the normal pipeline. Coordinates use the bottom-left
 * origin like the rest of the Lua screen API. Does not move the OS cursor.
 *
 * @function debug.emulateMouseMove
 * @param x integer
 * @param y integer
 * @return nil
 */
int LuaDebugExtra::EmulateMouseMove(lua_State* L)
{
	if (mouse == nullptr || mouseInput == nullptr)
		return 0;

	const int x = luaL_checkint(L, 1);
	const int y = globalRendering->viewSizeY - luaL_checkint(L, 2) - 1;

	const int2 prev = mouseInput->GetPos();
	mouseInput->SetPos(int2(x, y));
	mouse->MouseMove(x, y, x - prev.x, y - prev.y);

	return 0;
}


/*** Release everything currently held via emulation.
 *
 * @function debug.clearEmulatedInput
 * @return nil
 */
int LuaDebugExtra::ClearEmulatedInputLua(lua_State* L)
{
	ClearEmulatedInput();
	return 0;
}


void LuaDebugExtra::ClearEmulatedInput(bool fireReleases)
{
	// snapshot the emulated keys before clearing, so a fired release can't walk
	// the store we are emptying
	const std::set<int> keyCodes = KeyInput::GetEmulatedKeys();

	KeyInput::ClearEmulatedKeys();
	KeyInput::Update(keyBindings.GetFakeMetaKey());

	if (fireReleases && activeController != nullptr) {
		int numKeys = 0;
		const uint8_t* kbState = SDL_GetKeyboardState(&numKeys);

		// the store holds raw SDL2 keycodes; the event side wants the normalized code
		for (const int rawKey: keyCodes) {
			const SDL_Scancode sc = SDL_GetScancodeFromKey((SDL_Keycode)rawKey);

			if ((int)sc < numKeys && kbState[sc] != 0)
				continue;

			activeController->KeyReleased(CKeyCodes::GetNormalizedSymbol(rawKey), CScanCodes::GetNormalizedSymbol(sc));
		}
	}

	if (mouse == nullptr)
		return;

	// no-fire path (game teardown): drop the flags without dispatching into
	// handles that are already being destroyed
	if (!fireReleases) {
		mouse->ClearEmulatedButtons();
		return;
	}

	// SetButtonEmulated fires the release itself if the button ends up effectively up
	for (int button = 1; button <= NUM_BUTTONS; ++button) {
		if (mouse->IsButtonEmulated(button))
			mouse->SetButtonEmulated(button, false);
	}
}
