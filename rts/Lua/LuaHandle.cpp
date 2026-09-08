/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaHandle.h"
#include "lua_privileges.h"

#include "LuaGaia.h"
#include "LuaRules.h"
#include "LuaUI.h"

#include "LuaCallInCheck.h"
#include "LuaConfig.h"
#include "LuaHashString.h"
#include "LuaOpenGL.h"
#include "LuaEncoding.h"
#include "LuaMathExtra.h"
#include "LuaTableExtra.h"
#include "LuaTracyExtra.h"
#include "LuaUtils.h"
#include "LuaZip.h"
#include "Game/Game.h"
#include "Game/GameHelper.h"
#include "Game/Action.h"
#include "Game/GlobalUnsynced.h"
#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
#include "Sim/Misc/LosHandler.h"
#include "Net/Protocol/NetProtocol.h"
#include "Game/UI/KeySet.h"
#include "Game/UI/MiniMap.h"
#include "Rendering/GlobalRendering.h"
#include "Rml/Backends/RmlUi_Backend.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectile.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Units/Scripts/CobDeferredCallin.h"
#include "Sim/Units/Scripts/CobInstance.h" // for UNPACK{X,Z}
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Weapons/WeaponDef.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/VFSModes.h"
#include "System/creg/SerializeLuaState.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"
#include "System/GlobalConfig.h"
#include "System/Rectangle.h"
#include "System/ScopedFPUSettings.h"
#include "System/StringUtil.h"
#include "System/Log/ILog.h"
#include "System/Input/KeyInput.h"
#include "System/Platform/SDL1_keysym.h"


#include "LuaInclude.h"

#include "lib/luasocket/src/luasocket.h"

#include <SDL_keyboard.h>
#include <SDL_keycode.h>
#include <SDL_mouse.h>

#include "System/Misc/TracyDefs.h"
#include <tracy/TracyLua.hpp>

#include <algorithm>
#include <string>


CONFIG(float, LuaGarbageCollectionMemLoadMult).defaultValue(1.33f).minimumValue(1.0f).maximumValue(100.0f).description("How much the amount of Lua memory in use increases the rate of garbage collection.");
CONFIG(float, LuaGarbageCollectionRunTimeMult).defaultValue(5.0f).minimumValue(1.0f).description("How many milliseconds the garbage collected can run for in each GC cycle");


static spring::unsynced_set<const luaContextData*>    SYNCED_LUAHANDLE_CONTEXTS;
static spring::unsynced_set<const luaContextData*>  UNSYNCED_LUAHANDLE_CONTEXTS;
const  spring::unsynced_set<const luaContextData*>*          LUAHANDLE_CONTEXTS[2] = {&UNSYNCED_LUAHANDLE_CONTEXTS, &SYNCED_LUAHANDLE_CONTEXTS};

bool CLuaHandle::devMode = false;

const int* CLuaHandle::currentCobArgs = nullptr;

/***
 * @class Callins
 * 
 * Functions called by the Engine.
 *
 * To use these callins in a widget, prepend `widget:` and, for a gadget,
 * prepend `gadget:`. For example:
 *
 * ```lua
 * function widget:UnitCreated(unitID, unitDefID, unitTeam, builderID)
 *   -- ...
 * end
 * ```
 *
 * Some functions may differ between (synced) gadget and widgets. This is
 * because all information should be available to synced (game logic
 * controlling) gadgets, but restricted to unsynced gadget/widget. e.g.
 * information about an enemy unit only detected via radar and not yet in LOS.
 * 
 * In such cases the full (synced) param list is documented.
 * 
 * **Attention:** Some callins will only work on the unsynced portion of the gadget.
 * Due to the type-unsafe nature of lua parsing, those callins not firing up
 * might be hard to trace.
 * 
 * @see Gadget
 * @see Widget
 * @see Menu
 * @see Intro
 * @see SyncedCallins
 * @see UnsyncedCallins
 */

void CLuaHandle::PushTracebackFuncToRegistry(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	SPRING_LUA_OPEN_LIB(L, luaopen_debug);
		LuaPushString(L, "traceback");
		LuaUtils::PushDebugTraceback(L);
		lua_rawset(L, LUA_REGISTRYINDEX);
	// We only need the debug.traceback function, the others are unsafe for syncing.
	// Later CLuaHandle implementations decide themselves if they want to reload the lib or not (LuaUI does).
	LUA_UNLOAD_LIB(L, LUA_DBLIBNAME);
}


static void LUA_INSERT_CONTEXT(const luaContextData* D, const spring::unsynced_set<const luaContextData*>* S) {
	const_cast<  spring::unsynced_set<const luaContextData*>*  >(S)->insert(D);
}
static void LUA_ERASE_CONTEXT(const luaContextData* D, const spring::unsynced_set<const luaContextData*>* S) {
	const_cast<  spring::unsynced_set<const luaContextData*>*  >(S)->erase(D);
}

static int handlepanic(lua_State* L)
{
	throw content_error(luaL_optsstring(L, 1, "lua paniced"));
}



CLuaHandle::CLuaHandle(const string& _name, int _order, bool _userMode, bool _synced)
	: CEventClient(_name, _order, _synced)
	, userMode(_userMode)
	, killMe(false)
	// no shared pool for LuaIntro to protect against LoadingMT=1
	// do not use it for LuaMenu either; too many blocks allocated
	// by *other* states end up not being recycled which presently
	// forces clearing the shared pool on reload
	, D(_name != "LuaIntro" && name != "LuaMenu", true)
{
	D.owner = this;
	D.synced = _synced;

	D.gcCtrl.baseMemLoadMult = configHandler->GetFloat("LuaGarbageCollectionMemLoadMult");
	D.gcCtrl.baseRunTimeMult = configHandler->GetFloat("LuaGarbageCollectionRunTimeMult");

	currentCobArgs = nullptr;

	L = LUA_OPEN(&D);
	L_GC = lua_newthread(L);

	LUA_INSERT_CONTEXT(&D, LUAHANDLE_CONTEXTS[D.synced]);

	luaL_ref(L, LUA_REGISTRYINDEX);

	// needed for engine traceback
	PushTracebackFuncToRegistry(L);

	// prevent lua from calling c's exit()
	lua_atpanic(L, handlepanic);

	// register tracy functions in global scope
	tracy::LuaRegister(L);
	#ifdef TRACY_ENABLE
		lua_getglobal(L, "tracy");
		LuaTracyExtra::PushEntries(L);
		lua_pop(L, 1);
	#endif
}


CLuaHandle::~CLuaHandle()
{
	RECOIL_DETAILED_TRACY_ZONE;

	currentCobArgs = nullptr;

	// KillLua() must be called before us!
	assert(!IsValid());
	assert(!eventHandler.HasClient(this));
}


// can be called from a handler constructor or FreeHandler
// we care about calling Shutdown only in the latter case!
void CLuaHandle::KillLua(bool inFreeHandler)
{
	// 1. unlink from eventHandler, so no new events are getting triggered
	//FIXME when multithreaded lua is enabled, wait for all running events to finish (possible via a mutex?)
	eventHandler.RemoveClient(this);

	if (!IsValid())
		return;

	// 2. shutdown
	if (inFreeHandler)
		Shutdown();

	if(rmlui) {
		RmlGui::RemoveLua();
	}

	// 3. delete the lua_State
	//
	// must be done here: if called from a ctor, we want the
	// state to become non-valid so that LoadHandler returns
	// false and FreeHandler runs next
	LUA_ERASE_CONTEXT(&D, LUAHANDLE_CONTEXTS[D.synced]);
	LUA_CLOSE(&L);
}


/******************************************************************************/
/******************************************************************************/

/***
 * @function Script.Kill
 * @param killMessage string? Kill message.
 */
int CLuaHandle::KillActiveHandle(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	CLuaHandle* ah = GetHandle(L);

	if (ah != nullptr) {
		if ((lua_gettop(L) >= 1) && lua_isstring(L, 1))
			ah->killMsg = lua_tostring(L, 1);

		// get rid of us next GameFrame call
		ah->killMe = true;

		// don't process any further events
		eventHandler.RemoveClient(ah);
	}

	return 0;
}


/******************************************************************************/

bool CLuaHandle::AddEntriesToTable(lua_State* L, const char* name,
                                   bool (*entriesFunc)(lua_State*))
{
	const int top = lua_gettop(L);
	lua_pushstring(L, name);
	lua_rawget(L, -2);
	if (lua_istable(L, -1)) {
		bool success = entriesFunc(L);
		lua_settop(L, top);
		return success;
	}

	// make a new table
	lua_pop(L, 1);
	lua_pushstring(L, name);
	lua_newtable(L);
	if (!entriesFunc(L)) {
		lua_settop(L, top);
		return false;
	}
	lua_rawset(L, -3);

	lua_settop(L, top);
	return true;
}


/******************************************************************************/
/******************************************************************************/

void CLuaHandle::CheckStack()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!IsValid())
		return;

	const int top = lua_gettop(L);
	if (top != 0) {
		LOG_L(L_WARNING, "[LuaHandle::%s] %s stack-top = %i", __func__, name.c_str(), top);
		lua_settop(L, 0);
	}
}


int CLuaHandle::XCall(lua_State* srcState, const char* funcName)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const int top = lua_gettop(L);

	// push the function
	const LuaHashString funcHash(funcName);
	if (!funcHash.GetGlobalFunc(L))
		return 0;

	int retCount;

	if (srcState == L) {
		lua_insert(L, 1); // move the function to the beginning

		// call the function
		if (!RunCallIn(L, funcHash, top, LUA_MULTRET))
			return 0;

		retCount = lua_gettop(L);
	} else {
		const int srcCount = lua_gettop(srcState);

		LuaUtils::CopyData(L, srcState, srcCount);

		const bool origDrawingState = LuaOpenGL::IsDrawingEnabled(L);
		LuaOpenGL::SetDrawingEnabled(L, LuaOpenGL::IsDrawingEnabled(srcState));

		// call the function
		const bool failed = !RunCallIn(L, funcHash, srcCount, LUA_MULTRET);

		LuaOpenGL::SetDrawingEnabled(L, origDrawingState);

		if (failed)
			return 0;

		retCount = lua_gettop(L) - top;

		lua_settop(srcState, 0); // pop all passed arguments on caller stack
		if (retCount > 0)
			LuaUtils::CopyData(srcState, L, retCount); // push the new returned arguments on caller stack

		lua_settop(L, top); // revert the callee stack
	}

	return retCount;
}


/******************************************************************************/
/******************************************************************************/

int CLuaHandle::RunCallInTraceback(
	lua_State* L,
	const LuaHashString* hs,
	std::string* ts,
	int inArgs,
	int outArgs,
	int errFuncIndex,
	bool popErrorFunc
) {
	RECOIL_DETAILED_TRACY_ZONE;
	// do not signal floating point exceptions in user Lua code
	ScopedDisableFpuExceptions fe;

	struct ScopedLuaCall {
	public:
		ScopedLuaCall(
			CLuaHandle* handle,
			lua_State* state,
			const char* func,
			int _nInArgs,
			int _nOutArgs,
			int _errFuncIdx,
			bool _popErrFunc
		)
			: luaState(state)
			, luaHandle(handle)
			, luaFunc(func)

			, nInArgs(_nInArgs)
			, nOutArgs(_nOutArgs)
			, errFuncIdx(_errFuncIdx)
			, popErrFunc(_popErrFunc)
		{
			handle->SetHandleRunning(state, true); // inc
			const bool canDraw = LuaOpenGL::IsDrawingEnabled(state);

			SMatrixStateData prevMatState;
			GLMatrixStateTracker& matTracker = GetLuaContextData(state)->glMatrixTracker;

			if (canDraw) {
				prevMatState = matTracker.PushMatrixState();
				LuaOpenGL::InitMatrixState(state, luaFunc);
			}

			top = lua_gettop(state);
			// note1: disable GC outside of this scope to prevent sync errors and similar
			// note2: we collect garbage now in its own callin "CollectGarbage"
			// lua_gc(L, LUA_GCRESTART, 0);
			error = lua_pcall(state, nInArgs, nOutArgs, errFuncIdx);
			// only run GC inside of "SetHandleRunning(L, true) ... SetHandleRunning(L, false)"!
			lua_gc(state, LUA_GCSTOP, 0);

			if (canDraw) {
				LuaOpenGL::CheckMatrixState(state, luaFunc, error);
				matTracker.PopMatrixState(prevMatState);
			}

			handle->SetHandleRunning(state, false); // dec
		}

		~ScopedLuaCall() {
			assert(!popErrFunc); // deprecated!
			if (popErrFunc) {
				lua_remove(luaState, errFuncIdx);
			}
		}

		void CheckFixStack(std::string& traceStr) {
			// note: assumes error-handler has not been popped yet (!)
			const int curTop = lua_gettop(luaState);
			const int outArgs = (curTop - (GetTop() - 1)) + nInArgs;

			if (GetError() == 0) {
				if (nOutArgs != LUA_MULTRET) {
					if (outArgs != nOutArgs) {
						LOG_L(L_ERROR, "[SLC::%s] %d ret-vals but %d expected for callin %s", __func__, outArgs, nOutArgs, luaFunc);

						if (outArgs > nOutArgs)
							lua_pop(luaState, outArgs - nOutArgs);
					}
				} else {
					// should not be reachable without getting a LUA_ERR*
					if (outArgs < 0) {
						LOG_L(L_ERROR, "[SLC::%s] %d ret-vals (top={%d,%d} args=%d) for callin %s, corrupt stack", __func__, outArgs, curTop, GetTop(), nInArgs, luaFunc);
					}
				}
			} else {
				// traceback string is optionally left on the stack
				// might also have been popped in case of underflow
				constexpr int dbgOutArgs = 1;

				if (outArgs > dbgOutArgs) {
					LOG_L(L_ERROR, "[SLC::%s] %i excess values on stack for callin %s", __func__, outArgs - dbgOutArgs, luaFunc);
					// only leave traceback string on the stack, popped below
					lua_pop(luaState, outArgs - dbgOutArgs);
				} else if (outArgs < dbgOutArgs) {
					LOG_L(L_ERROR, "[SLC::%s] %d ret-vals (top={%d,%d} args=%d) for callin %s, corrupt stack", __func__, outArgs, curTop, GetTop(), nInArgs, luaFunc);
					// make the pop() below valid
					lua_pushnil(luaState);
				}

				traceStr.append("[Internal Lua error: Call failure] ");
				traceStr.append(luaL_optstring(luaState, -1, "[No traceback returned]"));

				// pop traceback string
				lua_pop(luaState, dbgOutArgs);

				// log only errors that lead to a crash
				luaHandle->callinErrors += (GetError() == LUA_ERRRUN);

				// catch clients that desync due to corrupted data
				if (GetError() == LUA_ERRRUN && GetHandleSynced(luaState))
					CLIENT_NETLOG(gu->myPlayerNum, LOG_LEVEL_INFO, traceStr);
			}
		}

		int GetTop() const { return top; }
		int GetError() const { return error; }

	private:
		lua_State* luaState;
		CLuaHandle* luaHandle;
		const char* luaFunc;

		int nInArgs;
		int nOutArgs;
		int errFuncIdx;
		bool popErrFunc;

		int top;
		int error;
	};

	// TODO: use closure so we do not need to copy args
	ScopedLuaCall call(this, L, (hs != nullptr)? hs->GetString(): "LUS::?", inArgs, outArgs, errFuncIndex, popErrorFunc);
	call.CheckFixStack(*ts);

	return (call.GetError());
}


