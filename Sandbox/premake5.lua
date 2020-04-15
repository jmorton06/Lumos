IncludeDir = {}
IncludeDir["GLFW"] = "../Dependencies/glfw/include/"
IncludeDir["Glad"] = "../Lumos/external/glad/include/"
IncludeDir["lua"] = "../Dependencies/lua/src/"
IncludeDir["stb"] = "../Lumos/external/stb/"
IncludeDir["OpenAL"] = "../Dependencies/OpenAL/include/"
IncludeDir["Box2D"] = "../Dependencies/Box2D/"
IncludeDir["Dependencies"] = "../Dependencies/"
IncludeDir["vulkan"] = "../Dependencies/vulkan/"
IncludeDir["jsonhpp"] = "../Lumos/external/jsonhpp/"
IncludeDir["Lumos"] = "../Lumos/src"
IncludeDir["External"] = "../Lumos/external/"
IncludeDir["ImGui"] = "../Dependencies/imgui/"

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
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.lua}",
		"%{IncludeDir.stb}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.OpenAL}",
		"%{IncludeDir.Box2D}",
		"%{IncludeDir.vulkan}",
		"%{IncludeDir.Dependencies}",
		"%{IncludeDir.External}",
		"%{IncludeDir.jsonhpp}",
		"%{IncludeDir.spdlog}",
		"%{IncludeDir.Lumos}",
	}

	links
	{
		"Lumos",
		"lua",
		"Box2D",
		"imgui"
	}

	cwd = os.getcwd() .. "/.."

	filter { "files:external/**"}
		warnings "Off"

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
		editandcontinue "Off"
		platforms {"x64"}
 		defaultplatform "x64"

		xcodebuildresources { "../Assets/textures/icon.icns" }

		xcodebuildsettings
		{
			['ARCHS'] = false,
			['CODE_SIGN_IDENTITY'] = 'Mac Developer',
			['PRODUCT_BUNDLE_IDENTIFIER'] = 'com.jmorton06',
			['DEVELOPMENT_TEAM'] = 'C5L4T5BF6L',
			['INFOPLIST_FILE'] = '../Lumos/src/Platform/macOS/Info.plist',
			--['ENABLE_HARDENED_RUNTIME'] = 'YES'

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

		SetRecommendedXcodeSettings()

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

	filter "system:ios"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"
		kind "WindowedApp"
		targetextension ".app"
		editandcontinue "Off"

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
			"CoreGraphics.framework",
			"UIKit.framework",
			"OpenAL.framework",
			"AudioToolbox.framework",
			"Foundation.framework",
			"SystemConfiguration.framework",
		}

		linkoptions
		{
			"../Dependencies/vulkan/libs/iOS/libMoltenVK.a"
		}

		xcodebuildsettings
		{
			['ARCHS'] = '$(ARCHS_STANDARD)',
			['ONLY_ACTIVE_ARCH'] = 'NO',
			['SDKROOT'] = 'iphoneos',
			['TARGETED_DEVICE_FAMILY'] = '1,2',
			['SUPPORTED_PLATFORMS'] = 'iphonesimulator iphoneos',
			['CODE_SIGN_IDENTITY[sdk=iphoneos*]'] = 'iPhone Developer',
			['IPHONEOS_DEPLOYMENT_TARGET'] = '12.1',
			['PRODUCT_BUNDLE_IDENTIFIER'] = 'com.jmorton06',
			['DEVELOPMENT_TEAM'] = 'C5L4T5BF6L',
			['INFOPLIST_FILE'] = '../Lumos/src/Platform/iOS/Client/Info.plist',
			['ASSETCATALOG_COMPILER_APPICON_NAME'] = 'AppIcon',
			['ASSETCATALOG_COMPILER_LAUNCHIMAGE_NAME'] = 'LaunchImage'
		}

		if _OPTIONS["teamid"] then
			xcodebuildsettings {
				["DEVELOPMENT_TEAM"] = _OPTIONS["teamid"]
			}
		end

		linkoptions { "-rpath @executable_path/Frameworks" }

		excludes 
		{ 
			("**.DS_Store")
		}

		xcodebuildresources 
		{
			"../Lumos/src/Platform/iOS/Client",
			--"Assets",
			--"Images.xcassets"
		}

		SetRecommendedXcodeSettings()

		filter {"system:ios", "configurations:release"}

			local source = "../Assets/**"
			local target = "../bin/release/Sandbox.app/Assets"
				
			buildmessage("copying "..source.." -> "..target)
			
			os.mkdir(target)

			postbuildcommands {
				"{COPY} "..source.." "..target
			}

		filter {"system:ios", "configurations:Production"}

			local source = "../Assets/**"
			local target = "../bin/dist/Sandbox.app/Assets"
				
			buildmessage("copying "..source.." -> "..target)
			
			os.mkdir(target)

			postbuildcommands {
				"{COPY} "..source.." "..target
			}

		filter {"system:ios", "configurations:debug"}

			local source = "../Assets/**"
			local target = "../bin/debug/Sandbox.app/Assets"
				
			buildmessage("copying "..source.." -> "..target)
			
			os.mkdir(target)

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
