/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include <cmath>
#include <string_view>

#include "LuaVFS.h"
#include "LuaInclude.h"
#include "LuaHandle.h"
#include "LuaHashString.h"
#include "LuaIO.h"
#include "LuaUtils.h"
#include "LuaZip.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Log/ILog.h"
#include "System/StringUtil.h"
#include "System/TimeProfiler.h"
#include "../tools/pr-downloader/src/pr-downloader.h"
#include "fmt/format.h"

#include "System/Misc/TracyDefs.h"
#include <tracy/TracyLua.hpp>

/***
 * The Virtual File System is an unified layer to access (read-only) the
 * different archives used at runtime. So you can access map, game & config
 * files via the same interface.
 * 
 * ## Overview
 * 
 * Although Spring can access the filesystem directly (via os module) it is
 * more common that you would want to access files included with your game or
 * Spring. Trouble is, most of these files are compressed into archives
 * (`.sdz`/`.sd7`) so random access would generally be a difficult procedure.
 * Fortunately, the Spring Lua system automatically provides access to mod and
 * base files via the VFS module.
 *
 * As an additional caveat, synced Lua cannot use the `os` and `io` modules,
 * so using VFS is mandatory there to have any file access at all.
 *
 * The VFS module doesn't simply open archives though. What it does is map
 * your game files, game dependencies and Spring content onto a virtual file
 * tree. All archives start from the 'roots' of the tree and share the same
 * virtual space, meaning that if two or more archives contain the same
 * resource file name the resources overlap and only one of the files will be
 * retrieved. Overlapping directories on the other hand are merged so the
 * resulting virtual directory contains the contents of both. Here is an
 * example of how this works:
 *
 * **Archive 1** (`games/mygame.sd7`)
 *
 * ```
 * textures
 * └── texture1.png
 * models
 * └── model1.mdl
 * ```
 *
 * **Archive 2** (`base/springcontent.sdz`)
 * 
 * ```
 * textures
 * ├── texture1.png
 * ├── texture2.png
 * └── texture3.png
 * ```
 *
 * **VFS**
 * 
 * ```
 * textures
 * ├── texture1.png
 * ├── texture2.png
 * └── texture3.png
 * models
 * └── model1.mdl
 * ```
 *
 * This raises the question: If both archives have a `texture1.png` then which
 * `texture1.png` is retreived via the VFS? The answer depends on the order the
 * archives are loaded and the VFS mode (more on modes below). Generally
 * however, each archive loaded overrides any archives loaded before it. The
 * standard order of loading (from first to last) is:
 * 
 *  1. The automatic dependencies `springcontent.sdz` and `maphelper.sdz`.
 *  2. Dependencies listed in your `modinfo.lua` (or `modinfo.tdf`), in the order listed.
 * Note that they are loaded fully and recursively, i.e. all the deeper dependencies of the 1st base-level dependency are
 * loaded before the 2nd base-level dependency. This breaks the usual "loaded later overrides loaded earlier" priority if
 * a dependency comes from multiple places, since only the first time an archive is loaded counts.
 *  3. Your mod archive.
 *
 * Loose files (not within any archive) in the engine dir are also visible
 * as if under the VFS root if loading under the `VFS.RAW` mode, though you
 * can also use full FS path (i.e. `C:/.../Spring/foo/bar.txt` is visible
 * both as that and as just `foo/bar.txt`). Note that `VFS.RAW` is only
 * accessible to unsynced Lua, all synced states are limited to loaded archives.
 *
 * ## Paths
 * 
 * Spring's VFS is **lowercase only**. Also it is **strongly** recommended to
 * use linux style path separators, e.g. `"foo/bar.txt"` and not `"foo\bar.txt"`.
 * 
 * ## Engine read files
 * 
 * The engine access a few files directly, most of them are lua files which
 * access other files themselves. Here the list of files that must exist in the
 * VFS (some of them don't have to be in the game/map archive cause there are
 * fallback solutions in `springcontent.sdz` & `maphelper.sdz`):
 * 
 * - `./`
 *   - anims/
 *     - `cursornormal.bmp/png`
 *   - gamedata/
 *     - `defs.lua`
 *     - `explosions.lua`
 *     - `explosion_alias.lua`
 *     - `icontypes.lua`
 *     - `messages.lua`
 *     - `modrules.lua`
 *     - `resources.lua`
 *     - `resources_map.lua`
 *     - `sidedata.lua`
 *     - `sounds.lua`
 *   - `luagaia/`
 *     - `main.lua`
 *     - `draw.lua`
 *   - `luarules/`
 *     - `main.lua`
 *     - `draw.lua`
 *   - `luaui/`
 *     - `main.lua`
 *   - `shaders/`
 *     - `?`
 *   - `luaai.lua`
 *   - `mapinfo.lua`
 *   - `mapoptions.lua`
 *   - `modinfo.lua`
 *   - `modoptions.lua`
 *   - `validmaps.lua`
 *
 * @table VFS
 */

