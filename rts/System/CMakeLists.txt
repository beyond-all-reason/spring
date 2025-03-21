if (TRACY_PROFILE_MEMORY)
	set(memoryProfileSource "${CMAKE_CURRENT_SOURCE_DIR}/TraceMemory.cpp")
endif()

add_library(SystemCommon STATIC)
target_include_directories(SystemCommon PRIVATE 
	"${CMAKE_SOURCE_DIR}/rts"
	#TODO(asio): Should be imported by find_package. To fix.
	"${CMAKE_SOURCE_DIR}/rts/lib/asio/include"
	#TODO(smalloc): MemPoolTypes.h requires smmalloc/smmalloc.h. To remove when feasible.
	"${CMAKE_SOURCE_DIR}/rts/lib"
	#TODO(7z): CRC.cpp requires 7zCrc.h. To remove when feasible. 
	"${CMAKE_SOURCE_DIR}/tools/pr-downloader/src/lib/7z"
)
target_sources(SystemCommon PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/AABB.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/AIScriptHandler.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Color.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Config/ConfigHandler.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Config/ConfigLocater.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Config/ConfigSource.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Config/ConfigVariable.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/CRC.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/EventClient.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/EventHandler.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/GlobalConfig.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Info.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Input/InputHandler.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Input/KeyInput.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Input/MouseInput.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/LoadSave/CregLoadSaveHandler.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/LoadSave/Demo.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/LoadSave/DemoReader.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/LoadSave/DemoRecorder.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/LoadSave/LoadSaveHandler.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/LoadSave/LuaLoadSaveHandler.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/LogOutput.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Main.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Math/SpringDampers.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Math/NURBS.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Matrix44f.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Quaternion.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Misc/RectangleOverlapHandler.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Misc/SpringTime.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Object.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/GameLoadThread.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Option.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Clipboard.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Platform/errorhandler.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Misc.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Platform/SharedLib.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Platform/ScopedFileLock.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Platform/SDL1_keysym.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Watchdog.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Platform/WindowManagerHelper.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Rectangle.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/SafeVector.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/SafeCStrings.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/SplashScreen.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/SpringApp.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/StartScriptGen.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Sync/DumpState.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Sync/FPUCheck.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Sync/Logger.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Sync/SHA512.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Sync/SyncChecker.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Sync/SyncDebugger.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Sync/SyncedFloat3.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Sync/backtrace.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Sync/get_executable_name.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/TdfParser.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Threading/ThreadPool.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/TimeProfiler.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/TimeUtil.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Transform.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/UriParser.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/StringUtil.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/type2.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/float3.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/float4.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/SpringMath.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/SpringMem.cpp"
	"${memoryProfileSource}"
)
target_link_libraries(SystemCommon PRIVATE SDL2::SDL2 Tracy::TracyClient prd::base64 lua gflags_nothreads_static prd::jsoncpp)

add_library(SystemCreg STATIC)
target_include_directories(SystemCreg PRIVATE 
	"${CMAKE_SOURCE_DIR}/rts"
	#TODO(smalloc): MemPoolTypes.h requires smmalloc/smmalloc.h. To remove when feasible.
	"${CMAKE_SOURCE_DIR}/rts/lib"
)
if (NOT(NO_CREG))
	target_sources(SystemCreg PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/creg/SerializeLuaState.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/creg/Serializer.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/creg/VarTypes.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/creg/creg.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/creg/creg_runtime_tests.cpp"
	)
endif ()
target_link_libraries(SystemCreg PRIVATE lua Tracy::TracyClient lua)

add_library(SystemFilesystem STATIC)
target_include_directories(SystemFilesystem PRIVATE 
	"${CMAKE_SOURCE_DIR}/rts"
	#TODO(7z): Archives/SevenZipArchive.cpp requires 7z.h. To remove when feasible. 
	"${CMAKE_SOURCE_DIR}/tools/pr-downloader/src/lib/7z"
	#TODO(smalloc): MemPoolTypes.h requires smmalloc/smmalloc.h. To remove when feasible.
	"${CMAKE_SOURCE_DIR}/rts/lib"
)
target_sources(SystemFilesystem PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/FileSystem/ArchiveNameResolver.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/FileSystem/ArchiveLoader.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/FileSystem/ArchiveScanner.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/FileSystem/CacheDir.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/FileSystem/DataDirLocater.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/FileSystem/DataDirsAccess.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/FileSystem/FileFilter.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/FileSystem/FileHandler.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/FileSystem/FileSystem.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/FileSystem/FileSystemAbstraction.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/FileSystem/FileSystemInitializer.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/FileSystem/GZFileHandler.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/FileSystem/Misc.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/FileSystem/RapidHandler.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/FileSystem/SimpleParser.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/FileSystem/VFSHandler.cpp"
)
target_link_libraries(SystemFilesystem PRIVATE Tracy::TracyClient)

