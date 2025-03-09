/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <string>
#include <sstream>
#include <iomanip>

#include <SDL.h>

#include "GlobalRendering.h"
#include "GlobalRenderingInfo.h"
#include "Rendering/VerticalSync.h"
#include "Rendering/GL/StreamBuffer.h"
#include "Rendering/GL/RenderBuffers.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/GL/glExtra.h"
#include "Rendering/GL/glxHandler.h"
#include "Rendering/UniformConstants.h"
#include "Rendering/Fonts/glFont.h"
#include "System/EventHandler.h"
#include "System/type2.h"
#include "System/TimeProfiler.h"
#include "System/SafeUtil.h"
#include "System/StringUtil.h"
#include "System/StringHash.h"
#include "System/Matrix44f.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/Platform/CrashHandler.h"
#include "System/Platform/MessageBox.h"
#include "System/Platform/Threading.h"
#include "System/Platform/WindowManagerHelper.h"
#include "System/Platform/errorhandler.h"
#include "System/ScopedResource.h"
#include "System/QueueToMain.h"
#include "System/creg/creg_cond.h"
#include "Game/Game.h"

#include <SDL_syswm.h>
#include <SDL_rect.h>

#include "System/Misc/TracyDefs.h"

CONFIG(bool, DebugGL).defaultValue(false).description("Enables GL debug-context and output. (see GL_ARB_debug_output)");
CONFIG(bool, DebugGLStacktraces).defaultValue(false).description("Create a stacktrace when an OpenGL error occurs");

CONFIG(int, GLContextMajorVersion).defaultValue(3).minimumValue(3).maximumValue(4);
CONFIG(int, GLContextMinorVersion).defaultValue(0).minimumValue(0).maximumValue(5);
CONFIG(int, MSAALevel).defaultValue(0).minimumValue(0).maximumValue(32).description("Enables multisample anti-aliasing; 'level' is the number of samples used.");
CONFIG(float, MinSampleShadingRate).defaultValue(0.0f).minimumValue(0.0f).maximumValue(1.0f).description("A value of 1.0 indicates that each sample in the framebuffer should be independently shaded. A value of 0.0 effectively allows the GL to ignore sample rate shading. Any value between 0.0 and 1.0 allows the GL to shade only a subset of the total samples within each covered fragment.");

CONFIG(int, ForceDisablePersistentMapping).defaultValue(0).minimumValue(0).maximumValue(1);
CONFIG(int, ForceDisableExplicitAttribLocs).defaultValue(0).minimumValue(0).maximumValue(1);
CONFIG(int, ForceDisableClipCtrl).defaultValue(0).minimumValue(0).maximumValue(1);
//CONFIG(int, ForceDisableShaders).defaultValue(0).minimumValue(0).maximumValue(1);
CONFIG(int, ForceDisableGL4).defaultValue(0).safemodeValue(1).minimumValue(0).maximumValue(1);

CONFIG(int, ForceCoreContext).defaultValue(0).minimumValue(0).maximumValue(1);
CONFIG(int, ForceSwapBuffers).defaultValue(1).minimumValue(0).maximumValue(1);
CONFIG(int, AtiHacks).defaultValue(-1).headlessValue(0).minimumValue(-1).maximumValue(1).description("Enables graphics drivers workarounds for users with AMD proprietary drivers.\n -1:=runtime detect, 0:=off, 1:=on");

// enabled in safemode, far more likely the gpu runs out of memory than this extension causes crashes!
CONFIG(bool, CompressTextures).defaultValue(false).safemodeValue(true).description("Runtime compress most textures to save VideoRAM.");
CONFIG(bool, DualScreenMode).defaultValue(false).description("Sets whether to split the screen in half, with one half for minimap and one for main screen. Right side is for minimap unless DualScreenMiniMapOnLeft is set.");
CONFIG(bool, DualScreenMiniMapOnLeft).defaultValue(false).description("When set, will make the left half of the screen the minimap when DualScreenMode is set.");
CONFIG(bool, TeamNanoSpray).defaultValue(true).headlessValue(false);

CONFIG(int, MinimizeOnFocusLoss).defaultValue(0).minimumValue(0).maximumValue(1).description("When set to 1 minimize Window if it loses key focus when in fullscreen mode.");

CONFIG(bool, Fullscreen).defaultValue(true).headlessValue(false).description("Sets whether the game will run in fullscreen, as opposed to a window. For Windowed Fullscreen of Borderless Window, set this to 0, WindowBorderless to 1, and WindowPosX and WindowPosY to 0.");
CONFIG(bool, WindowBorderless).defaultValue(false).description("When set and Fullscreen is 0, will put the game in Borderless Window mode, also known as Windowed Fullscreen. When using this, it is generally best to also set WindowPosX and WindowPosY to 0");
CONFIG(bool, BlockCompositing).defaultValue(false).safemodeValue(true).description("Disables kwin compositing to fix tearing, possible fixes low FPS in windowed mode, too.");

CONFIG(int, XResolution).defaultValue(0).headlessValue(8).minimumValue(0).description("Sets the width of the game screen. If set to 0 Spring will autodetect the current resolution of your desktop.");
CONFIG(int, YResolution).defaultValue(0).headlessValue(8).minimumValue(0).description("Sets the height of the game screen. If set to 0 Spring will autodetect the current resolution of your desktop.");
CONFIG(int, XResolutionWindowed).defaultValue(0).headlessValue(8).minimumValue(0).description("See XResolution, just for windowed.");
CONFIG(int, YResolutionWindowed).defaultValue(0).headlessValue(8).minimumValue(0).description("See YResolution, just for windowed.");
CONFIG(int, WindowPosX).defaultValue(0 ).description("Sets the horizontal position of the game window, if Fullscreen is 0. When WindowBorderless is set, this should usually be 0.");
CONFIG(int, WindowPosY).defaultValue(32).description("Sets the vertical position of the game window, if Fullscreen is 0. When WindowBorderless is set, this should usually be 0.");

//deprecated stuff
CONFIG(int, RendererHash).deprecated(true);
CONFIG(bool, FSAA).deprecated(true);
CONFIG(int, FSAALevel).deprecated(true);
CONFIG(bool, ForceDisableShaders).deprecated(true);


#define WINDOWS_NO_INVISIBLE_GRIPS 1

/**
 * @brief global rendering
 *
 * Global instance of CGlobalRendering
 */
alignas(CGlobalRendering) static std::byte globalRenderingMem[sizeof(CGlobalRendering)];

CGlobalRendering* globalRendering = nullptr;
GlobalRenderingInfo globalRenderingInfo;


CR_BIND(CGlobalRendering, )

CR_REG_METADATA(CGlobalRendering, (
	CR_MEMBER(teamNanospray),
	CR_MEMBER(drawSky),
	CR_MEMBER(drawWater),
	CR_MEMBER(drawGround),
	CR_MEMBER(drawMapMarks),
	CR_MEMBER(drawFog),

	CR_MEMBER(drawDebug),
	CR_MEMBER(drawDebugTraceRay),
	CR_MEMBER(drawDebugCubeMap),

	CR_MEMBER(glDebug),
	CR_MEMBER(glDebugErrors),

	CR_MEMBER(timeOffset),
	CR_MEMBER(lastTimeOffset),
	CR_MEMBER(lastFrameTime),
	CR_MEMBER(lastFrameStart),
	CR_MEMBER(lastSwapBuffersEnd),
	CR_MEMBER(weightedSpeedFactor),
	CR_MEMBER(drawFrame),
	CR_MEMBER(FPS),

	CR_IGNORED(numDisplays),

	CR_IGNORED(screenSizeX),
	CR_IGNORED(screenSizeY),
	CR_IGNORED(screenPosX),
	CR_IGNORED(screenPosY),

	CR_IGNORED(winPosX),
	CR_IGNORED(winPosY),
	CR_IGNORED(winSizeX),
	CR_IGNORED(winSizeY),
	CR_IGNORED(viewPosX),
	CR_IGNORED(viewPosY),
	CR_IGNORED(viewSizeX),
	CR_IGNORED(viewSizeY),
	CR_IGNORED(viewWindowOffsetY),
	CR_IGNORED(dualViewPosX),
	CR_IGNORED(dualViewPosY),
	CR_IGNORED(dualViewSizeX),
	CR_IGNORED(dualViewSizeY),
	CR_IGNORED(dualWindowOffsetY),
	CR_IGNORED(winBorder),
	CR_IGNORED(winChgFrame),
	CR_IGNORED(gmeChgFrame),
	CR_IGNORED(screenViewMatrix),
	CR_IGNORED(screenProjMatrix),
	CR_MEMBER(grTime),
	CR_IGNORED(pixelX),
	CR_IGNORED(pixelY),

	CR_IGNORED(minViewRange),
	CR_IGNORED(maxViewRange),
	CR_IGNORED(aspectRatio),

	CR_IGNORED(forceDisablePersistentMapping),
	CR_IGNORED(forceDisableGL4),
	CR_IGNORED(forceCoreContext),
	CR_IGNORED(forceSwapBuffers),

	CR_IGNORED(msaaLevel),
	CR_IGNORED(minSampleShadingRate),
	CR_IGNORED(maxTextureSize),
	CR_IGNORED(maxTexSlots),
	CR_IGNORED(maxFragShSlots),
	CR_IGNORED(maxCombShSlots),
	CR_IGNORED(maxTexAnisoLvl),

	CR_IGNORED(active),
	CR_IGNORED(compressTextures),

	CR_IGNORED(haveAMD),
	CR_IGNORED(haveMesa),
	CR_IGNORED(haveIntel),
	CR_IGNORED(haveNvidia),

	CR_IGNORED(amdHacks),
	CR_IGNORED(supportPersistentMapping),
	CR_IGNORED(supportExplicitAttribLoc),
	CR_IGNORED(supportTextureQueryLOD),
	CR_IGNORED(supportMSAAFrameBuffer),
	CR_IGNORED(supportDepthBufferBitDepth),
	CR_IGNORED(supportRestartPrimitive),
	CR_IGNORED(supportClipSpaceControl),
	CR_IGNORED(supportSeamlessCubeMaps),
	CR_IGNORED(supportFragDepthLayout),
	CR_IGNORED(haveGL4),
	CR_IGNORED(glslMaxVaryings),
	CR_IGNORED(glslMaxAttributes),
	CR_IGNORED(glslMaxDrawBuffers),
	CR_IGNORED(glslMaxRecommendedIndices),
	CR_IGNORED(glslMaxRecommendedVertices),
	CR_IGNORED(glslMaxUniformBufferBindings),
	CR_IGNORED(glslMaxUniformBufferSize),
	CR_IGNORED(glslMaxStorageBufferBindings),
	CR_IGNORED(glslMaxStorageBufferSize),
	CR_IGNORED(dualScreenMode),
	CR_IGNORED(dualScreenMiniMapOnLeft),

	CR_IGNORED(fullScreen),
	CR_IGNORED(borderless),

	CR_IGNORED(underExternalDebug),

	CR_IGNORED(sdlWindow),
	CR_IGNORED(glContext),

	CR_IGNORED(glExtensions),
	CR_IGNORED(glTimerQueries)
))


void CGlobalRendering::InitStatic() { globalRendering = new (globalRenderingMem) CGlobalRendering(); }
void CGlobalRendering::KillStatic() { globalRendering->PreKill();  spring::SafeDestruct(globalRendering); }

