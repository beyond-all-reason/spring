/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaFBOs.h"

#include "LuaInclude.h"

#include "LuaHandle.h"
#include "LuaHashString.h"
#include "LuaUtils.h"

#include "LuaOpenGL.h"
#include "LuaRBOs.h"
#include "LuaTextures.h"

#include "System/Log/ILog.h"
#include "System/Exceptions.h"
#include "fmt/format.h"

#include "System/Misc/TracyDefs.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/GlobalRendering.h"


/******************************************************************************
 * FBO
 * @see rts/Lua/LuaFBOs.cpp
******************************************************************************/

LuaFBOs::~LuaFBOs()
{
	RECOIL_DETAILED_TRACY_ZONE;
	for (const auto* fbo: fbos) {
		glDeleteFramebuffersEXT(1, &fbo->id);
	}
}


/******************************************************************************/
/******************************************************************************/

bool LuaFBOs::PushEntries(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	CreateMetatable(L);

	REGISTER_LUA_CFUNC(CreateFBO);
	REGISTER_LUA_CFUNC(DeleteFBO);
	REGISTER_LUA_CFUNC(IsValidFBO);
	REGISTER_LUA_CFUNC(ActiveFBO);
	REGISTER_LUA_CFUNC(RawBindFBO);

	if (GLAD_GL_VERSION_3_0)
		REGISTER_LUA_CFUNC(ClearAttachmentFBO);

	if (GLAD_GL_EXT_framebuffer_blit)
		REGISTER_LUA_CFUNC(BlitFBO);

	return true;
}


bool LuaFBOs::CreateMetatable(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	luaL_newmetatable(L, "FBO");
	HSTR_PUSH_CFUNC(L, "__gc",        meta_gc);
	HSTR_PUSH_CFUNC(L, "__index",     meta_index);
	HSTR_PUSH_CFUNC(L, "__newindex",  meta_newindex);
	lua_pop(L, 1);
	return true;
}


/******************************************************************************/
/******************************************************************************/

inline void CheckDrawingEnabled(lua_State* L, const char* caller)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!LuaOpenGL::IsDrawingEnabled(L)) {
		luaL_error(L, "%s(): OpenGL calls can only be used in Draw() "
		              "call-ins, or while creating display lists", caller);
	}
}


/******************************************************************************/
/******************************************************************************/

static GLenum GetBindingEnum(GLenum target)
{
	RECOIL_DETAILED_TRACY_ZONE;
	switch (target) {
		case GL_FRAMEBUFFER_EXT:      { return GL_FRAMEBUFFER_BINDING_EXT;      }
		case GL_DRAW_FRAMEBUFFER_EXT: { return GL_DRAW_FRAMEBUFFER_BINDING_EXT; }
		case GL_READ_FRAMEBUFFER_EXT: { return GL_READ_FRAMEBUFFER_BINDING_EXT; }
		default: {}
	}

	return 0;
}

/***
 * @alias Attachment
 * | "depth"
 * | "stencil"
 * | "color0" 
 * | "color1" 
 * | "color2" 
 * | "color3" 
 * | "color4" 
 * | "color5" 
 * | "color6" 
 * | "color7" 
 * | "color8" 
 * | "color9" 
 * | "color10"
 * | "color11"
 * | "color12"
 * | "color13"
 * | "color14"
 * | "color15"
 */
 
static GLenum ParseAttachment(const std::string& name)
{
	RECOIL_DETAILED_TRACY_ZONE;
	switch (hashString(name.c_str())) {
		case hashString(  "depth"): { return GL_DEPTH_ATTACHMENT  ; } break;
		case hashString("stencil"): { return GL_STENCIL_ATTACHMENT; } break;
		case hashString("color0" ): { return GL_COLOR_ATTACHMENT0 ; } break;
		case hashString("color1" ): { return GL_COLOR_ATTACHMENT1 ; } break;
		case hashString("color2" ): { return GL_COLOR_ATTACHMENT2 ; } break;
		case hashString("color3" ): { return GL_COLOR_ATTACHMENT3 ; } break;
		case hashString("color4" ): { return GL_COLOR_ATTACHMENT4 ; } break;
		case hashString("color5" ): { return GL_COLOR_ATTACHMENT5 ; } break;
		case hashString("color6" ): { return GL_COLOR_ATTACHMENT6 ; } break;
		case hashString("color7" ): { return GL_COLOR_ATTACHMENT7 ; } break;
		case hashString("color8" ): { return GL_COLOR_ATTACHMENT8 ; } break;
		case hashString("color9" ): { return GL_COLOR_ATTACHMENT9 ; } break;
		case hashString("color10"): { return GL_COLOR_ATTACHMENT10; } break;
		case hashString("color11"): { return GL_COLOR_ATTACHMENT11; } break;
		case hashString("color12"): { return GL_COLOR_ATTACHMENT12; } break;
		case hashString("color13"): { return GL_COLOR_ATTACHMENT13; } break;
		case hashString("color14"): { return GL_COLOR_ATTACHMENT14; } break;
		case hashString("color15"): { return GL_COLOR_ATTACHMENT15; } break;
		default                   : {                               } break;
	}

	return 0;
}


