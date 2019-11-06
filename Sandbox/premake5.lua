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
		"../Dependencies/glfw/include/",
		"../Dependencies/glad/include/",
		"../Dependencies/OpenAL/include/",
		"../Dependencies/stb/",
		"../Dependencies/Box2D/",
		"../Dependencies/vulkan/",
		"../Dependencies/",
		"../Lumos/external/",
		"../Lumos/external/jsonhpp/",
		"../Lumos/external/spdlog/include",
		"../Lumos/src"
	}

	links
	{
		"Lumos",
		"lua",
		"Box2D",
		"volk",
		"imgui"
	}

	cwd = os.getcwd() .. "/.."

	defines
	{
		--"LUMOS_DYNAMIC",
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

		links
		{
			"OpenAL32",
			"glfw",
			"glad"
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

		linkoptions 
		{ 
			"-framework OpenGL",
			"-framework Cocoa",
			"-framework IOKit", 
			"-framework CoreVideo",
			"-framework OpenAL",
			"-framework QuartzCore"
		}

		links
		{
			"glfw",
			"glad"
		}


		filter {"system:macosx", "configurations:release"}

			local source = "../Dependencies/vulkan/libs/macOS/**"
			local target = "../bin/release/"
			
			buildmessage("copying "..source.." -> "..target)
			
			postbuildcommands {
				"{COPY} "..source.." "..target
			}

		filter {"system:macosx", "configurations:dist"}

			local source = "../Dependencies/vulkan/libs/macOS/**"
			local target = "../bin/dist/"
			
			buildmessage("copying "..source.." -> "..target)
			
			postbuildcommands {
				"{COPY} "..source.." "..target
			}

		filter {"system:macosx", "configurations:debug"}

			local source = "../Dependencies/vulkan/libs/macOS/**"
			local target = "../bin/debug/"
			
			buildmessage("copying "..source.." -> "..target)
			
			postbuildcommands {
				"{COPY} "..source.." "..target
			}

	filter "system:ios"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"
		kind "WindowedApp"
		targetextension ".app"

		defines
		{
			"LUMOS_PLATFORM_IOS",
			"LUMOS_PLATFORM_MOBILE",
			"LUMOS_PLATFORM_UNIX",
			"LUMOS_RENDER_API_VULKAN",
			"VK_USE_PLATFORM_IOS_MVK",
			"LUMOS_IMGUI"
		}

		links
		{
			"QuartzCore.framework",
			"Metal.framework",
        	"IOKit.framework",
        	"CoreFoundation.framework",
			"CoreVideo.framework",
			"UIKit.framework",
			"OpenAL.framework"
		}

		xcodebuildsettings
		{
			["ARCHS"] = "$(ARCHS_STANDARD)",
			["ONLY_ACTIVE_ARCH"] = "NO",
			["SDKROOT"] = "iphoneos",
			["SUPPORTED_PLATFORMS"] = "iphonesimulator iphoneos",
			["CODE_SIGN_IDENTITY[sdk=iphoneos*]"] = "iPhone Developer",
			['IPHONEOS_DEPLOYMENT_TARGET'] = '12.1',
			['PRODUCT_BUNDLE_IDENTIFIER'] = "com.jmorton06",
			['INFOPLIST_FILE'] = "../Lumos/src/Platform/iOS/Client/Info.plist"
		}

		files
		{
			"../Lumos/src/Platform/iOS/Client/**",
			"res/**"
		}

		xcodebuildresources { "res/**" }

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

		links
		{
			"glfw",
			"glad"
		}

		linkoptions
		{
			"-L%{cfg.targetdir}"
		}

		linkoptions{ "-Wl,-rpath=\\$$ORIGIN" }

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