CGlobalRendering::CGlobalRendering()
	: timeOffset(0.0f)
	, lastTimeOffset(0.0f)
	, lastFrameTime(0.0f)
	, lastFrameStart(spring_notime)
	, lastSwapBuffersEnd(spring_notime)
	, weightedSpeedFactor(0.0f)
	, drawFrame(1)
	, FPS(1.0f)

	, numDisplays(1)

	, screenSizeX(1)
	, screenSizeY(1)

	// window geometry
	, winPosX(configHandler->GetInt("WindowPosX"))
	, winPosY(configHandler->GetInt("WindowPosY"))
	, winSizeX(1)
	, winSizeY(1)

	// viewport geometry
	, viewPosX(0)
	, viewPosY(0)
	, viewSizeX(1)
	, viewSizeY(1)
	, viewWindowOffsetY(0)

	// dual viewport geometry (DualScreenMode = 1)
	, dualWindowOffsetY(0)
	, dualViewPosX(0)
	, dualViewPosY(0)
	, dualViewSizeX(0)
	, dualViewSizeY(0)


	, winBorder{ 0 }

	, winChgFrame(0)
	, gmeChgFrame(0)

	, screenViewMatrix()
	, screenProjMatrix()

	, grTime()

	// pixel geometry
	, pixelX(0.01f)
	, pixelY(0.01f)

	// sane defaults
	, minViewRange(MIN_ZNEAR_DIST * 8.0f)
	, maxViewRange(MAX_VIEW_RANGE * 0.5f)
	, aspectRatio(1.0f)

	, forceDisableGL4(configHandler->GetInt("ForceDisableGL4"))
	, forceCoreContext(configHandler->GetInt("ForceCoreContext"))
	, forceSwapBuffers(configHandler->GetInt("ForceSwapBuffers"))

	// fallback
	, msaaLevel(configHandler->GetInt("MSAALevel"))
	, minSampleShadingRate(configHandler->GetFloat("MinSampleShadingRate"))
	, maxTextureSize(2048)
	, maxTexSlots(2)
	, maxFragShSlots(8)
	, maxCombShSlots(8)
	, maxTexAnisoLvl(0.0f)

	, drawSky(true)
	, drawWater(true)
	, drawGround(true)
	, drawMapMarks(true)
	, drawFog(true)

	, drawDebug(false)
	, drawDebugTraceRay(false)
	, drawDebugCubeMap(false)

	, glDebug(configHandler->GetBool("DebugGL"))
	, glDebugErrors(false)

	, teamNanospray(configHandler->GetBool("TeamNanoSpray"))
	, active(true)
	, compressTextures(false)

	, haveAMD(false)
	, haveMesa(false)
	, haveIntel(false)
	, haveNvidia(false)
	, amdHacks(false)

	, supportPersistentMapping(false)
	, supportExplicitAttribLoc(false)
	, supportTextureQueryLOD(false)
	, supportMSAAFrameBuffer(false)
	, supportDepthBufferBitDepth(16)
	, supportRestartPrimitive(false)
	, supportClipSpaceControl(false)
	, supportSeamlessCubeMaps(false)
	, supportFragDepthLayout(false)
	, haveGL4(false)

	, glslMaxVaryings(0)
	, glslMaxAttributes(0)
	, glslMaxDrawBuffers(0)
	, glslMaxRecommendedIndices(0)
	, glslMaxRecommendedVertices(0)
	, glslMaxUniformBufferBindings(0)
	, glslMaxUniformBufferSize(0)
	, glslMaxStorageBufferBindings(0)
	, glslMaxStorageBufferSize(0)

	, dualScreenMode(false)
	, dualScreenMiniMapOnLeft(false)
	, fullScreen(configHandler->GetBool("Fullscreen"))
	, borderless(configHandler->GetBool("WindowBorderless"))
	, underExternalDebug(false)
	, sdlWindow{nullptr}
	, glContext{nullptr}
	, glExtensions{}
	, glTimerQueries{0}
{
	verticalSync->WrapNotifyOnChange();
	configHandler->NotifyOnChange(this, {
		"DualScreenMode",
		"DualScreenMiniMapOnLeft",
		"Fullscreen",
		"WindowBorderless",
		"XResolution",
		"YResolution",
		"XResolutionWindowed",
		"YResolutionWindowed",
		"WindowPosX",
		"WindowPosY",
		"MinSampleShadingRate"
	});
	SetDualScreenParams();
}

CGlobalRendering::~CGlobalRendering()
{
	configHandler->RemoveObserver(this);
	verticalSync->WrapRemoveObserver();

	// protect against aborted startup
	if (glContext) {
		glDeleteQueries(glTimerQueries.size(), glTimerQueries.data());
	}

	DestroyWindowAndContext();
	KillSDL();
}

void CGlobalRendering::PreKill()
{
	UniformConstants::GetInstance().Kill(); //unsafe to kill in ~CGlobalRendering()
	RenderBuffer::KillStatic();
	GL::shapes.Kill();
	CShaderHandler::FreeInstance();
}


SDL_Window* CGlobalRendering::CreateSDLWindow(const char* title) const
{
	SDL_Window* newWindow = nullptr;

	const std::array aaLvls = {msaaLevel, msaaLevel / 2, msaaLevel / 4, msaaLevel / 8, msaaLevel / 16, msaaLevel / 32, 0};
	const std::array zbBits = {24, 32, 16};

	const char* wpfName = "";

	const char* frmts[2] = {
		"[GR::%s] error \"%s\" using %dx anti-aliasing and %d-bit depth-buffer for main window",
		"[GR::%s] using %dx anti-aliasing and %d-bit depth-buffer (PF=\"%s\") for main window",
	};

	bool borderless_ = configHandler->GetBool("WindowBorderless");
	bool fullScreen_ = configHandler->GetBool("Fullscreen");
	int winPosX_ = configHandler->GetInt("WindowPosX");
	int winPosY_ = configHandler->GetInt("WindowPosY");
	int2 newRes = GetCfgWinRes();

	// note:
	//   passing the minimized-flag is useless (state is not saved if minimized)
	//   and has no effect anyway, setting a minimum size for a window overrides
	//   it while disabling the SetWindowMinimumSize call still results in a 1x1
	//   window on the desktop
	//
	//   SDL_WINDOW_FULLSCREEN, for "real" fullscreen with a videomode change;
	//   SDL_WINDOW_FULLSCREEN_DESKTOP for "fake" fullscreen that takes the size of the desktop;
	//   and 0 for windowed mode.

	uint32_t sdlFlags  = (SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	         sdlFlags |= (borderless_ ? SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_WINDOW_FULLSCREEN) * fullScreen_;
	         sdlFlags |= (SDL_WINDOW_BORDERLESS * borderless_);

	for (size_t i = 0; i < (aaLvls.size()) && (newWindow == nullptr); i++) {
		if (i > 0 && aaLvls[i] == aaLvls[i - 1])
			break;

		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, aaLvls[i] > 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, aaLvls[i]    );

		for (size_t j = 0; j < (zbBits.size()) && (newWindow == nullptr); j++) {
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, zbBits[j]);

			if ((newWindow = SDL_CreateWindow(title, winPosX_, winPosY_, newRes.x, newRes.y, sdlFlags)) == nullptr) {
				LOG_L(L_WARNING, frmts[0], __func__, SDL_GetError(), aaLvls[i], zbBits[j]);
				continue;
			}

			LOG(frmts[1], __func__, aaLvls[i], zbBits[j], wpfName = SDL_GetPixelFormatName(SDL_GetWindowPixelFormat(newWindow)));
		}
	}

	if (newWindow == nullptr) {
		auto buf = fmt::sprintf("[GR::%s] could not create SDL-window\n", __func__);
		handleerror(nullptr, buf.c_str(), "ERROR", MBF_OK | MBF_EXCL);
		return nullptr;
	}

	UpdateWindowBorders(newWindow);

	return newWindow;
}

SDL_GLContext CGlobalRendering::CreateGLContext(const int2& minCtx)
{
	SDL_GLContext newContext = nullptr;

	constexpr int2 glCtxs[] = {{2, 0}, {2, 1},  {3, 0}, {3, 1}, {3, 2}, {3, 3},  {4, 0}, {4, 1}, {4, 2}, {4, 3}, {4, 4}, {4, 5}, {4, 6}};
	          int2 cmpCtx;

	if (std::find(&glCtxs[0], &glCtxs[0] + (sizeof(glCtxs) / sizeof(int2)), minCtx) == (&glCtxs[0] + (sizeof(glCtxs) / sizeof(int2)))) {
		handleerror(nullptr, "illegal OpenGL context-version specified, aborting", "ERROR", MBF_OK | MBF_EXCL);
		return nullptr;
	}

	if ((newContext = SDL_GL_CreateContext(sdlWindow)) != nullptr)
		return newContext;

	const char* frmts[] = {"[GR::%s] error (\"%s\") creating main GL%d.%d %s-context", "[GR::%s] created main GL%d.%d %s-context"};
	const char* profs[] = {"compatibility", "core"};

	char buf[1024] = {0};
	SNPRINTF(buf, sizeof(buf), frmts[false], __func__, SDL_GetError(), minCtx.x, minCtx.y, profs[forceCoreContext]);

	for (const int2 tmpCtx: glCtxs) {
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, tmpCtx.x);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, tmpCtx.y);

		for (uint32_t mask: {SDL_GL_CONTEXT_PROFILE_CORE, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY}) {
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, mask);

			if ((newContext = SDL_GL_CreateContext(sdlWindow)) == nullptr) {
				LOG_L(L_WARNING, frmts[false], __func__, SDL_GetError(), tmpCtx.x, tmpCtx.y, profs[mask == SDL_GL_CONTEXT_PROFILE_CORE]);
			} else {
				// save the lowest successfully created fallback compatibility-context
				if (mask == SDL_GL_CONTEXT_PROFILE_COMPATIBILITY && cmpCtx.x == 0 && tmpCtx.x >= minCtx.x)
					cmpCtx = tmpCtx;

				LOG_L(L_WARNING, frmts[true], __func__, tmpCtx.x, tmpCtx.y, profs[mask == SDL_GL_CONTEXT_PROFILE_CORE]);
			}

			// accepts nullptr's
			SDL_GL_DeleteContext(newContext);
		}
	}

	if (cmpCtx.x == 0) {
		handleerror(nullptr, buf, "ERROR", MBF_OK | MBF_EXCL);
		return nullptr;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, cmpCtx.x);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, cmpCtx.y);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);

	// should never fail at this point
	return (newContext = SDL_GL_CreateContext(sdlWindow));
}

