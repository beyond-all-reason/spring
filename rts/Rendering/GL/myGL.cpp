/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <array>
#include <vector>
#include <string>
#include <bit>

#include <SDL.h>
#if (!defined(HEADLESS) && !defined(_WIN32) && !defined(__APPLE__))
// need this for glXQueryCurrentRendererIntegerMESA (glxext)
#include <GL/glxew.h>
#endif

#include "myGL.h"
#include "VertexArray.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GlobalRenderingInfo.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/Textures/TextureFormat.h"
#include "Rendering/GL/VBO.h"
#include "Rendering/GL/TexBind.h"
#include "System/Log/ILog.h"
#include "System/Exceptions.h"
#include "System/StringUtil.h"
#include "System/SpringMath.h"
#include "System/Config/ConfigHandler.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Platform/MessageBox.h"
#include "fmt/printf.h"

#include "System/Misc/TracyDefs.h"

#define SDL_BPP(fmt) SDL_BITSPERPIXEL((fmt))

static std::array<CVertexArray, 2> vertexArrays;
static int currentVertexArray = 0;


/******************************************************************************/
/******************************************************************************/

CVertexArray* GetVertexArray()
{
	RECOIL_DETAILED_TRACY_ZONE;
	currentVertexArray = (currentVertexArray + 1) % vertexArrays.size();
	return &vertexArrays[currentVertexArray];
}


/******************************************************************************/

bool CheckAvailableVideoModes()
{
	RECOIL_DETAILED_TRACY_ZONE;
	// Get available fullscreen/hardware modes
	const int numDisplays = SDL_GetNumVideoDisplays();

	SDL_DisplayMode ddm = {0, 0, 0, 0, nullptr};
	SDL_DisplayMode cdm = {0, 0, 0, 0, nullptr};

	// ddm is virtual, contains all displays in multi-monitor setups
	// for fullscreen windows with non-native resolutions, ddm holds
	// the original screen mode and cdm is the changed mode
	SDL_GetDesktopDisplayMode(0, &ddm);
	SDL_GetCurrentDisplayMode(0, &cdm);

	globalRenderingInfo.availableVideoModes.clear();

	LOG(
		"[GL::%s] desktop={%ix%ix%ibpp@%iHz} current={%ix%ix%ibpp@%iHz}",
		__func__,
		ddm.w, ddm.h, SDL_BPP(ddm.format), ddm.refresh_rate,
		cdm.w, cdm.h, SDL_BPP(cdm.format), cdm.refresh_rate
	);

	for (int k = 0; k < numDisplays; ++k) {
		const int numModes = SDL_GetNumDisplayModes(k);

		if (numModes <= 0) {
			LOG("\tdisplay=%d bounds=N/A modes=N/A", k + 1);
			continue;
		}

		SDL_DisplayMode cm = {0, 0, 0, 0, nullptr};
		SDL_DisplayMode pm = {0, 0, 0, 0, nullptr};
		SDL_Rect db;
		SDL_GetDisplayBounds(k, &db);
		const std::string dn = SDL_GetDisplayName(k);

		LOG("\tDisplay (%s)=%d modes=%d bounds={x=%d, y=%d, w=%d, h=%d}", dn.c_str(), k + 1, numModes, db.x, db.y, db.w, db.h);

		for (int i = 0; i < numModes; ++i) {
			SDL_GetDisplayMode(k, i, &cm);

			// skip resolutions less than minimum / per dimension
			if (cm.w < globalRendering->minRes.x || cm.h < globalRendering->minRes.y)
				continue;

			// show only the largest refresh-rate and bit-depth per resolution
			if (cm.w == pm.w && cm.h == pm.h && (SDL_BPP(cm.format) < SDL_BPP(pm.format) || cm.refresh_rate < pm.refresh_rate))
				continue;

			globalRenderingInfo.availableVideoModes.emplace_back(GlobalRenderingInfo::AvailableVideoMode{
				dn,
				k + 1,
				cm.w,
				cm.h,
				static_cast<int32_t>(SDL_BPP(cm.format)),
				cm.refresh_rate
			});

			LOG("\t\t[%2i] %ix%ix%ibpp@%iHz", int(i + 1), cm.w, cm.h, SDL_BPP(cm.format), cm.refresh_rate);
			pm = cm;
		}
	}

	// we need at least 24bpp or window-creation will fail
	return (SDL_BPP(ddm.format) >= 24);
}