bool LuaVFS::PushCommon(lua_State* L)
{

	/*** @field VFS.RAW "r" Only select uncompressed files. */
	HSTR_PUSH_CSTRING(L, "RAW",       SPRING_VFS_RAW);
	/*** @field VFS.GAME "M" */
	HSTR_PUSH_CSTRING(L, "GAME",      SPRING_VFS_MOD); // synonym to MOD
	/*** @field VFS.MAP "m" */
	HSTR_PUSH_CSTRING(L, "MAP",       SPRING_VFS_MAP);
	/*** @field VFS.BASE "b" */
	HSTR_PUSH_CSTRING(L, "BASE",      SPRING_VFS_BASE);
	/*** @field VFS.MENU "e" */
	HSTR_PUSH_CSTRING(L, "MENU",      SPRING_VFS_MENU);
	/*** @field VFS.ZIP "Mmeb" Only select compressed files (`.sdz`, `.sd7`). */
	HSTR_PUSH_CSTRING(L, "ZIP",       SPRING_VFS_ZIP);
	/*** @field VFS.RAW_FIRST "rMmeb" Try uncompressed files first, then compressed. */
	HSTR_PUSH_CSTRING(L, "RAW_FIRST", SPRING_VFS_RAW_FIRST);
	/*** @field VFS.ZIP_FIRST "Mmebr" Try compressed files first, then uncompressed. */
	HSTR_PUSH_CSTRING(L, "ZIP_FIRST", SPRING_VFS_ZIP_FIRST);

	/***
	 * @deprecated
	 * @field VFS.MOD "M" Older spelling for `VFS.GAME`
	 */
	HSTR_PUSH_CSTRING(L, "MOD",       SPRING_VFS_MOD);
	/***
	 * @deprecated
	 * @field VFS.RAW_ONLY "r"
	 */
	HSTR_PUSH_CSTRING(L, "RAW_ONLY",  SPRING_VFS_RAW); // backwards compatibility
	/***
	 * @deprecated
	 * @field VFS.ZIP_ONLY "Mmeb"
	 */
	HSTR_PUSH_CSTRING(L, "ZIP_ONLY",  SPRING_VFS_ZIP); // backwards compatibility

	HSTR_PUSH_CFUNC(L, "PackU8",    PackU8);
	HSTR_PUSH_CFUNC(L, "PackU16",   PackU16);
	HSTR_PUSH_CFUNC(L, "PackU32",   PackU32);
	HSTR_PUSH_CFUNC(L, "PackS8",    PackS8);
	HSTR_PUSH_CFUNC(L, "PackS16",   PackS16);
	HSTR_PUSH_CFUNC(L, "PackS32",   PackS32);
	HSTR_PUSH_CFUNC(L, "PackF32",   PackF32);
	HSTR_PUSH_CFUNC(L, "UnpackU8",  UnpackU8);
	HSTR_PUSH_CFUNC(L, "UnpackU16", UnpackU16);
	HSTR_PUSH_CFUNC(L, "UnpackU32", UnpackU32);
	HSTR_PUSH_CFUNC(L, "UnpackS8",  UnpackS8);
	HSTR_PUSH_CFUNC(L, "UnpackS16", UnpackS16);
	HSTR_PUSH_CFUNC(L, "UnpackS32", UnpackS32);
	HSTR_PUSH_CFUNC(L, "UnpackF32", UnpackF32);

	// compression should be safe in synced context
	HSTR_PUSH_CFUNC(L, "ZlibCompress", ZlibCompress);
	HSTR_PUSH_CFUNC(L, "ZlibDecompress", ZlibDecompress);
	HSTR_PUSH_CFUNC(L, "CalculateHash", CalculateHash);

	return true;
}


bool LuaVFS::PushSynced(lua_State* L)
{
	PushCommon(L);

	HSTR_PUSH_CFUNC(L, "Include",    SyncInclude);
	HSTR_PUSH_CFUNC(L, "LoadFile",   SyncLoadFile);
	HSTR_PUSH_CFUNC(L, "FileExists", SyncFileExists);
	HSTR_PUSH_CFUNC(L, "DirList",    SyncDirList);
	HSTR_PUSH_CFUNC(L, "SubDirs",    SyncSubDirs);

	return true;
}


bool LuaVFS::PushUnsynced(lua_State* L)
{
	PushCommon(L);

	HSTR_PUSH_CFUNC(L, "Include",             UnsyncInclude);
	HSTR_PUSH_CFUNC(L, "LoadFile",            UnsyncLoadFile);
	HSTR_PUSH_CFUNC(L, "FileExists",          UnsyncFileExists);
	HSTR_PUSH_CFUNC(L, "DirList",             UnsyncDirList);
	HSTR_PUSH_CFUNC(L, "SubDirs",             UnsyncSubDirs);

	HSTR_PUSH_CFUNC(L, "GetFileAbsolutePath",      GetFileAbsolutePath);
	HSTR_PUSH_CFUNC(L, "GetArchiveContainingFile", GetArchiveContainingFile);

	HSTR_PUSH_CFUNC(L, "UseArchive",     UseArchive);
	HSTR_PUSH_CFUNC(L, "CompressFolder", CompressFolder);

	// Removed due to sync unsafety, see commit 0ee88788931f9f0b195eb5f895f1092fde4211c0
	// HSTR_PUSH_CFUNC(L, "MapArchive",     MapArchive);
	// HSTR_PUSH_CFUNC(L, "UnmapArchive",   UnmapArchive);

	return true;
}


