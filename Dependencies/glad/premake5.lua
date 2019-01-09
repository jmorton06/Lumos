project 'glad'
	kind 'StaticLib'
	cppdialect "C++14"
	files
	{
		"include/glad/glad.h",
        "include/KHR/khrplatform.h",
        "src/glad.c"
	}

	sysincludedirs
	{
		"include/"
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