/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_FBOS_H
#define LUA_FBOS_H

#include <vector>
#include <string>
#include <array>

#include "Rendering/GL/myGL.h"

struct lua_State;


class LuaFBOs {
public:
	class FBO;

	LuaFBOs() { fbos.reserve(8); }
	~LuaFBOs();

	void Clear() { fbos.clear(); }

	const FBO* GetLuaFBO(lua_State* L, int index);

	inline const FBO* GetActiveDrawFBO() const { return activeDrawFBO; }
	inline const FBO* GetActiveReadFBO() const { return activeReadFBO; }

public:
	static bool PushEntries(lua_State* L);
public:
	static void AttachObjectTexTarget(
		const char* funcName,
		GLenum fboTarget,
		GLenum texTarget,
		GLuint texId,
		GLenum attachID,
		GLenum attachLevel
	);
private:
	std::vector<FBO*> fbos;

	// Lua FBO only
	const FBO* activeDrawFBO = nullptr;
	const FBO* activeReadFBO = nullptr;

	static void SetActiveFBO(lua_State* L, GLenum target, const LuaFBOs::FBO* fbo);

	struct TempActiveFBO {
		TempActiveFBO(lua_State* L, GLenum target, const LuaFBOs::FBO* newFBO);
		~TempActiveFBO();
	private:
		LuaFBOs& fbos;
		const FBO* drawFBO;
		const FBO* readFBO;
	};

private: // helpers
	static bool CreateMetatable(lua_State* L);
	static bool AttachObject(
		const char* funcName,
		lua_State* L, int index,
		FBO* fbo, GLenum attachID,
		GLenum attachTarget = 0,
		GLenum attachLevel  = 0
	);
	static bool ApplyAttachment(lua_State* L, int index,
	                            FBO* fbo, GLenum attachID);
	static bool ApplyDrawBuffers(lua_State* L, int index);

private: // metatable methods
	static int meta_gc(lua_State* L);
	static int meta_index(lua_State* L);
	static int meta_newindex(lua_State* L);

private: // call-outs
	static int CreateFBO(lua_State* L);
	static int DeleteFBO(lua_State* L);
	static int IsValidFBO(lua_State* L);
	static int ActiveFBO(lua_State* L);
	static int RawBindFBO(lua_State* L); // unsafe
	static int BlitFBO(lua_State* L);
};

class LuaFBOs::FBO {
public:
	friend class LuaFBOs;

	GLuint index; // into LuaFBOs::fbos
	GLuint id;
	GLenum target;
	int luaRef;
	GLsizei xsize;
	GLsizei ysize;
	GLsizei zsize;
private:
	void Init(lua_State* L);
	void Free(lua_State* L);

	static constexpr size_t ColorAttachmentsCap = 16;
	std::array<GLenum, 2+ColorAttachmentsCap> attachmentFormats {0};
public:
	inline GLenum GetAttachmentFormat(GLenum attachment) const { return attachmentFormats[AttachmentArrayIndex(attachment)]; }
private:
	inline void SetAttachmentFormat(GLenum attachment, GLenum format) { attachmentFormats[AttachmentArrayIndex(attachment)] = format; }

	static inline size_t AttachmentArrayIndex(GLenum attachment) {
		assert(attachment == GL_STENCIL_ATTACHMENT
			|| attachment == GL_DEPTH_ATTACHMENT
			|| (attachment >= GL_COLOR_ATTACHMENT0 && attachment < GL_COLOR_ATTACHMENT0+ColorAttachmentsCap));
		return attachment == GL_STENCIL_ATTACHMENT? 0
			: attachment == GL_DEPTH_ATTACHMENT? 1
			: 2 +attachment-GL_COLOR_ATTACHMENT0;
	}
};


#endif /* LUA_FBOS_H */
