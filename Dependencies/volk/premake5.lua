project 'volk'
	kind 'StaticLib'
	systemversion "latest"
	cppdialect "C++14"
	staticruntime "On"
	
	files
	{
		"volk.h",
        "volk.c"
	}

	sysincludedirs
	{
		"../"
	}

	filter "system:windows"
		defines
		{
			"VK_USE_PLATFORM_WIN32_KHR"
		}

	filter "system:linux"
		defines
		{
			"VK_USE_PLATFORM_XCB_KHR"
		}
		buildoptions
    	{
    	  "-fPIC"
		}
	filter "system:macosx"
		defines
		{
			"VK_USE_PLATFORM_METAL_EXT",
			"VK_EXT_metal_surface"
		}

	filter "system:ios"
		defines
		{
			"VK_USE_PLATFORM_IOS_MVK"
		}

	filter "configurations:Debug"
	symbols "On"

	filter "configurations:Release"
	optimize "On"

	filter "configurations:Production"
	optimize "On"