bool CLuaHandle::RunCallInTraceback(lua_State* L, const LuaHashString& hs, int inArgs, int outArgs, int errFuncIndex, bool popErrFunc)
{
	RECOIL_DETAILED_TRACY_ZONE;
	std::string traceStr;
	const int error = RunCallInTraceback(L, &hs, &traceStr, inArgs, outArgs, errFuncIndex, popErrFunc);

	if (error == 0)
		return true;

	const auto& hn = GetName();
	const char* hsn = hs.GetString();
	const char* es = LuaErrorString(error);

	LOG_L(L_ERROR, "[%s::%s] error=%i (%s) callin=%s trace=%s", hn.c_str(), __func__, error, es, hsn, traceStr.c_str());

	if (error == LUA_ERRMEM) {
		// try to free some memory so other lua states can alloc again
		CollectGarbage(true);
		// kill the entire handle next frame
		KillActiveHandle(L);
	}

	return false;
}


/******************************************************************************
 * Common
 * @section common
******************************************************************************/

/*** Called when the addon is (re)loaded.
 *
 * @function Callins:Initialize
 */

/*** Called when the game is (re)loaded.
 *
 * @function Callins:LoadCode
 */
bool CLuaHandle::LoadCode(lua_State* L, std::string code, const string& debug)
{
	lua_settop(L, 0);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	LuaUtils::TracyRemoveAlsoExtras(code.data());
	const int error = luaL_loadbuffer(L, code.c_str(), code.size(), debug.c_str());

	if (error != 0) {
		LOG_L(L_ERROR, "[%s::%s] error=%i (%s) debug=%s msg=%s", name.c_str(), __func__, error, LuaErrorString(error), debug.c_str(), lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}

	static const LuaHashString cmdStr(__func__);

	// call Initialize immediately after load
	return (RunCallInTraceback(L, cmdStr, 0, 0, traceBack.GetErrFuncIdx(), false));
}


void CLuaHandle::InitLuaSocket(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;

	const std::string filename = "LuaSocket/socket.lua";
	CFileHandler f(filename, SPRING_VFS_BASE);
	if (!f.FileExists()) {
		LOG_L(L_ERROR, "Error loading %s (file does not exist)", filename.c_str());
		return;
	}

	LUA_OPEN_LIB(L, luaopen_socket_core);

	std::string code;
	if (f.LoadStringData(code)) {
		LoadCode(L, std::move(code), filename);
	} else {
		LOG_L(L_ERROR, "Error loading %s", filename.c_str());
	}
}


int CLuaHandle::LoadStringData(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	size_t len;
	const char *str    = luaL_checklstring(L, 1, &len);
	const char *chunkname = luaL_optstring(L, 2, str);

	auto handle = GetHandle(L);

	if (luaL_loadbuffer_privileged(L, str, len, chunkname, handle->GetDevMode()) != 0) {
		lua_pushnil(L);
		lua_insert(L, -2);
		return 2; // nil, then the error message
	}

	// set the chunk's fenv to the current fenv
	if (lua_istable(L, 3)) {
		lua_pushvalue(L, 3);
	} else {
		LuaUtils::PushCurrentFuncEnv(L, __func__);
	}

	if (lua_setfenv(L, -2) == 0)
		luaL_error(L, "[%s] error with setfenv", __func__);

	return 1;
}


/*** Called when the addon or the game is shutdown.
 *
 * @function Callins:Shutdown
 * @return nil
 */
void CLuaHandle::Shutdown()
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 3, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	// call the routine
	RunCallInTraceback(L, cmdStr, 0, 0, traceBack.GetErrFuncIdx(), false);
}


/*** Called when a player issues a UI command e.g. types /foo or /luarules foo.
 *
 * @function Callins:GotChatMsg
 * @param msg string
 * @param playerID PlayerID
 */
bool CLuaHandle::GotChatMsg(const string& msg, int playerID)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L, true);
	luaL_checkstack(L, 4, __func__);
	static const LuaHashString cmdStr(__func__);

	bool processed = false;
	if (cmdStr.GetGlobalFunc(L)) {
		lua_pushsstring(L, msg);
		lua_pushnumber(L, playerID);

		// call the routine
		if (RunCallIn(L, cmdStr, 2, 1)) {
			processed = luaL_optboolean(L, -1, false);
			lua_pop(L, 1);
		}
	}

	if (!processed && (this == luaUI)) {
		processed = luaUI->ConfigureLayout(msg); //FIXME deprecated
	}
	return processed;
}


/*** Called after `GamePreload` and before `GameStart`. See Lua_SaveLoad.
 *
 * @function Callins:Load
 * @param zipReader table
 */
void CLuaHandle::Load(IArchive* archive)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 4, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	// Load gets ZipFileReader userdatum as single argument
	LuaZipFileReader::PushNew(L, "", archive);

	// call the routine
	RunCallInTraceback(L, cmdStr, 1, 0, traceBack.GetErrFuncIdx(), false);
}


bool CLuaHandle::HasCallIn(lua_State* L, const string& name) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!IsValid())
		return false;

	if (name == "CollectGarbage")
		return true;

	//FIXME should be equal to below, but somehow it isn't and doesn't work as expected!?
// 	lua_getglobal(L, name.c_str());
// 	const bool found = !lua_isfunction(L, -1);
// 	lua_pop(L, 1);

	lua_pushvalue(L, LUA_GLOBALSINDEX);
	lua_pushsstring(L, name); // push the function name
	lua_rawget(L, -2);        // get the function
	const bool found = lua_isfunction(L, -1);
	lua_pop(L, 2);

	return found;
}

bool CLuaHandle::UpdateCallIn(lua_State* L, const string& name)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (HasCallIn(L, name)) {
		eventHandler.InsertEvent(this, name);
	} else {
		eventHandler.RemoveEvent(this, name);
	}
	return true;
}

/*** Game
 * @section game
 */

/*** Called before the 0 gameframe.
 *
 * Is not called when a saved game is loaded.
 *
 * @function Callins:GamePreload
 */
void CLuaHandle::GamePreload()
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 3, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	// call the routine
	RunCallInTraceback(L, cmdStr, 0, 0, traceBack.GetErrFuncIdx(), false);
}


/*** Called upon the start of the game.
 *
 * @function Callins:GameStart
 *
 * Is not called when a saved game is loaded.
 */
void CLuaHandle::GameStart()
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 3, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	// call the routine
	RunCallInTraceback(L, cmdStr, 0, 0, traceBack.GetErrFuncIdx(), false);
}


/*** Called when the game ends
 *
 * @function Callins:GameOver
 * @param winningAllyTeams AllyTeamID[] list of winning allyTeams, if empty the game result was undecided (like when dropping from an host).
 */
void CLuaHandle::GameOver(const std::vector<unsigned char>& winningAllyTeams)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 2, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_createtable(L, winningAllyTeams.size(), 0);
	for (unsigned int i = 0; i < winningAllyTeams.size(); i++) {
		lua_pushnumber(L, winningAllyTeams[i]);
		lua_rawseti(L, -2, i + 1);
	}

	// call the routine
	RunCallInTraceback(L, cmdStr, 1, 0, traceBack.GetErrFuncIdx(), false);
}


/*** Called when the game is paused.
 *
 * @function Callins:GamePaused
 * @param playerID PlayerID
 * @param paused boolean
 */
void CLuaHandle::GamePaused(int playerID, bool paused)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 5, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, playerID);
	lua_pushboolean(L, paused);

	// call the routine
	RunCallInTraceback(L, cmdStr, 2, 0, traceBack.GetErrFuncIdx(), false);
}

void CLuaHandle::RunDelayedFunctions(int frameNum)
{
	RECOIL_DETAILED_TRACY_ZONE;
	static const LuaHashString cmdStr(__func__);

	const auto currentFrameIterator = delayedCallsByFrame.find(frameNum);
	if (currentFrameIterator == delayedCallsByFrame.end())
		return;

	const auto& functions = currentFrameIterator->second;
	for (const auto& [function, args] : functions) {
		const LuaUtils::ScopedDebugTraceBack traceBack(L);
		luaL_checkstack(L, args.size() + 3, __func__); // the +3 is cargo-cult, most other callins do it like that

		lua_rawgeti(L, LUA_REGISTRYINDEX, function);
		luaL_unref(L, LUA_REGISTRYINDEX, function);
		for (const auto arg : args) {
			lua_rawgeti(L, LUA_REGISTRYINDEX, arg);
			luaL_unref(L, LUA_REGISTRYINDEX, arg);
		}
		RunCallInTraceback(L, cmdStr, (int) args.size(), 0, traceBack.GetErrFuncIdx(), false);
	}

	delayedCallsByFrame.erase(currentFrameIterator);
}

/*** Called for every game simulation frame (30 per second).
 *
 * @function Callins:GameFrame
 * @param frame integer Starts at frame 1
 */
void CLuaHandle::GameFrame(int frameNum)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (killMe) {
		const std::string msg = GetName() + ((!killMsg.empty())? ": " + killMsg: "");

		LOG("[%s] disabled %s", __func__, msg.c_str());
		delete this;
		return;
	}

	RunDelayedFunctions(frameNum);

	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 4, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);

	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, frameNum);

	// call the routine
	RunCallInTraceback(L, cmdStr, 1, 0, traceBack.GetErrFuncIdx(), false);
}

/*** Called at the end of every game simulation frame
 *
 * @function Callins:GameFramePost
 * @param frame integer Starts at frame 1
 */
void CLuaHandle::GameFramePost(int frameNum)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 4, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);

	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, frameNum);

	// call the routine
	RunCallInTraceback(L, cmdStr, 1, 0, traceBack.GetErrFuncIdx(), false);
}

/*** Called once to deliver the gameID
 *
 * @function Callins:GameID
 * @param gameID string encoded in hex.
 */
void CLuaHandle::GameID(const unsigned char* gameID, unsigned int numBytes)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 4, __func__);
	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	char buf[33];

	SNPRINTF(buf, sizeof(buf),
			"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
			gameID[ 0], gameID[ 1], gameID[ 2], gameID[ 3], gameID[ 4], gameID[ 5], gameID[ 6], gameID[ 7],
			gameID[ 8], gameID[ 9], gameID[10], gameID[11], gameID[12], gameID[13], gameID[14], gameID[15]);
	lua_pushstring(L, buf);

	RunCallInTraceback(L, cmdStr, 1, 0, traceBack.GetErrFuncIdx(), false);
}

/*** Teams
 * @section teams
 */

/*** Called when a team dies (see `Spring.KillTeam`).
 *
 * @function Callins:TeamDied
 * @param teamID TeamID
 */
void CLuaHandle::TeamDied(int teamID)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 4, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, teamID);

	// call the routine
	RunCallInTraceback(L, cmdStr, 1, 0, traceBack.GetErrFuncIdx(), false);
}


/*** @function Callins:TeamChanged
 *
 * @param teamID TeamID
 */
void CLuaHandle::TeamChanged(int teamID)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 4, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, teamID);

	// call the routine
	RunCallInTraceback(L, cmdStr, 1, 0, traceBack.GetErrFuncIdx(), false);
}


/*** Called whenever a player's status changes e.g. becoming a spectator.
 *
 * @function Callins:PlayerChanged
 * @param playerID PlayerID
 */
void CLuaHandle::PlayerChanged(int playerID)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 4, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, playerID);

	// call the routine
	RunCallInTraceback(L, cmdStr, 1, 0, traceBack.GetErrFuncIdx(), false);
}


/*** Called whenever a new player joins the game.
 *
 * @function Callins:PlayerAdded
 * @param playerID PlayerID
 */
void CLuaHandle::PlayerAdded(int playerID)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 4, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, playerID);

	// call the routine
	RunCallInTraceback(L, cmdStr, 1, 0, traceBack.GetErrFuncIdx(), false);
}


/*** Called whenever a player is removed from the game.
 *
 * @function Callins:PlayerRemoved
 * @param playerID PlayerID
 * @param reason integer
 */
void CLuaHandle::PlayerRemoved(int playerID, int reason)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 5, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, playerID);
	lua_pushnumber(L, reason);

	// call the routine
	RunCallInTraceback(L, cmdStr, 2, 0, traceBack.GetErrFuncIdx(), false);
}


/*** Units
 *
 * @section units
 */

inline void CLuaHandle::UnitCallIn(const LuaHashString& hs, const CUnit* unit)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 6, __func__);
	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	if (!hs.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);

	// call the routine
	RunCallInTraceback(L, hs, 3, 0, traceBack.GetErrFuncIdx(), false);
}


/*** Called at the moment the unit is created.
 *
 * @function Callins:UnitCreated
 * @param unitID UnitID
 * @param unitDefID UnitDefID
 * @param unitTeam TeamID
 * @param builderID UnitID?
 */
void CLuaHandle::UnitCreated(const CUnit* unit, const CUnit* builder)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 7, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	if (builder != nullptr)
		lua_pushnumber(L, builder->id);

	// call the routine
	RunCallInTraceback(L, cmdStr, (builder != nullptr)? 4: 3, 0, traceBack.GetErrFuncIdx(), false);
}


/*** Called at the moment the unit is completed.
 *
 * @function Callins:UnitFinished
 * @param unitID UnitID
 * @param unitDefID UnitDefID
 * @param unitTeam TeamID
 */
void CLuaHandle::UnitFinished(const CUnit* unit)
{
	static const LuaHashString cmdStr(__func__);
	UnitCallIn(cmdStr, unit);
}


/*** Called when a factory finishes construction of a unit.
 *
 * @function Callins:UnitFromFactory
 * @param unitID UnitID
 * @param unitDefID UnitDefID
 * @param unitTeam TeamID
 * @param factID UnitID
 * @param factDefID UnitDefID
 * @param userOrders boolean
 */
void CLuaHandle::UnitFromFactory(const CUnit* unit,
                                 const CUnit* factory, bool userOrders)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 9, __func__);
	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, factory->id);
	lua_pushnumber(L, factory->unitDef->id);
	lua_pushboolean(L, userOrders);

	// call the routine
	RunCallInTraceback(L, cmdStr, 6, 0, traceBack.GetErrFuncIdx(), false);
}


/*** Called when a living unit becomes a nanoframe again.
 *
 * @function Callins:UnitReverseBuilt
 * @param unitID UnitID
 * @param unitDefID UnitDefID
 * @param unitTeam TeamID
 */
