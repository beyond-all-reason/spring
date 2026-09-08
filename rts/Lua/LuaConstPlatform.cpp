/* This file is part of the Spring System (GPL v2 or later), see LICENSE.html */

#include "LuaConstPlatform.h"
#include "LuaUtils.h"
#include "Game/GameVersion.h"
#include "System/Platform/Hardware.h"
#include "System/Platform/Misc.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GlobalRenderingInfo.h"

/******************************************************************************
 * Platform constants
 * @see rts/Lua/LuaConstPlatform.cpp
******************************************************************************/

/*** Platform specific information
 *
 * @table Platform
 */

bool LuaConstPlatform::PushEntries(lua_State* L)
{
	/*** @field Platform.gpu string Full GPU device name */
	LuaPushNamedString(L, "gpu", globalRenderingInfo.gpuName);
	/*** @field Platform.gpuVendor "Nvidia"|"Intel"|"ATI"|"Mesa"|"Unknown" */
	LuaPushNamedString(L, "gpuVendor", globalRenderingInfo.gpuVendor);
	/*** @field Platform.gpuMemorySize integer Size of total GPU memory in MBs; only available for "Nvidia", (rest are 0) */
	LuaPushNamedNumber(L, "gpuMemorySize", globalRenderingInfo.gpuMemorySize.x);
	/*** @field Platform.glVersionShort string `major.minor.buildNumber` */
	LuaPushNamedString(L, "glVersionShort", globalRenderingInfo.glVersionShort.data());
	/*** @field Platform.glVersionNum integer */
	LuaPushNamedNumber(L, "glVersionNum", globalRenderingInfo.glVersionNum);
	/*** @field Platform.glslVersionShort string `major.minor` */
	LuaPushNamedString(L, "glslVersionShort", globalRenderingInfo.glslVersionShort.data());
	/*** @field Platform.glslVersionNum string */
	LuaPushNamedNumber(L, "glslVersionNum", globalRenderingInfo.glslVersionNum);

	/*** @field Platform.glVersion string Full version */
	LuaPushNamedString(L, "glVersion", globalRenderingInfo.glVersion);
	/*** @field Platform.glVendor string */
	LuaPushNamedString(L, "glVendor", globalRenderingInfo.glVendor);
	/*** @field Platform.glRenderer string */
	LuaPushNamedString(L, "glRenderer", globalRenderingInfo.glRenderer);
	/*** @field Platform.glslVersion string Full version */
	LuaPushNamedString(L, "glslVersion", globalRenderingInfo.glslVersion);
	/*** @field Platform.gladVersion string */
	LuaPushNamedString(L, "gladVersion", globalRenderingInfo.gladVersion);
	/*** @field Platform.glewVersion string */
	LuaPushNamedString(L, "glewVersion", globalRenderingInfo.gladVersion);

	/*** @field Platform.sdlVersionCompiledMajor integer */
	LuaPushNamedNumber(L, "sdlVersionCompiledMajor", globalRenderingInfo.sdlVersionCompiled.major);
	/*** @field Platform.sdlVersionCompiledMinor integer */
	LuaPushNamedNumber(L, "sdlVersionCompiledMinor", globalRenderingInfo.sdlVersionCompiled.minor);
	/*** @field Platform.sdlVersionCompiledPatch integer */
	LuaPushNamedNumber(L, "sdlVersionCompiledPatch", globalRenderingInfo.sdlVersionCompiled.patch);
	/*** @field Platform.sdlVersionLinkedMajor integer */
	LuaPushNamedNumber(L, "sdlVersionLinkedMajor", globalRenderingInfo.sdlVersionLinked.major);
	/*** @field Platform.sdlVersionLinkedMinor integer */
	LuaPushNamedNumber(L, "sdlVersionLinkedMinor", globalRenderingInfo.sdlVersionLinked.minor);
	/*** @field Platform.sdlVersionLinkedPatch integer */
	LuaPushNamedNumber(L, "sdlVersionLinkedPatch", globalRenderingInfo.sdlVersionLinked.patch);

	/*** @field Platform.availableVideoModes PlatformVideoMode[] */
	lua_pushstring(L, "availableVideoModes");
	lua_createtable(L, 0, globalRenderingInfo.availableVideoModes.size());
	for (int i = 0; i < globalRenderingInfo.availableVideoModes.size(); ++i) {
		lua_pushnumber(L, i + 1);
		lua_createtable(L, 0, 6);

		const auto& avm = globalRenderingInfo.availableVideoModes[i];
		/***
		* @class PlatformVideoMode
		*/

		/*** @field PlatformVideoMode.display integer */
		LuaPushNamedNumber(L, "display", avm.displayIndex);
		/*** @field PlatformVideoMode.displayName string */
		LuaPushNamedString(L, "displayName", avm.displayName);
		/*** @field PlatformVideoMode.w integer */
		LuaPushNamedNumber(L, "w", avm.width);
		/*** @field PlatformVideoMode.h integer */
		LuaPushNamedNumber(L, "h", avm.height);
		/*** @field PlatformVideoMode.bpp integer */
		LuaPushNamedNumber(L, "bpp", avm.bpp);
		/*** @field PlatformVideoMode.hz integer */
		LuaPushNamedNumber(L, "hz", avm.refreshRate);

		lua_rawset(L, -3);
	}
	lua_rawset(L, -3);

	/*** @field Platform.numDisplays integer */
	LuaPushNamedNumber(L, "numDisplays", globalRendering->numDisplays);

	/*** @field Platform.glSupportNonPowerOfTwoTex boolean */
	LuaPushNamedBool(L, "glSupportNonPowerOfTwoTex", true);
	/*** @field Platform.glSupportTextureQueryLOD boolean */
	LuaPushNamedBool(L, "glSupportTextureQueryLOD" , globalRendering->supportTextureQueryLOD);
	/*** @field Platform.glSupportMSAAFrameBuffer boolean */
	LuaPushNamedBool(L, "glSupportMSAAFrameBuffer" , globalRendering->supportMSAAFrameBuffer);

	/*** @field Platform.glHaveAMD boolean */
	LuaPushNamedBool(L, "glHaveAMD", globalRendering->haveAMD);
	/*** @field Platform.glHaveNVidia boolean */
	LuaPushNamedBool(L, "glHaveNVidia", globalRendering->haveNvidia);
	/*** @field Platform.glHaveIntel boolean */
	LuaPushNamedBool(L, "glHaveIntel", globalRendering->haveIntel);

	/*** @field Platform.glHaveGLSL boolean */
	LuaPushNamedBool(L, "glHaveGLSL", true);
	/*** @field Platform.glHaveGL4 boolean */
	LuaPushNamedBool(L, "glHaveGL4", globalRendering->haveGL4);

	/*** @field Platform.glSupportDepthBufferBitDepth integer */
	LuaPushNamedNumber(L, "glSupportDepthBufferBitDepth", globalRendering->supportDepthBufferBitDepth);

	/*** @field Platform.glSupportRestartPrimitive boolean */
	LuaPushNamedBool(L, "glSupportRestartPrimitive", globalRendering->supportRestartPrimitive);
	/*** @field Platform.glSupportClipSpaceControl boolean */
	LuaPushNamedBool(L, "glSupportClipSpaceControl", globalRendering->supportClipSpaceControl);
	/*** @field Platform.glSupportFragDepthLayout boolean */
	LuaPushNamedBool(L, "glSupportFragDepthLayout" , globalRendering->supportFragDepthLayout);
	/*** @field Platform.glSupportSeamlessCubeMaps boolean */
	LuaPushNamedBool(L, "glSupportSeamlessCubeMaps", globalRendering->supportSeamlessCubeMaps);

	/*** @field Platform.osName string full name of the OS */
	LuaPushNamedString(L, "osName", Platform::GetOSNameStr());
	/*** @field Platform.osVersion string */
	LuaPushNamedString(L, "osVersion", Platform::GetOSVersionStr());
	/*** @field Platform.osFamily "Windows"|"Linux"|"MacOSX"|"FreeBSD"|"Unknown" */
	LuaPushNamedString(L, "osFamily", Platform::GetOSFamilyStr());
	/*** @field Platform.architecture string CPU architecture (e.g., "x86_64", "arm64") */
	LuaPushNamedString(L, "architecture", Platform::GetArchitectureStr());
	/*** @field Platform.hwConfig string */
	LuaPushNamedString(L, "hwConfig", Platform::GetHardwareStr());
	/*** @field Platform.cpuLogicalCores integer */
	LuaPushNamedNumber(L, "cpuLogicalCores", Threading::GetLogicalCpuCores());
	/*** @field Platform.cpuPhysicalCores integer */
	LuaPushNamedNumber(L, "cpuPhysicalCores", Threading::GetPhysicalCpuCores());
	/*** @field Platform.cpuBrand string */
	LuaPushNamedString(L, "cpuBrand", Threading::GetCPUBrand());
	/*** @field Platform.totalRAM number Total physical system RAM in MBs. */
	LuaPushNamedNumber(L, "totalRAM", Platform::TotalRAM()/1e6);

	/*** @field Platform.sysInfoHash string */
	LuaPushNamedString(L, "sysInfoHash", Platform::GetSysInfoHash());
	/*** @field Platform.macAddrHash string */
	LuaPushNamedString(L, "macAddrHash", Platform::GetMacAddrHash());

	/*** @field Platform.isHeadless boolean Is this a headless build which only simulates and doesnt offer interactive IO? */
	LuaPushNamedBool(L, "isHeadless", SpringVersion::IsHeadless());

	/*** @field Platform.hasSyncChecksums boolean Whether the engine was built with sync-check support (i.e. Spring.GetPrevFrameSyncChecksum() returns a meaningful value). */
	#ifdef SYNCCHECK
		LuaPushNamedBool(L, "hasSyncChecksums", true);
	#else
		LuaPushNamedBool(L, "hasSyncChecksums", false);
	#endif

	return true;
}