bool CGlobalRendering::CreateWindowAndContext(const char* title)
{
	if (SDL_Init(SDL_INIT_VIDEO) == -1) {
		LOG_L(L_FATAL, "[GR::%s] error \"%s\" initializing SDL", __func__, SDL_GetError());
		return false;
	}

	if (!CheckAvailableVideoModes()) {
		handleerror(nullptr, "desktop color-depth should be at least 24 bits per pixel, aborting", "ERROR", MBF_OK | MBF_EXCL);
		return false;
	}

	// should be set to "3.0" (non-core Mesa is stuck there), see below
	const char* mesaGL = getenv("MESA_GL_VERSION_OVERRIDE");
	const char* softGL = getenv("LIBGL_ALWAYS_SOFTWARE");

	// get wanted resolution and context-version
	const int2 minCtx = (mesaGL != nullptr && std::strlen(mesaGL) >= 3)?
		int2{                  std::max(mesaGL[0] - '0', 3),                   std::max(mesaGL[2] - '0', 0)}:
		int2{configHandler->GetInt("GLContextMajorVersion"), configHandler->GetInt("GLContextMinorVersion")};

	// start with the standard (R8G8B8A8 + 24-bit depth + 8-bit stencil + DB) format
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,  24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	// create GL debug-context if wanted (more verbose GL messages, but runs slower)
	// note:
	//   requesting a core profile explicitly is needed to get versions later than
	//   3.0/1.30 for Mesa, other drivers return their *maximum* supported context
	//   in compat and do not make 3.0 itself available in core (though this still
	//   suffices for most of Spring)
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, forceCoreContext? SDL_GL_CONTEXT_PROFILE_CORE: SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, minCtx.x);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minCtx.y);


	if (msaaLevel > 0) {
		if (softGL != nullptr)
			LOG_L(L_WARNING, "MSAALevel > 0 and LIBGL_ALWAYS_SOFTWARE set, this will very likely crash!");

		// has to be even
		if (msaaLevel % 2 == 1)
			++msaaLevel;
	}

	if ((sdlWindow = CreateSDLWindow(title)) == nullptr)
		return false;

	if (configHandler->GetInt("MinimizeOnFocusLoss") == 0)
		SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");

	SetWindowAttributes(sdlWindow);

#if !defined(HEADLESS)
	// disable desktop compositing to fix tearing
	// (happens at 300fps, neither fullscreen nor vsync fixes it, so disable compositing)
	// On Windows Aero often uses vsync, and so when Spring runs windowed it will run with
	// vsync too, resulting in bad performance.
	if (configHandler->GetBool("BlockCompositing"))
		WindowManagerHelper::BlockCompositing(sdlWindow);
#endif

	if ((glContext = CreateGLContext(minCtx)) == nullptr)
		return false;

	gladLoadGL();
	GLX::Load(sdlWindow);

	if (!CheckGLContextVersion(minCtx)) {
		int ctxProfile = 0;
		SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &ctxProfile);

		const std::string errStr = fmt::format("current OpenGL version {}.{}(core={}) is less than required {}.{}(core={}), aborting",
			globalRenderingInfo.glContextVersion.x, globalRenderingInfo.glContextVersion.y, globalRenderingInfo.glContextIsCore,
			minCtx.x, minCtx.y, (ctxProfile == SDL_GL_CONTEXT_PROFILE_CORE)
		);

		handleerror(nullptr, errStr.c_str(), "ERROR", MBF_OK | MBF_EXCL);
		return false;
	}

	MakeCurrentContext(false);
	SDL_DisableScreenSaver();
	return true;
}


void CGlobalRendering::MakeCurrentContext(bool clear) const {
	SDL_GL_MakeCurrent(sdlWindow, clear ? nullptr : glContext);
}


void CGlobalRendering::DestroyWindowAndContext() {
	if (!sdlWindow)
		return;

	WindowManagerHelper::SetIconSurface(sdlWindow, nullptr);
	SetWindowInputGrabbing(false);

	SDL_GL_MakeCurrent(sdlWindow, nullptr);
	SDL_DestroyWindow(sdlWindow);

	#if !defined(HEADLESS)
	if (glContext)
		SDL_GL_DeleteContext(glContext);
	#endif

	sdlWindow = nullptr;
	glContext = nullptr;

	GLX::Unload();
}

void CGlobalRendering::KillSDL() const {
	#if !defined(HEADLESS)
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	#endif

	SDL_EnableScreenSaver();
	SDL_Quit();
}

void CGlobalRendering::PostInit() {
	// glewInit sets GL_INVALID_ENUM, get rid of it
	glGetError();

	char sdlVersionStr[64] = "";
	char glVidMemStr[64] = "unknown";

	QueryVersionInfo(sdlVersionStr, glVidMemStr);
	CheckGLExtensions();
	SetGLSupportFlags();
	QueryGLMaxVals();

	LogVersionInfo(sdlVersionStr, glVidMemStr);
	ToggleGLDebugOutput(0, 0, 0);

	UniformConstants::GetInstance().Init();
	ModelUniformData::Init();
	glGenQueries(glTimerQueries.size(), glTimerQueries.data());
	RenderBuffer::InitStatic();
	GL::shapes.Init();

	UpdateTimer();
}

void CGlobalRendering::SwapBuffers(bool allowSwapBuffers, bool clearErrors)
{
	spring_time pre;
	{
		SCOPED_TIMER("Misc::SwapBuffers");
		SCOPED_GL_DEBUGGROUP("Misc::SwapBuffers");
		assert(sdlWindow);

		// silently or verbosely clear queue at the end of every frame
		if (clearErrors || glDebugErrors)
			glClearErrors("GR", __func__, glDebugErrors);

		if (!allowSwapBuffers && !forceSwapBuffers)
			return;

		pre = spring_now();

		RenderBuffer::SwapRenderBuffers(); //all RBs are swapped here
		IStreamBufferConcept::PutBufferLocks();

		//https://stackoverflow.com/questions/68480028/supporting-opengl-screen-capture-by-third-party-applications
		glBindFramebuffer(GL_READ_FRAMEBUFFER_EXT, 0);

		SDL_GL_SwapWindow(sdlWindow);
		FrameMark;
	}
	// exclude debug from SCOPED_TIMER("Misc::SwapBuffers");
	eventHandler.DbgTimingInfo(TIMING_SWAP, pre, spring_now());
	globalRendering->lastSwapBuffersEnd = spring_now();
}

void CGlobalRendering::SetGLTimeStamp(uint32_t queryIdx) const
{
	if (!GLAD_GL_ARB_timer_query)
		return;

	glQueryCounter(glTimerQueries[(NUM_OPENGL_TIMER_QUERIES * (drawFrame & 1)) + queryIdx], GL_TIMESTAMP);
}

uint64_t CGlobalRendering::CalcGLDeltaTime(uint32_t queryIdx0, uint32_t queryIdx1) const
{
	if (!GLAD_GL_ARB_timer_query)
		return 0;

	const uint32_t queryBase = NUM_OPENGL_TIMER_QUERIES * (1 - (drawFrame & 1));

	assert(queryIdx0 < NUM_OPENGL_TIMER_QUERIES);
	assert(queryIdx1 < NUM_OPENGL_TIMER_QUERIES);
	assert(queryIdx0 < queryIdx1);

	GLuint64 t0 = 0;
	GLuint64 t1 = 0;

	GLint res = 0;

	// results from the previous frame should already (or soon) be available
	while (!res) {
		glGetQueryObjectiv(glTimerQueries[queryBase + queryIdx1], GL_QUERY_RESULT_AVAILABLE, &res);
	}

	glGetQueryObjectui64v(glTimerQueries[queryBase + queryIdx0], GL_QUERY_RESULT, &t0);
	glGetQueryObjectui64v(glTimerQueries[queryBase + queryIdx1], GL_QUERY_RESULT, &t1);

	// nanoseconds between timestamps
	return (t1 - t0);
}


void CGlobalRendering::CheckGLExtensions()
{
	#ifndef HEADLESS
	{
		GLint n = 0;
		glGetIntegerv(GL_NUM_EXTENSIONS, &n);
		for (auto i = 0; i < n; i++) {
			glExtensions.emplace(reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i)));
		}
	}
	// detect RenderDoc
	{
		constexpr GLenum GL_DEBUG_TOOL_EXT = 0x6789;
		constexpr GLenum GL_DEBUG_TOOL_NAME_EXT = 0x678A;
		constexpr GLenum GL_DEBUG_TOOL_PURPOSE_EXT = 0x678B;
		// For OpenGL:
		// if GL_EXT_debug_tool is present (see https://renderdoc.org/debug_tool.txt)
		if (glIsEnabled(GL_DEBUG_TOOL_EXT)) {
			auto debugStr = reinterpret_cast<const char*>(glGetString(GL_DEBUG_TOOL_NAME_EXT));
			LOG("[GR::%s] Detected external GL debug tool %s, enabling compatibility mode", __func__, debugStr);
			underExternalDebug = true;
		}
	}
	#endif

	if (underExternalDebug)
		return;

	char extMsg[ 128] = {0};
	char errMsg[2048] = {0};
	char* ptr = &extMsg[0];

	if (!GLAD_GL_ARB_multitexture       ) ptr += snprintf(ptr, sizeof(extMsg) - (ptr - extMsg), " multitexture ");
	if (!GLAD_GL_ARB_texture_env_combine) ptr += snprintf(ptr, sizeof(extMsg) - (ptr - extMsg), " texture_env_combine ");
	if (!GLAD_GL_ARB_texture_compression) ptr += snprintf(ptr, sizeof(extMsg) - (ptr - extMsg), " texture_compression ");
	if (!GLAD_GL_ARB_texture_float)       ptr += snprintf(ptr, sizeof(extMsg) - (ptr - extMsg), " texture_float ");
	if (!GLAD_GL_ARB_texture_non_power_of_two) ptr += snprintf(ptr, sizeof(extMsg) - (ptr - extMsg), " texture_non_power_of_two ");
	if (!GLAD_GL_ARB_framebuffer_object)       ptr += snprintf(ptr, sizeof(extMsg) - (ptr - extMsg), " framebuffer_object ");

	if (extMsg[0] == 0)
		return;

	SNPRINTF(errMsg, sizeof(errMsg),
		"OpenGL extension(s) GL_ARB_{%s} not found; update your GPU drivers!\n"
		"  GL renderer: %s\n"
		"  GL  version: %s\n",
		extMsg,
		globalRenderingInfo.glRenderer,
		globalRenderingInfo.glVersion);

	throw unsupported_error(errMsg);
}