/******************************************************************************/
/******************************************************************************/

const LuaFBOs::LuaFBO* LuaFBOs::GetLuaFBO(lua_State* L, int index)
{
	RECOIL_DETAILED_TRACY_ZONE;
	return static_cast<LuaFBO*>(LuaUtils::GetUserData(L, index, "FBO"));
}


/******************************************************************************/
/******************************************************************************/

void LuaFBOs::LuaFBO::Init(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	index  = -1u;
	id     = 0;
	target = GL_FRAMEBUFFER_EXT;
	luaRef = LUA_NOREF;
	xsize = 0;
	ysize = 0;
	zsize = 0;
}


void LuaFBOs::LuaFBO::Free(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (luaRef == LUA_NOREF)
		return;

	luaL_unref(L, LUA_REGISTRYINDEX, luaRef);
	luaRef = LUA_NOREF;

	glDeleteFramebuffersEXT(1, &id);
	id = 0;

	{
		// get rid of the userdatum
		LuaFBOs& activeFBOs = CLuaHandle::GetActiveFBOs(L);
		auto& fbos = activeFBOs.fbos;

		assert(index < fbos.size());
		assert(fbos[index] == this);

		fbos[index] = fbos.back();
		fbos[index]->index = index;
		fbos.pop_back();
	}
}


/******************************************************************************/
/******************************************************************************/

int LuaFBOs::meta_gc(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto* fbo = static_cast<LuaFBO*>(luaL_checkudata(L, 1, "FBO"));
	fbo->Free(L);
	return 0;
}


int LuaFBOs::meta_index(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const auto* fbo = static_cast<LuaFBO*>(luaL_checkudata(L, 1, "FBO"));

	if (fbo->luaRef == LUA_NOREF)
		return 0;

	// read the value from the ref table
	lua_rawgeti(L, LUA_REGISTRYINDEX, fbo->luaRef);
	lua_pushvalue(L, 2);
	lua_rawget(L, -2);
	return 1;
}


int LuaFBOs::meta_newindex(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto* fbo = static_cast<LuaFBO*>(luaL_checkudata(L, 1, "FBO"));

	if (fbo->luaRef == LUA_NOREF)
		return 0;

	if (lua_israwstring(L, 2)) {
		const std::string& key = lua_tostring(L, 2);
		const GLenum type = ParseAttachment(key);

		if (type != 0) {
			GLint currentFBO;
			glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &currentFBO);
			glBindFramebufferEXT(fbo->target, fbo->id);
			ApplyAttachment(L, 3, fbo, type);
			glBindFramebufferEXT(fbo->target, currentFBO);
		}
		else if (key == "drawbuffers") {
			GLint currentFBO;
			glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &currentFBO);
			glBindFramebufferEXT(fbo->target, fbo->id);
			ApplyDrawBuffers(L, 3);
			glBindFramebufferEXT(fbo->target, currentFBO);
		}
		else if (key == "readbuffer") {
			GLint currentFBO;
			glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &currentFBO);
			glBindFramebufferEXT(fbo->target, fbo->id);

			if (lua_isnumber(L, 3))
				glReadBuffer((GLenum)lua_toint(L, 3));

			glBindFramebufferEXT(fbo->target, currentFBO);
		}
		else if (key == "target") {
			return 0;// fbo->target = (GLenum)luaL_checkint(L, 3);
		}
	}

	// set the key/value in the ref table
	lua_rawgeti(L, LUA_REGISTRYINDEX, fbo->luaRef);
	lua_pushvalue(L, 2);
	lua_pushvalue(L, 3);
	lua_rawset(L, -3);
	return 0;
}



/******************************************************************************/
/******************************************************************************/

