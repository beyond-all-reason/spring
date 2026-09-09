/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Platform/WindowManagerHelper.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_video.h>

#include <windows.h>
#include <vector>


namespace WindowManagerHelper {

AdvancedColorInfo GetAdvancedColorInfo(uint32_t displayID)
{
	AdvancedColorInfo result;
#if !defined(HEADLESS)
	SDL_Rect displayBounds = {};
	if (!SDL_GetDisplayBounds(displayID, &displayBounds))
		return result;
	const POINT displayCenter = {
		displayBounds.x + displayBounds.w / 2,
		displayBounds.y + displayBounds.h / 2,
	};
	HMONITOR monitor = MonitorFromPoint(displayCenter, MONITOR_DEFAULTTONULL);
	if (monitor == nullptr)
		return result;

	MONITORINFOEXW monitorInfo = {};
	monitorInfo.cbSize = sizeof(monitorInfo);
	if (!GetMonitorInfoW(monitor, &monitorInfo))
		return result;

	UINT32 pathCount = 0;
	UINT32 modeCount = 0;
	if (GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &pathCount, &modeCount) != ERROR_SUCCESS)
		return result;

	std::vector<DISPLAYCONFIG_PATH_INFO> paths(pathCount);
	std::vector<DISPLAYCONFIG_MODE_INFO> modes(modeCount);
	if (QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &pathCount, paths.data(), &modeCount, modes.data(), nullptr) != ERROR_SUCCESS)
		return result;

	for (UINT32 index = 0; index < pathCount; ++index) {
		DISPLAYCONFIG_SOURCE_DEVICE_NAME source = {};
		source.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
		source.header.size = sizeof(source);
		source.header.adapterId = paths[index].sourceInfo.adapterId;
		source.header.id = paths[index].sourceInfo.id;
		if (DisplayConfigGetDeviceInfo(&source.header) != ERROR_SUCCESS)
			continue;
		if (_wcsicmp(source.viewGdiDeviceName, monitorInfo.szDevice) != 0)
			continue;

		DISPLAYCONFIG_GET_ADVANCED_COLOR_INFO colorInfo = {};
		colorInfo.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_ADVANCED_COLOR_INFO;
		colorInfo.header.size = sizeof(colorInfo);
		colorInfo.header.adapterId = paths[index].targetInfo.adapterId;
		colorInfo.header.id = paths[index].targetInfo.id;
		if (DisplayConfigGetDeviceInfo(&colorInfo.header) != ERROR_SUCCESS)
			return result;

		result.available = true;
		result.supported = colorInfo.advancedColorSupported;
		result.enabled = colorInfo.advancedColorEnabled;
		return result;
	}
#else
	(void)displayID;
#endif
	return result;
}

void BlockCompositing(SDL_Window* window)
{
#ifndef HEADLESS
	// only available in WinVista+
	HMODULE dwmapiDllHandle = LoadLibrary(L"dwmapi.dll");

	if (dwmapiDllHandle != nullptr) {
		using DwmEnableCompositionFunction = HRESULT(*)(UINT uCompositionAction);
		auto DwmEnableComposition = (DwmEnableCompositionFunction) ::GetProcAddress(dwmapiDllHandle, "DwmEnableComposition");
		if (DwmEnableComposition != nullptr) {
			static const unsigned int DWM_EC_DISABLECOMPOSITION = 0U;
			DwmEnableComposition(DWM_EC_DISABLECOMPOSITION);
		}

		FreeLibrary(dwmapiDllHandle);
	}
#endif
}


int GetWindowState(SDL_Window* window)
{
	int state = 0;
#ifndef HEADLESS
	WINDOWPLACEMENT wp;
	wp.length = sizeof(WINDOWPLACEMENT);

	const SDL_PropertiesID props = SDL_GetWindowProperties(window);
	HWND hwnd = (HWND)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
	if (!hwnd)
		return 0;

	if (GetWindowPlacement(hwnd, &wp)) {
		if (wp.showCmd == SW_SHOWMAXIMIZED)
			state = SDL_WINDOW_MAXIMIZED;
		if (wp.showCmd == SW_SHOWMINIMIZED)
			state = SDL_WINDOW_MINIMIZED;
	}
#endif
	return state;
}


// taken from http://stackoverflow.com/questions/27116152
void SetWindowResizable(SDL_Window* window, bool resizable)
{
#ifndef HEADLESS
	const SDL_PropertiesID props = SDL_GetWindowProperties(window);
	HWND hwnd = (HWND)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
	if (!hwnd)
		return;

	DWORD style = GetWindowLong(hwnd, GWL_STYLE);
	if (resizable) {
		style |= (WS_THICKFRAME | WS_MAXIMIZEBOX);
	} else {
		style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
	}
	SetWindowLong(hwnd, GWL_STYLE, style);
#endif
}

}; // namespace WindowManagerHelper