void CGlobalRendering::SetGLSupportFlags()
{
	const std::string& glVendor = StringToLower(globalRenderingInfo.glVendor);
	const std::string& glRenderer = StringToLower(globalRenderingInfo.glRenderer);
	const std::string& glVersion = StringToLower(globalRenderingInfo.glVersion);

	bool haveGLSL  = (glGetString(GL_SHADING_LANGUAGE_VERSION) != nullptr);
	haveGLSL &= static_cast<bool>(GLAD_GL_ARB_vertex_shader && GLAD_GL_ARB_fragment_shader);
	haveGLSL &= static_cast<bool>(GLAD_GL_VERSION_2_0); // we want OpenGL 2.0 core functions
	haveGLSL |= underExternalDebug;

	#ifndef HEADLESS
	if (!haveGLSL)
		throw unsupported_error("OpenGL shaders not supported, aborting");
	#endif

	haveAMD    = (  glVendor.find(   "ati ") != std::string::npos) || (  glVendor.find("amd ") != std::string::npos) ||
				 (glRenderer.find("radeon ") != std::string::npos) || (glRenderer.find("amd ") != std::string::npos); //it's amazing how inconsistent AMD detection can be
	haveIntel  = (  glVendor.find(  "intel") != std::string::npos);
	haveNvidia = (  glVendor.find("nvidia ") != std::string::npos);
	haveMesa   = (glRenderer.find("mesa ") != std::string::npos) || (glRenderer.find("gallium ") != std::string::npos) || (glVersion.find(" mesa ") != std::string::npos);

	if (haveAMD) {
		globalRenderingInfo.gpuName   = globalRenderingInfo.glRenderer;
		globalRenderingInfo.gpuVendor = "AMD";
	} else if (haveIntel) {
		globalRenderingInfo.gpuName   = globalRenderingInfo.glRenderer;
		globalRenderingInfo.gpuVendor = "Intel";
	} else if (haveNvidia) {
		globalRenderingInfo.gpuName   = globalRenderingInfo.glRenderer;
		globalRenderingInfo.gpuVendor = "Nvidia";
	} else if (haveMesa) {
		globalRenderingInfo.gpuName   = globalRenderingInfo.glRenderer;
		globalRenderingInfo.gpuVendor = globalRenderingInfo.glVendor;
	} else {
		globalRenderingInfo.gpuName   = "Unknown";
		globalRenderingInfo.gpuVendor = "Unknown";
	}

	supportPersistentMapping = GLAD_GL_ARB_buffer_storage;
	supportPersistentMapping &= (configHandler->GetInt("ForceDisablePersistentMapping") == 0);

	supportExplicitAttribLoc = GLAD_GL_ARB_explicit_attrib_location;
	supportExplicitAttribLoc &= (configHandler->GetInt("ForceDisableExplicitAttribLocs") == 0);

	supportTextureQueryLOD = GLAD_GL_ARB_texture_query_lod;

	for (size_t n = 0; (n < sizeof(globalRenderingInfo.glVersionShort) && globalRenderingInfo.glVersion[n] != 0); n++) {
		if ((globalRenderingInfo.glVersionShort[n] = globalRenderingInfo.glVersion[n]) == ' ') {
			globalRenderingInfo.glVersionShort[n] = 0;
			break;
		}
	}
	if (int2 glVerNum = { 0, 0 }; sscanf(globalRenderingInfo.glVersionShort.data(), "%d.%d", &glVerNum.x, &glVerNum.y) == 2) {
		globalRenderingInfo.glslVersionNum = glVerNum.x * 10 + glVerNum.y;
	}

	for (size_t n = 0; (n < sizeof(globalRenderingInfo.glslVersionShort) && globalRenderingInfo.glslVersion[n] != 0); n++) {
		if ((globalRenderingInfo.glslVersionShort[n] = globalRenderingInfo.glslVersion[n]) == ' ') {
			globalRenderingInfo.glslVersionShort[n] = 0;
			break;
		}
	}
	if (int2 glslVerNum = { 0, 0 }; sscanf(globalRenderingInfo.glslVersionShort.data(), "%d.%d", &glslVerNum.x, &glslVerNum.y) == 2) {
		globalRenderingInfo.glslVersionNum = glslVerNum.x * 100 + glslVerNum.y;
	}

	haveGL4 = static_cast<bool>(GLAD_GL_ARB_multi_draw_indirect);
	haveGL4 &= static_cast<bool>(GLAD_GL_ARB_uniform_buffer_object);
	haveGL4 &= static_cast<bool>(GLAD_GL_ARB_shader_storage_buffer_object);
	haveGL4 &= CheckShaderGL4();
	haveGL4 &= !forceDisableGL4;

	{
		// use some ATI bugfixes?
		const int amdHacksCfg = configHandler->GetInt("AtiHacks");
		amdHacks = haveAMD && !haveMesa;
		amdHacks &= (amdHacksCfg < 0); // runtime detect
		amdHacks |= (amdHacksCfg > 0); // user override
	}

	// runtime-compress textures? (also already required for SMF ground textures)
	// default to off because it reduces quality, smallest mipmap level is bigger
	if (GLAD_GL_ARB_texture_compression)
		compressTextures = configHandler->GetBool("CompressTextures");


	// not defined for headless builds
	supportRestartPrimitive = GLAD_GL_NV_primitive_restart;
	supportClipSpaceControl = GLAD_GL_ARB_clip_control;
	supportSeamlessCubeMaps = GLAD_GL_ARB_seamless_cube_map;
	supportMSAAFrameBuffer = GLAD_GL_EXT_framebuffer_multisample;
	// CC did not exist as an extension before GL4.5, too recent to enforce

	//stick to the theory that reported = exist
	//supportClipSpaceControl &= ((globalRenderingInfo.glContextVersion.x * 10 + globalRenderingInfo.glContextVersion.y) >= 45);
	supportClipSpaceControl &= (configHandler->GetInt("ForceDisableClipCtrl") == 0);

	//supportFragDepthLayout = ((globalRenderingInfo.glContextVersion.x * 10 + globalRenderingInfo.glContextVersion.y) >= 42);
	supportFragDepthLayout = GLAD_GL_ARB_conservative_depth; //stick to the theory that reported = exist

	//stick to the theory that reported = exist
	//supportMSAAFrameBuffer &= ((globalRenderingInfo.glContextVersion.x * 10 + globalRenderingInfo.glContextVersion.y) >= 32);

	for (const int bits : {16, 24, 32}) {
		bool supported = false;
		if (FBO::IsSupported()) {
			FBO fbo;
			fbo.Bind();
			fbo.CreateRenderBuffer(GL_COLOR_ATTACHMENT0_EXT, GL_RGBA8, 16, 16);
			const GLint format = DepthBitsToFormat(bits);
			fbo.CreateRenderBuffer(GL_DEPTH_ATTACHMENT_EXT, format, 16, 16);
			supported = (fbo.GetStatus() == GL_FRAMEBUFFER_COMPLETE_EXT);
			fbo.Unbind();
		}

		if (supported)
			supportDepthBufferBitDepth = std::max(supportDepthBufferBitDepth, bits);
	}

	//TODO figure out if needed
	if (globalRendering->amdHacks) {
		supportDepthBufferBitDepth = 24;
	}
}

void CGlobalRendering::QueryGLMaxVals()
{
	// maximum 2D texture size
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
	glGetIntegerv(GL_MAX_TEXTURE_COORDS, &maxTexSlots);
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxFragShSlots);
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxCombShSlots);

	if (GLAD_GL_EXT_texture_filter_anisotropic)
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxTexAnisoLvl);

	// some GLSL relevant information
	if (GLAD_GL_ARB_uniform_buffer_object) {
		glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &glslMaxUniformBufferBindings);
		glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE,      &glslMaxUniformBufferSize);
	}

	if (GLAD_GL_ARB_shader_storage_buffer_object) {
		glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &glslMaxStorageBufferBindings);
		glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE,      &glslMaxStorageBufferSize);
	}

	glGetIntegerv(GL_MAX_VARYING_FLOATS,                 &glslMaxVaryings);
	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS,                 &glslMaxAttributes);
	glGetIntegerv(GL_MAX_DRAW_BUFFERS,                   &glslMaxDrawBuffers);
	glGetIntegerv(GL_MAX_ELEMENTS_INDICES,               &glslMaxRecommendedIndices);
	glGetIntegerv(GL_MAX_ELEMENTS_VERTICES,              &glslMaxRecommendedVertices);

	// GL_MAX_VARYING_FLOATS is the maximum number of floats, we count float4's
	glslMaxVaryings /= 4;
}

void CGlobalRendering::QueryVersionInfo(char (&sdlVersionStr)[64], char (&glVidMemStr)[64])
{
	auto& grInfo = globalRenderingInfo;

	auto& sdlVC = grInfo.sdlVersionCompiled;
	auto& sdlVL = grInfo.sdlVersionLinked;

	SDL_VERSION(&sdlVC);
	SDL_GetVersion(&sdlVL);

#ifndef HEADLESS
	grInfo.gladVersion = "0.1.36";
#else
	grInfo.gladVersion = "headless stub";
#endif // HEADLESS

	if ((grInfo.glVersion   = (const char*) glGetString(GL_VERSION                 )) == nullptr) grInfo.glVersion   = "unknown";
	if ((grInfo.glVendor    = (const char*) glGetString(GL_VENDOR                  )) == nullptr) grInfo.glVendor    = "unknown";
	if ((grInfo.glRenderer  = (const char*) glGetString(GL_RENDERER                )) == nullptr) grInfo.glRenderer  = "unknown";
	if ((grInfo.glslVersion = (const char*) glGetString(GL_SHADING_LANGUAGE_VERSION)) == nullptr) grInfo.glslVersion = "unknown";
	if ((grInfo.sdlDriverName = (const char*) SDL_GetCurrentVideoDriver(           )) == nullptr) grInfo.sdlDriverName = "unknown";
	// should never be null with any driver, no harm in an extra check
	// (absence of GLSL version string would indicate bigger problems)
	if (std::strcmp(globalRenderingInfo.glslVersion, "unknown") == 0)
		throw unsupported_error("OpenGL shaders not supported, aborting");

	if (!ShowDriverWarning(grInfo.glVendor))
		throw unsupported_error("OpenGL drivers not installed, aborting");

	constexpr const char* sdlFmtStr = "%d.%d.%d (linked) / %d.%d.%d (compiled)";
	constexpr const char* memFmtStr = "%iMB (total) / %iMB (available)";

	SNPRINTF(sdlVersionStr, sizeof(sdlVersionStr), sdlFmtStr,
		sdlVL.major, sdlVL.minor, sdlVL.patch,
		sdlVC.major, sdlVC.minor, sdlVC.patch
	);

	if (!GetAvailableVideoRAM(&grInfo.gpuMemorySize.x, grInfo.glVendor))
		return;

	const GLint totalMemMB = grInfo.gpuMemorySize.x / 1024;
	const GLint availMemMB = grInfo.gpuMemorySize.y / 1024;

	SNPRINTF(glVidMemStr, sizeof(glVidMemStr), memFmtStr, totalMemMB, availMemMB);
}