#ifndef HEADLESS
static bool GetVideoMemInfoNV(GLint* memInfo)
{
	RECOIL_DETAILED_TRACY_ZONE;
	#if (defined(GLEW_NVX_gpu_memory_info))
	if (!GLEW_NVX_gpu_memory_info)
		return false;

	glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &memInfo[0]);
	glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &memInfo[1]);
	return true;
	#else
	return false;
	#endif
}

static bool GetVideoMemInfoATI(GLint* memInfo)
{
	RECOIL_DETAILED_TRACY_ZONE;
	#if (defined(GLEW_ATI_meminfo))
	if (!GLEW_ATI_meminfo)
		return false;

	// these are not disjoint, don't sum
	for (uint32_t param: {/*GL_VBO_FREE_MEMORY_ATI,*/ GL_TEXTURE_FREE_MEMORY_ATI/*, GL_RENDERBUFFER_FREE_MEMORY_ATI*/}) {
		glGetIntegerv(param, &memInfo[0]);

		memInfo[4] += (memInfo[0] + memInfo[2]); // total main plus aux. memory free in pool
		memInfo[5] += (memInfo[1] + memInfo[3]); // largest main plus aux. free block in pool
	}

	memInfo[0] = memInfo[4]; // return the VBO/RBO/TEX free sum
	memInfo[1] = memInfo[4]; // sic, just assume total >= free
	return true;
	#else
	return false;
	#endif
}

static bool GetVideoMemInfoMESA(GLint* memInfo)
{
	RECOIL_DETAILED_TRACY_ZONE;
	#if (defined(GLX_MESA_query_renderer))
	if (!GLXEW_MESA_query_renderer)
		return false;

	typedef PFNGLXQUERYCURRENTRENDERERINTEGERMESAPROC QCRIProc;

	static const GLubyte* qcriProcName = (const GLubyte*) "glXQueryCurrentRendererIntegerMESA";
	static const QCRIProc qcriProcAddr = (QCRIProc) glXGetProcAddress(qcriProcName);

	if (qcriProcAddr == nullptr)
		return false;

	// note: unlike the others, this value is returned in megabytes
	qcriProcAddr(GLX_RENDERER_VIDEO_MEMORY_MESA, reinterpret_cast<unsigned int*>(&memInfo[0]));

	memInfo[0] *= 1024;
	memInfo[1] = memInfo[0];
	return true;
	#else
	return false;
	#endif
}
#endif

bool GetAvailableVideoRAM(GLint* memory, const char* glVendor)
{
	RECOIL_DETAILED_TRACY_ZONE;
	#ifdef HEADLESS
	return false;
	#else
	GLint memInfo[4 + 2] = {-1, -1, -1, -1, 0, 0};

	switch (glVendor[0]) {
		case 'N': { if (!GetVideoMemInfoNV  (memInfo)) return false; } break; // "NVIDIA"
		case 'A': { if (!GetVideoMemInfoATI (memInfo)) return false; } break; // "ATI" or "AMD"
		case 'X': { if (!GetVideoMemInfoMESA(memInfo)) return false; } break; // "X.org"
		case 'M': { if (!GetVideoMemInfoMESA(memInfo)) return false; } break; // "Mesa"
		case 'V': { if (!GetVideoMemInfoMESA(memInfo)) return false; } break; // "VMware" (also ships a Mesa variant)
		case 'I': {                                    return false; } break; // "Intel"
		case 'T': {                                    return false; } break; // "Tungsten" (old, acquired by VMware)
		default : {                                    return false; } break;
	}

	// callers assume [0]=total and [1]=free
	memory[0] = std::max(memInfo[0], memInfo[1]);
	memory[1] = std::min(memInfo[0], memInfo[1]);
	return true;
	#endif
}