/******************************************************************************/
/******************************************************************************/

const string LuaVFS::GetModes(lua_State* L, int index, bool synced)
{
	const bool vfsOnly = (synced && !CLuaHandle::GetDevMode());

	const char* defModes = vfsOnly? SPRING_VFS_ZIP : SPRING_VFS_RAW_FIRST;
	const char* badModes = vfsOnly? SPRING_VFS_RAW SPRING_VFS_MENU : "";

	return CFileHandler::ForbidModes(luaL_optstring(L, index, defModes), badModes);
}


/******************************************************************************/

static int LoadFileWithModes(const std::string& fileName, std::string& data, const std::string& vfsModes)
{
	CFileHandler fh(fileName, vfsModes);

	if (!fh.FileExists())
		return (fh.LoadCode());

	return (fh.LoadStringData(data));
}


/******************************************************************************/
/******************************************************************************/


/***
 * Loads and runs lua code from a file in the VFS.
 * 
 * @function VFS.Include
 * 
 * The path is relative to the main Spring directory, e.g.
 * 
 * ```lua
 * VFS.Include('LuaUI/includes/filename.lua', nil, vfsmode)
 * ```
 * 
 * @param filename string
 * 
 * Path to file, lowercase only. Use linux style path separators, e.g.
 * `"foo/bar.txt"`.
 * 
 * @param environment table? (Default: the current function environment)
 * 
 * The environment arg sets the global environment (see generic lua refs). In
 * almost all cases, this should be left `nil` to preserve the current env.
 *  
 * If the provided, any non-local variables and functions defined in
 * `filename.lua` are then accessable via env. Vise-versa, any variables
 * defined in env prior to passing to `VFS.Include` are available to code in the
 * included file. Code running in `filename.lua` will see the contents of env in
 * place of the normal global environment.
 * 
 * @param mode string?
 * 
 * VFS modes are single char strings and can be concatenated;
 * doing specifies an order of preference for the mode (i.e. location) from
 * which to include files.
 * 
 * @return any module The return value of the included file.
 */
int LuaVFS::Include(lua_State* L, bool synced)
{
	const std::string fileName = luaL_checkstring(L, 1);
	      std::string fileData;

	#if 0
	ScopedOnceTimer timer("LuaVFS::Include(" + fileName + ")");
	#endif

	// the path may point to a file or dir outside of any data-dir
	// if (!LuaIO::IsSimplePath(fileName)) return 0;

	// note: this check must happen before luaL_loadbuffer gets called
	// it pushes new values on the stack and if only index 1 was given
	// to Include those by luaL_loadbuffer are pushed to index 2,3,...
	const bool hasCustomEnv = !lua_isnoneornil(L, 2);

	if (hasCustomEnv)
		luaL_checktype(L, 2, LUA_TTABLE);

	int loadCode = 0;
	int luaError = 0;

	const auto mode = GetModes(L, 3, synced);
	if ((loadCode = LoadFileWithModes(fileName, fileData, mode)) != 1) {
		std::string_view hint {""};
		if (loadCode == -1) // magic value from VFSHandler
			hint = "File not seen by VFS (missing or in different VFS mode)";

		const auto buf = fmt::format("[LuaVFS::{}(synced={})][loadvfs] file={} status={} cenv={} vfsmode={} {}", __func__, synced, fileName, loadCode, hasCustomEnv, mode, hint);
		lua_pushlstring(L, buf.c_str(), buf.size());
 		lua_error(L);
	}

	LuaUtils::TracyRemoveAlsoExtras(fileData.data());
	if ((luaError = luaL_loadbuffer(L, fileData.c_str(), fileData.size(), fileName.c_str())) != 0) {
		const auto buf = fmt::format("[LuaVFS::{}(synced={})][loadbuf] file={} error={} ({}) cenv={} vfsmode={}", __func__, synced, fileName, luaError, lua_tostring(L, -1), hasCustomEnv, mode);
		lua_pushlstring(L, buf.c_str(), buf.size());
		lua_error(L);
	}


	// set the chunk's fenv to the current fenv, or a user table
	if (hasCustomEnv) {
		luaL_checktype(L, 2, LUA_TTABLE);
		lua_pushvalue(L, 2);
	} else {
		LuaUtils::PushCurrentFuncEnv(L, __func__);
		luaL_checktype(L, -1, LUA_TTABLE);
	}

	// set the include fenv to the current function's fenv
	if (lua_setfenv(L, -2) == 0)
		luaL_error(L, "[LuaVFS::%s(synced=%d)][setfenv] file=%s type=%d cenv=%d", __func__, synced, fileName.c_str(), lua_type(L, -2), hasCustomEnv);


	const int paramTop = lua_gettop(L) - 1;

	if ((luaError = lua_pcall(L, 0, LUA_MULTRET, 0)) != 0) {
		const auto buf = fmt::format("[LuaVFS::{}(synced={})][pcall] file={} error={} ({}) ptop={} cenv={} vfsmode={}", __func__, synced, fileName, luaError, lua_tostring(L, -1), paramTop, hasCustomEnv, mode);
		lua_pushlstring(L, buf.c_str(), buf.size());
		lua_error(L);
	}

	// FIXME -- adjust stack?
	return lua_gettop(L) - paramTop;
}


