/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaFonts.h"

#include "LuaInclude.h"

#include "LuaHandle.h"
#include "LuaHashString.h"
#include "LuaUtils.h"

#include "LuaOpenGL.h"

#include "Rendering/GL/myGL.h"
#include "Rendering/Fonts/glFont.h"
#include "System/Exceptions.h"

#include "System/Misc/TracyDefs.h"


bool LuaFonts::PushEntries(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	CreateMetatable(L);

	REGISTER_LUA_CFUNC(LoadFont);
	REGISTER_LUA_CFUNC(DeleteFont);
	REGISTER_LUA_CFUNC(AddFallbackFont);
	REGISTER_LUA_CFUNC(ClearFallbackFonts);

	return true;
}


bool LuaFonts::CreateMetatable(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	luaL_newmetatable(L, "Font");

	HSTR_PUSH_CFUNC(L, "__gc",        meta_gc);
	HSTR_PUSH_CFUNC(L, "__index",     meta_index);
	LuaPushNamedString(L, "__metatable", "protected metatable");

		// push userdata callouts
		REGISTER_LUA_CFUNC(Print);
		REGISTER_LUA_CFUNC(PrintWorld);

		REGISTER_LUA_CFUNC(Begin);
		REGISTER_LUA_CFUNC(End);

		REGISTER_LUA_CFUNC(SubmitBuffered);

		REGISTER_LUA_CFUNC(WrapText);

		REGISTER_LUA_CFUNC(GetTextWidth);
		REGISTER_LUA_CFUNC(GetTextHeight);

		REGISTER_LUA_CFUNC(SetTextColor);
		REGISTER_LUA_CFUNC(SetOutlineColor);

		REGISTER_LUA_CFUNC(SetAutoOutlineColor);

		REGISTER_LUA_CFUNC(BindTexture);

	lua_pop(L, 1);
	return true;
}

/******************************************************************************/
/******************************************************************************/

inline void CheckDrawingEnabled(lua_State* L, const char* caller)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (LuaOpenGL::IsDrawingEnabled(L))
		return;

	luaL_error(L, "[%s::%s] OpenGL calls can only be used in Draw() call-ins", __func__, caller);
}


inline CglFont* tofont(lua_State* L, int idx)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto font = static_cast<std::shared_ptr<CglFont>*>(luaL_checkudata(L, idx, "Font"));

	if (*font == nullptr)
		luaL_error(L, "attempt to use a deleted font");

	return font->get();
}


/******************************************************************************/
/******************************************************************************/

int LuaFonts::meta_gc(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (lua_isnil(L, 1))
		return 0;

	auto font = std::move(*static_cast<std::shared_ptr<CglFont>*>(luaL_checkudata(L, 1, "Font")));
	font = {};

	return 0;
}


int LuaFonts::meta_index(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// first check if there is a function
	luaL_getmetatable(L, "Font");
	lua_pushvalue(L, 2);
	lua_rawget(L, -2);
	if (!lua_isnil(L, -1))
		return 1;

	lua_pop(L, 1);

	// couldn't find a function, so check properties
	auto f = tofont(L, 1);

	if (lua_israwstring(L, 2)) {
		switch (hashString(lua_tostring(L, 2))) {
			case hashString("size"): {
				lua_pushnumber(L, f->GetSize());
				return 1;
			} break;
			case hashString("path"): {
				lua_pushsstring(L, f->GetFilePath());
				return 1;
			} break;

			case hashString("height"):
			case hashString("lineheight"): {
				lua_pushnumber(L, f->GetLineHeight());
				return 1;
			} break;

			case hashString("descender"): {
				lua_pushnumber(L, f->GetDescender());
				return 1;
			} break;

			case hashString("outlinewidth"): {
				lua_pushnumber(L, f->GetOutlineWidth());
				return 1;
			} break;
			case hashString("outlineweight"): {
				lua_pushnumber(L, f->GetOutlineWeight());
				return 1;
			} break;

			case hashString("family"): {
				lua_pushsstring(L, f->GetFamily());
				return 1;
			} break;
			case hashString("style"): {
				lua_pushsstring(L, f->GetStyle());
				return 1;
			} break;

			case hashString("texturewidth"): {
				lua_pushnumber(L, f->GetTextureWidth());
				return 1;
			} break;
			case hashString("textureheight"): {
				lua_pushnumber(L, f->GetTextureHeight());
				return 1;
			} break;

			default: {
			} break;
		}
	}

	return 0;
}


/******************************************************************************/
/******************************************************************************/