bool ShowDriverWarning(const char* glVendor)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(glVendor != nullptr);

	const std::string& _glVendor = StringToLower(glVendor);

	// should be unreachable
	// note that checking for Microsoft stubs is no longer required
	// (context-creation will fail if no vendor-specific or pre-GL3
	// drivers are installed)
	if (_glVendor.find("unknown") != std::string::npos)
		return false;

	if (_glVendor.find("vmware") != std::string::npos) {
		const char* msg =
			"Running Spring with virtualized drivers can result in severely degraded "
			"performance and is discouraged. Prefer to use your host operating system.";

		LOG_L(L_WARNING, "%s", msg);
		Platform::MsgBox(msg, "Warning", MBF_EXCL);
		return true;
	}

	return true;
}


/******************************************************************************/

void WorkaroundATIPointSizeBug()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!globalRendering->amdHacks)
		return;

	GLboolean pointSpritesEnabled = false;
	glGetBooleanv(GL_POINT_SPRITE, &pointSpritesEnabled);
	if (pointSpritesEnabled)
		return;

	GLfloat atten[3] = {1.0f, 0.0f, 0.0f};
	glPointParameterfv(GL_POINT_DISTANCE_ATTENUATION, atten);
	glPointParameterf(GL_POINT_FADE_THRESHOLD_SIZE, 1.0f);
}

/******************************************************************************/

void RecoilGetTexParams(GLenum target, GLuint textureID, GLint level, TextureParameters& tp)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto texBind = GL::TexBind(target, textureID);

	glGetTexLevelParameteriv(target, level, GL_TEXTURE_INTERNAL_FORMAT, &tp.intFmt);
	glGetTexLevelParameteriv(target, level, GL_TEXTURE_WIDTH, &tp.sizeX);
	glGetTexLevelParameteriv(target, level, GL_TEXTURE_HEIGHT, &tp.sizeY);
	glGetTexLevelParameteriv(target, level, GL_TEXTURE_DEPTH, &tp.sizeZ);

	tp.isNormalizedDepth = false;
	tp.prefDataType = GL_UNSIGNED_BYTE;
	tp.bpp = 0;
	tp.chNum = 0;

	switch (tp.intFmt)
	{
	case GL_LUMINANCE32F_ARB: [[fallthrough]];
	case GL_INTENSITY32F_ARB: {
		tp.bpp = 32;
		tp.chNum = 1;
		tp.prefDataType = GL_FLOAT;
	} break;
	default: {
		GLint _cbits;
		glGetTexLevelParameteriv(target, level, GL_TEXTURE_RED_SIZE  , &_cbits); tp.bpp += _cbits; if (_cbits > 0) tp.chNum++;
		glGetTexLevelParameteriv(target, level, GL_TEXTURE_GREEN_SIZE, &_cbits); tp.bpp += _cbits; if (_cbits > 0) tp.chNum++;
		glGetTexLevelParameteriv(target, level, GL_TEXTURE_BLUE_SIZE , &_cbits); tp.bpp += _cbits; if (_cbits > 0) tp.chNum++;
		glGetTexLevelParameteriv(target, level, GL_TEXTURE_ALPHA_SIZE, &_cbits); tp.bpp += _cbits; if (_cbits > 0) tp.chNum++;
		glGetTexLevelParameteriv(target, level, GL_TEXTURE_DEPTH_SIZE, &_cbits); tp.bpp += _cbits; if (_cbits > 0) { tp.chNum++; tp.isNormalizedDepth = true; tp.prefDataType = GL_FLOAT; }

		if (tp.chNum > 0) {
			if (auto bytesPerChannel = tp.bpp / tp.chNum; bytesPerChannel == 4)
				tp.prefDataType = GL_UNSIGNED_INT;
			else if (bytesPerChannel == 2)
				tp.prefDataType = GL_UNSIGNED_SHORT;
		}
	} break;
	}

	{
		GLint isCompressed;
		glGetTexLevelParameteriv(target, level, GL_TEXTURE_COMPRESSED, &isCompressed);
		tp.isCompressed = isCompressed;
		if (isCompressed) {
			glGetTexLevelParameteriv(target, level, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &tp.imageSize);
		}
		else {
			tp.imageSize =
				std::max(tp.sizeX, 1) *
				std::max(tp.sizeY, 1) *
				std::max(tp.sizeZ, 1) *
				(tp.bpp >> 3);
		}
	}
}