void CLuaHandle::UnitReverseBuilt(const CUnit* unit)
{
	RECOIL_DETAILED_TRACY_ZONE;
	static const LuaHashString cmdStr(__func__);
	UnitCallIn(cmdStr, unit);
}


/*** Called when a unit being built starts decaying.
 *
 * @function Callins:UnitConstructionDecayed
 * @param unitID UnitID
 * @param unitDefID UnitDefID
 * @param unitTeam TeamID
 * @param timeSinceLastBuild number
 * @param iterationPeriod number
 * @param part number
 */
void CLuaHandle::UnitConstructionDecayed(const CUnit* unit, float timeSinceLastBuild, float iterationPeriod, float part)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 9, __func__);
	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, timeSinceLastBuild);
	lua_pushnumber(L, iterationPeriod);
	lua_pushnumber(L, part);

	// call the routine
	RunCallInTraceback(L, cmdStr, 6, 0, traceBack.GetErrFuncIdx(), false);
}


/*** Called when a unit is destroyed.
 *
 * @function Callins:UnitDestroyed
 * @param unitID UnitID
 * @param unitDefID UnitDefID
 * @param unitTeam TeamID
 * @param attackerID UnitID? Subject to visibility rules
 * @param attackerDefID UnitDefID? Subject to visibility rules
 * @param attackerTeam TeamID? Subject to visibility rules
 * @param weaponDefID WeaponDefID
 */
void CLuaHandle::UnitDestroyed(const CUnit* unit, const CUnit* attacker, int weaponDefID)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 9, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);

	if (!cmdStr.GetGlobalFunc(L))
		return;

	static constexpr int argCount = 3 + 3 + 1;

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);

	LuaUtils::PushAttackerInfo(L, attacker);

	lua_pushnumber(L, weaponDefID);

	// call the routine
	RunCallInTraceback(L, cmdStr, argCount, 0, traceBack.GetErrFuncIdx(), false);
}


/*** Called when a unit is transferred between teams. This is called before `UnitGiven` and in that moment unit is still assigned to the oldTeam.
 *
 * @function Callins:UnitTaken
 * @param unitID UnitID
 * @param unitDefID UnitDefID
 * @param oldTeam TeamID
 * @param newTeam TeamID
 */
void CLuaHandle::UnitTaken(const CUnit* unit, int oldTeam, int newTeam)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 7, __func__);
	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, oldTeam);
	lua_pushnumber(L, newTeam);

	// call the routine
	RunCallInTraceback(L, cmdStr, 4, 0, traceBack.GetErrFuncIdx(), false);
}


/*** Called when a unit is transferred between teams. This is called after `UnitTaken` and in that moment unit is assigned to the newTeam.
 *
 * @function Callins:UnitGiven
 * @param unitID UnitID
 * @param unitDefID UnitDefID
 * @param newTeam TeamID
 * @param oldTeam TeamID
 */
void CLuaHandle::UnitGiven(const CUnit* unit, int oldTeam, int newTeam)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 7, __func__);
	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, newTeam);
	lua_pushnumber(L, oldTeam);

	// call the routine
	RunCallInTraceback(L, cmdStr, 4, 0, traceBack.GetErrFuncIdx(), false);
}


/*** Called when a unit is idle (empty command queue).
 *
 * @function Callins:UnitIdle
 * @param unitID UnitID
 * @param unitDefID UnitDefID
 * @param unitTeam TeamID
 */
void CLuaHandle::UnitIdle(const CUnit* unit)
{
	static const LuaHashString cmdStr(__func__);
	UnitCallIn(cmdStr, unit);
}


/*** Called after when a unit accepts a command, after `AllowCommand` returns true.
 *
 * @function Callins:UnitCommand
 * @param unitID UnitID
 * @param unitDefID UnitDefID
 * @param unitTeam TeamID
 * @param cmdID integer
 * @param cmdParams table
 * @param options CommandOptions
 * @param cmdTag integer
 */
void CLuaHandle::UnitCommand(const CUnit* unit, const Command& command, int playerNum, bool fromSynced, bool fromLua)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 1 + 7 + 3, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	const int argc = LuaUtils::PushUnitAndCommand(L, unit, command);

	lua_pushnumber(L, playerNum);
	lua_pushboolean(L, fromSynced);
	lua_pushboolean(L, fromLua);

	// call the routine
	RunCallInTraceback(L, cmdStr, argc + 3, 0, traceBack.GetErrFuncIdx(), false);
}


/*** Called when a unit completes a command.
 *
 * @function Callins:UnitCmdDone
 * @param unitID UnitID
 * @param unitDefID UnitDefID
 * @param unitTeam TeamID
 * @param cmdID integer
 * @param cmdParams table
 * @param options CommandOptions
 * @param cmdTag integer
 */
void CLuaHandle::UnitCmdDone(const CUnit* unit, const Command& command)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 8, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	LuaUtils::PushUnitAndCommand(L, unit, command);

	// call the routine
	RunCallInTraceback(L, cmdStr, 7, 0, traceBack.GetErrFuncIdx(), false);
}


/*** Called when a unit is damaged (after UnitPreDamaged).
 *
 * @function Callins:UnitDamaged
 * @param unitID UnitID
 * @param unitDefID UnitDefID
 * @param unitTeam TeamID
 * @param damage number
 * @param paralyzer number
 * @param weaponDefID WeaponDefID
 * @param projectileID ProjectileID
 * @param attackerID UnitID
 * @param attackerDefID UnitDefID
 * @param attackerTeam TeamID
 */
void CLuaHandle::UnitDamaged(
	const CUnit* unit,
	const CUnit* attacker,
	float damage,
	int weaponDefID,
	int projectileID,
	bool paralyzer)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 11, __func__);

	static const LuaHashString cmdStr(__func__);
	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	if (!cmdStr.GetGlobalFunc(L))
		return;

	static constexpr int argCount = 7 + 3;

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, damage);
	lua_pushboolean(L, paralyzer);
	// these two do not count as information leaks
	lua_pushnumber(L, weaponDefID);
	lua_pushnumber(L, projectileID);

	LuaUtils::PushAttackerInfo(L, attacker);

	// call the routine
	RunCallInTraceback(L, cmdStr, argCount, 0, traceBack.GetErrFuncIdx(), false);
}

/*** Called when a unit changes its stun status.
 *
 * @function Callins:UnitStunned
 * @param unitID UnitID
 * @param unitDefID UnitDefID
 * @param unitTeam TeamID
 * @param stunned boolean
 */
void CLuaHandle::UnitStunned(
	const CUnit* unit,
	bool stunned)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 5, __func__);

	static const LuaHashString cmdStr(__func__);
	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushboolean(L, stunned);

	// call the routine
	RunCallInTraceback(L, cmdStr, 4, 0, traceBack.GetErrFuncIdx(), false);
}


/***
 * Called when a unit gains experience greater or equal to the minimum limit set by calling `Spring.SetExperienceGrade`.
 *
 * Should be called more reliably with small values of experience grade.
 *
 * @function Callins:UnitExperience
 *
 * @param unitID UnitID
 * @param unitDefID UnitDefID
 * @param unitTeam TeamID
 * @param experience number
 * @param oldExperience number
 */
void CLuaHandle::UnitExperience(const CUnit* unit, float oldExperience)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 8, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, unit->experience);
	lua_pushnumber(L, oldExperience);

	// call the routine
	RunCallInTraceback(L, cmdStr, 5, 0, traceBack.GetErrFuncIdx(), false);
}


/*** Called when a unit's harvestStorage is full (according to its unitDef's entry).
 *
 * @function Callins:UnitHarvestStorageFull
 * @param unitID UnitID
 * @param unitDefID UnitDefID
 * @param unitTeam TeamID
 */
void CLuaHandle::UnitHarvestStorageFull(const CUnit* unit)
{
	static const LuaHashString cmdStr(__func__);
	UnitCallIn(cmdStr, unit);
}


/******************************************************************************/

/*** Called when a unit emits a seismic ping.
 *
 * @function Callins:UnitSeismicPing
 *
 * See `seismicSignature`.
 *
 * @param x number
 * @param y number
 * @param z number
 * @param strength number
 * @param allyTeam AllyTeamID
 * @param unitID UnitID
 * @param unitDefID UnitDefID
 */
void CLuaHandle::UnitSeismicPing(const CUnit* unit, int allyTeam,
                                 const float3& pos, float strength)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 9, __func__);
	int readAllyTeam = GetHandleReadAllyTeam(L);
	if ((readAllyTeam >= 0) && (unit->losStatus[readAllyTeam] & LOS_INLOS)) {
		return; // don't need to see this ping
	}

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, pos.x);
	lua_pushnumber(L, pos.y);
	lua_pushnumber(L, pos.z);
	lua_pushnumber(L, strength);
	if (GetHandleFullRead(L)) {
		lua_pushnumber(L, allyTeam);
		lua_pushnumber(L, unit->id);
		lua_pushnumber(L, unit->unitDef->id);
	}

	// call the routine
	RunCallIn(L, cmdStr, GetHandleFullRead(L) ? 7 : 4, 0);
}


/******************************************************************************/

void CLuaHandle::LosCallIn(const LuaHashString& hs,
                           const CUnit* unit, int allyTeam)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 6, __func__);
	if (!hs.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->team);
	if (GetHandleFullRead(L)) {
		lua_pushnumber(L, allyTeam);
		lua_pushnumber(L, unit->unitDef->id);
	}

	// call the routine
	RunCallIn(L, hs, GetHandleFullRead(L) ? 4 : 2, 0);
}


/***
 * Called when a unit enters radar of an allyteam.
 *
 * Also called when a unit enters LOS without any radar coverage.
 *
 * @function Callins:UnitEnteredRadar
 * @param unitID UnitID
 * @param unitTeam TeamID
 * @param allyTeam AllyTeamID
 * @param unitDefID UnitDefID
 */
void CLuaHandle::UnitEnteredRadar(const CUnit* unit, int allyTeam)
{
	RECOIL_DETAILED_TRACY_ZONE;
	static const LuaHashString hs(__func__);
	LosCallIn(hs, unit, allyTeam);
}


/***
 * Called when a unit enters LOS of an allyteam.
 *
 * Its called after the unit is in LOS, so you can query that unit.
 *
 * @function Callins:UnitEnteredLos
 * @param unitID UnitID
 * @param unitTeam TeamID
 * @param allyTeam AllyTeamID who's LOS the unit entered.
 * @param unitDefID UnitDefID
 */
void CLuaHandle::UnitEnteredLos(const CUnit* unit, int allyTeam)
{
	static const LuaHashString hs(__func__);
	LosCallIn(hs, unit, allyTeam);
}


/***
 * Called when a unit leaves radar of an allyteam.
 *
 * Also called when a unit leaves LOS without any radar coverage.
 * For widgets, this is called just after a unit leaves radar coverage, so
 * widgets cannot get the position of units that left their radar.
 *
 * @function Callins:UnitLeftRadar
 * @param unitID UnitID
 * @param unitTeam TeamID
 * @param allyTeam AllyTeamID
 * @param unitDefID UnitDefID
 */
void CLuaHandle::UnitLeftRadar(const CUnit* unit, int allyTeam)
{
	RECOIL_DETAILED_TRACY_ZONE;
	static const LuaHashString hs(__func__);
	LosCallIn(hs, unit, allyTeam);
}


/***
 * Called when a unit leaves LOS of an allyteam.
 *
 * For widgets, this one is called just before the unit leaves los, so you can still get the position of a unit that left los.
 *
 * @function Callins:UnitLeftLos
 * @param unitID UnitID
 * @param unitTeam TeamID
 * @param allyTeam AllyTeamID
 * @param unitDefID UnitDefID
 */
void CLuaHandle::UnitLeftLos(const CUnit* unit, int allyTeam)
{
	static const LuaHashString hs(__func__);
	LosCallIn(hs, unit, allyTeam);
}


/******************************************************************************
 * Transport
 * @section transport
******************************************************************************/


/*** Called when a unit is loaded by a transport.
 *
 * @function Callins:UnitLoaded
 * @param unitID UnitID
 * @param unitDefID UnitDefID
 * @param unitTeam TeamID
 * @param transportID UnitID
 * @param transportTeam TeamID
 */
void CLuaHandle::UnitLoaded(const CUnit* unit, const CUnit* transport)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 8, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, transport->id);
	lua_pushnumber(L, transport->team);

	// call the routine
	RunCallInTraceback(L, cmdStr, 5, 0, traceBack.GetErrFuncIdx(), false);
}


/*** Called when a unit is unloaded by a transport.
 *
 * @function Callins:UnitUnloaded
 * @param unitID UnitID
 * @param unitDefID UnitDefID
 * @param unitTeam TeamID
 * @param transportID UnitID
 * @param transportTeam TeamID
 */
void CLuaHandle::UnitUnloaded(const CUnit* unit, const CUnit* transport)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 8, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, transport->id);
	lua_pushnumber(L, transport->team);

	// call the routine
	RunCallInTraceback(L, cmdStr, 5, 0, traceBack.GetErrFuncIdx(), false);
}


/******************************************************************************
 * Unit Interactions
 * @section unit_interactions
******************************************************************************/


/***
 *
 * @function Callins:UnitEnteredUnderwater
 * @param unitID UnitID
 * @param unitDefID UnitDefID
 * @param unitTeam TeamID
 */
void CLuaHandle::UnitEnteredUnderwater(const CUnit* unit)
{
	static const LuaHashString cmdStr(__func__);
	UnitCallIn(cmdStr, unit);
}


/***
 *
 * @function Callins:UnitEnteredWater
 * @param unitID UnitID
 * @param unitDefID UnitDefID
 * @param unitTeam TeamID
 */
void CLuaHandle::UnitEnteredWater(const CUnit* unit)
{
	static const LuaHashString cmdStr(__func__);
	UnitCallIn(cmdStr, unit);
}


/***
 *
 * @function Callins:UnitLeftAir
 *
 * @param unitID UnitID
 * @param unitDefID UnitDefID
 * @param unitTeam TeamID
 */
void CLuaHandle::UnitEnteredAir(const CUnit* unit)
{
	static const LuaHashString cmdStr(__func__);
	UnitCallIn(cmdStr, unit);
}


/***
 *
 * @function Callins:UnitLeftUnderwater
 *
 * @param unitID UnitID
 * @param unitDefID UnitDefID
 * @param unitTeam TeamID
 */
void CLuaHandle::UnitLeftUnderwater(const CUnit* unit)
{
	static const LuaHashString cmdStr(__func__);
	UnitCallIn(cmdStr, unit);
}

/***
 *
 * @function Callins:UnitLeftWater
 *
 * @param unitID UnitID
 * @param unitDefID UnitDefID
 * @param unitTeam TeamID
 */
void CLuaHandle::UnitLeftWater(const CUnit* unit)
{
	static const LuaHashString cmdStr(__func__);
	UnitCallIn(cmdStr, unit);
}


