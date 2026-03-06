project "LumosEditor"
	kind "WindowedApp"
	language "C++"
	editandcontinue "Off"

	files
	{
		"**.h",
		"**.cpp",
		"Source/**.h",
		"Source/**.cpp"
	}

	includedirs
	{
		"../Lumos/Source/Lumos",
	}

	externalincludedirs
	{
		"%{IncludeDir.entt}",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.lua}",
		"%{IncludeDir.stb}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.OpenAL}",
		"%{IncludeDir.Box2D}",
		"%{IncludeDir.vulkan}",
		"%{IncludeDir.External}",
		"%{IncludeDir.freetype}",
		"%{IncludeDir.SpirvCross}",
		"%{IncludeDir.cereal}",
		"%{IncludeDir.msdfgen}",
		"%{IncludeDir.msdf_atlas_gen}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.ozz}",
		"%{IncludeDir.Lumos}",
	}

	if _OPTIONS["shaderc"] then
		externalincludedirs { "%{IncludeDir.shaderc}" }
	end

	links
	{
		"Lumos",
		"lua",
		"box2d",
		"imgui",
		"freetype",
		"SpirvCross",
		"meshoptimizer",
		"msdf-atlas-gen",
		"ozz_animation",
		"ozz_animation_offline",
		"ozz_base"
	}

	defines
	{
		"IMGUI_USER_CONFIG=\"../../Lumos/Source/Lumos/ImGui/ImConfig.h\"",
	}

	filter { "files:External/**"}
		warnings "Off"

	filter "system:windows"
		cppdialect "C++17"
		staticruntime "Off"
		systemversion "latest"
		conformancemode "on"

		defines
		{
			"LUMOS_PLATFORM_WINDOWS",
			"LUMOS_RENDER_API_VULKAN",
			"VK_USE_PLATFORM_WIN32_KHR",
			"WIN32_LEAN_AND_MEAN",
			"_CRT_SECURE_NO_WARNINGS",
			"_DISABLE_EXTENDED_ALIGNED_STORAGE",
			"_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING",
			"LUMOS_VOLK",
			"USE_VMA_ALLOCATOR"
		}

		libdirs
		{
			"../Lumos/External/OpenAL/libs/Win32"
		}

		links
		{
			"glfw",
			"OpenAL32",
		}

		if _OPTIONS["shaderc"] then
			links { "shaderc" }
		end

		postbuildcommands { "xcopy /Y /C \"..\\Lumos\\External\\OpenAL\\libs\\Win32\\OpenAL32.dll\" \"$(OutDir)\"" }

		disablewarnings { 4307 }

		filter { 'system:windows', 'architecture:x86_64' }
		    defines { "LUMOS_SSE"  }

	filter "system:macosx"
		cppdialect "C++17"
		staticruntime "Off"
		systemversion "11.0"
		editandcontinue "Off"

		xcodebuildresources { "Assets.xcassets" }

		xcodebuildsettings
		{
			['ARCHS'] = '$(ARCHS_STANDARD)',
			['INFOPLIST_FILE'] = '../Lumos/Source/Lumos/Platform/macOS/Info.plist',
			['ASSETCATALOG_COMPILER_APPICON_NAME'] = 'AppIcon',
			['DEBUG_INFORMATION_FORMAT'] = 'dwarf-with-dsym'
		}

		filter {"system:macosx", "configurations:Release or Production"}
		xcodebuildsettings
		{
			['ENABLE_HARDENED_RUNTIME'] = 'YES',
			['CODE_SIGN_ENTITLEMENTS'] = '../Editor/LumosEditor.entitlements',
			['DEBUG_INFORMATION_FORMAT'] = 'dwarf-with-dsym',
			['GCC_GENERATE_DEBUGGING_SYMBOLS'] = 'YES'
		}

		filter "system:macosx"

		if game_project then
			xcodebuildsettings
			{
				['PRODUCT_NAME'] = game_project.title .. " Editor",
				['PRODUCT_BUNDLE_IDENTIFIER'] = game_project.bundle_id .. ".editor",
				['MARKETING_VERSION'] = game_project.version,
				['CURRENT_PROJECT_VERSION'] = game_project.build_number
			}
			xcodebuildresources { "../" .. game_project.rel_dir }
		end

		if settings.enable_signing then
		xcodebuildsettings
		{
			['CODE_SIGN_STYLE'] = 'Automatic',
			['DEVELOPMENT_TEAM'] = settings.developer_team,
			['PRODUCT_BUNDLE_IDENTIFIER'] = game_project and (game_project.bundle_id .. ".editor") or settings.bundle_identifier
		}
		end

		defines
		{
			"LUMOS_PLATFORM_MACOS",
			"LUMOS_PLATFORM_UNIX",
			"LUMOS_RENDER_API_VULKAN",
			"VK_USE_PLATFORM_METAL_EXT",
			"LUMOS_IMGUI"
		}

		linkoptions
		{
			"-framework Cocoa",
			"-framework IOKit",
			"-framework CoreVideo",
			"-framework OpenAL",
			"-framework QuartzCore",
			"-framework Metal",
			"-framework IOSurface",
			"-framework Foundation",
			"-framework UniformTypeIdentifiers",
			"../Lumos/External/vulkan/libs/macOS/libMoltenVK.a"
		}

		files
		{
			"../Resources/AppIcons/Assets.xcassets",
			"Source/**.mm"
		}

		links
		{
			"glfw",
		}

		if _OPTIONS["shaderc"] then
			links { "shaderc" }
		end

	filter "system:ios"
		cppdialect "C++17"
		staticruntime "Off"
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
			"IOSurface.framework",
			"UniformTypeIdentifiers.framework",
		}

		if _OPTIONS["shaderc"] then
			links { "shaderc" }
		end

		linkoptions
		{
			"../Lumos/External/vulkan/libs/iOS/libMoltenVK.a"
		}

		local ios_project_dir = game_project and ("../" .. game_project.rel_dir) or "../ExampleProject/"
		local ios_project_folder = game_project and game_project.folder_name or "ExampleProject"

		files
		{
			"../Resources/AppIcons/Assets.xcassets",
			"../Lumos/Assets/Shaders",
			"../Lumos/Source/Lumos/Platform/iOS/Client/**",
			ios_project_dir
		}

		xcodebuildsettings
		{
			['ARCHS'] = '$(ARCHS_STANDARD)',
			['ONLY_ACTIVE_ARCH'] = 'NO',
			['SDKROOT'] = 'iphoneos',
			['TARGETED_DEVICE_FAMILY'] = '1,2',
			['SUPPORTED_PLATFORMS'] = 'iphonesimulator iphoneos',
			['CODE_SIGN_IDENTITY'] = '',
			['IPHONEOS_DEPLOYMENT_TARGET'] = '13.0',
			['INFOPLIST_FILE'] = '../Lumos/Source/Lumos/Platform/iOS/Editor/Info.plist',
			['ASSETCATALOG_COMPILER_APPICON_NAME'] = 'AppIcon',
		}

		if game_project then
			xcodebuildsettings
			{
				['PRODUCT_BUNDLE_IDENTIFIER'] = game_project.bundle_id .. ".editor",
				['MARKETING_VERSION'] = game_project.version,
				['CURRENT_PROJECT_VERSION'] = game_project.build_number
			}
		end

		if settings.enable_signing then
		xcodebuildsettings
		{
			['PRODUCT_BUNDLE_IDENTIFIER'] = game_project and (game_project.bundle_id .. ".editor") or settings.bundle_identifier,
			['CODE_SIGN_IDENTITY[sdk=iphoneos*]'] = 'iPhone Developer',
			['DEVELOPMENT_TEAM'] = settings.developer_team
		}
		end

		if _OPTIONS["teamid"] then
			xcodebuildsettings {
				["DEVELOPMENT_TEAM"] = _OPTIONS["teamid"]
			}
		end

		if _OPTIONS["tvOS"] then
			xcodebuildsettings {
				["SDKROOT"] = _OPTIONS["tvos"],
				['TARGETED_DEVICE_FAMILY'] = '3',
				['SUPPORTED_PLATFORMS'] = 'tvos'
			}
		end

		linkoptions { "-rpath @executable_path/Frameworks" }

		excludes
		{
			("**.DS_Store")
		}

		xcodebuildresources
		{
			"../Lumos/Source/Platform/iOS/Client",
			"Assets.xcassets",
            "Shaders",
            ios_project_folder
		}

	filter "system:linux"
		cppdialect "C++17"
		staticruntime "Off"
		systemversion "latest"

		defines
		{
			"LUMOS_PLATFORM_LINUX",
			"LUMOS_PLATFORM_UNIX",
			"LUMOS_RENDER_API_VULKAN",
			"VK_USE_PLATFORM_XCB_KHR",
			"LUMOS_IMGUI",
			"LUMOS_VOLK",
			"USE_VMA_ALLOCATOR"
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

		if _OPTIONS["shaderc"] then
			links { "shaderc" }
		end

		links { "X11", "pthread", "dl", "atomic", "openal"}

		linkoptions { "-L%{cfg.targetdir}", "-Wl,-rpath=\\$$ORIGIN"}

		filter {'system:linux', 'architecture:x86_64'}
			buildoptions
			{
				"-msse4.1",
			}
		filter { 'system:linux', 'architecture:x86_64'}
		    defines { "LUMOS_SSE"  }


		filter "system:android"
			androidsdkversion "29"
			androidminsdkversion "29"
			cppdialect "C++17"
			staticruntime "Off"
			systemversion "latest"
			kind "WindowedApp"
			targetextension ".apk"
			editandcontinue "Off"

			defines
			{
				"LUMOS_PLATFORM_ANDROID",
				"LUMOS_PLATFORM_MOBILE",
				"LUMOS_PLATFORM_UNIX",
				"LUMOS_RENDER_API_VULKAN",
				"LUMOS_IMGUI"
			}

			--Adding assets from Game Project folder. Needs rework
			files
			{
				"../Lumos/Assets/Shaders",
				"../Lumos/Source/Lumos/Platform/Android/**",
				"../ExampleProject/Assets/Scenes",
				"../ExampleProject/Assets/Scripts",
				"../ExampleProject/Assets/Meshes",
				"../ExampleProject/Assets/Sounds",
				"../ExampleProject/Assets/Textures",
				"../ExampleProject/Example.lmproj"
			}

			links
			{
				"log" -- required for c++ logging
			}

			buildoptions
			{
				"-std=c++17" -- flag mainly here to test cmake compile options
			}

			linkoptions
			{
				"--no-undefined" -- this flag is used just to cmake link libraries
			}

			includedirs
			{
				"h"
			}

			androidabis
			{
				"arm64-v8a"
			}

			androiddependencies
			{
				"com.android.support:support-v4:27.1.0",
			}

			assetpackdependencies
			{
				"pack"
			}

	filter "configurations:Debug"
		defines { "LUMOS_DEBUG", "_DEBUG","TRACY_ENABLE","LUMOS_PROFILE_ENABLED","TRACY_ON_DEMAND" }
		symbols "On"
		runtime "Debug"
		optimize "Off"

	filter "configurations:Release"
defines { "LUMOS_RELEASE", "NDEBUG", "TRACY_ENABLE","LUMOS_PROFILE_ENABLED","TRACY_ON_DEMAND"}
		optimize "Speed"
		symbols "On"
		runtime "Release"

	filter "configurations:Production"
		defines { "LUMOS_PRODUCTION", "NDEBUG" }
		symbols "On"
		optimize "Full"
		runtime "Release"