add_library(SystemLog STATIC)
target_include_directories(SystemLog PRIVATE "${CMAKE_SOURCE_DIR}/rts")
target_sources(SystemLog PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/Log/Backend.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Log/DefaultFilter.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Log/DefaultFormatter.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Log/FramePrefixer.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Log/LogSinkHandler.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Log/LogUtil.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Log/TracySink.cpp"
)

add_library(SystemLogSinkStream STATIC)
target_include_directories(SystemLogSinkStream PRIVATE "${CMAKE_SOURCE_DIR}/rts")
target_sources(SystemLogSinkStream PRIVATE 
	"${CMAKE_CURRENT_SOURCE_DIR}/Log/StreamSink.cpp"
)

add_library(SystemLogSinkConsole STATIC)
target_include_directories(SystemLogSinkConsole PRIVATE "${CMAKE_SOURCE_DIR}/rts")
target_sources(SystemLogSinkConsole PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/Log/ConsoleSink.cpp"
)

add_library(SystemLogSinkFile STATIC)
target_include_directories(SystemLogSinkFile PRIVATE "${CMAKE_SOURCE_DIR}/rts")
target_sources(SystemLogSinkFile PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/Log/FileSink.cpp"
)

add_library(SystemLogSinkOutputDebugString STATIC)
target_include_directories(SystemLogSinkOutputDebugString PRIVATE "${CMAKE_SOURCE_DIR}/rts")
target_sources(SystemLogSinkOutputDebugString PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/Log/OutputDebugStringSink.cpp"
)

add_library(SystemThreading STATIC)
target_include_directories(SystemThreading PRIVATE "${CMAKE_SOURCE_DIR}/rts")
target_sources(SystemThreading PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/Platform/CpuID.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Threading.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Platform/ThreadAffinityGuard.cpp"
)
target_link_libraries(SystemThreading PRIVATE Tracy::TracyClient)

target_link_libraries(System INTERFACE
	SystemCommon
	SystemCreg
	SystemFilesystem
	SystemLog
	SystemLogSinkConsole
	SystemLogSinkFile
)
target_include_directories(System INTERFACE "${CMAKE_SOURCE_DIR}/rts")

if (UNIX)
	add_library(SystemThreadingLinux STATIC)
	target_include_directories(SystemThreadingLinux PRIVATE "${CMAKE_SOURCE_DIR}/rts")
	target_sources(SystemThreadingLinux PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Linux/CpuTopology.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Linux/Futex.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Linux/ThreadSupport.cpp"
	)
	target_link_libraries(SystemThreading PRIVATE SystemThreadingLinux)

	add_library(SystemPlatformLinux STATIC)
	target_include_directories(SystemPlatformLinux PRIVATE "${CMAKE_SOURCE_DIR}/rts")
	target_sources(SystemPlatformLinux PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Linux/CrashHandler.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Linux/Hardware.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Linux/SoLib.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Linux/MessageBox.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Linux/WindowManagerHelper.cpp"
	)
	target_link_libraries(SystemPlatformLinux PRIVATE SDL2::SDL2)
	target_link_libraries(System INTERFACE SystemPlatformLinux)
else (WIN32)
	add_library(SystemThreadingWindows STATIC)
	target_include_directories(SystemThreadingWindows PRIVATE "${CMAKE_SOURCE_DIR}/rts")
	target_sources(SystemThreadingWindows PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Win/CpuTopology.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Win/CriticalSection.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Win/Future.cpp"
	)
	target_link_libraries(SystemThreading PRIVATE SystemThreadingWindows)

	add_library(SystemPlatformWindows STATIC)
	target_include_directories(SystemPlatformWindows PRIVATE "${CMAKE_SOURCE_DIR}/rts")
	target_sources(SystemPlatformWindows PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Win/CrashHandler.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Win/DllLib.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Win/Hardware.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Win/MessageBox.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Win/seh.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Win/WinVersion.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Win/WindowManagerHelper.cpp"
	)
	target_link_libraries(SystemPlatformWindows PRIVATE SDL2::SDL2)
	target_link_libraries(System INTERFACE SystemPlatformWindows SystemLogSinkOutputDebugString)
endif ()

target_link_libraries(System INTERFACE SystemThreading)

add_subdirectory(FileSystem)
add_subdirectory(Net)