int LuaVFS::SyncInclude(lua_State* L)
{
	return Include(L, true);
}


int LuaVFS::UnsyncInclude(lua_State* L)
{
	return Include(L, false);
}


/******************************************************************************/

/***
 * Load raw text data from the VFS.
 * 
 * @function VFS.LoadFile
 * 
 * Returns file contents as a string. Unlike `VFS.Include` the file will not be
 * executed. This lets you pre-process the code. Use `loadstring` afterwards.
 *
 * @param filename string
 * 
 * Path to file, lowercase only. Use linux style path separators, e.g.
 * `"foo/bar.txt"`.
 *
 * @param mode string?
 * 
 * VFS modes are single char strings and can be concatenated;
 * doing specifies an order of preference for the mode (i.e. location) from
 * which to include files.
 * 
 * @return string? data The contents of the file.
 */
int LuaVFS::LoadFile(lua_State* L, bool synced)
{
	const string filename = luaL_checkstring(L, 1);
	// the path may point to a file or dir outside of any data-dir
	// if (!LuaIO::IsSimplePath(filename)) return 0;

	string data;
	if (LoadFileWithModes(filename, data, GetModes(L, 2, synced)) == 1) {
		LuaUtils::TracyRemoveAlsoExtras(data.data());
		lua_pushsstring(L, data);
		return 1;
	}
	return 0;
}


int LuaVFS::SyncLoadFile(lua_State* L)
{
	return LoadFile(L, true);
}


int LuaVFS::UnsyncLoadFile(lua_State* L)
{
	return LoadFile(L, false);
}


/******************************************************************************/

/***
 * Check if file exists in VFS.
 * 
 * @function VFS.FileExists
 * 
 * Example usage:
 * 
 * ```lua
 * if VFS.FileExists("mapconfig/custom_lava_config.lua", VFS.MAP) then
 *   # ...
 * end
 * ```
 * 
 * @param filename string
 * 
 * Path to file, lowercase only. Use linux style path separators, e.g.
 * `"foo/bar.txt"`.
 *
 * @param mode string?
 * 
 * VFS modes are single char strings and can be concatenated;
 * doing specifies an order of preference for the mode (i.e. location) from
 * which to include files.
 * 
 * @return boolean exists `true` if the file exists, otherwise `false`.
 */
int LuaVFS::FileExists(lua_State* L, bool synced)
{
	const std::string& filename = luaL_checkstring(L, 1);
	const std::string& vfsModes = GetModes(L, 2, synced);

	// FIXME: return 0, keep searches within the Spring directory
	// the path may point to a file or dir outside of any data-dir
	// if (!LuaIO::IsSimplePath(filename)) return 0;

	lua_pushboolean(L, CFileHandler::FileExists(filename, vfsModes));
	return 1;
}


int LuaVFS::  SyncFileExists(lua_State* L) { return FileExists(L,  true); }
int LuaVFS::UnsyncFileExists(lua_State* L) { return FileExists(L, false); }


/******************************************************************************/

/***
 * List files in a directory.
 * 
 * @function VFS.DirList
 * 
 * Example usage:
 * 
 * ```lua
 * local luaFiles = VFS.DirList('units/', '*.lua', nil, true)
 * ```
 * 
 * @param directory string
 * 
 * Path to directory, lowercase only. Use linux style path separators, e.g.
 * `"foo/bar/"`.
 *
 * @param pattern string? (Default: `"*"`)
 * 
 * @param mode string?
 * 
 * VFS modes are single char strings and can be concatenated;
 * doing specifies an order of preference for the mode (i.e. location) from
 * which to include files.
 * 
 * @param recursive boolean? (Default: `false`)
 * 
 * @return string[] filenames
 */
int LuaVFS::DirList(lua_State* L, bool synced)
{
	const std::string& dir = luaL_checkstring(L, 1);

	// FIXME: return 0, keep searches within the Spring directory
	// the path may point to a file or dir outside of any data-dir
	// if (!LuaIO::IsSimplePath(dir)) return 0;

	const std::string& pattern = luaL_optstring(L, 2, "*");
	const std::string& modes = GetModes(L, 3, synced);
	const bool recursive = luaL_optboolean(L, 4, false);

	LuaUtils::PushStringVector(L, CFileHandler::DirList(dir, pattern, modes, recursive));
	return 1;
}


int LuaVFS::SyncDirList(lua_State* L)
{
	return DirList(L, true);
}


int LuaVFS::UnsyncDirList(lua_State* L)
{
	return DirList(L, false);
}


/******************************************************************************/

