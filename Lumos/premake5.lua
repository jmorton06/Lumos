project "Lumos"
	kind "SharedLib"
	language "C++"

	files
	{
		"src/**.h",
		"src/**.c",
		"src/**.cpp",
		"res/**"
	}

	removefiles
	{
		"src/Platform/iOS/**"
	}

	includedirs
	{
		"../",
		"src/",
		"",
		"../src/"
	}

	sysincludedirs
	{
		"external/",
		"external/jsonhpp/",
		"external/stb/",
		"external/imgui/",
		"external/spdlog/include",
		"../Dependencies/",
		"../Dependencies/lua/src/",
		"../Dependencies/glfw/include/",
		"../Dependencies/glad/include/",
		"../Dependencies/OpenAL/include/",
		"../Dependencies/Box2D/",
		"../Dependencies/vulkan/"
	}

	links
	{
		"lua",
		"Box2D",
		"volk"
	}

	cwd = os.getcwd() .. "/.."

	defines
	{
		"LUMOS_ENGINE",
		"FREEIMAGE_LIB",
		"LUMOS_DYNAMIC",
		"LUMOS_ROOT_DIR="  .. cwd,
	}

	filter "system:windows"
		cppdialect "C++17"
		staticruntime "Off"
		systemversion "latest"

		removefiles
		{
			"src/Platform/GLFM/*.h",
			"src/Platform/GLFM/*.cpp",
			"src/Platform/Unix/*.h",
			"src/Platform/Unix/*.cpp"
		}

		pchheader "LM.h"
		pchsource "src/LM.cpp"

		defines
		{
			"LUMOS_PLATFORM_WINDOWS",
			"LUMOS_BUILD_DLL",
			"LUMOS_RENDER_API_OPENGL",
			"LUMOS_RENDER_API_VULKAN",
			"VK_USE_PLATFORM_WIN32_KHR",
			"WIN32_LEAN_AND_MEAN",
			"_CRT_SECURE_NO_WARNINGS",
			"_DISABLE_EXTENDED_ALIGNED_STORAGE",
			"_SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING",
			"LUMOS_IMGUI",
			"LUMOS_OPENAL"
		}

		links
		{
			"OpenAL32",
			"glfw",
			"glad",
			"opengl32.lib"
		}

		libdirs
		{
			"../Dependencies/OpenAL/libs/Win32"
		}

		buildoptions
		{
			"/MP"
		}

		characterset ("MBCS")

		filter 'files:external/**.cpp'
			flags  { 'NoPCH' }
		filter 'files:external/**.c'
			flags  { 'NoPCH' }

	filter "system:macosx"
		cppdialect "C++17"
		systemversion "latest"

		removefiles
		{
			"src/Platform/GLFM/*.h",
			"src/Platform/GLFM/*.cpp",
			"src/Platform/Windows/*.h",
			"src/Platform/Windows/*.cpp"
		}

		files
		{
			"src/Platform/macOS/*.mm"
		}

		defines
		{
			"LUMOS_PLATFORM_MACOS",
			"LUMOS_PLATFORM_UNIX",
			"LUMOS_RENDER_API_OPENGL",
			"LUMOS_RENDER_API_VULKAN",
			"VK_USE_PLATFORM_MACOS_MVK",
			"LUMOS_IMGUI",
			"LUMOS_OPENAL"
		}

		links
		{
			"QuartzCore.framework",
			"Metal.framework",
			"Cocoa.framework",
        	"IOKit.framework",
        	"CoreFoundation.framework",
			"CoreVideo.framework",
			"OpenAL.framework",
			"glfw",
			"glad"
		}

		libdirs
		{
			"../bin/**"
		}

		buildoptions
		{
			"-Wno-attributes"
		}

		filter { "action:xcode4" }
			pchheader "../Lumos/src/LM.h"
			pchsource "../Lumos/src/LM.cpp"

			filter 'files:external/**.cpp'
				flags  { 'NoPCH' }
			filter 'files:external/**.c'
				flags  { 'NoPCH' }
			filter 'files:src/**.m'
				flags  { 'NoPCH' }

	require 'Scripts/ios'
	filter "system:ios"
		cppdialect "C++17"
		systemversion "latest"
		kind "StaticLib"

		removefiles
		{
			"src/Platform/GLFW/*.h",
			"src/Platform/GLFW/*.cpp",
			"src/Platform/Windows/*.h",
			"src/Platform/Windows/*.cpp",
			"src/Platform/OpenGL/**.h",
			"src/Platform/OpenGL/**.cpp"
		}

		defines
		{
			"LUMOS_PLATFORM_IOS",
			"LUMOS_PLATFORM_MOBILE",
			"LUMOS_PLATFORM_UNIX",
			"LUMOS_RENDER_API_VULKAN",
			"VK_USE_PLATFORM_IOS_MVK",
			"LUMOS_IMGUI",
			"LUMOS_OPENAL"
		}

		links
		{
			"QuartzCore.framework",
			"Metal.framework",
			"Cocoa.framework",
        	"IOKit.framework",
        	"CoreFoundation.framework",
			"CoreVideo.framework",
			"OpenAL.framework"
		}

		libdirs
		{
			"../bin/**"
		}

		buildoptions
		{
			"-Wno-attributes"
		}

		xcodebuildresources { "res/**" }


	filter "system:linux"
		cppdialect "C++17"
		systemversion "latest"

		removefiles
		{
			"src/Platform/GLFM/*.h",
			"src/Platform/GLFM/*.cpp",
			"src/Platform/Windows/*.h",
			"src/Platform/Windows/*.cpp"
		}

		defines
		{
			"LUMOS_PLATFORM_LINUX",
			"LUMOS_PLATFORM_UNIX",
			"LUMOS_RENDER_API_OPENGL",
			"LUMOS_RENDER_API_VULKAN",
			"VK_USE_PLATFORM_XCB_KHR",
			"LUMOS_IMGUI"
		}

		links
		{
			"glfw",
			"glad"
		}

		linkoptions
		{
			"../Dependencies/OpenAL/libs/linux/libopenal.so"
		}

		linkoptions{ "-Wl,-rpath=\\$$ORIGIN" }

		libdirs
		{
			"../bin/**"
		}

		buildoptions
		{
			"-msse4.1",
			"-fpermissive",
			"-fPIC",
			"-Wignored-attributes"
		}

		links { "X11", "pthread"}

		pchheader "../Lumos/src/LM.h"
		pchsource "../Lumos/src/LM.cpp"

		filter 'files:external/**.cpp'
			flags  { 'NoPCH' }
		filter 'files:external/**.c'
			flags  { 'NoPCH' }
		filter 'files:src/**.c'
			flags  { 'NoPCH' }

	filter "configurations:Debug"
		defines "LUMOS_DEBUG"
		symbols "On"
		runtime "Debug"

	filter "configurations:Release"
		defines "LUMOS_RELEASE"
		optimize "On"
		symbols "On"
		runtime "Release"

	filter "configurations:Dist"
		defines "LUMOS_DIST"
		optimize "On"
		runtime "Release"
