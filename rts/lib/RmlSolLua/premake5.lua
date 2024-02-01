workspace "RmlSolLua"
	configurations { "Debug", "Release" }
	platforms { "x32", "x64" }
	symbols "On"

	cppdialect "C++17"

	filter "configurations:Debug"
		defines { "DEBUG", "_DEBUG" }
		editandcontinue "Off"
		optimize "Off"

	filter "configurations:Release"
		defines { "NDEBUG" }
		optimize "Size"
		flags { "LinkTimeOptimization" }
		staticruntime "On"

	-- Set our platform architectures.
	filter "platforms:x32"
		architecture "x32"
	filter "platforms:x64"
		architecture "x64"


project "RmlSolLua"
	kind "StaticLib"
	language "C++"
	targetdir "out"
	debugdir "out"

	-- Add files.
	files { "src/**" }
	includedirs { "src" }

	-- Library includes.
	includedirs {
		"dependencies/sol2/include/",
		"dependencies/luajit-2.0/src/",
		"dependencies/RmlUi/include/",
	}

	-- Links.
	links {
		"lua51",
		"RmlUi",
	}

	-- Defines
	defines {
		"RMLUI_STATIC_LIB",
		-- "RMLUI_NO_THIRDPARTY_CONTAINERS".	-- Enable to use STL containers.
	}