void CGlobalRendering::LogVersionInfo(const char* sdlVersionStr, const char* glVidMemStr) const
{
	LOG("[GR::%s]", __func__);
	LOG("\tSDL version : %s", sdlVersionStr);
	LOG("\tGL version  : %s", globalRenderingInfo.glVersion);
	LOG("\tGL vendor   : %s", globalRenderingInfo.glVendor);
	LOG("\tGL renderer : %s", globalRenderingInfo.glRenderer);
	LOG("\tGLSL version: %s", globalRenderingInfo.glslVersion);
	LOG("\tGLAD version: %s", globalRenderingInfo.gladVersion);
	LOG("\tGPU memory  : %s", glVidMemStr);
	LOG("\tSDL swap-int: %d", SDL_GL_GetSwapInterval());
	LOG("\tSDL driver  : %s", globalRenderingInfo.sdlDriverName);
	LOG("\t");
	LOG("\tInitialized OpenGL Context: %i.%i (%s)", globalRenderingInfo.glContextVersion.x, globalRenderingInfo.glContextVersion.y, globalRenderingInfo.glContextIsCore ? "Core" : "Compat");
	LOG("\tGLSL shader support       : %i", true);
	LOG("\tGL4 support               : %i", haveGL4);
	LOG("\tFBO extension support     : %i", FBO::IsSupported());
	LOG("\tNVX GPU mem-info support  : %i", IsExtensionSupported("GL_NVX_gpu_memory_info"));
	LOG("\tATI GPU mem-info support  : %i", IsExtensionSupported("GL_ATI_meminfo"));
	LOG("\tTexture clamping to edge  : %i", IsExtensionSupported("GL_EXT_texture_edge_clamp"));
	LOG("\tS3TC/DXT1 texture support : %i/%i", IsExtensionSupported("GL_EXT_texture_compression_s3tc"), IsExtensionSupported("GL_EXT_texture_compression_dxt1"));
	LOG("\ttexture query-LOD support : %i (%i)", supportTextureQueryLOD, IsExtensionSupported("GL_ARB_texture_query_lod"));
	LOG("\tMSAA frame-buffer support : %i (%i)", supportMSAAFrameBuffer, IsExtensionSupported("GL_EXT_framebuffer_multisample"));
	LOG("\tZ-buffer depth            : %i (-)" , supportDepthBufferBitDepth);
	LOG("\tprimitive-restart support : %i (%i)", supportRestartPrimitive, IsExtensionSupported("GL_NV_primitive_restart"));
	LOG("\tclip-space control support: %i (%i)", supportClipSpaceControl, IsExtensionSupported("GL_ARB_clip_control"));
	LOG("\tseamless cube-map support : %i (%i)", supportSeamlessCubeMaps, IsExtensionSupported("GL_ARB_seamless_cube_map"));
	LOG("\tfrag-depth layout support : %i (%i)", supportFragDepthLayout, IsExtensionSupported("GL_ARB_conservative_depth"));
	LOG("\tpersistent maps support   : %i (%i)", supportPersistentMapping, IsExtensionSupported("GL_ARB_buffer_storage"));
	LOG("\texplicit attribs location : %i (%i)", supportExplicitAttribLoc, IsExtensionSupported("GL_ARB_explicit_attrib_location"));
	LOG("\tmulti draw indirect       : %i (-)" , IsExtensionSupported("GL_ARB_multi_draw_indirect"));
	LOG("\tarray textures            : %i (-)" , IsExtensionSupported("GL_EXT_texture_array"));
	LOG("\tbuffer copy support       : %i (-)" , IsExtensionSupported("GL_ARB_copy_buffer"));
	LOG("\tindirect draw             : %i (-)" , IsExtensionSupported("GL_ARB_draw_indirect"));
	LOG("\tbase instance             : %i (-)" , IsExtensionSupported("GL_ARB_base_instance"));

	LOG("\t");
	LOG("\tmax. FBO samples              : %i", FBO::GetMaxSamples());
	LOG("\tmax. texture slots            : %i", maxTexSlots);
	LOG("\tmax. FS/program texture slots : %i/%i", maxFragShSlots, maxCombShSlots);
	LOG("\tmax. texture size             : %i", maxTextureSize);
	LOG("\tmax. texture anisotropy level : %f", maxTexAnisoLvl);
	LOG("\tmax. vec4 varyings/attributes : %i/%i", glslMaxVaryings, glslMaxAttributes);
	LOG("\tmax. draw-buffers             : %i", glslMaxDrawBuffers);
	LOG("\tmax. rec. indices/vertices    : %i/%i", glslMaxRecommendedIndices, glslMaxRecommendedVertices);
	LOG("\tmax. uniform buffer-bindings  : %i", glslMaxUniformBufferBindings);
	LOG("\tmax. uniform block-size       : %iKB", glslMaxUniformBufferSize / 1024);
	LOG("\tmax. storage buffer-bindings  : %i", glslMaxStorageBufferBindings);
	LOG("\tmax. storage block-size       : %iMB", glslMaxStorageBufferSize / (1024 * 1024));
	LOG("\t");
	LOG("\tenable AMD-hacks : %i", amdHacks);
	LOG("\tcompress MIP-maps: %i", compressTextures);

	GLint numberOfTextureFormats = 0;
	glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &numberOfTextureFormats);
	std::vector<GLint> textureFormats; textureFormats.resize(numberOfTextureFormats);
	glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, textureFormats.data());

	#define EnumToString(arg) { arg, #arg }
	std::unordered_map<GLenum, std::string> compressedEnumToString = {
		EnumToString(GL_COMPRESSED_RED_RGTC1),
		EnumToString(GL_COMPRESSED_SIGNED_RED_RGTC1),
		EnumToString(GL_COMPRESSED_RG_RGTC2),
		EnumToString(GL_COMPRESSED_SIGNED_RG_RGTC2),
		EnumToString(GL_COMPRESSED_RGBA_BPTC_UNORM),
		EnumToString(GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM),
		EnumToString(GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT),
		EnumToString(GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT),
		EnumToString(GL_COMPRESSED_RGB8_ETC2),
		EnumToString(GL_COMPRESSED_SRGB8_ETC2),
		EnumToString(GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2),
		EnumToString(GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2),
		EnumToString(GL_COMPRESSED_RGBA8_ETC2_EAC),
		EnumToString(GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC),
		EnumToString(GL_COMPRESSED_R11_EAC),
		EnumToString(GL_COMPRESSED_SIGNED_R11_EAC),
		EnumToString(GL_COMPRESSED_RG11_EAC),
		EnumToString(GL_COMPRESSED_SIGNED_RG11_EAC),
		EnumToString(GL_COMPRESSED_RGB_S3TC_DXT1_EXT),
		EnumToString(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT),
		EnumToString(GL_COMPRESSED_RGBA_S3TC_DXT3_EXT),
		EnumToString(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT),
		EnumToString(GL_COMPRESSED_RGBA_ASTC_4x4_KHR),
		EnumToString(GL_COMPRESSED_RGBA_ASTC_5x4_KHR),
		EnumToString(GL_COMPRESSED_RGBA_ASTC_5x5_KHR),
		EnumToString(GL_COMPRESSED_RGBA_ASTC_6x5_KHR),
		EnumToString(GL_COMPRESSED_RGBA_ASTC_6x6_KHR),
		EnumToString(GL_COMPRESSED_RGBA_ASTC_8x5_KHR),
		EnumToString(GL_COMPRESSED_RGBA_ASTC_8x6_KHR),
		EnumToString(GL_COMPRESSED_RGBA_ASTC_8x8_KHR),
		EnumToString(GL_COMPRESSED_RGBA_ASTC_10x5_KHR),
		EnumToString(GL_COMPRESSED_RGBA_ASTC_10x6_KHR),
		EnumToString(GL_COMPRESSED_RGBA_ASTC_10x8_KHR),
		EnumToString(GL_COMPRESSED_RGBA_ASTC_10x10_KHR),
		EnumToString(GL_COMPRESSED_RGBA_ASTC_12x10_KHR),
		EnumToString(GL_COMPRESSED_RGBA_ASTC_12x12_KHR),
		EnumToString(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR),
		EnumToString(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR),
		EnumToString(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR),
		EnumToString(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR),
		EnumToString(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR),
		EnumToString(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR),
		EnumToString(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR),
		EnumToString(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR),
		EnumToString(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR),
		EnumToString(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR),
		EnumToString(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR),
		EnumToString(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR),
		EnumToString(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR),
		EnumToString(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR),
		EnumToString(GL_PALETTE4_RGB8_OES),
		EnumToString(GL_PALETTE4_RGBA8_OES),
		EnumToString(GL_PALETTE4_R5_G6_B5_OES),
		EnumToString(GL_PALETTE4_RGBA4_OES),
		EnumToString(GL_PALETTE4_RGB5_A1_OES),
		EnumToString(GL_PALETTE8_RGB8_OES),
		EnumToString(GL_PALETTE8_RGBA8_OES),
		EnumToString(GL_PALETTE8_R5_G6_B5_OES),
		EnumToString(GL_PALETTE8_RGBA4_OES),
		EnumToString(GL_PALETTE8_RGB5_A1_OES)
	};
	#undef EnumToString

	LOG("\tNumber of compressed texture formats: %i", numberOfTextureFormats);
	std::ostringstream ss;
	for (auto tf : textureFormats) {
		auto it = compressedEnumToString.find(tf);
		if (it != compressedEnumToString.end())
			ss << it->second << ", ";
		else
			ss << "0x" << std::hex << tf << ", ";
	}
	ss.seekp(-2, std::ios_base::end);
	ss << ".";
	LOG("\tCompressed texture formats: %s", ss.str().c_str());
}

void CGlobalRendering::LogDisplayMode(SDL_Window* window) const
{
	// print final mode (call after SetupViewportGeometry, which updates viewSizeX/Y)
	SDL_DisplayMode dmode;
	SDL_GetWindowDisplayMode(window, &dmode);

	constexpr const std::array names = {
		"windowed::decorated",       // fs=0,bl=0
		"windowed::borderless",	     // fs=0,bl=1
		"fullscreen::exclusive",     // fs=1,bl=0
		"fullscreen::non-exclusive", // fs=1,bl=1
	};

	const int fs = fullScreen;
	const int bl = borderless;

	LOG("[GR::%s] display-mode set to %ix%ix%ibpp@%iHz (%s)", __func__, viewSizeX, viewSizeY, SDL_BITSPERPIXEL(dmode.format), dmode.refresh_rate, names[fs * 2 + bl]);
}

void CGlobalRendering::GetAllDisplayBounds(SDL_Rect& r) const
{
	int displayIdx = 0;
	GetDisplayBounds(r, &displayIdx);

	std::array<int, 4> mb = { r.x, r.y, r.x + r.w, r.y + r.h }; //L, T, R, B

	for (displayIdx = 1; displayIdx < numDisplays; ++displayIdx) {
		SDL_Rect db;
		GetDisplayBounds(db, &displayIdx);
		std::array<int, 4> b = { db.x, db.y, db.x + db.w, db.y + db.h }; //L, T, R, B

		if (b[0] < mb[0]) mb[0] = b[0];
		if (b[1] < mb[1]) mb[1] = b[1];
		if (b[2] > mb[2]) mb[2] = b[2];
		if (b[3] > mb[3]) mb[3] = b[3];
	}

	r = { mb[0], mb[1], mb[2] - mb[0], mb[3] - mb[1] };
}

void CGlobalRendering::GetWindowPosSizeBounded(int& x, int& y, int& w, int& h) const
{
	SDL_Rect r;
	GetAllDisplayBounds(r);

	x = std::clamp(x, r.x, r.x + r.w);
	y = std::clamp(y, r.y, r.y + r.h);
	w = std::max(w, minRes.x * (1 - fullScreen)); w = std::min(w, r.w - x);
	h = std::max(h, minRes.y * (1 - fullScreen)); h = std::min(h, r.h - y);
}

void CGlobalRendering::SetWindowTitle(const std::string& title)
{
	// SDL_SetWindowTitle deadlocks in case it's called from non-main thread (during the MT loading).

	static auto SetWindowTitleImpl = [](SDL_Window* sdlWindow, const std::string& title) {
		SDL_SetWindowTitle(sdlWindow, title.c_str());
	};

	if (Threading::IsMainThread())
		SetWindowTitleImpl(sdlWindow, title);
	else
		spring::QueuedFunction::Enqueue<decltype(SetWindowTitleImpl), SDL_Window*, const std::string&>(SetWindowTitleImpl, sdlWindow, title);
}