bool LuaFBOs::AttachObject(
	const char* funcName,
	lua_State* L,
	int index,
	LuaFBO* fbo,
	GLenum attachID,
	GLenum attachTarget,
	GLenum attachLevel
) {
	RECOIL_DETAILED_TRACY_ZONE;
	if (lua_isnil(L, index)) {
		// nil object
		glFramebufferTexture2DEXT(fbo->target, attachID, GL_TEXTURE_2D, 0, 0);
		glFramebufferRenderbufferEXT(fbo->target, attachID, GL_RENDERBUFFER_EXT, 0);
		return true;
	}
	if (lua_israwstring(L, index)) {
		// custom texture
		const LuaTextures& textures = CLuaHandle::GetActiveTextures(L);
		const LuaTextures::Texture* tex = textures.GetInfo(lua_tostring(L, index));

		if (tex == nullptr)
			return false;

		if (attachTarget == 0)
			attachTarget = tex->target;

		try {
			AttachObjectTexTarget(funcName, fbo->target, attachTarget, tex->id, attachID, attachLevel);
		}
		catch (const opengl_error& e) {
			luaL_error(L, "%s", e.what());
		}


		fbo->xsize = tex->xsize;
		fbo->ysize = tex->ysize;
		fbo->zsize = tex->zsize;
		return true;
	}

	// render buffer object
	const LuaRBOs::RBO* rbo = static_cast<LuaRBOs::RBO*>(LuaUtils::GetUserData(L, index, "RBO"));

	if (rbo == nullptr)
		return false;

	if (attachTarget == 0)
		attachTarget = rbo->target;

	glFramebufferRenderbufferEXT(fbo->target, attachID, attachTarget, rbo->id);

	fbo->xsize = rbo->xsize;
	fbo->ysize = rbo->ysize;
	fbo->zsize = 0; //RBO can't be 3D or CUBE_MAP
	return true;
}

void LuaFBOs::AttachObjectTexTarget(const char* funcName, GLenum fboTarget, GLenum texTarget, GLuint texId, GLenum attachID, GLenum attachLevel)
{
	RECOIL_DETAILED_TRACY_ZONE;
	//  glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, tex.target, texID, 0);
	switch (texTarget)
	{
	case GL_TEXTURE_1D:
		glFramebufferTexture1DEXT(fboTarget, attachID, texTarget, texId, attachLevel);
		break;
	case GL_TEXTURE_2D:
		glFramebufferTexture2DEXT(fboTarget, attachID, texTarget, texId, attachLevel);
		break;
	case GL_TEXTURE_2D_MULTISAMPLE:
		glFramebufferTexture2DEXT(fboTarget, attachID, texTarget, texId, 0);
		break;
	case GL_TEXTURE_2D_ARRAY: [[fallthrough]];
	case GL_TEXTURE_CUBE_MAP: [[fallthrough]];
	case GL_TEXTURE_3D: {
		if (!GLAD_GL_VERSION_3_2) {
			throw opengl_error(fmt::format("[LuaFBO::{}] Using of the attachment target {} requires OpenGL >= 3.2", funcName, texTarget));
		}

		glFramebufferTexture(fboTarget, attachID, texId, attachLevel); //attach the whole texture
	} break;
	default: {
		throw opengl_error(fmt::format("[LuaFBO::{}] Incorrect texture attach target {}", funcName, texTarget));
	} break;

	}
}


bool LuaFBOs::ApplyAttachment(
	lua_State* L,
	int index,
	LuaFBO* fbo,
	const GLenum attachID
) {
	RECOIL_DETAILED_TRACY_ZONE;
	if (attachID == 0)
		return false;

	if (!lua_istable(L, index))
		return AttachObject(__func__, L, index, fbo, attachID);

	const int table = (index < 0) ? index : (lua_gettop(L) + index + 1);

	GLenum target = 0;
	GLint  level  = 0;

	lua_rawgeti(L, table, 2);
	if (lua_isnumber(L, -1))
		target = (GLenum)lua_toint(L, -1);
	lua_pop(L, 1);

	lua_rawgeti(L, table, 3);
	if (lua_isnumber(L, -1))
		level = (GLint)lua_toint(L, -1);
	lua_pop(L, 1);

	lua_rawgeti(L, table, 1);
	const bool success = AttachObject(__func__, L, -1, fbo, attachID, target, level);
	lua_pop(L, 1);

	return success;
}


