IncludeDir = {}
IncludeDir["GLFW"] = "External/glfw/include/"
IncludeDir["Glad"] = "External/glad/include/"
IncludeDir["lua"] = "External/lua/src/"
IncludeDir["stb"] = "External/stb/"
IncludeDir["OpenAL"] = "External/OpenAL/include/"
IncludeDir["Box2D"] = "External/box2d/include/"
IncludeDir["external"] = "External/"
IncludeDir["vulkan"] = "External/vulkan/"
IncludeDir["Lumos"] = "Source"
IncludeDir["External"] = "External/"
IncludeDir["ImGui"] = "External/imgui/"
IncludeDir["freetype"] = "External/freetype/include"
IncludeDir["SpirvCross"] = "External/SPIRV-Cross"
IncludeDir["cereal"] = "External/cereal/include"
IncludeDir["spdlog"] = "External/spdlog/include"

project "Lumos"
	kind "StaticLib"
	language "C++"
	editandcontinue "Off"

	files
	{
		"Source/**.h",
		"Source/**.c",
		"Source/**.cpp"
	}

	removefiles
	{
		"Source/Platform/**"
	}

	includedirs
	{
		"../",
		"Source/",
		"",
		"../Source/"
	}

	sysincludedirs
	{
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.lua}",
		"%{IncludeDir.stb}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.OpenAL}",
		"%{IncludeDir.Box2D}",
		"%{IncludeDir.vulkan}",
		"%{IncludeDir.external}",
		"%{IncludeDir.External}",
		"%{IncludeDir.spdlog}",
		"%{IncludeDir.freetype}",
		"%{IncludeDir.SpirvCross}",
		"%{IncludeDir.cereal}",
		"%{IncludeDir.Lumos}",
	}

	links
	{
		"lua",
		"box2d",
		"imgui",
		"freetype",
		"SpirvCross"
	}

	defines
	{
		"LUMOS_ENGINE",
		"FREEIMAGE_LIB",
		"LUMOS_DYNAMIC",
		"LUMOS_ROOT_DIR="  .. root_dir,
		"IMGUI_USER_CONFIG=\"Source/ImGui/ImConfig.h\"",
		"LUMOS_PROFILE",
		"TRACY_ENABLE",
	}

	filter 'architecture:x86_64'
		defines { "LUMOS_SSE" ,"USE_VMA_ALLOCATOR"}

	filter "system:windows"
		cppdialect "C++17"
		staticruntime "on"
		systemversion "latest"
		disablewarnings { 4307 }
		characterset ("MBCS")

		pchheader "Precompiled.h"
		pchsource "Source/Precompiled.cpp"

		defines
		{
			"LUMOS_PLATFORM_WINDOWS",
			"LUMOS_RENDER_API_OPENGL",
			"LUMOS_RENDER_API_VULKAN",
			"VK_USE_PLATFORM_WIN32_KHR",
			"WIN32_LEAN_AND_MEAN",
			"_CRT_SECURE_NO_WARNINGS",
			"_DISABLE_EXTENDED_ALIGNED_STORAGE",
			"_SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING",
			"_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING",
			"LUMOS_IMGUI",
			"LUMOS_OPENAL",
			"LUMOS_VOLK"
		}

		files
		{
			"Source/Platform/Windows/*.h",
			"Source/Platform/Windows/*.cpp",

			"Source/Platform/GLFW/*.h",
			"Source/Platform/GLFW/*.cpp",

			"Source/Platform/OpenAL/*.h",
			"Source/Platform/OpenAL/*.cpp",

			"Source/Platform/OpenGL/*.h",
			"Source/Platform/OpenGL/*.cpp",

			"Source/Platform/Vulkan/*.h",
			"Source/Platform/Vulkan/*.cpp",

			"External/glad/src/glad_wgl.c"
		}

		links
		{
			"glfw",
		}

		buildoptions
		{
			"/MP", "/bigobj"
		}

		filter 'files:External/**.cpp'
			flags  { 'NoPCH' }
		filter 'files:External/**.c'
			flags  { 'NoPCH' }

	filter "system:macosx"
		cppdialect "C++17"
		systemversion "latest"

		files
		{
			"Source/Platform/macOS/*.mm",
			"Source/Platform/macOS/*.h",
			"Source/Platform/macOS/*.cpp",

			"Source/Platform/Unix/*.h",
			"Source/Platform/Unix/*.cpp",

			"Source/Platform/GLFW/*.h",
			"Source/Platform/GLFW/*.cpp",

			"Source/Platform/OpenAL/*.h",
			"Source/Platform/OpenAL/*.cpp",

			"Source/Platform/OpenGL/*.h",
			"Source/Platform/OpenGL/*.cpp",

			"Source/Platform/Vulkan/*.h",
			"Source/Platform/Vulkan/*.cpp"
		}

		defines
		{
			"LUMOS_PLATFORM_MACOS",
			"LUMOS_PLATFORM_UNIX",
			"LUMOS_RENDER_API_OPENGL",
			"LUMOS_RENDER_API_VULKAN",
			"VK_USE_PLATFORM_METAL_EXT",
			"LUMOS_IMGUI",
			"LUMOS_OPENAL",
			"LUMOS_VOLK"
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
			"SystemConfiguration.framework",
			"glfw"
		}

		libdirs
		{
			"../bin/**"
		}

		buildoptions
		{
			"-Wno-attributes",
			"-Wno-nullability-completeness",
			"-fdiagnostics-absolute-paths"
		}

		SetRecommendedXcodeSettings()

		pchheader "../Lumos/Source/Precompiled.h"
		pchsource "../Lumos/Source/Precompiled.cpp"

		filter 'files:External/**.cpp'
			flags  { 'NoPCH' }
		filter 'files:External/**.c'
			flags  { 'NoPCH' }
		filter 'files:Source/**.m'
			flags  { 'NoPCH' }
		filter 'files:Source/**.mm'
			flags  { 'NoPCH' }

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
			"USE_VMA_ALLOCATOR",
			"LUMOS_OPENAL"
		}

		files
		{
			"Source/Platform/iOS/*.mm",
			"Source/Platform/iOS/*.h",
			"Source/Platform/iOS/*.cpp",

			"Source/Platform/OpenAL/*.h",
			"Source/Platform/OpenAL/*.cpp",

			"Source/Platform/Unix/*.h",
			"Source/Platform/Unix/*.cpp",

			"Source/Platform/Vulkan/*.h",
			"Source/Platform/Vulkan/*.cpp"
		}

		removefiles
		{
			"Source/Platform/Unix/UnixFileSystem.cpp"
		}

		links
		{
			"QuartzCore.framework",
			"Metal.framework",
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
			"-Wno-attributes",
			"-Wno-nullability-completeness"
		}

		SetRecommendedXcodeSettings()

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
			"LUMOS_VOLK"
		}

		files
		{
			"Source/Platform/Unix/*.h",
			"Source/Platform/Unix/*.cpp",

			"Source/Platform/GLFW/*.h",
			"Source/Platform/GLFW/*.cpp",

			"Source/Platform/OpenAL/*.h",
			"Source/Platform/OpenAL/*.cpp",

			"Source/Platform/OpenGL/*.h",
			"Source/Platform/OpenGL/*.cpp",

			"Source/Platform/Vulkan/*.h",
			"Source/Platform/Vulkan/*.cpp"
		}

		links
		{
			"glfw"
		}

		linkoptions
		{
			"external/OpenAL/libs/linux/libopenal.so"
		}

		linkoptions{ "-Wl,-rpath=\\$$ORIGIN" }

		libdirs
		{
			"../bin/**"
		}

		buildoptions
		{
			"-fpermissive",
			"-fPIC",
			"-Wignored-attributes",
			"-Wno-psabi"
		}

		links { "X11", "pthread"}

		pchheader "../Lumos/Source/Precompiled.h"
		pchsource "../Lumos/Source/Precompiled.cpp"

		filter 'files:External/**.cpp'
			flags  { 'NoPCH' }
		filter 'files:External/**.c'
			flags  { 'NoPCH' }
		filter 'files:Source/**.c'
			flags  { 'NoPCH' }

		filter {'system:linux', 'architecture:x86_64'}
			buildoptions
			{
				"-msse4.1",
			}

	filter "configurations:Debug"
		defines { "LUMOS_DEBUG", "_DEBUG" }
		symbols "On"
		runtime "Debug"
		optimize "Debug"

	filter "configurations:Release"
		defines "LUMOS_RELEASE"
		optimize "Speed"
		symbols "On"
		runtime "Release"

	filter "configurations:Production"
		defines "LUMOS_PRODUCTION"
		symbols "Off"
		optimize "Full"
		runtime "Release"