/***
 *
 * @function Callins:UnitEnteredAir
 *
 * @param unitID UnitID
 * @param unitDefID UnitDefID
 * @param unitTeam TeamID
 */
void CLuaHandle::UnitLeftAir(const CUnit* unit)
{
	static const LuaHashString cmdStr(__func__);
	UnitCallIn(cmdStr, unit);
}


/*** Called when a unit cloaks.
 *
 * @function Callins:UnitCloaked
 *
 * @param unitID UnitID
 * @param unitDefID UnitDefID
 * @param unitTeam TeamID
 */
void CLuaHandle::UnitCloaked(const CUnit* unit)
{
	static const LuaHashString cmdStr(__func__);
	UnitCallIn(cmdStr, unit);
}


/*** Called when a unit decloaks.
 *
 * @function Callins:UnitDecloaked
 *
 * @param unitID UnitID
 * @param unitDefID UnitDefID
 * @param unitTeam TeamID
 */
void CLuaHandle::UnitDecloaked(const CUnit* unit)
{
	static const LuaHashString cmdStr(__func__);
	UnitCallIn(cmdStr, unit);
}


/*** Called when two units collide.
 *
 * Both units must be registered with `Script.SetWatchUnit`.
 *
 * @function Callins:UnitUnitCollision
 * @param colliderID UnitID
 * @param collideeID UnitID
 */
bool CLuaHandle::UnitUnitCollision(const CUnit* collider, const CUnit* collidee)
{
	RECOIL_DETAILED_TRACY_ZONE;
	static int returnValueDeprecationWarningIssued = -1;

	// if empty, we are not a LuaHandleSynced
	if (watchUnitDefs.empty())
		return false;

	if (!watchUnitDefs[collider->unitDef->id])
		return false;
	if (!watchUnitDefs[collidee->unitDef->id])
		return false;

	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 5, __func__);

	static const LuaHashString cmdStr(__func__);
	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	if (!cmdStr.GetGlobalFunc(L))
		return false;

	lua_pushnumber(L, collider->id);
	lua_pushnumber(L, collidee->id);

	RunCallInTraceback(L, cmdStr, 2, 1, traceBack.GetErrFuncIdx(), false);

	// if nothing is returned, this is the correct behaviour now.
	if (lua_isnone(L, -1))
		return false;

	if (returnValueDeprecationWarningIssued < gs->frameNum) {
		LOG_L(L_ERROR, "[%s] return value is deprecated and ignored.", __func__);
		returnValueDeprecationWarningIssued = gs->frameNum;
	}

	const bool ret = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return ret;
}


/*** Called when a unit collides with a feature.
 *
 * @function Callins:UnitFeatureCollision
 *
 * The unit must be registered with `Script.SetWatchUnit` and the feature registered with `Script.SetWatchFeature`.
 *
 * @param colliderID UnitID
 * @param collideeID UnitID
 */
bool CLuaHandle::UnitFeatureCollision(const CUnit* collider, const CFeature* collidee)
{
	RECOIL_DETAILED_TRACY_ZONE;
	static int returnValueDeprecationWarningIssued = -1;

	// if empty, we are not a LuaHandleSynced (and must always return false)
	if (watchUnitDefs.empty())
		return false;
	if (watchFeatureDefs.empty())
		return false;

	if (!watchUnitDefs[collider->unitDef->id])
		return false;
	if (!watchFeatureDefs[collidee->def->id])
		return false;

	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 5, __func__);

	static const LuaHashString cmdStr(__func__);
	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	if (!cmdStr.GetGlobalFunc(L))
		return false;

	lua_pushnumber(L, collider->id);
	lua_pushnumber(L, collidee->id);

	RunCallInTraceback(L, cmdStr, 2, 1, traceBack.GetErrFuncIdx(), false);

	// if nothing is returned, this is the correct behaviour now.
	if (lua_isnone(L, -1))
		return false;

	if (returnValueDeprecationWarningIssued < gs->frameNum) {
		LOG_L(L_ERROR, "[%s] return value is deprecated and ignored.", __func__);
		returnValueDeprecationWarningIssued = gs->frameNum;
	}

	const bool ret = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return ret;
}

/***
 *
 * @function Callins:UnitMoveFailed
 *
 * @param unitID UnitID
 * @param unitDefID UnitDefID
 * @param unitTeam TeamID
 */
void CLuaHandle::UnitMoveFailed(const CUnit* unit)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// if empty, we are not a LuaHandleSynced (and must always return false)
	if (watchUnitDefs.empty())
		return;
	if (!watchUnitDefs[unit->unitDef->id])
		return;

	static const LuaHashString cmdStr(__func__);
	UnitCallIn(cmdStr, unit);
}


/***
 *
 * @function Callins:UnitArrivedAtGoal
 *
 * @param unitID UnitID
 * @param unitDefID UnitDefID
 * @param unitTeam TeamID
 */
void CLuaHandle::UnitArrivedAtGoal(const CUnit* unit)
{
	RECOIL_DETAILED_TRACY_ZONE;

	static const LuaHashString cmdStr(__func__);
	UnitCallIn(cmdStr, unit);
}


/*** Called just before a unit is invalid, after it finishes its death animation.
 *
 * @function Callins:RenderUnitDestroyed
 *
 * @param unitID UnitID
 * @param unitDefID UnitDefID
 * @param unitTeam TeamID
 */
void CLuaHandle::RenderUnitDestroyed(const CUnit* unit)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 9, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);

	if (!cmdStr.GetGlobalFunc(L))
		return;

	const int argCount = 3;

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);

	// call the routine
	RunCallInTraceback(L, cmdStr, argCount, 0, traceBack.GetErrFuncIdx(), false);
}


/******************************************************************************
 * Features
 * @section features
******************************************************************************/


/*** Called when a feature is created.
 *
 * @function Callins:FeatureCreated
 *
 * @param featureID FeatureID
 * @param allyTeamID AllyTeamID
 */
void CLuaHandle::FeatureCreated(const CFeature* feature)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 5, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);
	static const LuaHashString cmdStr(__func__);

	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, feature->id);
	lua_pushnumber(L, feature->allyteam);

	// call the routine
	RunCallInTraceback(L, cmdStr, 2, 0, traceBack.GetErrFuncIdx(), false);
}


/*** Called when a feature is destroyed.
 *
 * @function Callins:FeatureDestroyed
 *
 * @param featureID FeatureID
 * @param allyTeamID AllyTeamID
 */
void CLuaHandle::FeatureDestroyed(const CFeature* feature)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 5, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, feature->id);
	lua_pushnumber(L, feature->allyteam);

	// call the routine
	RunCallInTraceback(L, cmdStr, 2, 0, traceBack.GetErrFuncIdx(), false);
}


/*** Called when a feature is damaged.
 *
 * @function Callins:FeatureDamaged
 *
 * @param featureID FeatureID
 * @param featureDefID FeatureDefID
 * @param featureTeam TeamID
 * @param damage number
 * @param weaponDefID WeaponDefID
 * @param projectileID ProjectileID
 * @param attackerID UnitID
 * @param attackerDefID UnitDefID
 * @param attackerTeam TeamID
 */
void CLuaHandle::FeatureDamaged(
	const CFeature* feature,
	const CUnit* attacker,
	float damage,
	int weaponDefID,
	int projectileID)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 11, __func__);
	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	int argCount = 6 + 3;

	lua_pushnumber(L, feature->id);
	lua_pushnumber(L, feature->def->id);
	lua_pushnumber(L, feature->team);
	lua_pushnumber(L, damage);

	// these two do not count as information leaks
	lua_pushnumber(L, weaponDefID);
	lua_pushnumber(L, projectileID);

	LuaUtils::PushAttackerInfo(L, attacker);

	// call the routine
	RunCallInTraceback(L, cmdStr, argCount, 0, traceBack.GetErrFuncIdx(), false);
}


/******************************************************************************
 * Projectiles
 * @section projectiles
 *
 * The following Callins are only called for weaponDefIDs registered
 * via Script.SetWatchWeapon or Script.SetWatchProjectile.
******************************************************************************/

/*** Called when the projectile is created.
 *
 * @function Callins:ProjectileCreated
 *
 * Note that weaponDefID is missing if the projectile is spawned as part of a burst, but `Spring.GetProjectileDefID` and `Spring.GetProjectileName` still work in callin scope using proID.
 *
 * @param proID ProjectileID
 * @param proOwnerID UnitID
 * @param weaponDefID WeaponDefID
 *
 * @see Script.SetWatchProjectile
 * @see Script.SetWatchWeapon
 */
void CLuaHandle::ProjectileCreated(const CProjectile* p)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// if empty, we are not a LuaHandleSynced
	if (watchProjectileDefs.empty())
		return;

	if (!p->weapon && !p->piece)
		return;

	assert(p->synced);

	const CUnit* owner = p->owner();
	const CWeaponProjectile* wp = p->weapon? static_cast<const CWeaponProjectile*>(p): nullptr;
	const WeaponDef* wd = p->weapon? wp->GetWeaponDef(): nullptr;

	// if this weapon-type is not being watched, bail
	if (p->weapon && (wd == nullptr || !watchProjectileDefs[wd->id]))
		return;
	if (p->piece && !watchProjectileDefs[watchProjectileDefs.size() - 1])
		return;

	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 5, __func__);

	static const LuaHashString cmdStr(__func__);

	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, p->id);
	lua_pushnumber(L, ((owner != nullptr)? owner->id: -1));
	lua_pushnumber(L, ((wd != nullptr)? wd->id: -1));

	// call the routine
	RunCallIn(L, cmdStr, 3, 0);
}


/*** Called when the projectile is destroyed.
 *
 * @function Callins:ProjectileDestroyed
 * @param proID ProjectileID
 * @param ownerID UnitID
 * @param proWeaponDefID WeaponDefID
 *
 * @see Script.SetWatchProjectile
 * @see Script.SetWatchWeapon
 */
void CLuaHandle::ProjectileDestroyed(const CProjectile* p)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// if empty, we are not a LuaHandleSynced
	if (watchProjectileDefs.empty())
		return;

	if (!p->weapon && !p->piece)
		return;

	assert(p->synced);

	int ownerID = p->GetOwnerID();
	int pwdefID = -1;

	if (p->weapon) {
		const CWeaponProjectile* wp = static_cast<const CWeaponProjectile*>(p);
		const WeaponDef* wd = wp->GetWeaponDef();

		// if this weapon-type is not being watched, bail
		if (wd == nullptr || !watchProjectileDefs[wd->id])
			return;

		pwdefID = wd->id;
	}
	if (p->piece && !watchProjectileDefs[watchProjectileDefs.size() - 1])
		return;

	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 6, __func__);

	static const LuaHashString cmdStr(__func__);

	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, p->id);
	lua_pushnumber(L, ownerID);
	lua_pushnumber(L, pwdefID);

	// call the routine
	RunCallIn(L, cmdStr, 3, 0);
}

/******************************************************************************/

/***
 * Helper to get Explosion visibility.
 */

bool CLuaHandle::IsExplosionVisible(const WeaponDef* weaponDef, const CExplosionParams& params)
{
	const int allyTeamID = CLuaHandle::GetHandleReadAllyTeam(L);
	return explGenHandler.PredictExplosionVisible(weaponDef, params, allyTeamID);
}

/*** Called when an explosion occurs.
 *
 * @function Callins:Explosion
 *
 * Only called for weaponDefIDs registered via Script.SetWatchExplosion or Script.SetWatchWeapon.
 *
 * @param weaponDefID WeaponDefID
 * @param px number
 * @param py number
 * @param pz number
 * @param attackerID UnitID
 * @param projectileID ProjectileID
  *
 * @return boolean noGfx if then no graphical effects are drawn by the engine for this explosion.
 *
 * @see Script.SetWatchExplosion
 * @see Script.SetWatchWeapon
 */
bool CLuaHandle::Explosion(int weaponDefID, const WeaponDef* weaponDef, const CExplosionParams& params)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// piece-projectile collision (*ALL* other
	// explosion events pass valid weaponDefIDs)
	if (weaponDefID < 0)
		return false;

	// if empty, we are a handle with no explosion watch support.
	if (watchExplosionDefs.empty())
		return false;
	if (!watchExplosionDefs[weaponDefID])
		return false;

	bool synced = GetHandleSynced(L);

	if (!synced && !IsExplosionVisible(weaponDef, params))
		return false;

	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 7, __func__);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return false;

	lua_pushnumber(L, weaponDefID);
	lua_pushnumber(L, params.pos.x);
	lua_pushnumber(L, params.pos.y);
	lua_pushnumber(L, params.pos.z);
	if (params.owner != nullptr) {
		lua_pushnumber(L, params.owner->id);
	} else {
		lua_pushnil(L); // for backward compatibility
	}
	lua_pushnumber(L, params.projectileID);

	// call the routine
	if (!RunCallIn(L, cmdStr, 6, 1))
		return false;

	// get the results
	const bool retval = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return synced && retval;
}


/*** Called when a units stockpile of weapons increases or decreases.
 *
 * @function Callins:StockpileChanged
 *
 * @param unitID UnitID
 * @param unitDefID UnitDefID
 * @param unitTeam TeamID
 * @param weaponNum integer
 * @param oldCount integer
 * @param newCount integer
 */
void CLuaHandle::StockpileChanged(const CUnit* unit,
                                  const CWeapon* weapon, int oldCount)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 8, __func__);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, weapon->weaponNum + LUA_WEAPON_BASE_INDEX);
	lua_pushnumber(L, oldCount);
	lua_pushnumber(L, weapon->numStockpiled);

	// call the routine
	RunCallIn(L, cmdStr, 6, 0);
}



/*** Receives messages from unsynced sent via `Spring.SendLuaRulesMsg` or `Spring.SendLuaUIMsg`.
 *
 * @function Callins:RecvLuaMsg
 * @param msg string
 * @param playerID PlayerID
 */
bool CLuaHandle::RecvLuaMsg(const string& msg, int playerID)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 8, __func__);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return false;

	lua_pushsstring(L, msg); // allows embedded 0's
	lua_pushnumber(L, playerID);

	// call the routine
	if (!RunCallIn(L, cmdStr, 2, 1))
		return false;

	const bool retval = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return retval;
}


/******************************************************************************/

void CLuaHandle::SetDevMode(bool value)
{
	if (value == devMode)
		return;

	devMode = value;

	for (const auto* lcd : LUAHANDLE_CONTEXTS) {
		for (const auto* lc : *lcd) {
			if (!lc || !lc->owner)
				continue;

			lc->owner->EnactDevMode();
		}
	}
}


/* Toggles between empty table and filling it with lua module functions. 
 */