/***
 * List sub-directories in a directory.
 * 
 * @function VFS.SubDirs
 * 
 * Example usage:
 * 
 * ```lua
 * local files = VFS.SubDirs('sounds/voice/' .. language, '*')
 * for _, file in ipairs(files) do
 * 	# ...
 * end
 * ```
 * 
 * @param directory string
 * 
 * Path to directory, lowercase only. Use linux style path separators, e.g.
 * `"foo/bar/"`.
 *
 * @param pattern string? (Default: `"*"`)
 * 
 * @param mode string?
 * 
 * VFS modes are single char strings and can be concatenated;
 * doing specifies an order of preference for the mode (i.e. location) from
 * which to include files.
 * 
 * @param recursive boolean? (Default: `false`)
 * 
 * @return string[] dirnames
 */
int LuaVFS::SubDirs(lua_State* L, bool synced)
{
	const std::string& dir = luaL_checkstring(L, 1);

	// FIXME: return 0, keep searches within the Spring directory
	// the path may point to a file or dir outside of any data-dir
	// if (!LuaIO::IsSimplePath(dir)) return 0;

	const std::string& pattern = luaL_optstring(L, 2, "*");
	const std::string& modes = GetModes(L, 3, synced);
	const bool recursive = luaL_optboolean(L, 4, false);

	LuaUtils::PushStringVector(L, CFileHandler::SubDirs(dir, pattern, modes, recursive));
	return 1;
}


int LuaVFS::SyncSubDirs(lua_State* L)
{
	return SubDirs(L, true);
}

int LuaVFS::UnsyncSubDirs(lua_State* L)
{
	return SubDirs(L, false);
}

/***
 * @function VFS.GetFileAbsolutePath
 *
 * @param filename string
 * 
 * Path to file, lowercase only. Use linux style path separators, e.g.
 * `"foo/bar.txt"`.
 *
 * @param mode string?
 * 
 * VFS modes are single char strings and can be concatenated;
 * doing specifies an order of preference for the mode (i.e. location) from
 * which to include files.
 * 
 * @return string? absolutePath
 */
int LuaVFS::GetFileAbsolutePath(lua_State* L)
{
	const std::string filename = luaL_checkstring(L, 1);

	// FIXME: return 0, keep searches within the Spring directory
	// the path may point to a file or dir outside of any data-dir
	// if (!LuaIO::IsSimplePath(filename)) return 0;

	if (!CFileHandler::FileExists(filename, GetModes(L, 2, false)))
		return 0;

	const std::string& absolutePath = CFileHandler::GetFileAbsolutePath(filename, GetModes(L, 2, false));

	if (absolutePath.empty())
		return 0;

	lua_pushsstring(L, absolutePath);
	return 1;
}

/******************************************************************************/
/******************************************************************************/

/***
 * @function VFS.GetArchiveContainingFile
 *
 * @param filename string
 * 
 * Path to file, lowercase only. Use linux style path separators, e.g.
 * `"foo/bar.txt"`.
 *
 * @param mode string?
 * 
 * VFS modes are single char strings and can be concatenated;
 * doing specifies an order of preference for the mode (i.e. location) from
 * which to include files.
 * 
 * @return string? archiveName
 */
int LuaVFS::GetArchiveContainingFile(lua_State* L)
{
	const std::string filename = luaL_checkstring(L, 1);

	// FIXME: return 0, keep searches within the Spring directory
	// the path may point to a file or dir outside of any data-dir
	// if (!LuaIO::IsSimplePath(filename)) return 0;

	if (!CFileHandler::FileExists(filename, GetModes(L, 2, false)))
		return 0;

	const std::string& archiveName = CFileHandler::GetArchiveContainingFile(filename, GetModes(L, 2, false));

	if (archiveName.empty())
		return 0;

	lua_pushsstring(L, archiveName);
	return 1;
}


/******************************************************************************/
/******************************************************************************/

/***
 * Temporarily load an archive from the VFS and run the given function,
 * which can make usage of the files in the archive.
 * 
 * @function VFS.UseArchive
 * @param archiveName string
 * @param fun(...) func
 * @return any ... Results of of the given function
 */
int LuaVFS::UseArchive(lua_State* L)
{
	// only from unsynced
	if (CLuaHandle::GetHandleSynced(L))
		return 0;

	const std::string& archiveName = luaL_checkstring(L, 1);
	const CArchiveScanner::ArchiveData& archiveData = archiveScanner->GetArchiveData(archiveName);
	if (archiveData.IsEmpty())
		luaL_error(L, "[VFS::%s] archive \"%s\" not found", __func__, archiveName.c_str());

	constexpr int funcIndex = 2;
	if (!lua_isfunction(L, funcIndex))
		luaL_error(L, "[VFS::%s] second argument should be a function", __func__);

	if (vfsHandler->HasArchive(archiveName))
		luaL_error(L, "[VFS::%s] archive \"%s\" already loaded", __func__, archiveName.c_str());

	// block other threads from getting the global until we are done
	vfsHandler->GrabLock();
	vfsHandler->SetName("LuaVFS");
	vfsHandler->UnMapArchives(false);

	// could be mod,map,etc
	vfsHandler->AddArchive(archiveName, false);

	const int callError = lua_pcall(L, lua_gettop(L) - funcIndex, LUA_MULTRET, 0);

	vfsHandler->RemoveArchive(archiveName);
	vfsHandler->ReMapArchives(false);
	vfsHandler->SetName("SpringVFS");
	vfsHandler->FreeLock();

	if (callError != 0)
		lua_error(L);

	return (lua_gettop(L) - funcIndex + 1);
}