void CGlobalRendering::SetWindowAttributes(SDL_Window* window)
{
	// Get wanted state
	borderless = configHandler->GetBool("WindowBorderless");
	fullScreen = configHandler->GetBool("Fullscreen");
	winPosX = configHandler->GetInt("WindowPosX");
	winPosY = configHandler->GetInt("WindowPosY");

	// update display count
	numDisplays = SDL_GetNumVideoDisplays();

	// get desired resolution
	// note that the configured fullscreen resolution is just
	// ignored by SDL if not equal to the user's screen size
	const int2 maxRes = GetMaxWinRes();
	      int2 newRes = GetCfgWinRes();

	LOG("[GR::%s][1] cfgFullScreen=%d numDisplays=%d winPos=<%d,%d> newRes=<%d,%d>", __func__, fullScreen, numDisplays, winPosX, winPosY, newRes.x, newRes.y);
	GetWindowPosSizeBounded(winPosX, winPosY, newRes.x, newRes.y);
	LOG("[GR::%s][2] cfgFullScreen=%d numDisplays=%d winPos=<%d,%d> newRes=<%d,%d>", __func__, fullScreen, numDisplays, winPosX, winPosY, newRes.x, newRes.y);

//	if (SDL_SetWindowFullscreen(window, 0) != 0)
//		LOG("[GR::%s][3][SDL_SetWindowFullscreen] err=\"%s\"", __func__, SDL_GetError());

	SDL_RestoreWindow(window);
	SDL_SetWindowMinimumSize(window, minRes.x, minRes.y);

	SDL_SetWindowPosition(window, winPosX, winPosY);
	SDL_SetWindowSize(window, newRes.x, newRes.y);

	if (SDL_SetWindowFullscreen(window, (borderless ? SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_WINDOW_FULLSCREEN) * fullScreen) != 0)
		LOG("[GR::%s][4][SDL_SetWindowFullscreen] err=\"%s\"", __func__, SDL_GetError());

	SDL_SetWindowBordered(window, borderless ? SDL_FALSE : SDL_TRUE);

	if (newRes == maxRes)
		SDL_MaximizeWindow(window);

	WindowManagerHelper::SetWindowResizable(window, !borderless && !fullScreen);
}

void CGlobalRendering::ConfigNotify(const std::string& key, const std::string& value)
{
	LOG("[GR::%s][1] key=%s val=%s", __func__, key.c_str(), value.c_str());

	if (key == "MinSampleShadingRate") {
		auto newMSSR = configHandler->GetFloat("MinSampleShadingRate");

		if (minSampleShadingRate != newMSSR) {
			minSampleShadingRate = newMSSR;
			SetMinSampleShadingRate();
		}
	}

	if (key == "DualScreenMode" || key == "DualScreenMiniMapOnLeft") {
		SetDualScreenParams();
		UpdateGLGeometry();

		if (game != nullptr)
			gmeChgFrame = drawFrame + 1; //need to do on next frame since config mutex is locked inside ConfigNotify

		return;
	}
	winChgFrame = drawFrame + 1; //need to do on next frame since config mutex is locked inside ConfigNotify
}

void CGlobalRendering::UpdateWindow()
{
	ZoneScoped;
	if (!spring::QueuedFunction::Empty()) {
		for (const auto& qf : spring::QueuedFunction::GetQueuedFunctions()) {
			qf->Execute();
		}
		spring::QueuedFunction::Clear();
	}

	if (gmeChgFrame == drawFrame)
		game->ResizeEvent();

	if (winChgFrame != drawFrame)
		return;

	if (sdlWindow == nullptr)
		return;

	SetWindowAttributes(sdlWindow);
	MakeCurrentContext(false);
}

void CGlobalRendering::UpdateTimer()
{
	grTime = spring_now();
}

bool CGlobalRendering::GetWindowInputGrabbing()
{
	return static_cast<bool>(SDL_GetWindowGrab(sdlWindow));
}

bool CGlobalRendering::SetWindowInputGrabbing(bool enable)
{
	// SDL_SetWindowGrab deadlocks in case it's called from non-main thread (during the MT loading).

	static auto SetWindowGrabImpl = [](SDL_Window* sdlWindow, bool enable) {
		SDL_SetWindowGrab(sdlWindow, enable ? SDL_TRUE : SDL_FALSE);
	};

	if (Threading::IsMainThread())
		SetWindowGrabImpl(sdlWindow, enable);
	else
		spring::QueuedFunction::Enqueue(SetWindowGrabImpl, sdlWindow, enable);

	return enable;
}

bool CGlobalRendering::ToggleWindowInputGrabbing()
{
	if (GetWindowInputGrabbing())
		return (SetWindowInputGrabbing(false));

	return (SetWindowInputGrabbing(true));
}

bool CGlobalRendering::SetWindowPosHelper(int displayIdx, int winRPosX, int winRPosY, int winSizeX_, int winSizeY_, bool fs, bool bl) const
{
#ifndef HEADLESS
	if (displayIdx < 0 || displayIdx >= numDisplays) {
		LOG_L(L_ERROR, "[GR::%s] displayIdx(%d) is out of bounds (%d,%d)", __func__, displayIdx, 0, numDisplays - 1);
		return false;
	}

	SDL_Rect db;
	GetDisplayBounds(db, &displayIdx);

	const int2 tlPos = { db.x + winRPosX            , db.y + winRPosY             };
	const int2 brPos = { db.x + winRPosX + winSizeX_, db.y + winRPosY + winSizeY_ };

	configHandler->Set("WindowPosX", tlPos.x);
	configHandler->Set("WindowPosY", tlPos.y);

	configHandler->Set(xsKeys[fs], winSizeX_);
	configHandler->Set(ysKeys[fs], winSizeY_);
	configHandler->Set("Fullscreen", fs);
	configHandler->Set("WindowBorderless", bl);
#endif

	return true;
}

int2 CGlobalRendering::GetMaxWinRes() const {
	SDL_DisplayMode dmode;
	SDL_GetDesktopDisplayMode(GetCurrentDisplayIndex(), &dmode);
	return {dmode.w, dmode.h};
}

int2 CGlobalRendering::GetCfgWinRes() const
{
	int2 res = {configHandler->GetInt(xsKeys[fullScreen]), configHandler->GetInt(ysKeys[fullScreen])};

	// copy Native Desktop Resolution if user did not specify a value
	// SDL2 can do this itself if size{X,Y} are set to zero but fails
	// with Display Cloning and similar, causing DVI monitors to only
	// run at (e.g.) 640x400 and HDMI devices at full-HD
	// TODO: make screen configurable?
	if (res.x <= 0 || res.y <= 0)
		res = GetMaxWinRes();

	return res;
}

int CGlobalRendering::GetCurrentDisplayIndex() const
{
	return sdlWindow ? SDL_GetWindowDisplayIndex(sdlWindow) : 0;
}

void CGlobalRendering::GetDisplayBounds(SDL_Rect& r, const int* di) const
{
	const int displayIndex = di ? *di : GetCurrentDisplayIndex();
	SDL_GetDisplayBounds(displayIndex, &r);
}

void CGlobalRendering::GetUsableDisplayBounds(SDL_Rect& r, const int* di) const
{
	const int displayIndex = di ? *di : GetCurrentDisplayIndex();
	SDL_GetDisplayUsableBounds(displayIndex, &r);
}

bool CGlobalRendering::IsExtensionSupported(const char* ext) const
{
	return glExtensions.contains(ext);
}


// only called on startup; change the config based on command-line args
void CGlobalRendering::SetFullScreen(bool cliWindowed, bool cliFullScreen)
{
	const bool cfgFullScreen = configHandler->GetBool("Fullscreen");

	fullScreen = (cfgFullScreen && !cliWindowed  );
	fullScreen = (cfgFullScreen ||  cliFullScreen);

	configHandler->Set("Fullscreen", fullScreen);
}

void CGlobalRendering::SetDualScreenParams()
{
	dualScreenMode = configHandler->GetBool("DualScreenMode");
	dualScreenMiniMapOnLeft = dualScreenMode && configHandler->GetBool("DualScreenMiniMapOnLeft");
}

static const auto compareSDLRectPosX = [](const SDL_Rect& a, const SDL_Rect& b) {
  return (a.x < b.x);
};

void CGlobalRendering::UpdateViewPortGeometry()
{
	viewPosY = 0;
	viewSizeY = winSizeY;
	viewWindowOffsetY = 0;

	if (!dualScreenMode) {
		viewPosX = 0;
		viewSizeX = winSizeX;

		return;
	}

	dualViewPosY = 0;
	dualViewSizeY = viewSizeY;
	dualWindowOffsetY = 0;

	// Use halfscreen dual and view if only 1 display or fullscreen
	if (numDisplays == 1 || fullScreen) {
		const int halfWinSize = winSizeX >> 1;

		viewPosX = halfWinSize * dualScreenMiniMapOnLeft;
		viewSizeX = halfWinSize;

		dualViewPosX = halfWinSize - viewPosX;
		dualViewSizeX = halfWinSize;

		return;
	}

	std::vector<SDL_Rect> screenRects;
	SDL_Rect winRect = { winPosX, winPosY, winSizeX, winSizeY };

	for(int i = 0 ; i < numDisplays ; ++i)
	{
		SDL_Rect screen, interRect;
		GetDisplayBounds(screen, &i);
		LOG("[GR::%s] Raw Screen %i: pos %dx%d | size %dx%d", __func__, i, screen.x, screen.y, screen.w, screen.h);
		// we only care about screenRects that overlap window
		if (!SDL_IntersectRect(&screen, &winRect, &interRect)) {
			LOG("[GR::%s] No intersection: pos %dx%d | size %dx%d", __func__, screen.x, screen.y, screen.w, screen.h);
			continue;
		}

		// make screen positions relative to each other
		interRect.x -= winPosX;
		interRect.y -= winPosY;

		screenRects.push_back(interRect);
	}

	std::sort(screenRects.begin(), screenRects.end(), compareSDLRectPosX);

	int i = 0;
	for (auto screen : screenRects) {
		LOG("[GR::%s] Screen %i: pos %dx%d | size %dx%d", __func__, ++i, screen.x, screen.y, screen.w, screen.h);
	}

	if (screenRects.size() == 1) {
		SDL_Rect screenRect = screenRects.front();

		const int halfWinSize = screenRect.w >> 1;

		viewPosX = halfWinSize * dualScreenMiniMapOnLeft;
		viewSizeX = halfWinSize;

		dualViewPosX = halfWinSize - viewPosX;
		dualViewSizeX = halfWinSize;

		return;
	}

	SDL_Rect dualScreenRect = dualScreenMiniMapOnLeft ? screenRects.front() : screenRects.back();

	if (dualScreenMiniMapOnLeft) {
		screenRects.erase(screenRects.begin());
	} else {
		screenRects.pop_back();
	}

	const SDL_Rect first = screenRects.front();
	const SDL_Rect last = screenRects.back();

	viewPosX = first.x;
	viewSizeX = last.x + last.w - first.x;
	viewSizeY = first.h;

	dualViewPosX = dualScreenRect.x;
	dualViewSizeX = dualScreenRect.w;
	dualViewSizeY = dualScreenRect.h;

	// We store the offset in relation to window top border for sdl mouse translation
	viewWindowOffsetY = first.y;
	dualWindowOffsetY = dualScreenRect.y;

	// In-game and GL coords y orientation is inverse of SDL screen coords
	viewPosY = winSizeY - (viewSizeY + viewWindowOffsetY);
	dualViewPosY = winSizeY - (dualViewSizeY + dualWindowOffsetY);

	LOG("[GR::%s] Wind: pos %dx%d | size %dx%d", __func__, winPosX, winPosY, winSizeX, winSizeY);
	LOG("[GR::%s] View: pos %dx%d | size %dx%d | yoff %d", __func__,  viewPosX, viewPosY, viewSizeX, viewSizeY, viewWindowOffsetY);
	LOG("[GR::%s] Dual: pos %dx%d | size %dx%d | yoff %d", __func__,  dualViewPosX, dualViewPosY, dualViewSizeX, dualViewSizeY, dualWindowOffsetY);
}

