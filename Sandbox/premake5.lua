project "Sandbox"
	kind "ConsoleApp"
	language "C++"

	files
	{
		"**.h",
		"**.cpp",
		"Scenes/**.h",
		"Scenes/**.cpp"
	}

	sysincludedirs
	{
		"../Lumos/external/spdlog/include",
		"../Lumos/external/",
		"../Lumos/external/stb/",
		"../Dependencies/lua/src/",
		"../Dependencies/GLFW/include/",
		"../Dependencies/glad/include/",
		"../Dependencies/OpenAL/include/",
		"../Dependencies/FreeType/include/",
		"../Dependencies/stb/",
		"../Dependencies/Box2D/",
		"../Dependencies/vulkan/",
		"../Dependencies/",
		"../Lumos/external/",
		"../Lumos/external/spdlog/include",
		"../Lumos/src"
	}

	links
	{
		"Lumos"
	}

	cwd = os.getcwd() .. "/.."

	defines
	{
		"JM_DYNAMIC",
		"JM_ROOT_DIR="  .. cwd,
	}

	filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"

		defines
		{
			"JM_PLATFORM_WINDOWS",
			"JM_RENDER_API_OPENGL",
			"JM_RENDER_API_VULKAN",
			"VK_USE_PLATFORM_WIN32_KHR",
			"WIN32_LEAN_AND_MEAN",
			"_CRT_SECURE_NO_WARNINGS",
			"_DISABLE_EXTENDED_ALIGNED_STORAGE"
		}

		buildoptions
		{
			"/MP"
		}

	filter "system:macosx"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"

		defines
		{
			"JM_PLATFORM_MACOS",
			"JM_PLATFORM_UNIX",
			"JM_RENDER_API_OPENGL",
			"JM_RENDER_API_VULKAN",
			"VK_USE_PLATFORM_MACOS_MVK",
			"JM_IMGUI"
		}

	filter "system:linux"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"

		defines
		{
			"JM_PLATFORM_LINUX",
			"JM_PLATFORM_UNIX",
			"JM_RENDER_API_OPENGL",
			"JM_RENDER_API_VULKAN",
			"VK_USE_PLATFORM_XCB_KHR",
			"JM_IMGUI"
		}

		buildoptions
		{
			"-msse4.1",
			"-fpermissive",
			"-Wattributes",
			"-fPIC",
			"-Wignored-attributes"
		}

		linkoptions
		{
			"-L%{cfg.targetdir}"
		}

		linkoptions{ "-Wl,-rpath=\\$$ORIGIN" }

	filter "configurations:Debug"
		defines "JM_DEBUG"
		symbols "On"

	filter "configurations:Release"
		defines "JM_RELEASE"
		optimize "On"

	filter "configurations:Dist"
		defines "JM_DIST"
		optimize "On"