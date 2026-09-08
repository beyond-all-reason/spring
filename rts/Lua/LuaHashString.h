/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <algorithm>
#include <string>

#include "LuaInclude.h"
#include "System/StringHash.h"

struct LuaHashString {
	public:
		LuaHashString(const char* s): hash(lua_calchash(s, slen = strlen(s))) {
			assert(slen <= sizeof(str));
			memset(str, 0, sizeof(str));
			memcpy(str, s, std::min(size_t(slen), sizeof(str) - 1));
		}

		LuaHashString(const LuaHashString& hs) { *this = hs; }

		LuaHashString& operator = (const LuaHashString& hs) {
			memcpy(str, hs.str, sizeof(str));

			slen = hs.slen;
			hash = hs.hash;
			return *this;
		}

		inline uint32_t GetHash() const { return hash; }
		inline const char* GetString() const { return str; }

	public:
		inline void Push(lua_State* L) const {
			lua_pushhstring(L, hash, str, slen);
		}

		inline void GetGlobal(lua_State* L) const {
			Push(L);
			lua_rawget(L, LUA_GLOBALSINDEX);
		}
		inline bool GetGlobalFunc(lua_State* L) const {
			GetGlobal(L);

			if (lua_isfunction(L, -1))
				return true;

			lua_pop(L, 1);
			return false;
		}

		inline void GetRegistry(lua_State* L) const {
			Push(L);
			lua_rawget(L, LUA_REGISTRYINDEX);
		}
		inline bool GetRegistryFunc(lua_State* L) const {
			GetRegistry(L);

			if (lua_isfunction(L, -1))
				return true;

			lua_pop(L, 1);
			return false;
		}

		inline void PushBool(lua_State* L, bool value) const {
			Push(L);
			lua_pushboolean(L, value);
			lua_rawset(L, -3);
		}
		inline void PushNumber(lua_State* L, lua_Number value) const {
			Push(L);
			lua_pushnumber(L, value);
			lua_rawset(L, -3);
		}
		inline void PushString(lua_State* L, const char* value) const {
			Push(L);
			lua_pushstring(L, value);
			lua_rawset(L, -3);
		}
		inline void PushString(lua_State* L, const std::string& value) const {
			Push(L);
			lua_pushsstring(L, value);
			lua_rawset(L, -3);
		}

		inline void PushCFunc(lua_State* L, int (*func)(lua_State*)) const {
			Push(L);
			lua_pushcfunction(L, func);
			lua_rawset(L, -3);
		}

	private:
		static constexpr size_t strMaxLen = 64;
		char str[strMaxLen];
		uint32_t slen = 0;
		uint32_t hash = 0;
};