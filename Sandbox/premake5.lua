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
		"LUMOS_DYNAMIC",
		"LUMOS_ROOT_DIR="  .. cwd,
	}

	filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"

		defines
		{
			"LUMOS_PLATFORM_WINDOWS",
			"LUMOS_RENDER_API_OPENGL",
			"LUMOS_RENDER_API_VULKAN",
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
			"LUMOS_PLATFORM_MACOS",
			"LUMOS_PLATFORM_UNIX",
			"LUMOS_RENDER_API_OPENGL",
			"LUMOS_RENDER_API_VULKAN",
			"VK_USE_PLATFORM_MACOS_MVK",
			"LUMOS_IMGUI"
		}

	filter "system:linux"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"

		defines
		{
			"LUMOS_PLATFORM_LINUX",
			"LUMOS_PLATFORM_UNIX",
			"LUMOS_RENDER_API_OPENGL",
			"LUMOS_RENDER_API_VULKAN",
			"VK_USE_PLATFORM_XCB_KHR",
			"LUMOS_IMGUI"
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
		defines "LUMOS_DEBUG"
		symbols "On"

	filter "configurations:Release"
		defines "LUMOS_RELEASE"
		optimize "On"
		symbols "On"

	filter "configurations:Dist"
		defines "LUMOS_DIST"
		optimize "On"