void glSaveTexture(const GLuint textureID, const char* filename, int level)
{
	TextureParameters params;
	RecoilGetTexParams(GL_TEXTURE_2D, textureID, 0, params);

	CBitmap bmp;
	GLenum extFormat = params.isNormalizedDepth ? GL_DEPTH_COMPONENT : CBitmap::GetExtFmt(params.chNum);

	bmp.Alloc(params.sizeX, params.sizeY, params.chNum, params.prefDataType);

	{
		auto texBind = GL::TexBind(GL_TEXTURE_2D, textureID);
		glGetTexImage(GL_TEXTURE_2D, level, extFormat, params.prefDataType, bmp.GetRawMem());
	}

	if (params.isNormalizedDepth) {
		//doesn't work, TODO: fix
		bmp.SaveFloat(filename);
	}
	else {
		bmp.Save(filename, params.bpp < 32);
	}
}

void RecoilTexStorage2D(GLenum target, GLint levels, GLint internalFormat, GLsizei width, GLsizei height)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (levels <= 0)
		levels = std::bit_width(static_cast<uint32_t>(std::max({ width , height })));

	if (GLEW_ARB_texture_storage) {
		glTexStorage2D(target, levels, internalFormat, width, height);
	} else {
		auto format = GL::GetInternalFormatDataFormat(internalFormat);
		auto type   = GL::GetInternalFormatDataType(internalFormat);

		for (int level = 0; level < levels; ++level)
			glTexImage2D(target, level, internalFormat, std::max(width >> level, 1), std::max(height >> level, 1), 0, format, type, nullptr);
	}
	glTexParameteri(target, GL_TEXTURE_BASE_LEVEL,          0);
	glTexParameteri(target, GL_TEXTURE_MAX_LEVEL , levels - 1);
}

void RecoilTexStorage3D(GLenum target, GLint levels, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (levels <= 0)
		levels = std::bit_width(static_cast<uint32_t>(std::max({ width , height, depth })));

	if (GLEW_ARB_texture_storage) {
		glTexStorage3D(target, levels, internalFormat, width, height, depth);
	} else {
		auto format = GL::GetInternalFormatDataFormat(internalFormat);
		auto type   = GL::GetInternalFormatDataType(internalFormat);

		for (int level = 0; level < levels; ++level)
			glTexImage3D(target, level, internalFormat, std::max(width >> level, 1), std::max(height >> level, 1), std::max(depth >> level, 1), 0, format, type, nullptr);
	}
	glTexParameteri(target, GL_TEXTURE_BASE_LEVEL,          0);
	glTexParameteri(target, GL_TEXTURE_MAX_LEVEL , levels - 1);
}

void RecoilTexStorageCubeMap(GLint levels, GLint internalFormat, GLsizei width, GLsizei height)
{
	if (levels <= 0)
		levels = std::bit_width(static_cast<uint32_t>(std::max(width , height)));

	if (GLEW_ARB_texture_storage) {
		glTexStorage2D(GL_TEXTURE_CUBE_MAP, levels, internalFormat, width, height);
	}
	else {
		auto format = GL::GetInternalFormatDataFormat(internalFormat);
		auto type = GL::GetInternalFormatDataType(internalFormat);

		for (auto cubeFace = GL_TEXTURE_CUBE_MAP_POSITIVE_X; cubeFace <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z; ++cubeFace) {
			for (int level = 0; level < levels; ++level)
				glTexImage2D(cubeFace, level, internalFormat, std::max(width >> level, 1), std::max(height >> level, 1), 0, format, type, nullptr);
		}
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL,          0);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL , levels - 1);
}


