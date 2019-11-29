project "Lumos"
	kind "StaticLib"
	language "C++"

	files
	{
		"src/**.h",
		"src/**.c",
		"src/**.cpp"
	}

	removefiles
	{
		"src/Platform/**"
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
		"external/spdlog/include",
		"../Dependencies/",
		"../Dependencies/lua/src/",
		"../Dependencies/glfw/include/",
		"../Dependencies/glad/include/",
		"../Dependencies/OpenAL/include/",
		"../Dependencies/Box2D/",
		"../Dependencies/vulkan/",
		"../Dependencies/imgui/"
	}

	links
	{
		"lua",
		"Box2D",
		"volk",
		"imgui"
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
		staticruntime "on"
		systemversion "latest"

		pchheader "lmpch.h"
		pchsource "src/lmpch.cpp"

		defines
		{
			"LUMOS_PLATFORM_WINDOWS",
			--"LUMOS_BUILD_DLL",
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

		files
		{
			"src/Platform/Windows/*.h",
			"src/Platform/Windows/*.cpp",

			"src/Platform/GLFW/*.h",
			"src/Platform/GLFW/*.cpp",

			"src/Platform/OpenAL/*.h",
			"src/Platform/OpenAL/*.cpp",

			"src/Platform/OpenGL/*.h",
			"src/Platform/OpenGL/*.cpp",

			"src/Platform/Vulkan/*.h",
			"src/Platform/Vulkan/*.cpp"
		}

		links
		{
			"glfw",
			"glad"
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

		files
		{
			"src/Platform/macOS/*.mm",
			"src/Platform/macOS/*.h",
			"src/Platform/macOS/*.cpp",

			"src/Platform/Unix/*.h",
			"src/Platform/Unix/*.cpp",

			"src/Platform/GLFW/*.h",
			"src/Platform/GLFW/*.cpp",

			"src/Platform/OpenAL/*.h",
			"src/Platform/OpenAL/*.cpp",

			"src/Platform/OpenGL/*.h",
			"src/Platform/OpenGL/*.cpp",

			"src/Platform/Vulkan/*.h",
			"src/Platform/Vulkan/*.cpp"
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
			pchheader "../Lumos/src/lmpch.h"
			pchsource "../Lumos/src/lmpch.cpp"

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

		files
		{
			"src/Platform/iOS/*.mm",
			"src/Platform/iOS/*.h",
			"src/Platform/iOS/*.cpp",

			"src/Platform/GLFM/*.h",
			"src/Platform/GLFM/*.cpp",

			"src/Platform/OpenAL/*.h",
			"src/Platform/OpenAL/*.cpp",

			"src/Platform/Unix/*.h",
			"src/Platform/Unix/*.cpp",

			"src/Platform/Vulkan/*.h",
			"src/Platform/Vulkan/*.cpp"
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

		defines
		{
			"LUMOS_PLATFORM_LINUX",
			"LUMOS_PLATFORM_UNIX",
			"LUMOS_RENDER_API_OPENGL",
			"LUMOS_RENDER_API_VULKAN",
			"VK_USE_PLATFORM_XCB_KHR",
			"LUMOS_IMGUI",
			"LUMOS_OPENAL"
		}

		files
		{
			"src/Platform/Unix/*.h",
			"src/Platform/Unix/*.cpp",

			"src/Platform/GLFW/*.h",
			"src/Platform/GLFW/*.cpp",

			"src/Platform/OpenAL/*.h",
			"src/Platform/OpenAL/*.cpp",

			"src/Platform/OpenGL/*.h",
			"src/Platform/OpenGL/*.cpp",

			"src/Platform/Vulkan/*.h",
			"src/Platform/Vulkan/*.cpp"
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

		pchheader "../Lumos/src/lmpch.h"
		pchsource "../Lumos/src/lmpch.cpp"

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