void CLuaHandle::SwapEnableModule(lua_State* L, bool enabled, const char* moduleName, lua_CFunction func) const
{
	// check if module table already exists
	lua_getglobal(L, moduleName);
	const bool missing = lua_isnil(L, -1);
	lua_pop(L, 1);

	// create an empty module table
	if (missing) {
		lua_createtable(L, 0, 0);
		lua_setglobal(L, moduleName);
	}

	if (enabled) {
		// relink methods
		lua_pushvalue(L, LUA_GLOBALSINDEX);
		LUA_OPEN_LIB(L, func);
		lua_pop(L, 1);
	}
	else {
		// unlink all methods
		lua_getglobal(L, moduleName);
		lua_pushnil(L);
		while (lua_next(L, -2))
		{
			lua_pop(L, 1);		// pop value
			lua_pushnil(L);
			lua_rawset(L, -3);	// pop new value and key
			lua_pushnil(L);		// restart iteration
		}
		lua_pop(L, 1);
	}
}


void CLuaHandle::HandleLuaMsg(int playerID, int script, int mode, const std::vector<std::uint8_t>& data)
{
	RECOIL_DETAILED_TRACY_ZONE;
	std::string msg;
	msg.resize(data.size());
	std::copy(data.begin(), data.end(), msg.begin());

	switch (script) {
		case LUA_HANDLE_ORDER_UI: {
			if (luaUI != nullptr) {
				bool sendMsg = false;

				switch (mode) {
					case 0: { sendMsg = true; } break;
					case 's': { sendMsg = gu->spectating; } break;
					case 'a': {
						const CPlayer* player = playerHandler.Player(playerID);

						if (player == nullptr)
							return;

						if (gu->spectatingFullView) {
							sendMsg = true;
						} else if (player->spectator) {
							sendMsg = gu->spectating;
						} else {
							const int msgAllyTeam = teamHandler.AllyTeam(player->team);
							sendMsg = teamHandler.Ally(msgAllyTeam, gu->myAllyTeam);
						}
					} break;
				}

				if (sendMsg)
					luaUI->RecvLuaMsg(msg, playerID);
			}
		} break;

		case LUA_HANDLE_ORDER_GAIA: {
			if (luaGaia != nullptr)
				luaGaia->RecvLuaMsg(msg, playerID);
		} break;

		case LUA_HANDLE_ORDER_RULES: {
			if (luaRules != nullptr)
				luaRules->RecvLuaMsg(msg, playerID);
		} break;
	}
}


/*** Called when a chat command '/save' or '/savegame' is received.
 *
 * @function Callins:Save
 * @param zip table a userdatum representing the savegame zip file. See Lua_SaveLoad.
 */
void CLuaHandle::Save(zipFile archive)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// LuaUI does not get this call-in
	if (GetUserMode())
		return;

	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 3, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	// Save gets ZipFileWriter userdatum as single argument
	LuaZipFileWriter::PushNew(L, "", archive);

	// call the routine
	RunCallIn(L, cmdStr, 1, 0);
}


/*** Called when the unsynced copy of the height-map is altered.
 *
 * @function Callins:UnsyncedHeightMapUpdate
 * @return integer x1
 * @return integer z1
 * @return integer x2
 * @return integer z2
 */
void CLuaHandle::UnsyncedHeightMapUpdate(const SRectangle& rect)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 6, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, rect.x1);
	lua_pushnumber(L, rect.z1);
	lua_pushnumber(L, rect.x2);
	lua_pushnumber(L, rect.z2);

	// call the routine
	RunCallIn(L, cmdStr, 4, 0);
}


/*** Called for every draw frame (including when the game is paused) and at least once per sim frame except when catching up.
 *
 * @function Callins:Update
 * @param dt number the time since the last update.
 */
void CLuaHandle::Update()
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 2, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	if (game) // null in LuaMenu
		lua_pushnumber(L, game->updateDeltaSeconds);

	RunCallIn(L, cmdStr, game ? 1 : 0, 0);
}


/*** Called whenever the window is resized.
 *
 * @function Callins:ViewResize
 * @param viewSizeX integer
 * @param viewSizeY integer
 */
void CLuaHandle::ViewResize()
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 5, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	const int winPosY_bl = globalRendering->screenSizeY - globalRendering->winSizeY - globalRendering->winPosY; //! origin BOTTOMLEFT

	lua_createtable(L, 0, 16);
	LuaPushNamedNumber(L, "screenSizeX", globalRendering->screenSizeX);
	LuaPushNamedNumber(L, "screenSizeY", globalRendering->screenSizeY);
	LuaPushNamedNumber(L, "screenPosX", globalRendering->screenPosX);
	LuaPushNamedNumber(L, "screenPosY", globalRendering->screenPosY);
	LuaPushNamedNumber(L, "windowSizeX", globalRendering->winSizeX);
	LuaPushNamedNumber(L, "windowSizeY", globalRendering->winSizeY);
	LuaPushNamedNumber(L, "windowPosX",  globalRendering->winPosX);
	LuaPushNamedNumber(L, "windowPosY",  winPosY_bl);
	LuaPushNamedNumber(L, "windowBorderTop"   , globalRendering->winBorder[0]);
	LuaPushNamedNumber(L, "windowBorderLeft"  , globalRendering->winBorder[1]);
	LuaPushNamedNumber(L, "windowBorderBottom", globalRendering->winBorder[2]);
	LuaPushNamedNumber(L, "windowBorderRight" , globalRendering->winBorder[3]);
	LuaPushNamedNumber(L, "viewSizeX",   globalRendering->viewSizeX);
	LuaPushNamedNumber(L, "viewSizeY",   globalRendering->viewSizeY);
	LuaPushNamedNumber(L, "viewPosX",    globalRendering->viewPosX);
	LuaPushNamedNumber(L, "viewPosY",    globalRendering->viewPosY);

	// call the routine
	RunCallIn(L, cmdStr, 1, 0);
}

/*** Called whenever fonts are updated. Signals the game display lists
 *   and other caches should be discarded.
 *
 * Gets called before other Update and Draw callins.
 *
 * @function Callins:FontsChanged
 */
void CLuaHandle::FontsChanged()
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 2, __func__);

	static const LuaHashString cmdStr(__func__);

	if (!cmdStr.GetGlobalFunc(L))
		return;

	RunCallIn(L, cmdStr, 0, 0);
}

/***
 * @function Callins:SunChanged
 */
void CLuaHandle::SunChanged()
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 2, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	// call the routine
	RunCallIn(L, cmdStr, 0, 0);
}

/*** Used to set the default command when a unit is selected.
 * 
 * @function Callins:DefaultCommand
 * @param type "unit"|"feature" The type of the object pointed at.
 * @param id ObjectID The `unitID` or `featureID`.
 * @param cmd integer The current command ID.
 * @return integer cmdID The command ID to use as the default, or nil to keep the current ID.
 */
bool CLuaHandle::DefaultCommand(const CUnit* unit,
                                const CFeature* feature, int& cmd)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 5, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return false;

	if (unit) {
		LuaPushString(L, "unit");
		lua_pushnumber(L, unit->id);
	}
	else if (feature) {
		LuaPushString(L, "feature");
		lua_pushnumber(L, feature->id);
	}
	else {
		lua_pushnil(L);
		lua_pushnil(L);
	}
	lua_pushnumber(L, cmd);

/* FIXME
	else if (groundPos) {
		LuaPushString(L, "ground");
		lua_pushnumber(L, groundPos->x);
		lua_pushnumber(L, groundPos->y);
		lua_pushnumber(L, groundPos->z);
		args = 4;
	}
	else {
		LuaPushString(L, "selection");
		args = 1;
	}
*/

	// call the routine
	if (!RunCallIn(L, cmdStr, 3, 1))
		return false;

	if (!lua_isnumber(L, -1)) {
		lua_pop(L, 1);
		return false;
	}

	cmd = lua_toint(L, -1);
	lua_pop(L, 1);
	return true;
}


void CLuaHandle::RunDrawCallIn(const LuaHashString& hs)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 2, __func__);
	if (!hs.GetGlobalFunc(L))
		return;

	LuaOpenGL::SetDrawingEnabled(L, true);

	// call the routine
	RunCallIn(L, hs, 0, 0);

	LuaOpenGL::SetDrawingEnabled(L, false);
}

#define DRAW_CALLIN(name)                     \
void CLuaHandle::name()                       \
{                                             \
	static const LuaHashString cmdStr(#name); \
	RunDrawCallIn(cmdStr);                    \
}


/*** Draw* Functions
 *
 * @section draw
 *
 * Inside the Draw* functions, you can use the Lua OpenGL Api to draw graphics.
 *
 * Avoid doing heavy calculations inside these callins; ideally, do the calculations elsewhere and use Draw callins only for drawing.
 */

/*** Use this callin to update textures, shaders, etc.
 *
 * @function Callins:DrawGenesis
 *
 * Doesn't render to screen!
 * Also available to LuaMenu.
 */
DRAW_CALLIN(DrawGenesis)

/*** Spring draws command queues, 'map stuff', and map marks.
 *
 * @function Callins:DrawWorld
 */
DRAW_CALLIN(DrawWorld)

/*** Spring draws units, features, some water types, cloaked units, and the sun.
 *
 * @function Callins:DrawWorldPreUnit
 */
DRAW_CALLIN(DrawWorldPreUnit)

/*** Called before decals are drawn
 *
 * @function Callins:DrawPreDecals
 */
DRAW_CALLIN(DrawPreDecals)

/***
 * @function Callins:DrawWaterPost
 */
DRAW_CALLIN(DrawWaterPost)

/*** Invoked after semi-transparent shadows pass is about to conclude
 * @function Callins:DrawShadowPassTransparent
 *
 * This callin has depth and color buffer of shadowmap bound via FBO as well as the FFP state to do "semi-transparent" shadows pass (traditionally only used to draw shadows of shadow casting semi-transparent particles). Can be used to draw nice colored shadows.
 */
DRAW_CALLIN(DrawShadowPassTransparent)

/*** @function Callins:DrawWorldShadow
 *
 */
DRAW_CALLIN(DrawWorldShadow)

/*** @function Callins:DrawWorldReflection
 *
 */
DRAW_CALLIN(DrawWorldReflection)

/*** @function Callins:DrawWorldRefraction
 *
 */
DRAW_CALLIN(DrawWorldRefraction)

/*** Runs at the start of the forward pass when a custom map shader has been assigned via `Spring.SetMapShader` (convenient for setting uniforms).
 *
 * @function Callins:DrawGroundPreForward
 */
DRAW_CALLIN(DrawGroundPreForward)

/*** @function Callins:DrawGroundPostForward
 *
 */
DRAW_CALLIN(DrawGroundPostForward)

/*** Runs at the start of the deferred pass when a custom map shader has been assigned via `Spring.SetMapShader` (convenient for setting uniforms).
 *
 * @function Callins:DrawGroundPreDeferred
 */
DRAW_CALLIN(DrawGroundPreDeferred)

/*** @function Callins:DrawGroundDeferred
 *
 */
DRAW_CALLIN(DrawGroundDeferred)

/*** This runs at the end of its respective deferred pass.
 *
 * @function Callins:DrawGroundPostDeferred
 *
 * Allows proper frame compositing (with ground flashes/decals/foliage/etc, which are drawn between it and `DrawWorldPreUnit`) via `gl.CopyToTexture`.
 */
DRAW_CALLIN(DrawGroundPostDeferred)

/*** Runs at the end of the unit deferred pass.
 *
 * @function Callins:DrawUnitsPostDeferred
 *
 * Informs Lua code it should make use of the $model_gbuffer_* textures before another pass overwrites them (and to allow proper blending with e.g. cloaked objects which are drawn between these events and DrawWorld via gl.CopyToTexture). N.B. The *PostDeferred events are only sent (and only have a real purpose) if forward drawing is disabled.
 */
DRAW_CALLIN(DrawUnitsPostDeferred)

/*** Runs at the end of the feature deferred pass to inform Lua code it should make use of the $model_gbuffer_* textures before another pass overwrites them (and to allow proper blending with e.g. cloaked objects which are drawn between these events and DrawWorld via gl.CopyToTexture). N.B. The *PostDeferred events are only sent (and only have a real purpose) if forward drawing is disabled.
 *
 * @function Callins:DrawFeaturesPostDeferred
 */
DRAW_CALLIN(DrawFeaturesPostDeferred)

/*** @function Callins:DrawShadowUnitsLua
 *
 */
DRAW_CALLIN(DrawShadowUnitsLua)

/*** @function Callins:DrawShadowFeaturesLua
 *
 */
DRAW_CALLIN(DrawShadowFeaturesLua)

/*** @function Callins:DrawBuildSquare
 * Called when build square data is computed, before engine rendering.
 * Grid dimensions can be inferred from UnitDefs[unitDefID].xsize and UnitDefs[unitDefID].zsize.
 * Grid origin in square coords: x - xsize/2, z - zsize/2 (accounting for facing).
 * @param unitDefID UnitDefID
 * @param x integer build position x
 * @param z integer build position z
 * @param facing number build facing
 * @param statuses table flat 1D row-major array of BuildSquareStatus values: BLOCKED=0, OCCUPIED=1, RECLAIMABLE=2, OPEN=3
 */
void CLuaHandle::DrawBuildSquare(int unitDefID, int x, int z, int facing, const std::vector<uint8_t>& statuses)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 8, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, unitDefID);
	lua_pushnumber(L, x);
	lua_pushnumber(L, z);
	lua_pushnumber(L, facing);

	lua_createtable(L, statuses.size(), 0);
	for (size_t i = 0; i < statuses.size(); ++i) {
		lua_pushinteger(L, statuses[i]);
		lua_rawseti(L, -2, i + 1);
	}

	LuaOpenGL::SetDrawingEnabled(L, true);
	RunCallIn(L, cmdStr, 5, 0);
	LuaOpenGL::SetDrawingEnabled(L, false);
}

/***
 * DrawWorldPreParticles is called multiples times per draw frame.
 * Each call has a different permutation of values for drawAboveWater, drawBelowWater, drawReflection, and drawRefraction.
 *
 * @function Callins:DrawWorldPreParticles
 * @param drawAboveWater boolean
 * @param drawBelowWater boolean
 * @param drawReflection boolean
 * @param drawRefraction boolean
 */
void CLuaHandle::DrawWorldPreParticles(bool drawAboveWater, bool drawBelowWater, bool drawReflection, bool drawRefraction)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 6, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushboolean(L, drawAboveWater);
	lua_pushboolean(L, drawBelowWater);
	lua_pushboolean(L, drawReflection);
	lua_pushboolean(L, drawRefraction);

	LuaOpenGL::SetDrawingEnabled(L, true);

	RunCallIn(L, cmdStr, 4, 0);

	LuaOpenGL::SetDrawingEnabled(L, false);
}

inline void CLuaHandle::DrawScreenCommon(const LuaHashString& cmdStr)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, globalRendering->viewSizeX);
	lua_pushnumber(L, globalRendering->viewSizeY);

	LuaOpenGL::SetDrawingEnabled(L, true);

	// call the routine
	RunCallIn(L, cmdStr, 2, 0);

	LuaOpenGL::SetDrawingEnabled(L, false);
}

/*** Also available to LuaMenu.
 *
 * @function Callins:DrawScreen
 * @param viewSizeX integer
 * @param viewSizeY integer
 */