void CGlobalRendering::UpdatePixelGeometry()
{
	pixelX = 1.0f / viewSizeX;
	pixelY = 1.0f / viewSizeY;

	aspectRatio = viewSizeX / float(viewSizeY);
}


void CGlobalRendering::ReadWindowPosAndSize()
{
#ifdef HEADLESS
	screenSizeX = 8;
	screenSizeY = 8;
	winSizeX = 8;
	winSizeY = 8;
	winPosX = 0;
	winPosY = 0;
	winBorder = { 0 };
#else

	SDL_Rect screenSize;
	GetDisplayBounds(screenSize);

	// no other good place to set these
	screenSizeX = screenSize.w;
	screenSizeY = screenSize.h;
	screenPosX  = screenSize.x;
	screenPosY  = screenSize.y;

	//probably redundant
	if (!borderless)
		UpdateWindowBorders(sdlWindow);

	SDL_GetWindowSize(sdlWindow, &winSizeX, &winSizeY);
	SDL_GetWindowPosition(sdlWindow, &winPosX, &winPosY);

	//enforce >=0 https://github.com/beyond-all-reason/spring/issues/23
	//winPosX = std::max(winPosX, 0);
	//winPosY = std::max(winPosY, 0);
#endif

	// should be done by caller
	// UpdateViewPortGeometry();
}

void CGlobalRendering::SaveWindowPosAndSize()
{
#ifdef HEADLESS
	return;
#endif

	if (fullScreen)
		return;

	// do not save if minimized
	// note that maximized windows are automagically restored; SDL2
	// apparently detects if the resolution is maximal and sets the
	// flag (but we also check if winRes equals maxRes to be safe)
	if ((SDL_GetWindowFlags(sdlWindow) & SDL_WINDOW_MINIMIZED) != 0)
		return;

	// do not notify about changes to block update loop
	configHandler->Set("WindowPosX", winPosX, false, false);
	configHandler->Set("WindowPosY", winPosY, false, false);
	configHandler->Set("XResolutionWindowed", winSizeX, false, false);
	configHandler->Set("YResolutionWindowed", winSizeY, false, false);
}


void CGlobalRendering::UpdateGLConfigs()
{
	LOG("[GR::%s]", __func__);

	// re-read configuration value
	verticalSync->SetInterval();
}

void CGlobalRendering::UpdateScreenMatrices()
{
	// .x := screen width (meters), .y := eye-to-screen (meters)
	static float2 screenParameters = { 0.36f, 0.60f };

	const int remScreenSize = screenSizeY - winSizeY; // remaining desktop size (ssy >= wsy)
	const int bottomWinCoor = remScreenSize - winPosY; // *bottom*-left origin

	const float vpx = viewPosX + winPosX;
	const float vpy = viewPosY + bottomWinCoor;
	const float vsx = viewSizeX; // same as winSizeX except in dual-screen mode
	const float vsy = viewSizeY; // same as winSizeY
	const float ssx = screenSizeX;
	const float ssy = screenSizeY;
	const float hssx = 0.5f * ssx;
	const float hssy = 0.5f * ssy;

	const float zplane = screenParameters.y * (ssx / screenParameters.x);
	const float znear = zplane * 0.5f;
	const float zfar = zplane * 2.0f;
	constexpr float zfact = 0.5f;

	const float left = (vpx - hssx) * zfact;
	const float bottom = (vpy - hssy) * zfact;
	const float right = ((vpx + vsx) - hssx) * zfact;
	const float top = ((vpy + vsy) - hssy) * zfact;

	LOG("[GR::%s] vpx=%f, vpy=%f, vsx=%f, vsy=%f, ssx=%f, ssy=%f, screenPosX=%d, screenPosY=%d", __func__, vpx, vpy, vsx, vsy, ssx, ssy, screenPosX, screenPosY);

	// translate s.t. (0,0,0) is on the zplane, on the window's bottom-left corner
	screenViewMatrix = CMatrix44f{ float3{left / zfact, bottom / zfact, -zplane} };
	screenProjMatrix = CMatrix44f::ClipPerspProj(left, right, bottom, top, znear, zfar, supportClipSpaceControl * 1.0f);
}

void CGlobalRendering::UpdateWindowBorders(SDL_Window* window) const
{
#ifndef HEADLESS
	assert(window);

	SDL_GetWindowBordersSize(window, &winBorder[0], &winBorder[1], &winBorder[2], &winBorder[3]);

	#if defined(_WIN32) && (WINDOWS_NO_INVISIBLE_GRIPS == 1)
	// W/A for 8 px Aero invisible borders https://github.com/libsdl-org/SDL/commit/7c60bec493404905f512c835f502f1ace4eff003
	{
		auto scopedLib = spring::ScopedResource(
			LoadLibrary("dwmapi.dll"),
			[](HMODULE lib) { if (lib) FreeLibrary(lib); }
		);

		if (scopedLib == nullptr)
			return;

		using DwmGetWindowAttributeT = HRESULT WINAPI(
			HWND,
			DWORD,
			PVOID,
			DWORD
		);

		static auto* DwmGetWindowAttribute = reinterpret_cast<DwmGetWindowAttributeT*>(GetProcAddress(scopedLib, "DwmGetWindowAttribute"));

		if (!DwmGetWindowAttribute)
			return;

		SDL_SysWMinfo wmInfo;
		SDL_VERSION(&wmInfo.version);
		SDL_GetWindowWMInfo(window, &wmInfo);
		HWND& hwnd = wmInfo.info.win.window;

		RECT rect, frame;

		static constexpr DWORD DWMWA_EXTENDED_FRAME_BOUNDS = 9; // https://docs.microsoft.com/en-us/windows/win32/api/dwmapi/ne-dwmapi-dwmwindowattribute
		DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &frame, sizeof(RECT));
		GetWindowRect(hwnd, &rect);

		winBorder[0] -= std::max(0l, frame.top   - rect.top    );
		winBorder[1] -= std::max(0l, frame.left  - rect.left   );
		winBorder[2] -= std::max(0l, rect.bottom - frame.bottom);
		winBorder[3] -= std::max(0l, rect.right  - frame.right);

		LOG_L(L_DEBUG, "[GR::%s] Working around Windows 10+ thick borders SDL2 issue, borders are slimmed by TLBR(%d,%d,%d,%d)", __func__,
			static_cast<int>(std::max(0l, frame.top   - rect.top    )),
			static_cast<int>(std::max(0l, frame.left  - rect.left   )),
			static_cast<int>(std::max(0l, rect.bottom - frame.bottom)),
			static_cast<int>(std::max(0l, rect.right  - frame.right ))
		);
	}
	LOG_L(L_DEBUG, "[GR::%s] Storing window borders {%d, %d, %d, %d}", __func__, winBorder[0], winBorder[1], winBorder[2], winBorder[3]);
	#endif
#endif
}

void CGlobalRendering::UpdateGLGeometry()
{
	LOG("[GR::%s][1] winSize=<%d,%d>", __func__, winSizeX, winSizeY);

	ReadWindowPosAndSize();
	UpdateViewPortGeometry();
	UpdatePixelGeometry();
	UpdateScreenMatrices();

	LOG("[GR::%s][2] winSize=<%d,%d>", __func__, winSizeX, winSizeY);
}

void CGlobalRendering::InitGLState()
{
	LOG("[GR::%s]", __func__);

	glShadeModel(GL_SMOOTH);

	glClearDepth(1.0f);
	glDepthRange(0.0f, 1.0f);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	// avoid precision loss with default DR transform
	if (supportClipSpaceControl)
		glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);

	if (supportSeamlessCubeMaps)
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	// MSAA rasterization
	msaaLevel *= CheckGLMultiSampling();
	ToggleMultisampling();

	SetMinSampleShadingRate();

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	LoadViewport();

	// this does not accomplish much
	// SwapBuffers(true, true);
	LogDisplayMode(sdlWindow);
}

void CGlobalRendering::ToggleMultisampling() const
{
	if (msaaLevel > 0)
		glEnable(GL_MULTISAMPLE);
	else
		glDisable(GL_MULTISAMPLE);
}

bool CGlobalRendering::CheckShaderGL4() const
{
#ifndef HEADLESS
	//the code below doesn't make any sense, but here only to test if the shader can be compiled
	constexpr static const char* vsSrc = R"(
#version 430 core

layout (location = 0) in vec3 pos;
layout (location = 6) in uvec4 instData;

layout(std140, binding = 0) readonly buffer MatrixBuffer {
	mat4 mat[];
};

out Data {
	float vFloat;
};
void main()
{
	vFloat = float(instData.y);
	gl_Position = mat[instData.x] * vec4(pos, 1.0);
}
)";

	constexpr static const char* fsSrc = R"(
#version 430 core

in Data {
	float vFloat;
};
out vec4 fragColor;
void main()
{
	fragColor = vec4(1.0, 1.0, 1.0, vFloat);
}
)";
	auto testShader = Shader::GLSLProgramObject("[GL-TestShader]");
	// testShader.Release() as part of the ~GLSLProgramObject() will delete GLSLShaderObject's
	testShader.AttachShaderObject(new Shader::GLSLShaderObject(GL_VERTEX_SHADER  , vsSrc));
	testShader.AttachShaderObject(new Shader::GLSLShaderObject(GL_FRAGMENT_SHADER, fsSrc));

	testShader.SetLogReporting(false); //no need to spam guinea pig shader errors
	testShader.Link();
	testShader.Enable();
	testShader.Disable();
	testShader.Validate();

	return testShader.IsValid();
#else
	return false;
#endif
}

int CGlobalRendering::DepthBitsToFormat(int bits)
{
	switch (bits)
	{
	case 16:
		return GL_DEPTH_COMPONENT16;
	case 24:
		return GL_DEPTH_COMPONENT24;
	case 32:
		return GL_DEPTH_COMPONENT32;
	default:
		return GL_DEPTH_COMPONENT; //should never hit this
	}
}

void CGlobalRendering::SetMinSampleShadingRate()
{
#ifndef HEADLESS
	if (!GLAD_GL_VERSION_4_0)
		return;

	if (msaaLevel > 0 && minSampleShadingRate > 0.0f) {
		// Enable sample shading
		glEnable(GL_SAMPLE_SHADING);
		glMinSampleShading(minSampleShadingRate);
	}
	else {
		glDisable(GL_SAMPLE_SHADING);
	}
#endif // !HEADLESS
}

bool CGlobalRendering::SetWindowMinMaximized(bool maximize) const
{
	static constexpr uint32_t mmFlags[] = {
		SDL_WINDOW_MINIMIZED,
		SDL_WINDOW_MAXIMIZED
	};
	if ((SDL_GetWindowFlags(sdlWindow) & mmFlags[maximize]) != 0)
		return false; //already in desired state

	if (maximize)
		SDL_MaximizeWindow(sdlWindow);
	else
		SDL_MinimizeWindow(sdlWindow);

	return (SDL_GetWindowFlags(sdlWindow) & mmFlags[maximize]) != 0;
}

/**
 * @brief multisample verify
 * @return whether verification passed
 *
 * Tests whether FSAA was actually enabled
 */
