#pragma once

struct SDL_Window;

// glad/glad_glx.h loads X11.h, which has conflicting definitions with Recoil (such as KeyPressed)
// thus move it to standalone file here
struct GLX {
	static void Load(SDL_Window* window);
	static void Unload();
	static bool GetVideoMemInfoMESA(int* memInfo);
private:
	static inline bool supported = false;
};