void CLuaHandle::DrawScreen()
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 4, __func__);
	static const LuaHashString cmdStr(__func__);

	DrawScreenCommon(cmdStr);
}


/***
 * @function Callins:DrawScreenEffects
 * @param viewSizeX integer
 * @param viewSizeY integer
 */
void CLuaHandle::DrawScreenEffects()
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 4, __func__);
	static const LuaHashString cmdStr(__func__);

	DrawScreenCommon(cmdStr);
}


/*** Similar to DrawScreenEffects, this can be used to alter the contents of a frame after it has been completely rendered (i.e. World, MiniMap, Menu, UI).
 *
 * @function Callins:DrawScreenPost
 *
 * Note: This callin is invoked after the software rendered cursor (configuration variable HardwareCursor=0) is drawn.
 *
 * @param viewSizeX integer
 * @param viewSizeY integer
 */
void CLuaHandle::DrawScreenPost()
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 4, __func__);
	static const LuaHashString cmdStr(__func__);

	DrawScreenCommon(cmdStr);
}


/***
 *
 * @function Callins:DrawInMiniMap
 * @param sx integer relative to the minimap's position and scale.
 * @param sy integer relative to the minimap's position and scale.
 */
void CLuaHandle::DrawInMiniMap()
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 4, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, minimap->GetSizeX());
	lua_pushnumber(L, minimap->GetSizeY());

	const bool origDrawingState = LuaOpenGL::IsDrawingEnabled(L);
	LuaOpenGL::SetDrawingEnabled(L, true);

	// call the routine
	RunCallIn(L, cmdStr, 2, 0);

	LuaOpenGL::SetDrawingEnabled(L, origDrawingState);
}


/***
 *
 * @function Callins:DrawInMiniMapBackground
 * @param sx integer relative to the minimap's position and scale.
 * @param sy integer relative to the minimap's position and scale.
 */
void CLuaHandle::DrawInMiniMapBackground()
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 4, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, minimap->GetSizeX());
	lua_pushnumber(L, minimap->GetSizeY());

	const bool origDrawingState = LuaOpenGL::IsDrawingEnabled(L);
	LuaOpenGL::SetDrawingEnabled(L, true);

	// call the routine
	RunCallIn(L, cmdStr, 2, 0);

	LuaOpenGL::SetDrawingEnabled(L, origDrawingState);
}

void CLuaHandle::DrawObjectsLua(std::initializer_list<bool> bools, const char* func) {
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	const int extraArgs = bools.size();
	luaL_checkstack(L, 2 + extraArgs, func);
	const LuaHashString cmdStr(func);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	for (auto b : bools) {
		lua_pushboolean(L, b);
	}

	const bool origDrawingState = LuaOpenGL::IsDrawingEnabled(L);
	LuaOpenGL::SetDrawingEnabled(L, true);

	// call the routine
	RunCallIn(L, cmdStr, extraArgs, 0);

	LuaOpenGL::SetDrawingEnabled(L, origDrawingState);
}


void CLuaHandle::DrawOpaqueUnitsLua(bool deferredPass, bool drawReflection, bool drawRefraction)
{
	RECOIL_DETAILED_TRACY_ZONE;
	DrawObjectsLua({ deferredPass, drawReflection, drawRefraction }, __func__);
}

void CLuaHandle::DrawOpaqueFeaturesLua(bool deferredPass, bool drawReflection, bool drawRefraction)
{
	RECOIL_DETAILED_TRACY_ZONE;
	DrawObjectsLua({ deferredPass, drawReflection, drawRefraction }, __func__);
}

void CLuaHandle::DrawAlphaUnitsLua(bool drawReflection, bool drawRefraction)
{
	RECOIL_DETAILED_TRACY_ZONE;
	DrawObjectsLua({ drawReflection, drawRefraction }, __func__);
}

void CLuaHandle::DrawAlphaFeaturesLua(bool drawReflection, bool drawRefraction)
{
	RECOIL_DETAILED_TRACY_ZONE;
	DrawObjectsLua({ drawReflection, drawRefraction }, __func__);
}


/*** Called every 60 frames, calculating delta between `GameFrame` and `GameProgress`.
 *
 * Can give an ETA about catching up with simulation for mid-game join players.
 *
 * @function Callins:GameProgress
 * @param serverFrameNum integer
 */
void CLuaHandle::GameProgress(int frameNum)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 3, __func__);

	static const LuaHashString cmdStr(__func__);

	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, frameNum);

	// call the routine
	RunCallIn(L, cmdStr, 1, 0);
}

void CLuaHandle::Pong(uint8_t pingTag, const spring_time pktSendTime, const spring_time pktRecvTime)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 1 + 1 + 3, __func__);

	static const LuaHashString cmdStr(__func__);

	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, pingTag);
	lua_pushnumber(L, pktSendTime.toMilliSecsf());
	lua_pushnumber(L, pktRecvTime.toMilliSecsf());

	// call the routine
	RunCallIn(L, cmdStr, 3, 0);
}


/*** Called when the keymap changes
 *
 * @function Callins:KeyMapChanged
 *
 * Can be caused due to a change in language or keyboard
 */
bool CLuaHandle::KeyMapChanged()
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 3, __func__);
	static const LuaHashString cmdStr(__func__);

	// if the call is not defined, do not take the event
	if (!cmdStr.GetGlobalFunc(L))
		return false;

	if (!RunCallIn(L, cmdStr, 0, 0))
		return false;

	return true;
}


/*** Input
 *
 * @section input
 */


/*** Key Modifier Params
 *
 * @class KeyModifiers
 * @x_helper
 *
 * @field alt boolean Alt key pressed
 * @field ctrl boolean Ctrl key pressed
 * @field meta boolean Meta/GUI key pressed
 * @field shift boolean Shift key pressed
 */


/*** Called repeatedly when a key is pressed down.
 *
 * @function Callins:KeyPress
 *
 * Return true if you don't want other callins or the engine to also receive this keypress. A list of key codes can be seen at the SDL wiki.
 *
 * @param keyCode integer
 * @param mods KeyModifiers
 * @param isRepeat boolean If you want an action to occur only once check for isRepeat == false.
 * @param label string the name of the key
 * @param utf32char integer (deprecated) always 0
 * @param scanCode integer
 * @param actionList table? the list of actions for this keypress, when available
 * @return boolean halt whether to halt the chain for consumers of the keypress
 */
bool CLuaHandle::KeyPress(int keyCode, int scanCode, bool isRepeat)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L, false);

	const bool isGame = game != nullptr;

	luaL_checkstack(L, 7 + isGame, __func__);
	static const LuaHashString cmdStr(__func__);

	// if the call is not defined, do not take the event
	if (!cmdStr.GetGlobalFunc(L))
		return false;

	//FIXME we should never had started using directly SDL consts, somaeday we should weakly force lua-devs to fix their code
	lua_pushinteger(L, SDL21_keysyms(keyCode));

	lua_createtable(L, 0, 4);
	LuaPushNamedBool(L, "alt",   !!KeyInput::GetKeyModState(KMOD_ALT));
	LuaPushNamedBool(L, "ctrl",  !!KeyInput::GetKeyModState(KMOD_CTRL));
	LuaPushNamedBool(L, "meta",  !!KeyInput::GetKeyModState(KMOD_GUI));
	LuaPushNamedBool(L, "shift", !!KeyInput::GetKeyModState(KMOD_SHIFT));

	lua_pushboolean(L, isRepeat);

	CKeySet ks(keyCode);
	lua_pushsstring(L, ks.GetString(true));
	lua_pushinteger(L, 0); //FIXME remove, was deprecated utf32 char (now uses TextInput for that)
	lua_pushinteger(L, scanCode);

	if (isGame) {
		int i = 1;
		lua_createtable(L, 0, game->GetLastActionList().size());
		for (const Action& action: game->GetLastActionList()) {
			lua_createtable(L, 0, 3); {
				LuaPushNamedString(L, "command",   action.command);
				LuaPushNamedString(L, "extra",     action.extra);
				LuaPushNamedString(L, "boundWith", action.boundWith);
			}
			lua_rawseti(L, -2, i++);
		}
	}

	// call the function
	if (!RunCallIn(L, cmdStr, 6 + isGame, 1))
		return false;

	const bool retval = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return retval;
}


/*** Called when the key is released.
 *
 * @function Callins:KeyRelease
 *
 * @param keyCode integer
 * @param mods KeyModifiers
 * @param label string the name of the key
 * @param utf32char integer (deprecated) always 0
 * @param scanCode integer
 * @param actionList table? the list of actions for this keyrelease, when available
 *
 * @return boolean
 */
bool CLuaHandle::KeyRelease(int keyCode, int scanCode)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L, false);

	const bool isGame = game != nullptr;

	luaL_checkstack(L, 6 + isGame, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return false;

	lua_pushinteger(L, SDL21_keysyms(keyCode));

	lua_createtable(L, 0, 4);
	LuaPushNamedBool(L, "alt",   !!KeyInput::GetKeyModState(KMOD_ALT));
	LuaPushNamedBool(L, "ctrl",  !!KeyInput::GetKeyModState(KMOD_CTRL));
	LuaPushNamedBool(L, "meta",  !!KeyInput::GetKeyModState(KMOD_GUI));
	LuaPushNamedBool(L, "shift", !!KeyInput::GetKeyModState(KMOD_SHIFT));

	CKeySet ks(keyCode);
	lua_pushsstring(L, ks.GetString(true));
	lua_pushinteger(L, 0); //FIXME remove, was deprecated utf32 char (now uses TextInput for that)
	lua_pushinteger(L, scanCode);

	if (isGame) {
		int i = 1;
		lua_createtable(L, 0, game->GetLastActionList().size());
		for (const Action& action: game->GetLastActionList()) {
			lua_createtable(L, 0, 3); {
				LuaPushNamedString(L, "command",   action.command);
				LuaPushNamedString(L, "extra",     action.extra);
				LuaPushNamedString(L, "boundWith", action.boundWith);
			}
			lua_rawseti(L, -2, i++);
		}
	}

	// call the function
	if (!RunCallIn(L, cmdStr, 5 + isGame, 1))
		return false;

	const bool retval = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return retval;
}


/*** Called whenever a key press results in text input.
 *
 * @function Callins:TextInput
 *
 * @param utf8char string
 */
bool CLuaHandle::TextInput(const std::string& utf8)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 3, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return false;

	lua_pushsstring(L, utf8);
	//lua_pushnumber(L, UTF8toUTF32(utf8));

	// call the function
	if (!RunCallIn(L, cmdStr, 1, 1))
		return false;

	const bool retval = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return retval;
}


/***
 *
 * @function Callins:TextEditing
 *
 * @param utf8 string
 * @param start integer
 * @param length integer
 */
bool CLuaHandle::TextEditing(const std::string& utf8, unsigned int start, unsigned int length)
{
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 5, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return false;

	lua_pushsstring(L, utf8);
	lua_pushinteger(L, start);
	lua_pushinteger(L, length);

	// call the function
	if (!RunCallIn(L, cmdStr, 3, 1))
		return false;

	const bool retval = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return retval;
}


/*** Called when a mouse button is pressed.
 *
 * The button parameter supports up to 7 buttons. Must return true for `MouseRelease` and other functions to be called.
 *
 * @function Callins:MousePress
 * @param x integer
 * @param y integer
 * @param button integer
 * @return boolean becomeMouseOwner
 */
bool CLuaHandle::MousePress(int x, int y, int button)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 5, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return false;

	lua_pushnumber(L, x - globalRendering->viewPosX);
	lua_pushnumber(L, globalRendering->viewSizeY - y - 1);
	lua_pushnumber(L, button);

	// call the function
	if (!RunCallIn(L, cmdStr, 3, 1))
		return false;

	const bool retval = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return retval;
}


/*** Called when a mouse button is released.
 *
 * @function Callins:MouseRelease
 *
 * Please note that in order to have Spring call `Spring.MouseRelease`, you need to have a `Spring.MousePress` call-in in the same addon that returns true.
 *
 * @param x integer
 * @param y integer
 * @param button integer
 * @return boolean becomeMouseOwner
 */
void CLuaHandle::MouseRelease(int x, int y, int button)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 5, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, x - globalRendering->viewPosX);
	lua_pushnumber(L, globalRendering->viewSizeY - y - 1);
	lua_pushnumber(L, button);

	// call the function
	RunCallIn(L, cmdStr, 3, 0);
}


/*** Called when the mouse is moved.
 *
 * @function Callins:MouseMove
 *
 * @param x integer final x position
 * @param y integer final y position
 * @param dx integer distance travelled in x
 * @param dy integer distance travelled in y
 * @param button integer
 */
bool CLuaHandle::MouseMove(int x, int y, int dx, int dy, int button)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 7, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return false;

	lua_pushnumber(L, x - globalRendering->viewPosX);
	lua_pushnumber(L, globalRendering->viewSizeY - y - 1);
	lua_pushnumber(L, dx);
	lua_pushnumber(L, -dy);
	lua_pushnumber(L, button);

	// call the function
	if (!RunCallIn(L, cmdStr, 5, 1))
		return false;

	const bool retval = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return retval;
}


/*** Called when the mouse wheel is moved.
 *
 * @function Callins:MouseWheel
 *
 * @param up boolean the direction
 * @param value number the amount travelled
 */
bool CLuaHandle::MouseWheel(bool up, float value)
{
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 4, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return false;

	lua_pushboolean(L, up);
	lua_pushnumber(L, value);

	// call the function
	if (!RunCallIn(L, cmdStr, 2, 1))
		return false;

	const bool retval = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return retval;
}

/*** Called every `Update`.
 *
 * @function Callins:IsAbove
 *
 * Must return true for `Mouse*` events and `Spring.GetToolTip` to be called.
 *
 * @param x integer
 * @param y integer
 * @return boolean isAbove
 */
bool CLuaHandle::IsAbove(int x, int y)
{
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 4, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return false;

	lua_pushnumber(L, x - globalRendering->viewPosX);
	lua_pushnumber(L, globalRendering->viewSizeY - y - 1);

	// call the function
	if (!RunCallIn(L, cmdStr, 2, 1))
		return false;

	const bool retval = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return retval;
}


/*** Called when `Spring.IsAbove` returns true.
 *
 * @function Callins:GetTooltip
 * @param x integer
 * @param y integer
 * @return string tooltip
 */
string CLuaHandle::GetTooltip(int x, int y)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L, "");
	luaL_checkstack(L, 4, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return "";

	lua_pushnumber(L, x - globalRendering->viewPosX);
	lua_pushnumber(L, globalRendering->viewSizeY - y - 1);

	// call the function
	if (!RunCallIn(L, cmdStr, 2, 1))
		return "";

	const string retval = luaL_optsstring(L, -1, "");
	lua_pop(L, 1);
	return retval;
}

/*** Called when a command is issued.
 *
 * @function Callins:ActiveCommandChanged
 * @param cmdId integer?
 * @param cmdType integer?
 */
