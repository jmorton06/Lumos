project "msdfgen"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
    staticruntime "Off"
	warnings "off"

	files
	{
		"core/**.h",
		"core/**.hpp",
		"core/**.cpp",
		"ext/**.h",
		"ext/**.hpp",
		"ext/**.cpp",
		"lib/**.cpp",
		"include/**.h"
	}

	externalincludedirs
	{
		"../../freetype/include",
		"include"
	}

	defines
	{
		"MSDFGEN_USE_CPP11"
	}

	links
	{
		"freetype"
	}

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"

	filter "configurations:Production"
		runtime "Release"
		optimize "on"
        symbols "off"