/** -- Not exported.
 * 
 * Permanently loads an archive into the VFS (to load zipped music collections
 * etc.).
 * 
 * Does nothing if the archive is already loaded in the VFS (won't reload even
 * if there are changes made to the archive). If checksum is given it checks if
 * the to be loaded file is correct, if not then it won't load it and return
 * false.
 * 
 * @function VFS.MapArchive
 * @param archiveName string
 * @param checksum string?
 * @return boolean
 */
int LuaVFS::MapArchive(lua_State* L)
{
	// only from unsynced
	if (CLuaHandle::GetHandleSynced(L))
		return 0;

	const int args = lua_gettop(L); // number of arguments

	const std::string& archiveName = luaL_checkstring(L, 1);
	const CArchiveScanner::ArchiveData& archiveData = archiveScanner->GetArchiveData(archiveName);
	if (archiveData.IsEmpty())
		luaL_error(L, "[VFS::%s] archive not found: %s", __func__, archiveName.c_str());

	if (args >= 2) {
		sha512::hex_digest argChecksum;
		sha512::hex_digest hexChecksum;

		std::fill(argChecksum.begin(), argChecksum.end(), 0);
		std::memcpy(argChecksum.data(), lua_tostring(L, 2), std::min(argChecksum.size() - 1, strlen(lua_tostring(L, 2))));
		sha512::dump_digest(archiveScanner->GetArchiveSingleChecksumBytes(archiveName), hexChecksum);

		if (argChecksum != hexChecksum)
			luaL_error(L, "[VFS::%s] incorrect checksum for archive: %s (got: %s, expected: %s)",
				__func__, archiveName.c_str(), argChecksum.data(), hexChecksum.data());
	}

	if (!vfsHandler->AddArchive(archiveName, false))
		luaL_error(L, "[VFS::%s] failed to load archive: %s", archiveName.c_str());

	lua_pushboolean(L, true);
	return 1;
}

/** -- Not exported.
 * 
 * Removes an already loaded archive (see `VFS.MapArchive`).
 * 
 * @function VFS.UnmapArchive
 * @param archiveName string
 * @return boolean
 */
int LuaVFS::UnmapArchive(lua_State* L)
{
	// only from unsynced
	if (CLuaHandle::GetHandleSynced(L))
		return 0;

	const std::string& archiveName = luaL_checkstring(L, 1);
	const CArchiveScanner::ArchiveData& archiveData = archiveScanner->GetArchiveData(archiveName);
	if (archiveData.IsEmpty())
		luaL_error(L, "[VFS::%s] archive not found: %s", __func__, archiveName.c_str());

	LOG("[LuaVFS::%s] archive=%s", __func__, archiveName.c_str());

	if (!vfsHandler->RemoveArchive(archiveName))
		luaL_error(L, "[VFS::%s] failed to remove archive: %s", __func__, archiveName.c_str());

	lua_pushboolean(L, true);
	return 1;
}



/******************************************************************************/

/***
 * Compresses the specified folder.
 * @function VFS.CompressFolder
 * @param folderPath string
 * @param archiveType string? (Default: `"zip"`)The compression type (can
 * currently be only `"zip"`).
 * @param compressedFilePath string? (Default: `folderPath .. ".sdz"`)
 * @param includeFolder boolean? (Default: `false`) Whether the archive should
 * have the specified folder as root.
 * @param mode string?
 */
int LuaVFS::CompressFolder(lua_State* L)
{
	const std::string& folderPath = luaL_checkstring(L, 1);
	const std::string& archiveType = luaL_optstring(L, 2, "zip");

	if (archiveType != "zip" && archiveType != "7z") //TODO: add 7z support
		luaL_error(L, ("Unsupported archive type " + archiveType).c_str());

	 // "sdz" is the default type if not specified
	const std::string& compressedFilePath = luaL_optstring(L, 3, (folderPath + ".sdz").c_str());
	const std::string& modes = GetModes(L, 5, false);

	const bool includeFolder = luaL_optboolean(L, 4, false);

	if (CFileHandler::FileExists(compressedFilePath, modes))
		luaL_error(L, ("File already exists " + compressedFilePath).c_str());

	if (archiveType == "zip") {
		LuaZipFolder::ZipFolder(L, folderPath, compressedFilePath, includeFolder, modes);
	} else if (archiveType == "7z") {
		SevenZipFolder(L, folderPath, compressedFilePath, includeFolder, modes);
	}

	return 0;
}

/******************************************************************************/

int LuaVFS::SevenZipFolder(lua_State* L, const string& folderPath, const string& zipFilePath, bool includeFolder, const string& modes)
{
	luaL_error(L, "7z-compression is not implemented yet.");
	return 0;
}


/***
 * @function VFS.ZlibCompress
 * @param uncompressed string Data to compress.
 * @return string? compressed Compressed data, or `nil` on error.
 */
