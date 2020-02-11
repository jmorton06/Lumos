project "Sandbox"
	kind "WindowedApp"
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
		"../Dependencies/OpenAL/include/",
		"../Dependencies/stb/",
		"../Dependencies/Box2D/",
		"../Dependencies/vulkan/",
		"../Dependencies/",
		"../Lumos/external/",
		"../Lumos/external/jsonhpp/",
		"../Lumos/external/spdlog/include",
		"../Lumos/external/glad/include/",
		"../Lumos/src"
	}

	links
	{
		"Lumos",
		"lua",
		"Box2D",
		"imgui"
	}

	cwd = os.getcwd() .. "/.."

	defines
	{
	}

	filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"
		entrypoint "WinMainCRTStartup"

		defines
		{
			"LUMOS_PLATFORM_WINDOWS",
			"LUMOS_RENDER_API_OPENGL",
			"LUMOS_RENDER_API_VULKAN",
			"VK_USE_PLATFORM_WIN32_KHR",
			"WIN32_LEAN_AND_MEAN",
			"_CRT_SECURE_NO_WARNINGS",
			"_DISABLE_EXTENDED_ALIGNED_STORAGE",
			"LUMOS_ROOT_DIR="  .. cwd,
			"LUMOS_VOLK"
		}

		libdirs
		{
			"../Dependencies/OpenAL/libs/Win32"
		}

		links
		{
			"glfw",
			"OpenGL32",
			"OpenAL32"
		}

		buildoptions
		{
			"/MP"
		}

	filter "system:macosx"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"
		platforms {"x64"}
		defaultplatform "x64"
		xcodebuildsettings {
			['ALWAYS_SEARCH_USER_PATHS'] = "NO",
			['MACOSX_DEPLOYMENT_TARGET'] = "10.12",
			['CLANG_ENABLE_OBJC_WEAK'] = "YES",
			--['CODE_SIGN_IDENTITY'] = "Mac Developer",
			--['PRODUCT_BUNDLE_IDENTIFIER'] = "com.jmorton06",
			--['ENABLE_HARDENED_RUNTIME'] = "YES",
			['ENABLE_TESTABILITY'] = "YES",
			["WARNING_CFLAGS"] = "-Wall -Wextra " ..
            "-Wno-missing-field-initializers " ..
            "-Wno-unknown-pragmas " ..
            "-Wno-unused-parameter " ..
            "-Wno-unused-local-typedef " ..
            "-Wno-missing-braces " ..
            "-Wno-microsoft-anon-tag ",

		}

		xcodebuildresources { "res/textures/icon.icns" }


		files
		{
			"../Lumos/src/Platform/macOS/Info.plist"
		}

		defines
		{
			"LUMOS_PLATFORM_MACOS",
			"LUMOS_PLATFORM_UNIX",
			"LUMOS_RENDER_API_OPENGL",
			"LUMOS_RENDER_API_VULKAN",
			"VK_EXT_metal_surface",
			"LUMOS_IMGUI",
			"LUMOS_ROOT_DIR="  .. cwd,
			"LUMOS_VOLK"
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
		}


		filter {"system:macosx", "configurations:release"}

			local source = "../Dependencies/vulkan/libs/macOS/**"
			local target = "../bin/release/"
			
			buildmessage("copying "..source.." -> "..target)
			
			postbuildcommands {
				"{COPY} "..source.." "..target
			}

		filter {"system:macosx", "configurations:Production"}

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


		filter "files:**.plist"
			buildaction "Resource"

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
			"LUMOS_IMGUI",
			"LUMOS_ROOT_DIR="
		}

		links
		{
			"QuartzCore.framework",
			"Metal.framework",
        	"IOKit.framework",
        	"CoreFoundation.framework",
			"CoreVideo.framework",
			"CoreGraphics.framework",
			"UIKit.framework",
			"OpenAL.framework"
		}

		libdirs
		{
			"../Dependencies/vulkan/libs/iOS"
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
			['INFOPLIST_FILE'] = "../Lumos/src/Platform/iOS/Client/Info.plist",
			--["ENABLE_BITCODE"] = "NO"
		}

		files
		{
			"../Lumos/src/Platform/iOS/Client/**",
			"../Lumos/res/**",
			"../Sandbox/res/**",
		}

		linkoptions { "-rpath @executable_path/Frameworks" }

		xcodebuildresources 
		{
			"../Lumos/src/Platform/iOS/Client/**",
			"../Lumos/res/**",
			"../Sandbox/res/**", 
		}

		filter {"system:ios", "configurations:release"}

			local source = "../Dependencies/vulkan/libs/iOS/**"
			local target = "../bin/release/"
			
			buildmessage("copying "..source.." -> "..target)
			
			postbuildcommands {
				"{COPY} "..source.." "..target
			}

		filter {"system:ios", "configurations:Production"}

			local source = "../Dependencies/vulkan/libs/iOS/**"
			local target = "../bin/dist/"
			
			buildmessage("copying "..source.." -> "..target)
			
			postbuildcommands {
				"{COPY} "..source.." "..target
			}

		filter {"system:ios", "configurations:debug"}

			local source = "../Dependencies/vulkan/libs/iOS/**"
			local target = "../bin/debug/"
			
			buildmessage("copying "..source.." -> "..target)
			
			postbuildcommands {
				"{COPY} "..source.." "..target
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
			"LUMOS_IMGUI",
			"LUMOS_ROOT_DIR="  .. cwd,
			"LUMOS_VOLK"
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
		}

		links { "X11", "pthread", "dl"}

		linkoptions { "-L%{cfg.targetdir}", "-Wl,-rpath=\\$$ORIGIN" }

	filter "configurations:Debug"
		defines "LUMOS_DEBUG"
		optimize "Off"
		symbols "On"
		runtime "Debug"

	filter "configurations:Release"
		defines "LUMOS_RELEASE"
		optimize "On"
		symbols "On"
		runtime "Release"

	filter "configurations:Production"
		defines "LUMOS_PRODUCTION"
		symbols "Off"
		optimize "Full"
		runtime "Release"
