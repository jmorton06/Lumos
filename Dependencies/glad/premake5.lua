project 'glad'
	kind 'StaticLib'
	systemversion "latest"
	cppdialect "C++14"
	staticruntime "On"
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

	filter "system:windows"
		files
		{
			"include/glad/glad_wgl.h",
			"src/glad_wgl.c"
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

	filter "configurations:Production"
	optimize "On"