int LuaVFS::ZlibCompress(lua_State* L)
{
	size_t inSize = 0;
	const std::uint8_t* inData = reinterpret_cast<const std::uint8_t*>(luaL_checklstring(L, 1, &inSize));

	const std::vector<std::uint8_t> compressed = zlib::deflate(inData, inSize);

	if (!compressed.empty()) {
		lua_pushlstring(L, reinterpret_cast<const char*>(compressed.data()), compressed.size());
		return 1;
	}

	return luaL_error(L, "Error while compressing");
}

/***
 * @function VFS.ZlibDecompress
 * @param compressed string Data to decompress.
 * @return string? uncompressed Uncompressed data, or `nil` on error.
 */
int LuaVFS::ZlibDecompress(lua_State* L)
{
	size_t inSize = 0;
	const std::uint8_t* inData = reinterpret_cast<const std::uint8_t*>(luaL_checklstring(L, 1, &inSize));

	const std::vector<std::uint8_t> uncompressed = zlib::inflate(inData, inSize);

	if (!uncompressed.empty()) {
		lua_pushlstring(L, reinterpret_cast<const char*>(uncompressed.data()), uncompressed.size());
		return 1;
	}

	return luaL_error(L, "Error while decompressing");
}


/***
 * @alias HashType
 * | 0 # MD5
 * | 1 # SHA512
 */

/***
 * Calculates hash (in base64 form) of a given string.
 * 
 * @function VFS.CalculateHash
 * @param input string
 * @param hashType HashType Hash type.
 * @return string? hash
 */
int LuaVFS::CalculateHash(lua_State* L)
{
	size_t slen = 0;

	const char* sstr = luaL_checklstring(L, 1, &slen);
	const char* hash = "";

	enum {
		HASHTYPE_MD5 = 0,
		HASHTYPE_SHA = 1,
	};

	switch (luaL_checkint(L, 2)) {
		case HASHTYPE_MD5: {
			// base64(MD5); pr-downloader only accepts type=0
			lua_pushstring(L, hash = CalcHash(sstr, slen, HASHTYPE_MD5));
			free((char*) hash);
		} break;
		case HASHTYPE_SHA: {
			sha512::hex_digest hexHash;
			sha512::raw_digest rawHash;

			hexHash.fill(0);
			rawHash.fill(0);

			sha512::calc_digest({sstr, sstr + slen}, rawHash);
			sha512::dump_digest(rawHash, hexHash);

			lua_pushstring(L, hexHash.data());
		} break;
		default: {
			luaL_error(L, "[VFS::%s] unsupported hash type", __func__);
		} break;
	}

	return 1;
}

/******************************************************************************/
/******************************************************************************/
//
//  NOTE: Endianess should be handled
//

template <typename T>
int PackType(lua_State* L)
{
	std::vector<T> vals;
	std::vector<char> buf;

	if (lua_istable(L, 1)) {
		vals.reserve(lua_objlen(L, 1));

		for (int i = 1; lua_rawgeti(L, 1, i), lua_isnumber(L, -1); lua_pop(L, 1), i++) {
			vals.push_back(static_cast<T>(lua_tonumber(L, -1)));
		}
	} else {
		vals.resize(lua_gettop(L));

		for (size_t i = 0; i < vals.size(); i++) {
			if (!lua_isnumber(L, i + 1))
				break;

			vals[i] = static_cast<T>(lua_tonumber(L, i + 1));
		}
	}

	if (vals.empty())
		return 0;

	buf.resize(sizeof(T) * vals.size(), 0);

	for (size_t i = 0; i < vals.size(); i++) {
		memcpy(buf.data() + (i * sizeof(T)), &vals[i], sizeof(T));
	}

	lua_pushlstring(L, buf.data(), buf.size());
	return 1;
}


/***
 * Convert unsigned 8-bit integer(s) to binary string.
 * @function VFS.PackU8 
 * @param ... integer Numbers to pack.
 * @return string
 */
/***
 * Convert unsigned 8-bit integer(s) to binary string.
 * @function VFS.PackU8 
 * @param numbers integer[] Numbers to pack.
 * @return string
 */
int LuaVFS::PackU8(lua_State* L) { return PackType<std::uint8_t >(L); }

/***
 * Convert unsigned 16-bit integer(s) to binary string.
 * @function VFS.PackU16 
 * @param ... integer Numbers to pack.
 * @return string
 */
/***
 * Convert unsigned 16-bit integer(s) to binary string.
 * @function VFS.PackU16 
 * @param numbers integer[] Numbers to pack.
 * @return string
 */
int LuaVFS::PackU16(lua_State* L) { return PackType<std::uint16_t>(L); }

/***
 * Convert unsigned 32-bit integer(s) to binary string.
 * @function VFS.PackU32 
 * @param ... integer Numbers to pack.
 * @return string
 */
/***
 * Convert unsigned 32-bit integer(s) to binary string.
 * @function VFS.PackU32 
 * @param numbers integer[] Numbers to pack.
 * @return string
 */