void CLuaHandle::ActiveCommandChanged(const SCommandDescription* cmdDesc)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 5, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	if (cmdDesc) {
		lua_pushnumber(L, cmdDesc->id);
		lua_pushnumber(L, cmdDesc->type);

		// call the function
		RunCallIn(L, cmdStr, 2, 0);
	} else {
		RunCallIn(L, cmdStr, 0, 0);
	}
}

/*** Called whenever the camera rotation changes
 *
 * @function Callins:CameraRotationChanged
 * @param rotX number Camera rotation around the x axis in radians.
 * @param rotY number Camera rotation around the y axis in radians.
 * @param rotZ number Camera rotation around the z axis in radians.
 */
void CLuaHandle::CameraRotationChanged(const float3& rot)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 6, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, rot.x);
	lua_pushnumber(L, rot.y);
	lua_pushnumber(L, rot.z);

	RunCallIn(L, cmdStr, 3, 0);
}

/*** Called whenever the camera position changes
 *
 * @function Callins:CameraPositionChanged
 * @param posX number Camera position x in world coordinates
 * @param posY number Camera position y in world coordinates
 * @param posZ number Camera position z in world coordinates
 */
void CLuaHandle::CameraPositionChanged(const float3& pos)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 6, __func__);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, pos.x);
	lua_pushnumber(L, pos.y);
	lua_pushnumber(L, pos.z);

	RunCallIn(L, cmdStr, 3, 0);
}

/*** Called when the MiniMap rotation changes
 * 
 * @function Callins:MiniMapRotationChanged
 * @param newRot number MiniMap rotation in radians
 * @param oldRot number MiniMap old rotation in radians
 */
void CLuaHandle::MiniMapRotationChanged(const float newRot, const float oldRot)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 5, __func__);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, newRot);
	lua_pushnumber(L, oldRot);

	RunCallIn(L, cmdStr, 2, 0);
}

/*** Called when the MiniMap minimizes or maximizes changes
 * 
 * @function Callins:MiniMapStateChanged
 * @param isMinimized boolean
 * @param isMaximized boolean
 * @param isSlaved boolean
 */
void CLuaHandle::MiniMapStateChanged(const bool isMinimized,
									const bool isMaximized,
									const bool isSlaved)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 5, __func__);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushboolean(L, isMinimized);
	lua_pushboolean(L, isMaximized);
	lua_pushboolean(L, isSlaved);


	RunCallIn(L, cmdStr, 3, 0);
}

/*** Called when the MiniMap Geometry changes
 * 
 * @function Callins:MiniMapGeometryChanged
 * @param newPosX integer in pixels
 * @param newPosY integer in pixels
 * @param newDimX integer in pixels
 * @param newDimY integer in pixels
 * @param oldPosX integer in pixels
 * @param oldPosY integer in pixels
 * @param oldDimX integer in pixels
 * @param oldDimY integer in pixels
 */
void CLuaHandle::MiniMapGeometryChanged(const int2 newPos, const int2 newDim, const int2 oldPos, const int2 oldDim)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 11, __func__); // 3 + 8 args

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, newPos.x);
	lua_pushnumber(L, newPos.y);
	lua_pushnumber(L, newDim.x);
	lua_pushnumber(L, newDim.y);
	
	lua_pushnumber(L, oldPos.x);
	lua_pushnumber(L, oldPos.y);
	lua_pushnumber(L, oldDim.x);
	lua_pushnumber(L, oldDim.y);

	RunCallIn(L, cmdStr, 8, 0);
}

/*** Called when a command is issued.
 *
 * @function Callins:CommandNotify
 * @param cmdID integer
 * @param cmdParams table
 * @param options CommandOptions
 * @return boolean delete Returning true deletes the command and does not send it through the network.
 */
bool CLuaHandle::CommandNotify(const Command& cmd)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 5, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return false;

	// push the command id
	lua_pushnumber(L, cmd.GetID());

	// push the params list
	LuaUtils::PushCommandParamsTable(L, cmd, false);
	// push the options table
	LuaUtils::PushCommandOptionsTable(L, cmd, false);

	// call the function
	if (!RunCallIn(L, cmdStr, 3, 1))
		return false;

	// get the results
	const bool retval = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return retval;
}


/*** Called when text is entered into the console (e.g. `Spring.Echo`).
 *
 * @function Callins:AddConsoleLine
 * @param msg string
 * @param priority integer
 */
bool CLuaHandle::AddConsoleLine(const string& msg, const string& section, int level)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L, true);
	luaL_checkstack(L, 4, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return true;

	lua_pushsstring(L, msg);
	lua_pushnumber(L, level);

	// call the function
	return RunCallIn(L, cmdStr, 2, 0);
}


/*** Called when a unit is added to or removed from a control group.
 *
 * @function Callins:GroupChanged
 * @param groupID GroupID
 */
bool CLuaHandle::GroupChanged(int groupID)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 3, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return false;

	lua_pushnumber(L, groupID);

	// call the routine
	return RunCallIn(L, cmdStr, 1, 0);
}

/***
 * @function Callins:WorldTooltip
 * @param type "unit"
 * @param unitId UnitID
 * @return string tooltip
 */
/***
 * @function Callins:WorldTooltip
 * @param type "feature"
 * @param featureId FeatureID
 * @return string tooltip
 */
/***
 * @function Callins:WorldTooltip
 * @param type "ground"
 * @param posX number
 * @param posY number
 * @param posZ number
 * @return string tooltip
 */
/***
 * @function Callins:WorldTooltip
 * @param type "selection"
 * @return string tooltip
 */
string CLuaHandle::WorldTooltip(const CUnit* unit,
                                const CFeature* feature,
                                const float3* groundPos)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L, "");
	luaL_checkstack(L, 6, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return "";

	int args;
	if (unit) {
		LuaPushString(L, "unit");
		lua_pushnumber(L, unit->id);
		args = 2;
	}
	else if (feature) {
		LuaPushString(L, "feature");
		lua_pushnumber(L, feature->id);
		args = 2;
	}
	else if (groundPos) {
		LuaPushString(L, "ground");
		lua_pushnumber(L, groundPos->x);
		lua_pushnumber(L, groundPos->y);
		lua_pushnumber(L, groundPos->z);
		args = 4;
	}
	else {
		LuaPushString(L, "selection");
		args = 1;
	}

	// call the routine
	if (!RunCallIn(L, cmdStr, args, 1))
		return "";

	const string retval = luaL_optstring(L, -1, "");
	lua_pop(L, 1);
	return retval;
}

/***
 * @function Callins:MapDrawCmd
 * @param playerID PlayerID
 * @param type "point"
 * @param posX number
 * @param posY number
 * @param posZ number
 * @param label string
 */
/***
 * @function Callins:MapDrawCmd
 * @param playerID PlayerID
 * @param type "line"
 * @param pos1X number
 * @param pos1Y number
 * @param pos1Z number
 * @param pos2X number
 * @param pos2Y number
 * @param pos2Z number
 */
/***
 * @function Callins:MapDrawCmd
 * @param playerID PlayerID
 * @param type "erase"
 * @param posX number
 * @param posY number
 * @param posZ number
 * @param radius number
 */
bool CLuaHandle::MapDrawCmd(int playerID, int type,
                            const float3* pos0,
                            const float3* pos1,
                            const string* label)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 9, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return false;

	int args;

	lua_pushnumber(L, playerID);

	if (type == MAPDRAW_POINT) {
		LuaPushString (L, "point");
		lua_pushnumber(L, pos0->x);
		lua_pushnumber(L, pos0->y);
		lua_pushnumber(L, pos0->z);
		lua_pushsstring(L, *label);
		args = 6;
	}
	else if (type == MAPDRAW_LINE) {
		LuaPushString (L, "line");
		lua_pushnumber(L, pos0->x);
		lua_pushnumber(L, pos0->y);
		lua_pushnumber(L, pos0->z);
		lua_pushnumber(L, pos1->x);
		lua_pushnumber(L, pos1->y);
		lua_pushnumber(L, pos1->z);
		args = 8;
	}
	else if (type == MAPDRAW_ERASE) {
		LuaPushString (L, "erase");
		lua_pushnumber(L, pos0->x);
		lua_pushnumber(L, pos0->y);
		lua_pushnumber(L, pos0->z);
		lua_pushnumber(L, 100.0f);  // radius
		args = 6;
	}
	else {
		LOG_L(L_WARNING, "Unknown MapDrawCmd() type: %i", type);
		lua_pop(L, 2); // pop the function and playerID
		return false;
	}

	// call the routine
	if (!RunCallIn(L, cmdStr, args, 1))
		return false;

	// take the event?
	const bool retval = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return retval;
}


/***
 * @enum READY_MESSAGE
 * @field WAITING "Waiting for players"
 * @field HOST_WAITING "Waiting for players, press key to force start" when waiting and currently the host, key is the first key bound to the action forcestart
 * @field CHOOSE_POS "Choose start pos" when current player is not ready and is not a spectator
 * @field STARTING "Starting in n" where n is the number of seconds
 */

/***
 * @enum READY_STATE
 * @field MISSING "missing" when the player hasn't joined yet (NETMSG_PLAYERNAME wasn't received)
 * @field READY "ready"
 * @field NOT_READY "notready"
 */

/*** Fired when the pregame stage is reached
 *
 * Pregame is the stage where player readiness is managed before a game starts.
 * Game only starts once all players are ready.
 *
 * @function Callins:GameSetup
 * @param state READY_MESSAGE the current message the engine would display to the player
 * @param ready boolean whether the player is currently ready or not
 * @param playerStates table<PlayerID,READY_STATE> indexed by playerID
 * @return boolean? gameHandled disables the engine ui when true
 * @return boolean? newReady whether the player is ready (ignored unless `gameHandled = true`)
 */
bool CLuaHandle::GameSetup(const string& state, bool& ready,
                           const std::vector< std::pair<int, std::string> >& playerStates)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 5, __func__);

	static const LuaHashString cmdStr(__func__);

	if (!cmdStr.GetGlobalFunc(L))
		return false;

	lua_pushsstring(L, state);
	lua_pushboolean(L, ready);

	lua_createtable(L, playerStates.size(), 0);

	for (const auto& playerState: playerStates) {
		lua_pushsstring(L, playerState.second);
		lua_rawseti(L, -2, playerState.first);
	}

	// call the routine
	if (!RunCallIn(L, cmdStr, 3, 2))
		return false;

	if (lua_isboolean(L, -2)) {
		if (lua_toboolean(L, -2)) {
			// only allow ready-state change if Lua takes the event
			if (lua_isboolean(L, -1))
				ready = lua_toboolean(L, -1);

			lua_pop(L, 2);
			return true;
		}
	}
	lua_pop(L, 2);
	return false;
}



/*** @function Callins:RecvSkirmishAIMessage
 *
 * @param aiTeam TeamID
 * @param dataStr string
 */
const char* CLuaHandle::RecvSkirmishAIMessage(int aiTeam, const char* inData, int inSize, size_t* outSize)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L, nullptr);
	luaL_checkstack(L, 4, __func__);

	static const LuaHashString cmdStr(__func__);

	// <this> is either CLuaRules* or CLuaUI*,
	// but the AI call-in is always unsynced!
	if (!cmdStr.GetGlobalFunc(L))
		return nullptr;

	lua_pushnumber(L, aiTeam);

	int argCount = 1;
	const char* outData = nullptr;

	if (inData != nullptr) {
		if (inSize < 0)
			inSize = strlen(inData);

		lua_pushlstring(L, inData, inSize);
		argCount = 2;
	}

	if (!RunCallIn(L, cmdStr, argCount, 1))
		return nullptr;

	if (lua_isstring(L, -1))
		outData = lua_tolstring(L, -1, outSize);

	lua_pop(L, 1);
	return outData;
}

/*** Downloads
 * @section downloads
 */

/*** Called when a Pr-downloader download is queued
 *
 * @function Callins:DownloadQueued
 * @param id integer
 * @param name string
 * @param type string
 */
void CLuaHandle::DownloadQueued(int ID, const string& archiveName, const string& archiveType)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 3, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushinteger(L, ID);
	lua_pushsstring(L, archiveName);
	lua_pushsstring(L, archiveType);

	// call the routine
	RunCallInTraceback(L, cmdStr, 3, 0, traceBack.GetErrFuncIdx(), false);
}


/*** Called when a Pr-downloader download is started via VFS.DownloadArchive.
 *
 * @function Callins:DownloadStarted
 * @param id integer
 */
void CLuaHandle::DownloadStarted(int ID)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 1, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushinteger(L, ID);

	// call the routine
	RunCallInTraceback(L, cmdStr, 1, 0, traceBack.GetErrFuncIdx(), false);
}

/*** Called when a Pr-downloader download finishes successfully.
 *
 * @function Callins:DownloadFinished
 * @param id integer
 */
void CLuaHandle::DownloadFinished(int ID)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 1, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushinteger(L, ID);

	// call the routine
	RunCallInTraceback(L, cmdStr, 1, 0, traceBack.GetErrFuncIdx(), false);
}

/*** Called when a Pr-downloader download fails to complete.
 *
 * @function Callins:DownloadFailed
 * @param id integer
 * @param errorID integer
 */
void CLuaHandle::DownloadFailed(int ID, int errorID)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 2, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushinteger(L, ID);
	lua_pushinteger(L, errorID);

	// call the routine
	RunCallInTraceback(L, cmdStr, 2, 0, traceBack.GetErrFuncIdx(), false);
}

/*** Called incrementally during a Pr-downloader download.
 *
 * @function Callins:DownloadProgress
 * @param id integer
 * @param downloaded integer
 * @param total integer
 */
void CLuaHandle::DownloadProgress(int ID, long downloaded, long total)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 3, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushinteger(L, ID);
	lua_pushnumber(L, downloaded);
	lua_pushnumber(L, total);

	// call the routine
	RunCallInTraceback(L, cmdStr, 3, 0, traceBack.GetErrFuncIdx(), false);
}

/******************************************************************************/
/******************************************************************************/