bool LuaFBOs::ApplyDrawBuffers(lua_State* L, int index)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (lua_isnumber(L, index)) {
		glDrawBuffer((GLenum)lua_toint(L, index));
		return true;
	}
	if (lua_istable(L, index) && GLAD_GL_ARB_draw_buffers) {
		int buffers[32] = {GL_NONE};
		const int count = LuaUtils::ParseIntArray(L, index, buffers, sizeof(buffers) / sizeof(buffers[0]));

		glDrawBuffersARB(count, reinterpret_cast<const GLenum*>(&buffers[0]));
		return true;
	}

	return false;
}


/******************************************************************************/
/******************************************************************************/

/***
 * User Data FBO
 * @class FBO
 */

/***
 * @class FBODescription
 * @field depth string?
 * @field stencil string?
 * @field color0 string?
 * @field color1 string?
 * @field color2 string?
 * @field color3 string?
 * @field color4 string?
 * @field color5 string?
 * @field color6 string?
 * @field color7 string?
 * @field color8 string?
 * @field color9 string?
 * @field color10 string?
 * @field color11 string?
 * @field color12 string?
 * @field color13 string?
 * @field color14 string?
 * @field color15 string?
 * @field drawbuffers (integer|GL)[]? e.g. `{ GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT3_EXT, ..}`
 * @field readbuffer (integer|GL)? e.g. `GL_COLOR_ATTACHMENT0_EXT`
 */

/***
 * @function gl.CreateFBO
 * @param fboDesc FBODescription
 * @return FBO fbo
 */
int LuaFBOs::CreateFBO(lua_State* L)
{
	LuaFBO fbo;
	fbo.Init(L);

	const int table = 1;
/*
	if (lua_istable(L, table)) {
		lua_getfield(L, table, "target");
		if (lua_isnumber(L, -1)) {
			fbo.target = (GLenum)lua_toint(L, -1);
		} else {
			lua_pop(L, 1);
		}
	}
*/
	const GLenum bindTarget = GetBindingEnum(fbo.target);
	if (bindTarget == 0)
		return 0;

	// maintain a lua table to hold RBO references
 	lua_newtable(L);
	fbo.luaRef = luaL_ref(L, LUA_REGISTRYINDEX);
	if (fbo.luaRef == LUA_NOREF)
		return 0;

	GLint currentFBO;
	glGetIntegerv(bindTarget, &currentFBO);

	glGenFramebuffersEXT(1, &fbo.id);
	glBindFramebufferEXT(fbo.target, fbo.id);


	auto* fboPtr = static_cast<LuaFBO*>(lua_newuserdata(L, sizeof(LuaFBO)));
	*fboPtr = fbo;

	luaL_getmetatable(L, "FBO");
	lua_setmetatable(L, -2);

	// parse the initialization table
	if (lua_istable(L, table)) {
		for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
			if (lua_israwstring(L, -2)) {
				const std::string& key = lua_tostring(L, -2);
				const GLenum type = ParseAttachment(key);
				if (type != 0) {
					ApplyAttachment(L, -1, fboPtr, type);
				}
				else if (key == "drawbuffers") {
					ApplyDrawBuffers(L, -1);
				}
			}
		}
	}

	// revert to the old fbo
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, currentFBO);

	if (fboPtr->luaRef != LUA_NOREF) {
		LuaFBOs& activeFBOs = CLuaHandle::GetActiveFBOs(L);
		auto& fbos = activeFBOs.fbos;

		fbos.push_back(fboPtr);
		fboPtr->index = fbos.size() - 1;
	}

	return 1;
}


/***
 * This doesn't delete the attached objects!
 * 
 * @function gl.DeleteFBO
 * @param fbo FBO
 */
int LuaFBOs::DeleteFBO(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (lua_isnil(L, 1))
		return 0;

	auto* fbo = static_cast<LuaFBO*>(luaL_checkudata(L, 1, "FBO"));
	fbo->Free(L);
	return 0;
}


/***
 * @function gl.IsValidFBO
 * @param fbo FBO
 * @param target GL?
 * @return boolean valid
 * @return number? status
 */
int LuaFBOs::IsValidFBO(lua_State* L)
{
	if (lua_isnil(L, 1) || !lua_isuserdata(L, 1)) {
		lua_pushboolean(L, false);
		return 1;
	}

	const auto* fbo = static_cast<LuaFBO*>(luaL_checkudata(L, 1, "FBO"));

	if ((fbo->id == 0) || (fbo->luaRef == LUA_NOREF)) {
		lua_pushboolean(L, false);
		return 1;
	}

	const GLenum target = (GLenum)luaL_optint(L, 2, fbo->target);
	const GLenum bindTarget = GetBindingEnum(target);

	if (bindTarget == 0) {
		lua_pushboolean(L, false);
		return 1;
	}

	GLint currentFBO;
	glGetIntegerv(bindTarget, &currentFBO);

	glBindFramebufferEXT(target, fbo->id);
	const GLenum status = glCheckFramebufferStatus(target);
	glBindFramebufferEXT(target, currentFBO);

	lua_pushboolean(L, (status == GL_FRAMEBUFFER_COMPLETE_EXT));
	lua_pushnumber(L, status);
	return 2;
}