void RecoilBuildMipmaps(const GLenum target, GLint internalFormat, const GLsizei width, const GLsizei height, const GLenum format, const GLenum type, const void* data, int32_t levels)
{
	RECOIL_DETAILED_TRACY_ZONE;

	if (globalRendering->compressTextures) {
		switch (internalFormat) {
			case 4:
			case GL_RGBA8 :
			case GL_RGBA :  internalFormat = GL_COMPRESSED_RGBA_ARB; break;

			case 3:
			case GL_RGB8 :
			case GL_RGB :   internalFormat = GL_COMPRESSED_RGB_ARB; break;

			case GL_LUMINANCE: internalFormat = GL_COMPRESSED_LUMINANCE_ARB; break;
		}
	}

	// the number of required levels was not specified, assume the request for
	// mipmapped texture, determine the number of levels
	if (levels <= 0)
		levels = std::bit_width(static_cast<uint32_t>(std::max(width , height)));

	// cannot use glTexStorage2D/RecoilTexStorage2D as they don't support GL_COMPRESSED textures
	glTexImage2D(target, 0, internalFormat, width, height, 0, format, type, data);
	for (int level = 1; level < levels; ++level)
		glTexImage2D(target, level, internalFormat, std::max(width >> level, 1), std::max(height >> level, 1), 0, format, type, nullptr);

	glTexParameteri(target, GL_TEXTURE_BASE_LEVEL,          0);
	glTexParameteri(target, GL_TEXTURE_MAX_LEVEL , levels - 1);

	if (globalRendering->amdHacks) {
		glEnable(target);
		glGenerateMipmap(target);
		glDisable(target);
	} else {
		glGenerateMipmap(target);
	}
}

bool glSpringBlitImages(
	GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ,
	GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ,
	GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth)
{
	RECOIL_DETAILED_TRACY_ZONE;
	TextureParameters srcTexParams;
	TextureParameters dstTexParams;
	RecoilGetTexParams(srcTarget, srcName, srcLevel, srcTexParams);
	RecoilGetTexParams(dstTarget, dstName, dstLevel, dstTexParams);
	const bool sameIntFormat = (srcTexParams.intFmt == dstTexParams.intFmt);
	const bool fineDims = (srcWidth <= dstTexParams.sizeX && srcHeight <= dstTexParams.sizeY);

	if (GLEW_ARB_copy_image && fineDims && sameIntFormat) {
		glCopyImageSubData(
			srcName, srcTarget, srcLevel, srcX, srcY, srcZ,
			dstName, dstTarget, dstLevel, dstX, dstY, dstZ,
			srcWidth, srcHeight, srcDepth
		);
		return true;
	}

	if (dstTexParams.isCompressed) //can't be rendered into
		return false;

	if (!GLEW_EXT_framebuffer_blit || !GLEW_EXT_texture_array)
		return false;

	bool result = true;

	GLint currDrawFBO;
	GLint currReadFBO;

	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING_EXT, &currDrawFBO);
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING_EXT, &currReadFBO);

	GLuint newDrawFBO;
	GLuint newReadFBO;
	glGenFramebuffersEXT(1, &newDrawFBO);
	glGenFramebuffersEXT(1, &newReadFBO);

	glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, newDrawFBO);
	glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, newReadFBO);

	const GLenum blitfilter = (srcWidth == dstTexParams.sizeX && srcHeight == dstTexParams.sizeY) ? GL_NEAREST : GL_LINEAR;
	for (int z = 0; result && z < srcDepth; z++) {
		// GL_READ_FRAMEBUFFER
		{
			switch (srcTarget)
			{
			case GL_TEXTURE_1D:
				glFramebufferTexture1DEXT(GL_READ_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0, srcTarget, srcName, srcLevel);
				break;
			case GL_TEXTURE_2D:
				glFramebufferTexture2DEXT(GL_READ_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0, srcTarget, srcName, srcLevel);
				break;
			case GL_TEXTURE_3D:
				glFramebufferTexture3DEXT(GL_READ_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0, srcTarget, srcName, srcLevel, srcZ + z);
				break;
			case GL_TEXTURE_1D_ARRAY: [[fallthrough]];
			case GL_TEXTURE_2D_ARRAY:
				glFramebufferTextureLayerEXT(GL_READ_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0, srcName, srcLevel, srcZ + z);
				break;
			default:
				result = false;
				assert(false);
				break;
			}
			glReadBuffer(GL_COLOR_ATTACHMENT0);
			const auto fbStatus = glCheckFramebufferStatusEXT(GL_READ_FRAMEBUFFER_EXT);
			result &= (fbStatus == GL_FRAMEBUFFER_COMPLETE_EXT);
		}

		// GL_DRAW_FRAMEBUFFER
		if (result)
		{
			switch (dstTarget)
			{
			case GL_TEXTURE_1D:
				glFramebufferTexture1DEXT(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, dstTarget, dstName, dstLevel);
				break;
			case GL_TEXTURE_2D:
				glFramebufferTexture2DEXT(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, dstTarget, dstName, dstLevel);
				break;
			case GL_TEXTURE_3D:
				glFramebufferTexture3DEXT(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, dstTarget, dstName, dstLevel, dstZ + z);
				break;
			case GL_TEXTURE_1D_ARRAY: [[fallthrough]];
			case GL_TEXTURE_2D_ARRAY:
				glFramebufferTextureLayerEXT(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, dstName, dstLevel, dstZ + z);
				break;

			default:
				result = false;
				assert(false);
				break;
			}
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			const auto fbStatus = glCheckFramebufferStatusEXT(GL_DRAW_FRAMEBUFFER);
			result &= (fbStatus == GL_FRAMEBUFFER_COMPLETE_EXT);
		}

		if (result) {
			glBlitFramebufferEXT(srcX, srcY, srcX + srcWidth, srcY + srcHeight, dstX, dstY, dstX + srcWidth, dstY + srcHeight, GL_COLOR_BUFFER_BIT, blitfilter);
		}
	}

	if (currDrawFBO)
		glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, currDrawFBO);
	if (currReadFBO)
		glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, currReadFBO);

	glDeleteFramebuffersEXT(1, &newDrawFBO);
	glDeleteFramebuffersEXT(1, &newReadFBO);

	return result;
}

