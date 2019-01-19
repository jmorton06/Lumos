project "Lumos"
	kind "SharedLib"
	language "C++"

	files
	{
		"src/**.h",
		"src/**.c",
		"src/**.cpp",

		"external/imgui/imgui.h",
        "external/imgui/imconfig.h",
		"external/imgui/imgui.cpp",
		"external/imgui/imgui_demo.cpp",
		"external/imgui/imgui_draw.cpp",
		"external/imgui/imgui_internal.h",
		"external/imgui/imgui_widgets.cpp",
		"external/imgui/imstb_rectpack.h",
		"external/imgui/imstb_textedit.h",
		"external/imgui/imstb_truetype.h",
		"external/tinygltf/json.hpp",
		"external/tinygltf/stb_image_write.h",
		"external/tinygltf/stb_image.h",
		"external/tinygltf/tiny_gltf.h",
		"external/stb/stb_vorbis.c",
		"external/simplex/**.h",
		"external/simplex/**.cpp"
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
		"external/stb/",
		"external/spdlog/include",
		"../Dependencies/",
		"../Dependencies/lua/src/",
		"../Dependencies/GLFW/include/",
		"../Dependencies/glad/include/",
		"../Dependencies/OpenAL/include/",
		"../Dependencies/Box2D/",
		"../Dependencies/vulkan/"
	}

	links
	{
		"glfw",
		"lua",
		"Box2D",
		"glad"
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

		files
		{
			"external/imgui/examples/imgui_impl_vulkan.h",
			"external/imgui/examples/imgui_impl_vulkan.cpp",
			"external/imgui/examples/imgui_impl_opengl3.h",
			"external/imgui/examples/imgui_impl_opengl3.cpp",
			"external/imgui/examples/imgui_impl_glfw.h",
			"external/imgui/examples/imgui_impl_glfw.cpp"
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
			"vulkan-1.lib",
			"OpenAL32"
		}

		libdirs
		{
			"../Dependencies/vulkan/libs/windows/64bit",
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
			"external/imgui/examples/imgui_impl_vulkan.h",
			"external/imgui/examples/imgui_impl_vulkan.cpp",
			"external/imgui/examples/imgui_impl_opengl3.h",
			"external/imgui/examples/imgui_impl_opengl3.cpp",
			"external/imgui/examples/imgui_impl_glfw.h",
			"external/imgui/examples/imgui_impl_glfw.cpp",
			"src/Platform/GraphicsAPI/Vulkan/MakeMetalView.mm"
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
			"MoltenVK"
		}

		os.copyfile("../Dependencies/vulkan/libs/macOS/libMoltenVK.dylib","../bin")

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

		files
		{
			"external/imgui/examples/imgui_impl_vulkan.h",
			"external/imgui/examples/imgui_impl_vulkan.cpp",
			"external/imgui/examples/imgui_impl_opengl3.h",
			"external/imgui/examples/imgui_impl_opengl3.cpp",
			"external/imgui/examples/imgui_impl_glfw.h",
			"external/imgui/examples/imgui_impl_glfw.cpp"
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

		linkoptions
		{
			"../Dependencies/OpenAL/libs/linux/libopenal.so"
			--"../Dependencies/vulkan/libs/linux/libvulkan.so.1"
		}

		os.copyfile("../Dependencies/vulkan/libs/linux/libvulkan.so.1","../bin")

		linkoptions{ "-Wl,-rpath=\\$$ORIGIN" }

		libdirs
		{
			"../bin/**",
			"../Dependencies/vulkan/libs/linux/"
		}

		buildoptions
		{
			"-msse4.1",
			"-fpermissive",
			"-fPIC",
			"-Wignored-attributes"
		}

		links { "X11", "pthread", "vulkan"}

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

	filter "configurations:Release"
		defines "LUMOS_RELEASE"
		optimize "On"

	filter "configurations:Dist"
		defines "LUMOS_DIST"
		optimize "On"