int LuaFonts::LoadFont(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto f = CglFont::LoadFont(luaL_checkstring(L, 1), luaL_optint(L, 2, 14), luaL_optint(L, 3, 2), luaL_optfloat(L, 4, 15.0f));
	if (f == nullptr)
		return 0;

	auto shPtrFontPtr = static_cast<decltype(f)*>(lua_newuserdata(L, sizeof(decltype(f))));
	memset(shPtrFontPtr, 0, sizeof(decltype(f)));

	*shPtrFontPtr = std::move(f);

	luaL_getmetatable(L, "Font");
	lua_setmetatable(L, -2);
	return 1;
}


int LuaFonts::DeleteFont(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	return meta_gc(L);
}

/*** Adds a fallback font for the font rendering engine.
 *
 * Fonts added first will have higher priority.
 * When a glyph isn't found when rendering a font, the fallback fonts
 * will be searched first, otherwise os fonts will be used.
 *
 * The application should listen for the unsynced 'FontsChanged' callin so
 * modules can clear any already reserved display lists or other relevant
 * caches.
 *
 * Note the callin won't be executed at the time of calling this method,
 * but later, on the Update cycle (before other Update and Draw callins).
 *
 * @function gl.AddFallbackFont
 * @param filePath string VFS path to the file, for example "fonts/myfont.ttf". Uses VFS.RAW_FIRST access mode.
 * @return boolean success
 */
int LuaFonts::AddFallbackFont(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;

	const auto font = luaL_checkstring(L, 1);

	const bool res = CFontTexture::AddFallbackFont(font);
	lua_pushboolean(L, res);
	return 1;
}

/*** Clears all fallback fonts.
 *
 * See the note at 'AddFallbackFont' about the 'FontsChanged' callin,
 * it also applies when calling this method.
 *
 * @function gl.ClearFallbackFonts
 * @return nil
 */
int LuaFonts::ClearFallbackFonts(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;

	CFontTexture::ClearFallbackFonts();
	return 0;
}

/******************************************************************************/
/******************************************************************************/

int LuaFonts::Print(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	CheckDrawingEnabled(L, __func__);

	const int args = lua_gettop(L); // number of arguments

	auto f = tofont(L, 1);

	const string text(luaL_checkstring(L, 2),lua_strlen(L, 2));
	const float x     = luaL_checkfloat(L, 3);
	const float y     = luaL_checkfloat(L, 4);
	const float size  = luaL_optfloat(L, 5, f->GetSize());

	int options = FONT_NEAREST;

	if ((args >= 6) && lua_isstring(L, 6)) {
		const char* c = lua_tostring(L, 6);
		while (*c != 0) {
			switch (*(c++)) {
				case 'c': { options |= FONT_CENTER;       } break;
				case 'r': { options |= FONT_RIGHT;        } break;

				case 'a': { options |= FONT_ASCENDER;     } break;
				case 't': { options |= FONT_TOP;          } break;
				case 'v': { options |= FONT_VCENTER;      } break;
				case 'x': { options |= FONT_BASELINE;     } break;
				case 'b': { options |= FONT_BOTTOM;       } break;
				case 'd': { options |= FONT_DESCENDER;    } break;

				case 's': { options |= FONT_SHADOW;       } break;
				case 'o':
				case 'O': { options |= FONT_OUTLINE;      } break;

				case 'n': { options ^= FONT_NEAREST;      } break;
				case 'B': { options |= FONT_BUFFERED;     } break; // for DrawBuffered

				case 'N': { options |= FONT_NORM;         } break;
				case 'S': { options |= FONT_SCALE;        } break;
				default: break;
			}
		}
	}

	f->glPrint(x, y, size, options, text);
	return 0;
}

int LuaFonts::PrintWorld(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	CheckDrawingEnabled(L, __func__);

	const int args = lua_gettop(L); // number of arguments

	auto f = tofont(L, 1);

	const string text(luaL_checkstring(L, 2), lua_strlen(L, 2));
	float3 pos = {
		luaL_checkfloat(L, 3),
		luaL_checkfloat(L, 4),
		luaL_checkfloat(L, 5)
	};
	const auto size = luaL_optfloat(L, 6, f->GetSize());

	int options = FONT_NEAREST;

	if ((args >= 7) && lua_isstring(L, 7)) {
		const char* c = lua_tostring(L, 7);
		while (*c != 0) {
			switch (*(c++)) {
			case 'c': { options |= FONT_CENTER;       } break;
			case 'r': { options |= FONT_RIGHT;        } break;

			case 'a': { options |= FONT_ASCENDER;     } break;
			case 't': { options |= FONT_TOP;          } break;
			case 'v': { options |= FONT_VCENTER;      } break;
			case 'x': { options |= FONT_BASELINE;     } break;
			case 'b': { options |= FONT_BOTTOM;       } break;
			case 'd': { options |= FONT_DESCENDER;    } break;

			case 's': { options |= FONT_SHADOW;       } break;
			case 'o':
			case 'O': { options |= FONT_OUTLINE;      } break;

			case 'n': { options ^= FONT_NEAREST;      } break;
			case 'B': { options |= FONT_BUFFERED;     } break; // for DrawBuffered

			case 'N': { options |= FONT_NORM;         } break;
			case 'S': { options |= FONT_SCALE;        } break;
			default: break;
			}
		}
	}

	f->glWorldPrint(pos, size, text, options);
	return 0;
}