/******************************************************************************/

void ClearScreen()
{
	RECOIL_DETAILED_TRACY_ZONE;
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, 1, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);
	glColor3f(1, 1, 1);
}


/******************************************************************************/

static unsigned int LoadProgram(GLenum, const char*, const char*);

/**
 * True if the program in DATADIR/shaders/filename is
 * loadable and can run inside our graphics server.
 *
 * @param target glProgramStringARB target: GL_FRAGMENT_PROGRAM_ARB GL_VERTEX_PROGRAM_ARB
 * @param filename Name of the file under shaders with the program in it.
 */

bool ProgramStringIsNative(GLenum target, const char* filename)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// clear any current GL errors so that the following check is valid
	glClearErrors("GL", __func__, globalRendering->glDebugErrors);

	const GLuint tempProg = LoadProgram(target, filename, (target == GL_VERTEX_PROGRAM_ARB? "vertex": "fragment"));

	if (tempProg == 0)
		return false;

	glSafeDeleteProgram(tempProg);
	return true;
}


/**
 * Presumes the last GL operation was to load a vertex or
 * fragment program.
 *
 * If it was invalid, display an error message about
 * what and where the problem in the program source is.
 *
 * @param filename Only substituted in the message.
 * @param program The program text (used to enhance the message)
 */