bool CGlobalRendering::CheckGLMultiSampling() const
{
	if (msaaLevel == 0)
		return false;
	if (!GLAD_GL_ARB_multisample)
		return false;

	GLint buffers = 0;
	GLint samples = 0;

	glGetIntegerv(GL_SAMPLE_BUFFERS, &buffers);
	glGetIntegerv(GL_SAMPLES, &samples);

	return (buffers != 0 && samples != 0);
}

bool CGlobalRendering::CheckGLContextVersion(const int2& minCtx) const
{
	#ifdef HEADLESS
	return true;
	#else
	int2 tmpCtx = {0, 0};

	glGetIntegerv(GL_MAJOR_VERSION, &tmpCtx.x);
	glGetIntegerv(GL_MINOR_VERSION, &tmpCtx.y);

	GLint profile = 0;
	glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile);

	if (profile != 0)
		globalRenderingInfo.glContextIsCore = (profile == GL_CONTEXT_CORE_PROFILE_BIT);
	else
		globalRenderingInfo.glContextIsCore = !GLAD_GL_ARB_compatibility;

	// keep this for convenience
	globalRenderingInfo.glContextVersion = tmpCtx;

	// compare major * 10 + minor s.t. 4.1 evaluates as larger than 3.2
	return ((tmpCtx.x * 10 + tmpCtx.y) >= (minCtx.x * 10 + minCtx.y));
	#endif
}



#if defined(_WIN32) && !defined(HEADLESS)
	#if defined(_MSC_VER) && _MSC_VER >= 1600
		#define _GL_APIENTRY __stdcall
	#else
		#include <windef.h>
		#define _GL_APIENTRY APIENTRY
	#endif
#else
	#define _GL_APIENTRY
#endif


#if (defined(GL_ARB_debug_output) && !defined(HEADLESS))

#ifndef GL_DEBUG_SOURCE_API
#define GL_DEBUG_SOURCE_API                GL_DEBUG_SOURCE_API_ARB
#define GL_DEBUG_SOURCE_WINDOW_SYSTEM      GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB
#define GL_DEBUG_SOURCE_SHADER_COMPILER    GL_DEBUG_SOURCE_SHADER_COMPILER_ARB
#define GL_DEBUG_SOURCE_THIRD_PARTY        GL_DEBUG_SOURCE_THIRD_PARTY_ARB
#define GL_DEBUG_SOURCE_APPLICATION        GL_DEBUG_SOURCE_APPLICATION_ARB
#define GL_DEBUG_SOURCE_OTHER              GL_DEBUG_SOURCE_OTHER_ARB

#define GL_DEBUG_TYPE_ERROR                GL_DEBUG_TYPE_ERROR_ARB
#define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR  GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB
#define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR   GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB
#define GL_DEBUG_TYPE_PORTABILITY          GL_DEBUG_TYPE_PORTABILITY_ARB
#define GL_DEBUG_TYPE_PERFORMANCE          GL_DEBUG_TYPE_PERFORMANCE_ARB
#if (defined(GL_DEBUG_TYPE_MARKER_ARB) && defined(GL_DEBUG_TYPE_PUSH_GROUP_ARB) && defined(GL_DEBUG_TYPE_POP_GROUP_ARB))
#define GL_DEBUG_TYPE_MARKER               GL_DEBUG_TYPE_MARKER_ARB
#define GL_DEBUG_TYPE_PUSH_GROUP           GL_DEBUG_TYPE_PUSH_GROUP_ARB
#define GL_DEBUG_TYPE_POP_GROUP            GL_DEBUG_TYPE_POP_GROUP_ARB
#else
#define GL_DEBUG_TYPE_MARKER               -1u
#define GL_DEBUG_TYPE_PUSH_GROUP           -2u
#define GL_DEBUG_TYPE_POP_GROUP            -3u
#endif
#define GL_DEBUG_TYPE_OTHER                GL_DEBUG_TYPE_OTHER_ARB

#define GL_DEBUG_SEVERITY_HIGH             GL_DEBUG_SEVERITY_HIGH_ARB
#define GL_DEBUG_SEVERITY_MEDIUM           GL_DEBUG_SEVERITY_MEDIUM_ARB
#define GL_DEBUG_SEVERITY_LOW              GL_DEBUG_SEVERITY_LOW_ARB

#define GL_DEBUG_OUTPUT_SYNCHRONOUS        GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB
#define GLDEBUGPROC                        GLDEBUGPROCARB
#endif

#ifndef glDebugMessageCallback
#define glDebugMessageCallback  glDebugMessageCallbackARB
#define glDebugMessageControl   glDebugMessageControlARB
#endif

constexpr static std::array<GLenum,  7> msgSrceEnums = {GL_DONT_CARE, GL_DEBUG_SOURCE_API, GL_DEBUG_SOURCE_WINDOW_SYSTEM, GL_DEBUG_SOURCE_SHADER_COMPILER, GL_DEBUG_SOURCE_THIRD_PARTY, GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_SOURCE_OTHER};
constexpr static std::array<GLenum, 10> msgTypeEnums = {GL_DONT_CARE, GL_DEBUG_TYPE_ERROR, GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR, GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR, GL_DEBUG_TYPE_PORTABILITY, GL_DEBUG_TYPE_PERFORMANCE, GL_DEBUG_TYPE_MARKER, GL_DEBUG_TYPE_PUSH_GROUP, GL_DEBUG_TYPE_POP_GROUP, GL_DEBUG_TYPE_OTHER};
constexpr static std::array<GLenum,  4> msgSevrEnums = {GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, GL_DEBUG_SEVERITY_MEDIUM, GL_DEBUG_SEVERITY_HIGH};

static inline const char* glDebugMessageSourceName(GLenum msgSrce) {
	switch (msgSrce) {
		case GL_DEBUG_SOURCE_API            : return             "API"; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM  : return   "WINDOW_SYSTEM"; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER: return "SHADER_COMPILER"; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY    : return     "THIRD_PARTY"; break;
		case GL_DEBUG_SOURCE_APPLICATION    : return     "APPLICATION"; break;
		case GL_DEBUG_SOURCE_OTHER          : return           "OTHER"; break;
		case GL_DONT_CARE                   : return       "DONT_CARE"; break;
		default                             :                         ; break;
	}

	return "UNKNOWN";
}

static inline const char* glDebugMessageTypeName(GLenum msgType) {
	switch (msgType) {
		case GL_DEBUG_TYPE_ERROR              : return       "ERROR"; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return  "DEPRECATED"; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR : return   "UNDEFINED"; break;
		case GL_DEBUG_TYPE_PORTABILITY        : return "PORTABILITY"; break;
		case GL_DEBUG_TYPE_PERFORMANCE        : return "PERFORMANCE"; break;
		case GL_DEBUG_TYPE_MARKER             : return      "MARKER"; break;
		case GL_DEBUG_TYPE_PUSH_GROUP         : return  "PUSH_GROUP"; break;
		case GL_DEBUG_TYPE_POP_GROUP          : return   "POP_GROUP"; break;
		case GL_DEBUG_TYPE_OTHER              : return       "OTHER"; break;
		case GL_DONT_CARE                     : return   "DONT_CARE"; break;
		default                               :                     ; break;
	}

	return "UNKNOWN";
}

static inline const char* glDebugMessageSeverityName(GLenum msgSevr) {
	switch (msgSevr) {
		case GL_DEBUG_SEVERITY_HIGH  : return      "HIGH"; break;
		case GL_DEBUG_SEVERITY_MEDIUM: return    "MEDIUM"; break;
		case GL_DEBUG_SEVERITY_LOW   : return       "LOW"; break;
		case GL_DONT_CARE            : return "DONT_CARE"; break;
		default                      :                   ; break;
	}

	return "UNKNOWN";
}

static void _GL_APIENTRY glDebugMessageCallbackFunc(
	GLenum msgSrce,
	GLenum msgType,
	GLuint msgID,
	GLenum msgSevr,
	GLsizei length,
	const GLchar* dbgMessage,
	const GLvoid* userParam
) {
	switch (msgID) {
		case 131169: { return; } break; // "Framebuffer detailed info: The driver allocated storage for renderbuffer N."
		case 131185: { return; } break; // "Buffer detailed info: Buffer object 260 (bound to GL_PIXEL_UNPACK_BUFFER_ARB, usage hint is GL_STREAM_DRAW) has been mapped in DMA CACHED memory."
		default: {} break;
	}

	const char* msgSrceStr = glDebugMessageSourceName(msgSrce);
	const char* msgTypeStr = glDebugMessageTypeName(msgType);
	const char* msgSevrStr = glDebugMessageSeverityName(msgSevr);

	LOG_L(L_WARNING, "[OPENGL_DEBUG] id=%u source=%s type=%s severity=%s msg=\"%s\"", msgID, msgSrceStr, msgTypeStr, msgSevrStr, dbgMessage);

	if ((userParam == nullptr) || !(*reinterpret_cast<const bool*>(userParam)))
		return;

	CrashHandler::PrepareStacktrace();
	CrashHandler::Stacktrace(Threading::GetCurrentThread(), "rendering", LOG_LEVEL_WARNING);
	CrashHandler::CleanupStacktrace();
}
#endif


bool CGlobalRendering::ToggleGLDebugOutput(unsigned int msgSrceIdx, unsigned int msgTypeIdx, unsigned int msgSevrIdx) const
{
#if (defined(GL_ARB_debug_output) && !defined(HEADLESS))
	if (!(GLAD_GL_ARB_debug_output || GLAD_GL_KHR_debug))
		return false;

	if (glDebug) {
		const char* msgSrceStr = glDebugMessageSourceName(msgSrceEnums[msgSrceIdx %= msgSrceEnums.size()]);
		const char* msgTypeStr = glDebugMessageTypeName(msgTypeEnums[msgTypeIdx %= msgTypeEnums.size()]);
		const char* msgSevrStr = glDebugMessageSeverityName(msgSevrEnums[msgSevrIdx %= msgSevrEnums.size()]);

		const static bool dbgTraces = configHandler->GetBool("DebugGLStacktraces");
		// install OpenGL debug message callback; typecast is a workaround
		// for #4510 (change in callback function signature with GLEW 1.11)
		// use SYNCHRONOUS output, we want our callback to run in the same
		// thread as the bugged GL call (for proper stacktraces)
		// CB userParam is const, but has to be specified sans qualifiers
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback((GLDEBUGPROC)&glDebugMessageCallbackFunc, (void*)&dbgTraces);
		glDebugMessageControl(msgSrceEnums[msgSrceIdx], msgTypeEnums[msgTypeIdx], msgSevrEnums[msgSevrIdx], 0, nullptr, GL_TRUE);

		LOG("[GR::%s] OpenGL debug-message callback enabled (source=%s type=%s severity=%s)", __func__, msgSrceStr, msgTypeStr, msgSevrStr);
	}
	else {
		glDebugMessageCallback(nullptr, nullptr);
		glDisable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

		LOG("[GR::%s] OpenGL debug-message callback disabled", __func__);
	}
	configHandler->Set("DebugGL", globalRendering->glDebug);
#endif
	return true;
}

void CGlobalRendering::LoadViewport()
{
	glViewport(viewPosX, viewPosY, viewSizeX, viewSizeY);
}

void CGlobalRendering::LoadDualViewport()
{
	glViewport(dualViewPosX, dualViewPosY, dualViewSizeX, dualViewSizeY);
}