/***
 * @function gl.ActiveFBO
 * @param fbo FBO
 * @param func fun(...)
 * @param ... any args
 */
/***
 * @function gl.ActiveFBO
 * @param fbo FBO
 * @param target GL?
 * @param func fun(...)
 * @param ... any args
 */
int LuaFBOs::ActiveFBO(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	CheckDrawingEnabled(L, __func__);
	
	const auto* fbo = static_cast<LuaFBO*>(luaL_checkudata(L, 1, "FBO"));

	if (fbo->id == 0)
		return 0;

	int funcIndex = 2;

	// target and matrix manipulation options
	GLenum target = fbo->target;
	if (lua_israwnumber(L, funcIndex)) {
		target = (GLenum)lua_toint(L, funcIndex);
		funcIndex++;
	}

	bool identities = false;

	if (lua_isboolean(L, funcIndex)) {
		identities = lua_toboolean(L, funcIndex);
		funcIndex++;
	}

	if (!lua_isfunction(L, funcIndex))
		luaL_error(L, "Incorrect arguments to gl.ActiveFBO()");

	const GLenum bindTarget = GetBindingEnum(target);

	if (bindTarget == 0)
		return 0;

	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0, 0, fbo->xsize, fbo->ysize);
	if (identities) {
		glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);  glPushMatrix(); glLoadIdentity();
	}

	GLint currentFBO = 0;
	glGetIntegerv(bindTarget, &currentFBO);
	glBindFramebufferEXT(target, fbo->id);

	const int error = lua_pcall(L, (lua_gettop(L) - funcIndex), 0, 0);

	glBindFramebufferEXT(target, currentFBO);
	if (identities) {
		glMatrixMode(GL_PROJECTION); glPopMatrix();
		glMatrixMode(GL_MODELVIEW);  glPopMatrix();
	}
	glPopAttrib();

	if (error != 0) {
		LOG_L(L_ERROR, "gl.ActiveFBO: error(%i) = %s", error, lua_tostring(L, -1));
		lua_error(L);
	}

	return 0;
}


/***
 * Bind default or specified via rawFboId numeric id of FBO
 * 
 * @function gl.RawBindFBO
 * @param fbo nil
 * @param target GL? (Default: `GL_FRAMEBUFFER_EXT`)
 * @param rawFboId integer? (Default: `0`)
 * @return nil
 */
/***
 * @function gl.RawBindFBO
 * @param fbo FBO
 * @param target GL? (Default: `fbo.target`)
 * @return number previouslyBoundRawFboId
 */
int LuaFBOs::RawBindFBO(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	//CheckDrawingEnabled(L, __func__);

	if (lua_isnil(L, 1)) {
		// revert to default or specified FB
		glBindFramebufferEXT((GLenum) luaL_optinteger(L, 2, GL_FRAMEBUFFER_EXT), luaL_optinteger(L, 3, 0));
		return 0;
	}
		
	const auto* fbo = static_cast<LuaFBO*>(luaL_checkudata(L, 1, "FBO"));

	if (fbo->id == 0)
		return 0;

	GLint currentFBO = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &currentFBO);
	glBindFramebufferEXT((GLenum) luaL_optinteger(L, 2, fbo->target), fbo->id);

	lua_pushnumber(L, currentFBO);
	return 1;
}


/******************************************************************************/

/*** needs `GLAD_GL_EXT_framebuffer_blit`
 *
 * @function gl.BlitFBO
 * @param x0Src number
 * @param y0Src number
 * @param x1Src number
 * @param y1Src number
 * @param x0Dst number
 * @param y0Dst number
 * @param x1Dst number
 * @param y1Dst number
 * @param mask number? (Default: GL_COLOR_BUFFER_BIT)
 * @param filter number? (Default: GL_NEAREST)
 */
