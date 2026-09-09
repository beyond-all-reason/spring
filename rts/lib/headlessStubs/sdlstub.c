/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/*
 * documentation for the functions in this file can be found at:
 * https://wiki.libsdl.org/SDL3/
 */

#include <SDL3/SDL.h>

#ifndef SDL_INIT_EVERYTHING
#define SDL_INIT_EVERYTHING (SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMEPAD | SDL_INIT_SENSOR | SDL_INIT_HAPTIC)
#endif

#ifndef SDL_HAS_KEYREPEAT
#define SDL_HAS_KEYREPEAT
typedef enum SDL_KeyRepeat {
	SDL_KEY_REPEAT_OFF,
	SDL_KEY_REPEAT_INITIAL_DELAY,
	SDL_KEY_REPEAT
} SDL_KeyRepeat;
#endif


#ifdef __cplusplus
extern "C" {
#endif


static struct SDL_Surface stubSurface;
static bool stubKeyState[1];
static int stubVersionInt = SDL_VERSION;
static Uint32 stubSubSystemsInit = 0;


void SDL_free(void* p) {
	return;
}

bool SDL_Init(Uint32 flags) {

	stubSurface.w = 512;
	stubSurface.h = 512;
	stubSubSystemsInit = SDL_INIT_EVERYTHING;

	return true;
}

Uint32 SDL_WasInit(Uint32 flags) {
	return (stubSubSystemsInit & flags);
}

bool SDL_InitSubSystem(Uint32 flags) {
	return true;
}

void SDL_QuitSubSystem(Uint32 flags) {
}

const char* SDL_GetError(void) {
	return "using the SDL stub library";
}

bool SDL_GL_SetAttribute(SDL_GLAttr attr, int value) {
	return true;
}

SDL_Window* SDL_CreateWindow(const char* title, int w, int h, SDL_WindowFlags flags) {
	static int foo;
	return (SDL_Window*)(&foo);
}

void SDL_DestroyWindow(SDL_Window * window) {}
bool SDL_MinimizeWindow(SDL_Window * window) { return true; }
bool SDL_MaximizeWindow(SDL_Window * window) { return true; }
bool SDL_RestoreWindow(SDL_Window * window) { return true; }

bool SDL_GL_MakeCurrent(SDL_Window * window, SDL_GLContext context){
	return true;
}

const char *SDL_GetWindowTitle(SDL_Window * window) {
	return "";
}

Uint32 SDL_GetWindowPixelFormat(SDL_Window* window) { return 0; }
const char* SDL_GetPixelFormatName(Uint32 format) { return ""; }

struct SDL_IOStream* SDL_IOFromFile(const char* file, const char* mode) {
	return NULL;
}

SDL_Surface* SDL_LoadBMP_IO(SDL_IOStream* src, bool closeio) {
	return &stubSurface;
}

void SDL_Quit(void) {
}

SDL_Surface* SDL_CreateSurface(int width, int height, SDL_PixelFormat format) {
	return NULL;
}

SDL_Surface* SDL_CreateSurfaceFrom(int width, int height, SDL_PixelFormat format, void* pixels, int pitch) {
	return NULL;
}

void SDL_DestroySurface(SDL_Surface* surface) {
}

bool SDL_GL_SwapWindow(SDL_Window* window) {
	return true;
}

bool SDL_SetWindowRelativeMouseMode(SDL_Window* window, bool enabled) { return true; }
void SDL_WarpMouseInWindow(SDL_Window* window, float x, float y) {
}

bool SDL_HideWindow(SDL_Window* window) {
	return true;
}

bool SDL_SetWindowFullscreen(SDL_Window* window, bool fullscreen) {
	return true;
}

bool SDL_SetWindowFullscreenMode(SDL_Window* window, const SDL_DisplayMode* mode) {
	return true;
}

bool SDL_GetWindowBordersSize(SDL_Window* window,
	int* top, int* left,
	int* bottom, int* right) {

	if (top)
		*top = 0;

	if (left)
		*left = 0;

	if (bottom)
		*bottom = 0;

	if (right)
		*right = 0;

	return true;
}

bool SDL_SetWindowBordered(SDL_Window* window, bool bordered) {
	return true;
}

bool SDL_SetKeyRepeat(SDL_KeyRepeat repeat, int delay, int interval) {
	return true;
}

void SDL_SetModState(SDL_Keymod modstate) {
}

bool SDL_PollEvent(SDL_Event* event) { return false; }
bool SDL_PushEvent(SDL_Event* event) { return false; }

void SDL_FlushEvents(Uint32 minType, Uint32 maxType) {}
void SDL_PumpEvents(void) {}

bool SDL_SetWindowTitle(SDL_Window* window, const char* title) {
	return true;
}

bool SDL_SetWindowIcon(SDL_Window* window, SDL_Surface* icon) {
	return true;
}

bool SDL_SetWindowMinimumSize(SDL_Window* window, int min_w, int min_h) {
	return true;
}

bool SDL_GL_GetAttribute(SDL_GLAttr attr, int* value) {
	*value = 0;
	return true;
}


SDL_GLContext SDL_GL_CreateContext(SDL_Window* window) {
	static int foo;
	return (SDL_GLContext)&foo;
}

bool SDL_GL_DestroyContext(SDL_GLContext context) {
	return true;
}


bool SDL_SetWindowMouseGrab(SDL_Window* window, bool grabbed) {
	return true;
}

bool SDL_GetWindowMouseGrab(SDL_Window* window) {
	return false;
}

SDL_WindowFlags SDL_GetWindowFlags(SDL_Window* window) {
	return (SDL_WindowFlags)0;
}

bool SDL_EnableScreenSaver(void) { return true; }
bool SDL_DisableScreenSaver(void) { return true; }

char* SDL_GetClipboardText(void) {
	static char empty[] = "";
	return empty;
}

bool SDL_SetClipboardText(const char* text) {
	return true;
}

const bool *SDL_GetKeyboardState(int* numkeys) {
	if (numkeys)
		*numkeys = 0;
	return stubKeyState;
}

SDL_Keymod SDL_GetModState(void) {
	return 0;
}

SDL_Keycode SDL_GetKeyFromScancode(SDL_Scancode scancode, SDL_Keymod modstate, bool key_event) {
	return 0;
}

SDL_Scancode SDL_GetScancodeFromKey(SDL_Keycode key, SDL_Keymod *modstate) {
	return 0;
}

const char* SDL_GetScancodeName(SDL_Scancode scancode) {
	return "";
}

SDL_Scancode SDL_GetScancodeFromName(const char* name) {
	return 0;
}

int SDL_GetVersion(void) {
	return stubVersionInt;
}

const char* SDL_GetCurrentVideoDriver(void) {
	return "headless stub";
}

bool SDL_ShowCursor(void) {
	return true;
}

bool SDL_HideCursor(void) {
	return true;
}

bool SDL_CursorVisible(void) {
	return false;
}

Uint32 SDL_GetMouseState(float* x, float* y) {
	if (x) *x = 0.0f;
	if (y) *y = 0.0f;
	return 0;
}

SDL_JoystickID* SDL_GetJoysticks(int* count) {
	if (count) *count = 0;
	return NULL;
}

const char* SDL_GetJoystickName(SDL_Joystick* joystick) {
	return "";
}

SDL_Joystick* SDL_OpenJoystick(SDL_JoystickID joystick) {
	return NULL;
}

void SDL_CloseJoystick(SDL_Joystick* joystick) {
}

SDL_JoystickID SDL_GetJoystickID(SDL_Joystick* joystick) {
	return 0;
}

bool SDL_IsGamepad(SDL_JoystickID joystickId) {
	return false;
}

SDL_Gamepad *SDL_OpenGamepad(SDL_JoystickID joystickId) {
	return NULL;
}

void SDL_CloseGamepad(SDL_Gamepad *gamepad) {
}

const char *SDL_GetGamepadName(SDL_Gamepad *gamepad) {
	return "";
}

SDL_Joystick *SDL_GetGamepadJoystick(SDL_Gamepad *gamepad) {
	return NULL;
}

bool SDL_GamepadHasButton(SDL_Gamepad *gamepad,
														 SDL_GamepadButton button) {
	return false;
}

bool SDL_GamepadHasAxis(SDL_Gamepad *gamepad, SDL_GamepadAxis axis) {
	return false;
}

bool SDL_GetGamepadButton(SDL_Gamepad *gamepad,
															  SDL_GamepadButton button) {
	return false;
}

Sint16 SDL_GetGamepadAxis(SDL_Gamepad *gamepad, SDL_GamepadAxis axis) {
	return 0;
}

const char* SDL_GetGamepadStringForAxis(SDL_GamepadAxis axis) {
	return "";
}

const char* SDL_GetGamepadStringForButton(SDL_GamepadButton button) {
	return "";
}

char *SDL_GetGamepadMapping(SDL_Gamepad *gamepad) {
	static char empty[] = "";
	return empty;
}

SDL_Gamepad *SDL_GetGamepadFromID(SDL_JoystickID joyid) {
	return NULL;
}

const SDL_DisplayMode* SDL_GetCurrentDisplayMode(SDL_DisplayID displayID) {
	static SDL_DisplayMode stubMode = {0};
	stubMode.format = SDL_PIXELFORMAT_RGB24;
	stubMode.w = 640;
	stubMode.h = 480;
	stubMode.refresh_rate = 100;
	return &stubMode;
}

const SDL_DisplayMode* SDL_GetWindowFullscreenMode(SDL_Window* window) {
	static SDL_DisplayMode stubMode = {0};
	stubMode.format = SDL_PIXELFORMAT_RGB24;
	stubMode.w = 640;
	stubMode.h = 480;
	stubMode.refresh_rate = 100;
	return &stubMode;
}

SDL_DisplayID SDL_GetDisplayForWindow(SDL_Window* window) {
	return 0;
}

SDL_DisplayID* SDL_GetDisplays(int* count) {
	if (count) *count = 0;
	return NULL;
}

SDL_DisplayMode** SDL_GetFullscreenDisplayModes(SDL_DisplayID display, int* count) {
	if (count) *count = 0;
	return NULL;
}

bool SDL_GetDisplayBounds(SDL_DisplayID display, SDL_Rect* rect) {
	if (rect == NULL) return false;
	rect->w = 640;
	rect->h = 480;
	rect->x = 0;
	rect->y = 0;
	return true;
}

bool SDL_GetDisplayUsableBounds(SDL_DisplayID display, SDL_Rect* rect) {
	if (rect == NULL) return false;
	rect->w = 640;
	rect->h = 480;
	rect->x = 0;
	rect->y = 0;
	return true;
}

const char* SDL_GetDisplayName(SDL_DisplayID display) {
	return "";
}

SDL_PropertiesID SDL_GetWindowProperties(SDL_Window* window) {
	return 0;
}

void* SDL_GetPointerProperty(SDL_PropertiesID props, const char* name, void* defaultValue) {
	return defaultValue;
}

int64_t SDL_GetNumberProperty(SDL_PropertiesID props, const char* name, int64_t defaultValue) {
	return defaultValue;
}

int SDL_PeepEvents(SDL_Event* events, int numevents, SDL_EventAction action, Uint32 minType, Uint32 maxType) {
	return 0;
}

bool SDL_HasRectIntersection(const SDL_Rect * A, const SDL_Rect * B) {
	return true;
}

bool SDL_GetRectIntersection(const SDL_Rect * A, const SDL_Rect * B, SDL_Rect * result) {
	return true;
}

bool SDL_GL_GetSwapInterval(int* interval) {
	if (interval) *interval = 0;
	return true;
}

bool SDL_GL_SetSwapInterval(int interval) {
	return true;
}

bool SDL_SetWindowPosition(SDL_Window * window, int x, int y) {
	return true;
}

bool SDL_SetWindowSize(SDL_Window * window, int w, int h) {
	return true;
}


SDL_PowerState SDL_GetPowerInfo(int *secs, int *pct) {
	return SDL_POWERSTATE_UNKNOWN;
}

bool SDL_SetHint(const char* name, const char* value) {
	return true;
}

bool SDL_SetTextInputArea(SDL_Window* window, const SDL_Rect* rect, int cursor) {
	return true;
}

bool SDL_StartTextInput(SDL_Window* window) {
	return true;
}

bool SDL_StopTextInput(SDL_Window* window) {
	return true;
}

bool SDL_CaptureMouse(bool enabled) {
	return true;
}

bool SDL_SetSurfaceAlphaMod(SDL_Surface* surface, Uint8 alpha) {
	return true;
}
bool SDL_SetSurfaceBlendMode(SDL_Surface* surface, SDL_BlendMode mode) {
	return true;
}

bool SDL_BlitSurface(SDL_Surface* src, const SDL_Rect* srcrect, SDL_Surface* dst, const SDL_Rect* dstrect) {
	return true;
}

bool SDL_PauseAudioDevice(SDL_AudioDeviceID devid) {
	return true;
}

bool SDL_ResumeAudioDevice(SDL_AudioDeviceID devid) {
	return true;
}

SDL_AudioDeviceID* SDL_GetAudioPlaybackDevices(int *count) {
	if (count)
		*count = 0;
	return NULL;
}

const char* SDL_GetAudioDeviceName(SDL_AudioDeviceID devid) {
	return NULL;
}

SDL_AudioStream* SDL_OpenAudioDeviceStream(SDL_AudioDeviceID devid, const SDL_AudioSpec* spec, SDL_AudioStreamCallback callback, void* userdata) {
	return NULL;
}

void SDL_DestroyAudioStream(SDL_AudioStream *stream) {
}

SDL_AudioDeviceID SDL_GetAudioStreamDevice(SDL_AudioStream *stream) {
	return 0;
}

bool SDL_GetAudioStreamFormat(SDL_AudioStream *stream, SDL_AudioSpec *src_spec, SDL_AudioSpec *dst_spec) {
	return false;
}

bool SDL_PutAudioStreamData(SDL_AudioStream *stream, const void* buf, int len) {
	return true;
}

#ifdef __cplusplus
} // extern "C"
#endif