/******************************************************************************/
/******************************************************************************/

int LuaFonts::Begin(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	CheckDrawingEnabled(L, __func__);
	auto f = tofont(L, 1);
	f->Begin();
	return 0;
}

int LuaFonts::End(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	CheckDrawingEnabled(L, __func__);
	auto f = tofont(L, 1);
	f->End();
	return 0;
}

int LuaFonts::SubmitBuffered(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	CheckDrawingEnabled(L, __func__);
	auto f = tofont(L, 1);

	if (luaL_optboolean(L, 2, true)) // world or not
		f->DrawBuffered();
	else
		f->DrawWorldBuffered();

	return 0;
}


/******************************************************************************/
/******************************************************************************/

int LuaFonts::WrapText(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto f = tofont(L, 1);

	std::string text(luaL_checkstring(L, 2), lua_strlen(L, 2));

	const float maxWidth   = luaL_checkfloat(L, 3);
	const float maxHeight  = luaL_optfloat(L, 4, CglFont::MAX_HEIGHT_DEFAULT);
	const float size       = luaL_optfloat(L, 5, f->GetSize());

	const int lines = f->WrapInPlace(text, size, maxWidth, maxHeight);

	lua_pushsstring(L, text);
	lua_pushnumber(L, lines);
	return 2;
}

/******************************************************************************/
/******************************************************************************/

int LuaFonts::GetTextWidth(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto f = tofont(L, 1);

	lua_pushnumber(L, f->GetTextWidth(std::string(luaL_checkstring(L, 2), lua_strlen(L, 2))));
	return 1;
}


int LuaFonts::GetTextHeight(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto f = tofont(L, 1);

	const std::string text(luaL_checkstring(L, 2), lua_strlen(L, 2));
	float descender;
	int lines;
	const float height = f->GetTextHeight(text,&descender,&lines);

	lua_pushnumber(L, height);
	lua_pushnumber(L, descender);
	lua_pushnumber(L, lines);
	return 3;
}


/******************************************************************************/
/******************************************************************************/

static int SetTextColorShared(lua_State* L, bool outline)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto f = tofont(L, 1);

	const int args = lua_gettop(L); // number of arguments

	if (args < 2)
		luaL_error(L, "[%s] insufficient arguments to font:SetText%sColor(font,table|number*)", __func__, outline? "Outline": "");

	float4 color = {1.0f, 1.0f, 1.0f, 1.0f};

	if (lua_istable(L, 2)) {
		LuaUtils::ParseFloatArray(L, 2, &color.x, 4);
	} else if (args >= 4) {
		color.x = luaL_optfloat(L, 2, color.x);
		color.y = luaL_optfloat(L, 3, color.y);
		color.z = luaL_optfloat(L, 4, color.z);
		color.w = luaL_optfloat(L, 5, color.w);
	} else if (!lua_isnil(L, 2)) {
		luaL_error(L, "[%s] incorrect arguments to font:SetText%sColor(font,table|number*)", __func__, outline? "Outline": "");
	}

	if (outline)
		f->SetOutlineColor(&color);
	else
		f->SetTextColor(&color);

	return 0;
}

int LuaFonts::SetTextColor(lua_State* L) { return (SetTextColorShared(L, false)); }
int LuaFonts::SetOutlineColor(lua_State* L) { return (SetTextColorShared(L, true)); }


int LuaFonts::SetAutoOutlineColor(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto f = tofont(L, 1);
	f->SetAutoOutlineColor(luaL_checkboolean(L, 2));
	return 0;
}


/******************************************************************************/
/******************************************************************************/

int LuaFonts::BindTexture(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	CheckDrawingEnabled(L, __func__);

	auto f = tofont(L, 1);

	glBindTexture(GL_TEXTURE_2D, f->GetTexture());
	glEnable(GL_TEXTURE_2D);

	return 0;
}
