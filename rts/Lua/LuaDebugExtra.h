/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_DEBUG_EXTRA_H
#define LUA_DEBUG_EXTRA_H

struct lua_State;

class LuaDebugExtra {
	public:
		static bool PushEntries(lua_State* L);

		// drops all emulated key/button state; with fireReleases (default) it also
		// dispatches balancing KeyReleased/MouseRelease. Shared by
		// debug.clearEmulatedInput, the focus-loss handler (fire), and game teardown
		// (no fire - the handles are being destroyed).
		static void ClearEmulatedInput(bool fireReleases = true);

	private:
		static int EmulateKeyPress(lua_State* L);
		static int EmulateKeyRelease(lua_State* L);
		static int EmulateMousePress(lua_State* L);
		static int EmulateMouseRelease(lua_State* L);
		static int EmulateMouseMove(lua_State* L);
		static int EmulateMouseWheel(lua_State* L);
		static int ClearEmulatedInputLua(lua_State* L);
};

#endif /* LUA_DEBUG_EXTRA_H */
