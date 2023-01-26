project "msdf-atlas-gen"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
    staticruntime "on"
	warnings "off"

	files
	{
		"msdf-atlas-gen/**.h",
    	"msdf-atlas-gen/**.hpp",
    	"msdf-atlas-gen/**.cpp"
	}


	files
	{
		"msdfgen/core/**.h",
		"msdfgen/core/**.hpp",
		"msdfgen/core/**.cpp",
		"msdfgen/ext/**.h",
		"msdfgen/ext/**.hpp",
		"msdfgen/ext/**.cpp",
		"msdfgen/lib/**.cpp",
		"msdfgen/include/**.h"
	}

	includedirs
	{
		"msdf-atlas-gen",
		"msdfgen/include"
	}

	externalincludedirs
	{
		"msdfgen",
		"msdfgen/include",
		"../freetype/include"
	}

	defines
	{
		"_CRT_SECURE_NO_WARNINGS",
		"MSDFGEN_USE_CPP11"
	}

	links
	{
		"freetype"
	}
	
	-- links
	-- {
	-- 	"msdfgen",
	-- 	"pthread"
	-- }

	removefiles
	{
		"msdf-atlas-gen/main.cpp"
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