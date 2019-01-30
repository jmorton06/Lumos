project 'volk'
	kind 'StaticLib'
	systemversion "latest"
	cppdialect "C++14"
	files
	{
		"volk.h",
        "volk.c"
	}

	sysincludedirs
	{
		"../"
	}

	filter "system:linux"
		buildoptions
    	{
    	  "-fPIC"
		}

	filter "configurations:Debug"
	symbols "On"

	filter "configurations:Release"
	optimize "On"

	filter "configurations:Dist"
	optimize "On"