/*** needs `GLAD_GL_EXT_framebuffer_blit`
 *
 * @function gl.BlitFBO
 * @param fboSrc FBO
 * @param x0Src number
 * @param y0Src number
 * @param x1Src number
 * @param y1Src number
 * @param fboDst FBO
 * @param x0Dst number
 * @param y0Dst number
 * @param x1Dst number
 * @param y1Dst number
 * @param mask number? (Default: GL_COLOR_BUFFER_BIT)
 * @param filter number? (Default: GL_NEAREST)
 */
int LuaFBOs::BlitFBO(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (lua_israwnumber(L, 1)) {
		const GLint x0Src = (GLint)luaL_checknumber(L, 1);
		const GLint y0Src = (GLint)luaL_checknumber(L, 2);
		const GLint x1Src = (GLint)luaL_checknumber(L, 3);
		const GLint y1Src = (GLint)luaL_checknumber(L, 4);

		const GLint x0Dst = (GLint)luaL_checknumber(L, 5);
		const GLint y0Dst = (GLint)luaL_checknumber(L, 6);
		const GLint x1Dst = (GLint)luaL_checknumber(L, 7);
		const GLint y1Dst = (GLint)luaL_checknumber(L, 8);

		const GLbitfield mask = (GLbitfield)luaL_optint(L, 9, GL_COLOR_BUFFER_BIT);
		const GLenum filter = (GLenum)luaL_optint(L, 10, GL_NEAREST);

		glBlitFramebufferEXT(x0Src, y0Src, x1Src, y1Src,  x0Dst, y0Dst, x1Dst, y1Dst,  mask, filter);
		return 0;
	}

	const auto* fboSrc = (lua_isnil(L, 1))? nullptr: static_cast<LuaFBO*>(luaL_checkudata(L, 1, "FBO"));
	const auto* fboDst = (lua_isnil(L, 6))? nullptr: static_cast<LuaFBO*>(luaL_checkudata(L, 6, "FBO"));

	// if passed a non-nil arg, userdatum buffer must always be valid
	// otherwise the default framebuffer is substituted as its target
	if (fboSrc != nullptr && fboSrc->id == 0)
		return 0;
	if (fboDst != nullptr && fboDst->id == 0)
		return 0;

	const GLint x0Src = (GLint)luaL_checknumber(L, 2);
	const GLint y0Src = (GLint)luaL_checknumber(L, 3);
	const GLint x1Src = (GLint)luaL_checknumber(L, 4);
	const GLint y1Src = (GLint)luaL_checknumber(L, 5);

	const GLint x0Dst = (GLint)luaL_checknumber(L, 7);
	const GLint y0Dst = (GLint)luaL_checknumber(L, 8);
	const GLint x1Dst = (GLint)luaL_checknumber(L, 9);
	const GLint y1Dst = (GLint)luaL_checknumber(L, 10);

	const GLbitfield mask = (GLbitfield)luaL_optint(L, 11, GL_COLOR_BUFFER_BIT);
	const GLenum filter = (GLenum)luaL_optint(L, 12, GL_NEAREST);

	GLint currentFBO;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &currentFBO);

	glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, (fboSrc == nullptr)? 0: fboSrc->id);
	glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, (fboDst == nullptr)? 0: fboDst->id);

	glBlitFramebufferEXT(x0Src, y0Src, x1Src, y1Src,  x0Dst, y0Dst, x1Dst, y1Dst,  mask, filter);

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, currentFBO);
	return 0;
}


namespace Impl {
	template<class Type, auto glClearBufferFuncPtr>
	static inline void ClearBuffer(lua_State* L, int startIdx, GLenum bufferType, GLint drawBuffer) {
		Type values[4];
		values[0] = spring::SafeCast<Type>(luaL_optnumber(L, startIdx + 0, 0));
		values[1] = spring::SafeCast<Type>(luaL_optnumber(L, startIdx + 1, 0));
		values[2] = spring::SafeCast<Type>(luaL_optnumber(L, startIdx + 2, 0));
		values[3] = spring::SafeCast<Type>(luaL_optnumber(L, startIdx + 3, 0));
		(*glClearBufferFuncPtr)(bufferType, drawBuffer, values);
	}
}

/*** needs `Platform.glVersionNum >= 30`
 * Clears the "attachment" of the currently bound FBO type "target" with "clearValues"
 * 
 * @function gl.ClearAttachmentFBO
 * @param target number? (Default: `GL.FRAMEBUFFER`)
 * @param attachment GL|Attachment (e.g. `"color0"` or `GL.COLOR_ATTACHMENT0`)
 * @param clearValue0 number? (Default: `0`)
 * @param clearValue1 number? (Default: `0`)
 * @param clearValue2 number? (Default: `0`)
 * @param clearValue3 number? (Default: `0`)
 * @return boolean success
 */

