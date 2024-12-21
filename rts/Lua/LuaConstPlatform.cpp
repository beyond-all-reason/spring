/* This file is part of the Spring System (GPL v2 or later), see LICENSE.html */

#include "LuaConstPlatform.h"
#include "LuaUtils.h"
#include "System/Platform/Misc.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GlobalRenderingInfo.h"

/******************************************************************************
 * Platform constants
 * @module Platform
 * @see rts/Lua/LuaConstPlatform.cpp
******************************************************************************/

/*** Platform specific information
 *
 * @table Platform
 * @string gpu full GPU device name
 * @string gpuVendor one of "Nvidia", "Intel", "ATI", "Mesa", "Unknown"
 * @string glVersionShort major.minor.buildNumber
 * @string glslVersionShort major.minor
 * @string glVersion full version
 * @string glVendor
 * @string glRenderer
 * @string glslVersion full version
 * @string glewVersion
 * @string osName full name of the OS
 * @string osFamily one of "Windows", "Linux", "MacOSX", "FreeBSD", "Unknown"
 * @number numDisplays
 * @number gpuMemorySize size of total GPU memory in MBs; only available for "Nvidia", (rest are 0)
 * @number sdlVersionCompiledMajor
 * @number sdlVersionCompiledMinor
 * @number sdlVersionCompiledPatch
 * @number sdlVersionLinkedMajor
 * @number sdlVersionLinkedMinor
 * @number sdlVersionLinkedPatch
 * @bool glSupportNonPowerOfTwoTex
 * @bool glSupportTextureQueryLOD
 * @bool glSupport24bitDepthBuffer
 * @bool glSupportRestartPrimitive
 * @bool glSupportClipSpaceControl
 * @bool glSupportFragDepthLayout
 */

bool LuaConstPlatform::PushEntries(lua_State* L)
{
	LuaPushNamedString(L, "gpu", globalRenderingInfo.gpuName);
	LuaPushNamedString(L, "gpuVendor", globalRenderingInfo.gpuVendor);
	LuaPushNamedNumber(L, "gpuMemorySize", globalRenderingInfo.gpuMemorySize.x);
	LuaPushNamedString(L, "glVersionShort", globalRenderingInfo.glVersionShort.data());
	LuaPushNamedString(L, "glslVersionShort", globalRenderingInfo.glslVersionShort.data());
	LuaPushNamedNumber(L, "glslVersionNum", globalRenderingInfo.glslVersionNum);

	LuaPushNamedString(L, "glVersion", globalRenderingInfo.glVersion);
	LuaPushNamedString(L, "glVendor", globalRenderingInfo.glVendor);
	LuaPushNamedString(L, "glRenderer", globalRenderingInfo.glRenderer);
	LuaPushNamedString(L, "glslVersion", globalRenderingInfo.glslVersion);
	LuaPushNamedString(L, "glewVersion", globalRenderingInfo.glewVersion);

	LuaPushNamedNumber(L, "sdlVersionCompiledMajor", globalRenderingInfo.sdlVersionCompiled.major);
	LuaPushNamedNumber(L, "sdlVersionCompiledMinor", globalRenderingInfo.sdlVersionCompiled.minor);
	LuaPushNamedNumber(L, "sdlVersionCompiledPatch", globalRenderingInfo.sdlVersionCompiled.patch);
	LuaPushNamedNumber(L, "sdlVersionLinkedMajor", globalRenderingInfo.sdlVersionLinked.major);
	LuaPushNamedNumber(L, "sdlVersionLinkedMinor", globalRenderingInfo.sdlVersionLinked.minor);
	LuaPushNamedNumber(L, "sdlVersionLinkedPatch", globalRenderingInfo.sdlVersionLinked.patch);

	lua_pushstring(L, "availableVideoModes");
	lua_createtable(L, 0, globalRenderingInfo.availableVideoModes.size());
	for (int i = 0; i < globalRenderingInfo.availableVideoModes.size(); ++i) {
		lua_pushnumber(L, i + 1);
		lua_createtable(L, 0, 6);

		const auto& avm = globalRenderingInfo.availableVideoModes[i];

		LuaPushNamedNumber(L, "display", avm.displayIndex);
		LuaPushNamedString(L, "displayName", avm.displayName);
		LuaPushNamedNumber(L, "w", avm.width);
		LuaPushNamedNumber(L, "h", avm.height);
		LuaPushNamedNumber(L, "bpp", avm.bpp);
		LuaPushNamedNumber(L, "hz", avm.refreshRate);

		lua_rawset(L, -3);
	}
	lua_rawset(L, -3);

	LuaPushNamedNumber(L, "numDisplays", globalRendering->numDisplays);

	LuaPushNamedBool(L, "glSupportNonPowerOfTwoTex", true);
	LuaPushNamedBool(L, "glSupportTextureQueryLOD" , globalRendering->supportTextureQueryLOD);
	LuaPushNamedBool(L, "glSupportMSAAFrameBuffer" , globalRendering->supportMSAAFrameBuffer);

	LuaPushNamedBool(L, "glHaveAMD", globalRendering->haveAMD);
	LuaPushNamedBool(L, "glHaveNVidia", globalRendering->haveNvidia);
	LuaPushNamedBool(L, "glHaveIntel", globalRendering->haveIntel);

	LuaPushNamedBool(L, "glHaveGLSL", true);
	LuaPushNamedBool(L, "glHaveGL4", globalRendering->haveGL4);

	LuaPushNamedNumber(L, "glSupportDepthBufferBitDepth", globalRendering->supportDepthBufferBitDepth);

	LuaPushNamedBool(L, "glSupportRestartPrimitive", globalRendering->supportRestartPrimitive);
	LuaPushNamedBool(L, "glSupportClipSpaceControl", globalRendering->supportClipSpaceControl);
	LuaPushNamedBool(L, "glSupportFragDepthLayout" , globalRendering->supportFragDepthLayout);
	LuaPushNamedBool(L, "glSupportSeamlessCubeMaps", globalRendering->supportSeamlessCubeMaps);

	LuaPushNamedString(L, "osName", Platform::GetOSNameStr());
	LuaPushNamedString(L, "osVersion", Platform::GetOSVersionStr());
	LuaPushNamedString(L, "osFamily", Platform::GetOSFamilyStr());
	LuaPushNamedString(L, "hwConfig", Platform::GetHardwareStr());
	LuaPushNamedNumber(L, "cpuLogicalCores", Threading::GetLogicalCpuCores());
	LuaPushNamedNumber(L, "cpuPhysicalCores", Threading::GetPhysicalCpuCores());
	LuaPushNamedString(L, "cpuBrand", Threading::GetCPUBrand());

	LuaPushNamedString(L, "sysInfoHash", Platform::GetSysInfoHash());
	LuaPushNamedString(L, "macAddrHash", Platform::GetMacAddrHash());

	return true;
}