int LuaVFS::PackU32(lua_State* L) { return PackType<std::uint32_t>(L); }

/***
 * Convert signed 8-bit integer(s) to binary string.
 * @function VFS.PackS8 
 * @param ... integer Numbers to pack.
 * @return string
 */
/***
 * Convert signed 8-bit integer(s) to binary string.
 * @function VFS.PackS8 
 * @param numbers integer[] Numbers to pack.
 * @return string
 */
int LuaVFS::PackS8(lua_State* L) { return PackType<std::int8_t>(L); }

/***
 * Convert signed 16-bit integer(s) to binary string.
 * @function VFS.PackS16 
 * @param ... integer Numbers to pack.
 * @return string
 */
/***
 * Convert signed 16-bit integer(s) to binary string.
 * @function VFS.PackS16 
 * @param numbers integer[] Numbers to pack.
 * @return string
 */
int LuaVFS::PackS16(lua_State* L) { return PackType<std::int16_t>(L); }

/***
 * Convert signed 32-bit integer(s) to binary string.
 * @function VFS.PackS32 
 * @param ... integer Numbers to pack.
 * @return string
 */
/***
 * Convert signed 32-bit integer(s) to binary string.
 * @function VFS.PackS32 
 * @param numbers integer[] Numbers to pack.
 * @return string
 */
int LuaVFS::PackS32(lua_State* L) { return PackType<std::int32_t>(L); }

/***
 * Convert signed 32-bit float(s) to binary string.
 * @function VFS.PackS32 
 * @param ... integer Numbers to pack.
 * @return string
 */
/***
 * Convert signed 32-bit float(s) to binary string.
 * @function VFS.PackS32 
 * @param numbers integer[] Numbers to pack.
 * @return string
 */
int LuaVFS::PackF32(lua_State* L) { return PackType<float>(L); }


/******************************************************************************/

template <typename T>
int UnpackType(lua_State* L)
{
	if (!lua_isstring(L, 1))
		return 0;

	size_t len;
	const char* str = lua_tolstring(L, 1, &len);

	if (lua_isnumber(L, 2)) {
		const int pos = lua_toint(L, 2);
		if ((pos < 1) || ((size_t)pos >= len))
			return 0;

		const int offset = (pos - 1);
		str += offset;
		len -= offset;
	}

	const size_t eSize = sizeof(T);
	if (len < eSize)
		return 0;

	if (!lua_isnumber(L, 3)) {
		lua_pushnumber(L, *(reinterpret_cast<const T*>(str)));
		return 1;
	}

	const size_t maxCount = len / eSize;
	int tableCount = lua_toint(L, 3);
	if (tableCount < 0)
		tableCount = maxCount;

	lua_createtable(L, tableCount = std::min((int)maxCount, tableCount), 0);
	for (int i = 0; i < tableCount; i++) {
		lua_pushnumber(L, *(reinterpret_cast<const T*>(str) + i));
		lua_rawseti(L, -2, (i + 1));
	}
	return 1;
}


/***
 * Convert a binary string to an unsigned 8-bit integer.
 * @function VFS.UnpackU8
 * @param str string Binary string.
 * @param pos integer? Byte offset.
 * @return integer
 */
int LuaVFS::UnpackU8(lua_State* L) { return UnpackType<std::uint8_t>(L); }

/***
 * Convert a binary string to an unsigned 16-bit integer.
 * @function VFS.UnpackU16
 * @param str string Binary string.
 * @param pos integer? Byte offset.
 * @return integer
 */
int LuaVFS::UnpackU16(lua_State* L) { return UnpackType<std::uint16_t>(L); }

/***
 * Convert a binary string to an unsigned 32-bit integer.
 * @function VFS.UnpackU32
 * @param str string Binary string.
 * @param pos integer? Byte offset.
 * @return integer
 */
int LuaVFS::UnpackU32(lua_State* L) { return UnpackType<std::uint32_t>(L); }

/***
 * Convert a binary string to a signed 8-bit integer.
 * @function VFS.UnpackS8
 * @param str string Binary string.
 * @param pos integer? Byte offset.
 * @return integer
 */
int LuaVFS::UnpackS8(lua_State* L) { return UnpackType<std::int8_t>(L); }

/***
 * Convert a binary string to a signed 16-bit integer.
 * @function VFS.UnpackS16
 * @param str string Binary string.
 * @param pos integer? Byte offset.
 * @return integer
 */
int LuaVFS::UnpackS16(lua_State* L) { return UnpackType<std::int16_t>(L); }

/***
 * Convert a binary string to a signed 32-bit integer.
 * @function VFS.UnpackS32
 * @param str string Binary string.
 * @param pos integer? Byte offset.
 * @return integer
 */
int LuaVFS::UnpackS32(lua_State* L) { return UnpackType<std::int32_t>(L); }

/***
 * Convert a binary string to a signed 32-bit float.
 * @function VFS.UnpackF32
 * @param str string Binary string.
 * @param pos integer? Byte offset.
 * @return integer
 */
int LuaVFS::UnpackF32(lua_State* L) { return UnpackType<float>(L); }


/******************************************************************************/
/******************************************************************************/
