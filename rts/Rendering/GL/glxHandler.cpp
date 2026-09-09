#include "glxHandler.h"

#if !defined(HEADLESS) && !defined(_WIN32) && !defined(__APPLE__)

#include <SDL3/SDL_video.h>
#include <glad/glad_glx.h>

void GLX::Load(SDL_Window* window)
{
	supported = false;

	const SDL_PropertiesID props = SDL_GetWindowProperties(window);
	Display* display = (Display*)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
	if (display) {
		supported = static_cast<bool>(gladLoadGLX(display, DefaultScreen(display)));
	}
	// Wayland: gladLoadGLX not applicable
}

void GLX::Unload()
{
	if (!supported)
		return;
	
	gladUnloadGLX();
}

bool GLX::GetVideoMemInfoMESA(int* memInfo)
{
	if (!supported)
		return false;

#if (defined(GLX_MESA_query_renderer))
	if (!GLAD_GLX_MESA_query_renderer)
		return false;

	// note: unlike the others, this value is returned in megabytes
	glad_glXQueryCurrentRendererIntegerMESA(GLX_RENDERER_VIDEO_MEMORY_MESA, reinterpret_cast<unsigned int*>(&memInfo[0]));

	memInfo[0] *= 1024;
	memInfo[1] = memInfo[0];
	return true;
#else
	return false;
#endif
}
#else
void GLX::Load(SDL_Window* window) {}
void GLX::Unload() {}
bool GLX::GetVideoMemInfoMESA(int* memInfo) { return false; }
#endif