int LuaFBOs::ClearAttachmentFBO(lua_State* L)
{
	const auto ReportErrorAndReturn = [L](const char* errMsg = "", const char* func = __func__) {
		LOG_L(L_ERROR, "[gl.%s] Error: %s", func, errMsg);
		lua_pushboolean(L, false);
		return 1;
	};

#ifdef DEBUG
	glClearErrors("gl", __func__, globalRendering->glDebugErrors);
#endif

	int nextArg = 1;

	GLenum target = luaL_optint(L, nextArg++, GL_FRAMEBUFFER);
	GLenum queryType = 0;
	switch (target)
	{
		case GL_READ_FRAMEBUFFER:
			queryType = GL_READ_FRAMEBUFFER_BINDING;
			break;
		case GL_DRAW_FRAMEBUFFER: [[fallthrough]];
		case GL_FRAMEBUFFER:
			queryType = GL_DRAW_FRAMEBUFFER_BINDING;
			break;
		default:
			return ReportErrorAndReturn(fmt::format("invalid target type({}) Only GL.READ_FRAMEBUFFER|GL.DRAW_FRAMEBUFFER|GL.FRAMEBUFFER are accepted", target).c_str());
	}

	GLint fboID = 0;
	glGetIntegerv(queryType, &fboID);

	if (fboID == 0)
		return ReportErrorAndReturn(fmt::format("no non-default fbo object is bound for target({})", target).c_str());


	GLenum bufferType = 0;
	GLenum attachment = 0;
	GLenum drawBuffer = 0;

	if (lua_isstring(L, nextArg)) {
		const char* attachmentStr = luaL_checkstring(L, nextArg++);
		switch (hashString(attachmentStr)) {
			case hashString("color0"): { bufferType = GL_COLOR;   attachment = GL_COLOR_ATTACHMENT0;  drawBuffer = 0; } break;
			case hashString("color1"): { bufferType = GL_COLOR;   attachment = GL_COLOR_ATTACHMENT1;  drawBuffer = 1; } break;
			case hashString("color2"): { bufferType = GL_COLOR;   attachment = GL_COLOR_ATTACHMENT2;  drawBuffer = 2; } break;
			case hashString("color3"): { bufferType = GL_COLOR;   attachment = GL_COLOR_ATTACHMENT3;  drawBuffer = 3; } break;
			case hashString("color4"): { bufferType = GL_COLOR;   attachment = GL_COLOR_ATTACHMENT4;  drawBuffer = 4; } break;
			case hashString("color5"): { bufferType = GL_COLOR;   attachment = GL_COLOR_ATTACHMENT5;  drawBuffer = 5; } break;
			case hashString("color6"): { bufferType = GL_COLOR;   attachment = GL_COLOR_ATTACHMENT6;  drawBuffer = 6; } break;
			case hashString("color7"): { bufferType = GL_COLOR;   attachment = GL_COLOR_ATTACHMENT7;  drawBuffer = 7; } break;
			case hashString("color8"): { bufferType = GL_COLOR;   attachment = GL_COLOR_ATTACHMENT8;  drawBuffer = 8; } break;
			case hashString("color9"): { bufferType = GL_COLOR;   attachment = GL_COLOR_ATTACHMENT9;  drawBuffer = 9; } break;
			case hashString("color10"): { bufferType = GL_COLOR;   attachment = GL_COLOR_ATTACHMENT10; drawBuffer = 10; } break;
			case hashString("color11"): { bufferType = GL_COLOR;   attachment = GL_COLOR_ATTACHMENT11; drawBuffer = 11; } break;
			case hashString("color12"): { bufferType = GL_COLOR;   attachment = GL_COLOR_ATTACHMENT12; drawBuffer = 12; } break;
			case hashString("color13"): { bufferType = GL_COLOR;   attachment = GL_COLOR_ATTACHMENT13; drawBuffer = 13; } break;
			case hashString("color14"): { bufferType = GL_COLOR;   attachment = GL_COLOR_ATTACHMENT14; drawBuffer = 14; } break;
			case hashString("color15"): { bufferType = GL_COLOR;   attachment = GL_COLOR_ATTACHMENT15; drawBuffer = 15; } break;
			case hashString("depth"): { bufferType = GL_DEPTH;   attachment = GL_DEPTH_ATTACHMENT;   drawBuffer = 0; } break;
			case hashString("stencil"): { bufferType = GL_STENCIL; attachment = GL_STENCIL_ATTACHMENT; drawBuffer = 0; } break;
			default:
				return ReportErrorAndReturn(fmt::format("invalid attachment string ({})", attachmentStr).c_str());
		}
	}
	else if (lua_isnumber(L, nextArg)) {
		switch (attachment = luaL_checkint(L, nextArg++)) {
			case GL_COLOR_ATTACHMENT0: { bufferType = GL_COLOR; drawBuffer = 0; } break;
			case GL_COLOR_ATTACHMENT1: { bufferType = GL_COLOR; drawBuffer = 1; } break;
			case GL_COLOR_ATTACHMENT2: { bufferType = GL_COLOR; drawBuffer = 2; } break;
			case GL_COLOR_ATTACHMENT3: { bufferType = GL_COLOR; drawBuffer = 3; } break;
			case GL_COLOR_ATTACHMENT4: { bufferType = GL_COLOR; drawBuffer = 4; } break;
			case GL_COLOR_ATTACHMENT5: { bufferType = GL_COLOR; drawBuffer = 5; } break;
			case GL_COLOR_ATTACHMENT6: { bufferType = GL_COLOR; drawBuffer = 6; } break;
			case GL_COLOR_ATTACHMENT7: { bufferType = GL_COLOR; drawBuffer = 7; } break;
			case GL_COLOR_ATTACHMENT8: { bufferType = GL_COLOR; drawBuffer = 8; } break;
			case GL_COLOR_ATTACHMENT9: { bufferType = GL_COLOR; drawBuffer = 9; } break;
			case GL_COLOR_ATTACHMENT10: { bufferType = GL_COLOR; drawBuffer = 10; } break;
			case GL_COLOR_ATTACHMENT11: { bufferType = GL_COLOR; drawBuffer = 11; } break;
			case GL_COLOR_ATTACHMENT12: { bufferType = GL_COLOR; drawBuffer = 12; } break;
			case GL_COLOR_ATTACHMENT13: { bufferType = GL_COLOR; drawBuffer = 13; } break;
			case GL_COLOR_ATTACHMENT14: { bufferType = GL_COLOR; drawBuffer = 14; } break;
			case GL_COLOR_ATTACHMENT15: { bufferType = GL_COLOR; drawBuffer = 15; } break;
			case GL_DEPTH_ATTACHMENT: { bufferType = GL_DEPTH; drawBuffer = 0; } break;
			case GL_STENCIL_ATTACHMENT: { bufferType = GL_STENCIL; drawBuffer = 0; } break;
			default:
				return ReportErrorAndReturn(fmt::format("invalid attachment type ({})", attachment).c_str());
		}
	}

	GLint attachmentType = GL_NONE;
	glGetFramebufferAttachmentParameteriv(target, attachment, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &attachmentType);

	if (attachmentType == GL_NONE || attachmentType == GL_FRAMEBUFFER_DEFAULT)
		return ReportErrorAndReturn(fmt::format("invalid attachment object type ({})", attachmentType).c_str());

	GLint objectId = 0;
	glGetFramebufferAttachmentParameteriv(target, attachment, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &objectId);
	if (objectId == 0)
		return ReportErrorAndReturn(fmt::format("invalid attachment object id ({})", objectId).c_str());

	GLint componentType = 0;
	glGetFramebufferAttachmentParameteriv(target, attachment, GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE, &componentType);

	switch (componentType)
	{
		case GL_INT:
			Impl::ClearBuffer<GLint, &glClearBufferiv>(L, nextArg, bufferType, drawBuffer);
			break;
		case GL_UNSIGNED_INT:
			Impl::ClearBuffer<GLuint, &glClearBufferuiv>(L, nextArg, bufferType, drawBuffer);
			break;
		case GL_SIGNED_NORMALIZED: [[fallthrough]];   // is this considered a fixed point value?
		case GL_UNSIGNED_NORMALIZED: [[fallthrough]]; // is this considered a fixed point value?
		case GL_FLOAT:
			Impl::ClearBuffer<GLfloat, &glClearBufferfv>(L, nextArg, bufferType, drawBuffer);
			break;
		default:
			return ReportErrorAndReturn(fmt::format("invalid attachment component type ({}), means the attachment is invalid", componentType).c_str());
	}

	assert(glGetError() == GL_NO_ERROR);

	lua_pushboolean(L, true);
	return 1;
}


/******************************************************************************/
/******************************************************************************/