static bool CheckParseErrors(GLenum target, const char* filename, const char* program)
{
	RECOIL_DETAILED_TRACY_ZONE;
	GLint errorPos = -1;
	GLint isNative =  0;

	glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorPos);
	glGetProgramivARB(target, GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB, &isNative);

	if (errorPos != -1) {
		const char* fmtString =
			"[%s] shader compilation error at index %d (near "
			"\"%.30s\") when loading %s-program file %s:\n%s";
		const char* tgtString = (target == GL_VERTEX_PROGRAM_ARB)? "vertex": "fragment";
		const char* errString = (const char*) glGetString(GL_PROGRAM_ERROR_STRING_ARB);

		if (errString != NULL) {
			LOG_L(L_ERROR, fmtString, __func__, errorPos, program + errorPos, tgtString, filename, errString);
		} else {
			LOG_L(L_ERROR, fmtString, __func__, errorPos, program + errorPos, tgtString, filename, "(null)");
		}

		return true;
	}

	if (isNative != 1) {
		GLint aluInstrs, maxAluInstrs;
		GLint texInstrs, maxTexInstrs;
		GLint texIndirs, maxTexIndirs;
		GLint nativeTexIndirs, maxNativeTexIndirs;
		GLint nativeAluInstrs, maxNativeAluInstrs;

		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_ALU_INSTRUCTIONS_ARB,            &aluInstrs);
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_ALU_INSTRUCTIONS_ARB,        &maxAluInstrs);
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_TEX_INSTRUCTIONS_ARB,            &texInstrs);
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_TEX_INSTRUCTIONS_ARB,        &maxTexInstrs);
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_TEX_INDIRECTIONS_ARB,            &texIndirs);
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_TEX_INDIRECTIONS_ARB,        &maxTexIndirs);
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB,     &nativeTexIndirs);
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB, &maxNativeTexIndirs);
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB,     &nativeAluInstrs);
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB, &maxNativeAluInstrs);

		if (aluInstrs > maxAluInstrs)
			LOG_L(L_ERROR, "[%s] too many ALU instructions in %s (%d, max. %d)\n", __FUNCTION__, filename, aluInstrs, maxAluInstrs);

		if (texInstrs > maxTexInstrs)
			LOG_L(L_ERROR, "[%s] too many texture instructions in %s (%d, max. %d)\n", __FUNCTION__, filename, texInstrs, maxTexInstrs);

		if (texIndirs > maxTexIndirs)
			LOG_L(L_ERROR, "[%s] too many texture indirections in %s (%d, max. %d)\n", __FUNCTION__, filename, texIndirs, maxTexIndirs);

		if (nativeTexIndirs > maxNativeTexIndirs)
			LOG_L(L_ERROR, "[%s] too many native texture indirections in %s (%d, max. %d)\n", __FUNCTION__, filename, nativeTexIndirs, maxNativeTexIndirs);

		if (nativeAluInstrs > maxNativeAluInstrs)
			LOG_L(L_ERROR, "[%s] too many native ALU instructions in %s (%d, max. %d)\n", __FUNCTION__, filename, nativeAluInstrs, maxNativeAluInstrs);

		return true;
	}

	return false;
}


static unsigned int LoadProgram(GLenum target, const char* filename, const char* program_type)
{
	RECOIL_DETAILED_TRACY_ZONE;
	GLuint ret = 0;

	if (!GLEW_ARB_vertex_program)
		return ret;
	if (target == GL_FRAGMENT_PROGRAM_ARB && !GLEW_ARB_fragment_program)
		return ret;

	CFileHandler file(std::string("shaders/") + filename);
	if (!file.FileExists()) {
		std::string c = fmt::sprintf("[myGL::LoadProgram] Cannot find %s-program file '%s'", program_type, filename);
		throw content_error(c);
	}

	std::vector<unsigned char> fbuf;

	if (!file.IsBuffered()) {
		fbuf.resize(file.FileSize(), 0);
		file.Read(fbuf.data(), fbuf.size());
	} else {
		fbuf = std::move(file.GetBuffer());
	}

	if (fbuf.back() != '\0')
		fbuf.emplace_back('\0'); //vmware driver can't deal with non-null terminated strings

	glGenProgramsARB(1, &ret);
	glBindProgramARB(target, ret);
	glProgramStringARB(target, GL_PROGRAM_FORMAT_ASCII_ARB, fbuf.size() - 1, fbuf.data()); //NV driver refuses to deal with null-terminated endings

	if (CheckParseErrors(target, filename, reinterpret_cast<char*>(fbuf.data())))
		ret = 0;

	return ret;
}

unsigned int LoadVertexProgram(const char* filename)
{
	RECOIL_DETAILED_TRACY_ZONE;
	return LoadProgram(GL_VERTEX_PROGRAM_ARB, filename, "vertex");
}

unsigned int LoadFragmentProgram(const char* filename)
{
	RECOIL_DETAILED_TRACY_ZONE;

	return LoadProgram(GL_FRAGMENT_PROGRAM_ARB, filename, "fragment");
}


void glSafeDeleteProgram(GLuint program)
{
	if (!GLEW_ARB_vertex_program || (program == 0))
		return;

	glDeleteProgramsARB(1, &program);
}


/******************************************************************************/

void glClearErrors(const char* cls, const char* fnc, bool verbose)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (verbose) {
		for (int count = 0, error = 0; ((error = glGetError()) != GL_NO_ERROR) && (count < 10000); count++) {
			LOG_L(L_ERROR, "[GL::%s][%s::%s][frame=%u] count=%04d error=0x%x", __func__, cls, fnc, globalRendering->drawFrame, count, error);
		}
	} else {
		for (int count = 0; (glGetError() != GL_NO_ERROR) && (count < 10000); count++);
	}
}