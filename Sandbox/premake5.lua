IncludeDir = {}
IncludeDir["GLFW"] = "../Lumos/external/glfw/include/"
IncludeDir["Glad"] = "../Lumos/external/glad/include/"
IncludeDir["lua"] = "../Lumos/external/lua/src/"
IncludeDir["stb"] = "../Lumos/external/stb/"
IncludeDir["OpenAL"] = "../Lumos/external/OpenAL/include/"
IncludeDir["Box2D"] = "../Lumos/external/box2d/include/"
IncludeDir["vulkan"] = "../Lumos/external/vulkan/"
IncludeDir["Lumos"] = "../Lumos/src"
IncludeDir["External"] = "../Lumos/external/"
IncludeDir["ImGui"] = "../Lumos/external/imgui/"
IncludeDir["freetype"] = "../Lumos/external/freetype/include"
IncludeDir["SpirvCross"] = "../Lumos/external/SPIRV-Cross"
IncludeDir["cereal"] = "../Lumos/external/cereal/include"
IncludeDir["spdlog"] = "../Lumos/external/spdlog/include"

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
		"%{IncludeDir.External}",
		"%{IncludeDir.spdlog}",
		"%{IncludeDir.freetype}",
		"%{IncludeDir.SpirvCross}",
		"%{IncludeDir.cereal}",
		"%{IncludeDir.Lumos}",
	}

	links
	{
		"Lumos",
		"lua",
		"box2d",
		"imgui",
		"freetype",
		"SpirvCross"
}

defines
{
	"LUMOS_PROFILE",
	"TRACY_ENABLE",
}

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
			"_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING",
			"LUMOS_ROOT_DIR="  .. root_dir,
			"LUMOS_VOLK",
	"LUMOS_SSE",
		}

		libdirs
		{
			"../Lumos/external/OpenAL/libs/Win32"
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

		disablewarnings { 4307 }

	filter "system:macosx"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"
		editandcontinue "Off"

xcodebuildresources { "Images.xcassets", "libMoltenVK.dylib" }

		xcodebuildsettings
		{
			['ARCHS'] = false,
			['CODE_SIGN_IDENTITY'] = 'Mac Developer',
			['PRODUCT_BUNDLE_IDENTIFIER'] = 'com.jmorton06',
			['DEVELOPMENT_TEAM'] = 'C5L4T5BF6L',
			['INFOPLIST_FILE'] = '../Lumos/src/Platform/macOS/Info.plist',
			['ASSETCATALOG_COMPILER_APPICON_NAME'] = 'AppIcon'
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
			"LUMOS_ROOT_DIR="  .. root_dir,
			"LUMOS_VOLK",
			"LUMOS_SSE"
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

		files
		{
	"../Resources/MacOSIcons/Images.xcassets",
	"../Lumos/external/vulkan/libs/macOS/libMoltenVK.dylib"
		}

		links
		{
			"glfw",
		}

		SetRecommendedXcodeSettings()

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
            "MetalKit.framework",
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
			"../Lumos/external/vulkan/libs/iOS/libMoltenVK.a"
		}

		files
		{
			"../Resources/IOSIcons/Images.xcassets",
	"../Lumos/res/EngineShaders",
	"../Lumos/src/Platform/iOS/Client/**",
            "res/shaders",
            "res/scenes",
            "res/scripts",
            "res/meshes",
            "res/sounds",
            "res/textures",
	"Sandbox.lmproj",
	"Launch Screen.storyboard"
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
			"Images.xcassets",
            "EngineShaders",
            "shaders",
            "meshes",
            "scenes",
            "scripts",
            "sounds",
            "textures",
            "Sandbox.lmproj"
		}
		SetRecommendedXcodeSettings()

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
			"LUMOS_ROOT_DIR="  .. root_dir,
			"LUMOS_VOLK"
		}

		buildoptions
		{
			"-fpermissive",
			"-Wattributes",
			"-fPIC",
			"-Wignored-attributes",
			"-Wno-psabi"
		}

		links
		{
			"glfw",
		}

		links { "X11", "pthread", "dl", "atomic", "stdc++fs"}

		linkoptions { "-L%{cfg.targetdir}", "-Wl,-rpath=\\$$ORIGIN" }

		if _OPTIONS["arch"] ~= "arm" then
			buildoptions
			{
				"-msse4.1",
			}

			defines { "LUMOS_SSE" ,"USE_VMA_ALLOCATOR"}
		end

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