void CLuaHandle::CollectGarbage(bool forced)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const float gcMemLoadMult = D.gcCtrl.baseMemLoadMult;
	const float gcRunTimeMult = D.gcCtrl.baseRunTimeMult;

	if (!forced && spring_lua_alloc_skip_gc(gcMemLoadMult))
		return;

	LUA_CALL_IN_CHECK_NAMED(L, (GetLuaContextData(L)->synced)? "Lua::CollectGarbage::Synced": "Lua::CollectGarbage::Unsynced");

	lua_lock(L_GC);
	SetHandleRunning(L_GC, true);

	// note: total footprint INCLUDING garbage, in KB
	int  gcMemFootPrint = lua_gc(L_GC, LUA_GCCOUNT, 0);
	int  gcItersInBatch = 0;
	int& gcStepsPerIter = D.gcCtrl.numStepsPerIter;

	// if gc runs at a fixed rate, the upper limit to base runtime will
	// quickly be reached since Lua's footprint can easily exceed 100MB
	// and OOM exceptions become a concern when catching up
	// OTOH if gc is tied to sim-speed the increased number of calls can
	// mean too much time is spent on it, must weigh the per-call period
	const float gcSpeedFactor = std::clamp(gs->speedFactor * (1 - gs->PreSimFrame()) * (1 - gs->paused), 1.0f, 50.0f);
	const float gcBaseRunTime = smoothstep(10.0f, 100.0f, gcMemFootPrint / 1024);
	const float gcLoopRunTime = std::clamp((gcBaseRunTime * gcRunTimeMult) / gcSpeedFactor, D.gcCtrl.minLoopRunTime, D.gcCtrl.maxLoopRunTime);

	const spring_time startTime = spring_gettime();
	const spring_time   endTime = startTime + spring_msecs(gcLoopRunTime);

	// perform GC cycles until time runs out or iteration-limit is reached
	while (forced || (gcItersInBatch < D.gcCtrl.itersPerBatch && spring_gettime() < endTime)) {
		gcItersInBatch++;

		if (!lua_gc(L_GC, LUA_GCSTEP, gcStepsPerIter))
			continue;

		// garbage-collection cycle finished
		const int gcMemFootPrintNow = lua_gc(L_GC, LUA_GCCOUNT, 0);
		const int gcMemFootPrintDif = gcMemFootPrintNow - gcMemFootPrint;

		gcMemFootPrint = gcMemFootPrintNow;

		// early-exit if cycle didn't free any memory
		if (gcMemFootPrintDif == 0)
			break;
	}

	// don't collect garbage outside of CollectGarbage
	lua_gc(L_GC, LUA_GCSTOP, 0);
	SetHandleRunning(L_GC, false);
	lua_unlock(L_GC);


	const spring_time finishTime = spring_gettime();

	if (gcStepsPerIter > 1 && gcItersInBatch > 0) {
		// runtime optimize number of steps to process in a batch
		const float avgLoopIterTime = (finishTime - startTime).toMilliSecsf() / gcItersInBatch;

		gcStepsPerIter -= (avgLoopIterTime > (gcRunTimeMult * 0.150f));
		gcStepsPerIter += (avgLoopIterTime < (gcRunTimeMult * 0.075f));
		gcStepsPerIter  = std::clamp(gcStepsPerIter, D.gcCtrl.minStepsPerIter, D.gcCtrl.maxStepsPerIter);
	}

	eventHandler.DbgTimingInfo(TIMING_GC, startTime, finishTime);
}

/******************************************************************************/
/******************************************************************************/

bool CLuaHandle::AddBasicCalls(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LuaPushString(L, "Script");
	lua_createtable(L, 0, 17); {
		LuaPushNamedCFunc(L, "Kill",               KillActiveHandle);
		LuaPushNamedCFunc(L, "UpdateCallIn",       CallOutUpdateCallIn);
		LuaPushNamedCFunc(L, "GetName",            CallOutGetName);
		LuaPushNamedCFunc(L, "GetSynced",          CallOutGetSynced);
		LuaPushNamedCFunc(L, "GetFullCtrl",        CallOutGetFullCtrl);
		LuaPushNamedCFunc(L, "GetFullRead",        CallOutGetFullRead);
		LuaPushNamedCFunc(L, "GetCtrlTeam",        CallOutGetCtrlTeam);
		LuaPushNamedCFunc(L, "GetReadTeam",        CallOutGetReadTeam);
		LuaPushNamedCFunc(L, "GetReadAllyTeam",    CallOutGetReadAllyTeam);
		LuaPushNamedCFunc(L, "GetSelectTeam",      CallOutGetSelectTeam);
		LuaPushNamedCFunc(L, "GetGlobal",          CallOutGetGlobal);
		LuaPushNamedCFunc(L, "GetRegistry",        CallOutGetRegistry);
		LuaPushNamedCFunc(L, "GetCallInList",      CallOutGetCallInList);
		LuaPushNamedCFunc(L, "DelayByFrames",      CallOutDelayByFrames);
		LuaPushNamedCFunc(L, "IsEngineMinVersion", CallOutIsEngineMinVersion);
		// special team constants

		/*** @field Script.NO_ACCESS_TEAM -1 */
		LuaPushNamedNumber(L, "NO_ACCESS_TEAM",  CEventClient::NoAccessTeam);
		/*** @field Script.ALL_ACCESS_TEAM -2 */
		LuaPushNamedNumber(L, "ALL_ACCESS_TEAM", CEventClient::AllAccessTeam);
	}
	lua_rawset(L, -3);

	lua_getglobal(L, "table");
	LuaTableExtra::PushEntries(L);
	lua_pop(L, 1);

	return true;
}

bool CLuaHandle::AddCommonModules(lua_State* L)
{
	if (!AddEntriesToTable(L, "Encoding",           LuaEncoding::PushEntries        ))
		return false;
	if (!AddEntriesToTable(L, "math",               LuaMathExtra::PushEntries       ))
		return false;
	return true;
}

/***
 * @function Script.GetName
 * @return string name
 */
int CLuaHandle::CallOutGetName(lua_State* L)
{
	lua_pushsstring(L, GetHandle(L)->GetName());
	return 1;
}


/***
 * @function Script.GetSynced
 * @return boolean synced
 */
int CLuaHandle::CallOutGetSynced(lua_State* L)
{
	lua_pushboolean(L, GetHandleSynced(L));
	return 1;
}


/***
 * @function Script.GetFullCtrl
 * @return boolean fullCtrl
 */
int CLuaHandle::CallOutGetFullCtrl(lua_State* L)
{
	lua_pushboolean(L, GetHandleFullCtrl(L));
	return 1;
}


/***
 * @function Script.GetFullRead
 * @return boolean fullRead
 */
int CLuaHandle::CallOutGetFullRead(lua_State* L)
{
	lua_pushboolean(L, GetHandleFullRead(L));
	return 1;
}


/***
 * @function Script.GetCtrlTeam
 * @return TeamID teamID
 */
int CLuaHandle::CallOutGetCtrlTeam(lua_State* L)
{
	lua_pushnumber(L, GetHandleCtrlTeam(L));
	return 1;
}


/***
 * @function Script.GetReadTeam
 * @return TeamID teamID
 */
int CLuaHandle::CallOutGetReadTeam(lua_State* L)
{
	lua_pushnumber(L, GetHandleReadTeam(L));
	return 1;
}


/***
 * @function Script.GetReadAllyTeam
 * @return AllyTeamID allyTeamID
 */
int CLuaHandle::CallOutGetReadAllyTeam(lua_State* L)
{
	lua_pushnumber(L, GetHandleReadAllyTeam(L));
	return 1;
}


/***
 * @function Script.GetSelectTeam
 * @return TeamID teamID
 */
int CLuaHandle::CallOutGetSelectTeam(lua_State* L)
{
	lua_pushnumber(L, GetHandleSelectTeam(L));
	return 1;
}


/***
 * @function Script.GetGlobal
 * @return integer? global
 */
int CLuaHandle::CallOutGetGlobal(lua_State* L)
{
	if (devMode) {
		lua_pushvalue(L, LUA_GLOBALSINDEX);
		return 1;
	}
	return 0;
}


/***
 * @function Script.GetRegistry
 * @return integer? registry
 */
int CLuaHandle::CallOutGetRegistry(lua_State* L)
{
	if (devMode) {
		lua_pushvalue(L, LUA_REGISTRYINDEX);
		return 1;
	}
	return 0;
}


/** Documented at LuaUtils::IsEngineMinVersion */
int CLuaHandle::CallOutIsEngineMinVersion(lua_State* L)
{
	return (LuaUtils::IsEngineMinVersion(L));
}

/***
 * @function Script.DelayByFrames
 * @param frameDelay integer
 * @param fun(...) func
 */
int CLuaHandle::CallOutDelayByFrames(lua_State* L)
{
	int argCount = lua_gettop(L);
	if (argCount < 2 || !lua_isnumber(L, 1) || !lua_isfunction(L, 2))
		luaL_error(L, "Incorrect arguments to DelayByFrames(positive number frameDelay, func[, args])");

	const auto frameDelay = lua_tointeger(L, 1);
	if (frameDelay <= 0)
		luaL_error(L, "Incorrect arguments to DelayByFrames(positive number frameDelay, func[, args])");

	argCount -= 2;

	std::vector <int> args;
	args.reserve(argCount);
	while (argCount--)
		args.push_back(luaL_ref(L, LUA_REGISTRYINDEX));
	std::reverse(args.begin(), args.end()); // ref has stack semantics, but pcall expects the last arg at the top

	GetHandle(L)->delayedCallsByFrame[gs->GetLuaSimFrame() + frameDelay]
		.emplace_back(luaL_ref(L, LUA_REGISTRYINDEX), std::move(args));

	return 0;
}

int CLuaHandle::CallOutGetCallInList(lua_State* L)
{
	std::vector<std::string> eventList;
	eventHandler.GetEventList(eventList);
	lua_createtable(L, 0, eventList.size());
	for (const auto& event : eventList) {
		lua_pushsstring(L, event);
		lua_createtable(L, 0, 2); {
			lua_pushliteral(L, "unsynced");
			lua_pushboolean(L, eventHandler.IsUnsynced(event));
			lua_rawset(L, -3);
			lua_pushliteral(L, "controller");
			lua_pushboolean(L, eventHandler.IsController(event));
			lua_rawset(L, -3);
		}
		lua_rawset(L, -3);
	}
	return 1;
}


/***
 * @function Script.UpdateCallIn
 * @param name string
 * @return nil
 */
int CLuaHandle::CallOutUpdateCallIn(lua_State* L)
{

	const string name = luaL_checkstring(L, 1);
	CLuaHandle* lh = GetHandle(L);
	lh->UpdateCallIn(L, name);
	return 0;
}

void CLuaHandle::InitializeRmlUi()
{
	rmlui = RmlGui::InitializeLua(L);
}


/******************************************************************************/
/******************************************************************************/

int CLuaHandle::UnpackCobArg(lua_State* L)
{
	if (currentCobArgs == nullptr) {
		luaL_error(L, "Error in UnpackCobArg(), no current args");
	}
	const int arg = luaL_checkint(L, 1) - 1;
	if ((arg < 0) || (arg >= MAX_LUA_COB_ARGS)) {
		luaL_error(L, "Error in UnpackCobArg(), bad index");
	}
	const int value = currentCobArgs[arg];
	lua_pushnumber(L, UNPACKX(value));
	lua_pushnumber(L, UNPACKZ(value));
	return 2;
}


void CLuaHandle::Cob2Lua(const LuaHashString& name, const CUnit* unit,
                        int& argsCount, int args[MAX_LUA_COB_ARGS])
{
	const bool synced = GetHandleSynced(L);
	static int callDepth = 0;
	if (callDepth >= 16) {
		LOG_L(L_WARNING, "[%s::%s] call overflow: %s", GetName().c_str(), __func__, name.GetString());
		args[0] = 0; // failure
		return;
	}

	if (!synced && !LuaUtils::IsUnitInLos(L, unit))
		return;

	LUA_CALL_IN_CHECK(L);

	const int top = lua_gettop(L);

	if (!lua_checkstack(L, 1 + 3 + argsCount)) {
		if (synced)
			LOG_L(L_WARNING, "[%s::%s] lua_checkstack() error: %s", GetName().c_str(),  __func__, name.GetString());
		args[0] = 0; // failure
		lua_settop(L, top);
		return;
	}

	if (!name.GetGlobalFunc(L)) {
		if (synced)
			LOG_L(L_WARNING, "[%s::%s] missing function: %s", GetName().c_str(), __func__, name.GetString());
		args[0] = 0; // failure
		lua_settop(L, top);
		return;
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	for (int a = 0; a < argsCount; a++) {
		lua_pushnumber(L, args[a]);
	}

	// call the routine
	callDepth++;
	const int* oldArgs = currentCobArgs;
	currentCobArgs = args;

	const bool error = !RunCallIn(L, name, 3 + argsCount, LUA_MULTRET);

	currentCobArgs = oldArgs;
	callDepth--;

	// bail on error
	if (error) {
		args[0] = 0; // failure
		lua_settop(L, top);
		return;
	}

	// get the results
	const int retArgs = std::min(lua_gettop(L) - top, (MAX_LUA_COB_ARGS - 1));
	for (int a = 1; a <= retArgs; a++) {
		const int index = (a + top);
		if (lua_isnumber(L, index)) {
			args[a] = lua_toint(L, index);
		}
		else if (lua_isboolean(L, index)) {
			args[a] = lua_toboolean(L, index) ? 1 : 0;
		}
		else if (lua_istable(L, index)) {
			lua_rawgeti(L, index, 1);
			lua_rawgeti(L, index, 2);
			if (lua_isnumber(L, -2) && lua_isnumber(L, -1)) {
				const int x = lua_toint(L, -2);
				const int z = lua_toint(L, -1);
				args[a] = PACKXZ(x, z);
			} else {
				args[a] = 0;
			}
			lua_pop(L, 2);
		}
		else {
			args[a] = 0;
		}
	}

	args[0] = 1; // success
	lua_settop(L, top);
}


void CLuaHandle::Cob2LuaBatch(const LuaHashString& name, std::vector<CCobDeferredCallin>& callins)
{
	const bool synced = GetHandleSynced(L);
	static int callDepth = 0;
	if (callDepth >= 16) {
		LOG_L(L_WARNING, "[%s::%s] call overflow: %s", GetName().c_str(), __func__, name.GetString());
		return;
	}

	LUA_CALL_IN_CHECK(L);

	const int top = lua_gettop(L);
	int argsCount = 0;

	if (!lua_checkstack(L, 1 + 3 + argsCount)) {
		LOG_L(L_WARNING, "[%s::%s] lua_checkstack() error: %s", GetName().c_str(), __func__, name.GetString());
		lua_settop(L, top);
		return;
	}

	if (!name.GetGlobalFunc(L)) {
		LOG_L(L_WARNING, "[%s::%s] missing function: %s", GetName().c_str(), __func__, name.GetString());
		lua_settop(L, top);
		return;
	}

	// count can be smaller because of LOS when unsynced
	lua_createtable(L, callins.size(), 0);

	int i = 0;
	for(auto& callin: callins) {
		// TODO check if unit still alive
		if (!synced && !LuaUtils::IsUnitInLos(L, callin.unit))
			continue;

		lua_createtable(L, callin.argCount+1, 0);

		for (int a = 0; a < callin.argCount; a++) {
			lua_pushnumber(L, callin.luaArgs[a]);
			lua_rawseti(L, -2, a + 1);
		}

		lua_rawseti(L, -2, ++i);
	}

	// call the routine
	callDepth++;

	const bool error = !RunCallIn(L, name, 1 + argsCount, LUA_MULTRET);

	callDepth--;

	lua_settop(L, top);
}

/******************************************************************************/
/******************************************************************************/
