project "Lumos"
	kind "StaticLib"
	language "C++"
	editandcontinue "Off"
	staticruntime "Off"

	files
	{
		"Source/**.h",
		"Source/**.c",
		"Source/**.cpp",
		"External/lz4/lz4.c",
		"External/lz4/lz4.h"
	}

	removefiles
	{
		"Source/Lumos/Platform/**",
		-- Unity build optimization: exclude individual Lua binding files
		-- They're combined in LuaBindingsUnity.cpp to share Sol2 template instantiations
		"Source/Lumos/Scripting/Lua/MathsLua.cpp",
		"Source/Lumos/Scripting/Lua/ImGuiLua.cpp",
		"Source/Lumos/Scripting/Lua/PhysicsLua.cpp"
	}

	includedirs
	{
		"",
		"../",
		"Source/",
		"Source/Lumos",
		"Assets/Shaders"
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
		"%{IncludeDir.external}",
		"%{IncludeDir.External}",
		"%{IncludeDir.freetype}",
		"%{IncludeDir.SpirvCross}",
		"%{IncludeDir.cereal}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.msdfgen}",
		"%{IncludeDir.msdf_atlas_gen}",
		"%{IncludeDir.ozz}",
		"%{IncludeDir.Lumos}",
	}

	if _OPTIONS["shaderc"] then
		externalincludedirs { "%{IncludeDir.shaderc}" }
	end

	links
	{
		"lua",
		"box2d",
		"imgui",
		"freetype",
		"SpirvCross",
		"meshoptimizer",
		"msdf-atlas-gen",
		"ozz_base",
		"ozz_animation",
		"ozz_animation_offline",
	}

	defines
	{
		"LUMOS_ENGINE",
		"FREEIMAGE_LIB",
		"IMGUI_USER_CONFIG=\"Source/Lumos/ImGui/ImConfig.h\"",
	}

	filter "system:windows"
		cppdialect "C++17"
		systemversion "latest"
		disablewarnings { 4307 }
		characterset ("Unicode")
		conformancemode "on"
		pchheader "Precompiled.h"
		pchsource "Source/Precompiled.cpp"
		--buildoptions { "-msse4.1" }

		defines
		{
			"LUMOS_PLATFORM_WINDOWS",
			"LUMOS_RENDER_API_VULKAN",
			"VK_USE_PLATFORM_WIN32_KHR",
			"WIN32_LEAN_AND_MEAN",
			"_CRT_SECURE_NO_WARNINGS",
			"_DISABLE_EXTENDED_ALIGNED_STORAGE",
			"_SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING",
			"_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING",
			"LUMOS_IMGUI",
			"LUMOS_OPENAL",
			"LUMOS_VOLK",
			"LUMOS_USE_GLFW_WINDOWS",
			"USE_VMA_ALLOCATOR",
		}

		files
		{
			"Source/Lumos/Platform/Windows/*.h",
			"Source/Lumos/Platform/Windows/*.cpp",

			"Source/Lumos/Platform/GLFW/*.h",
			"Source/Lumos/Platform/GLFW/*.cpp",

			"Source/Lumos/Platform/OpenAL/*.h",
			"Source/Lumos/Platform/OpenAL/*.cpp",

			"Source/Lumos/Platform/Vulkan/*.h",
			"Source/Lumos/Platform/Vulkan/*.cpp"
		}

		links
		{
			"Dbghelp",
			"glfw",
			"Dwmapi.lib"
		}

		if _OPTIONS["shaderc"] then
			defines { "LUMOS_SHADERC" }
			links { "shaderc" }
		end

		filter "action:vs*"
			buildoptions { "/bigobj" }
		filter 'files:External/**.cpp'
			flags  { 'NoPCH' }
		filter 'files:External/**.c'
		    flags { 'NoPCH' }

		filter { 'system:windows', 'architecture:x86_64' }
		    defines { "LUMOS_SSE"  }

	filter "system:macosx"
		cppdialect "C++17"
		systemversion "11.0"

		-- Swift back-deploy libs would have to be linked by every app using libLumos. Store.swift is all macOS 12+.
		xcodebuildsettings { ['OTHER_SWIFT_FLAGS'] = '-runtime-compatibility-version none' }

		files
		{
			"Source/Lumos/Platform/macOS/*.mm",
			"Source/Lumos/Platform/macOS/*.h",
			"Source/Lumos/Platform/macOS/*.cpp",
			"Source/Lumos/Platform/Apple/*.swift",

			"Source/Lumos/Platform/Unix/*.h",
			"Source/Lumos/Platform/Unix/*.cpp",

			"Source/Lumos/Platform/GLFW/*.h",
			"Source/Lumos/Platform/GLFW/*.cpp",

			"Source/Lumos/Platform/OpenAL/*.h",
			"Source/Lumos/Platform/OpenAL/*.cpp",

			"Source/Lumos/Platform/Vulkan/*.h",
			"Source/Lumos/Platform/Vulkan/*.cpp"
		}

		removefiles
    	{
    		"Source/Precompiled.h",
    		"Source/Precompiled.cpp"
    	}

		defines
		{
			"LUMOS_PLATFORM_MACOS",
			"LUMOS_PLATFORM_UNIX",
			"LUMOS_RENDER_API_VULKAN",
			"VK_USE_PLATFORM_METAL_EXT",
			"LUMOS_IMGUI",
			"LUMOS_OPENAL",
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
			"AudioToolbox.framework",
			"CoreAudio.framework",
			"MediaPlayer.framework",
			"AVKit.framework",
			"CoreServices.framework",
			"CoreMIDI.framework",
			"SystemConfiguration.framework",
			"StoreKit.framework",
			"glfw"
		}

		libdirs
		{
			"../bin/**",
		}

		if _OPTIONS["shaderc"] then
			defines { "LUMOS_SHADERC" }
			links { "shaderc" }
		end

		buildoptions
		{
			"-Wno-attributes",
			"-Wno-nullability-completeness",
			"-fdiagnostics-absolute-paths",
		}

		pchheader "Source/Precompiled.pch"
		pchsource "Source/Precompiled.cpp"

		filter 'files:External/**.cpp'
			flags  { 'NoPCH' }
		filter 'files:External/**.c'
			flags  { 'NoPCH' }
		filter 'files:Source/Lumos/**.m'
			flags  { 'NoPCH' }
		filter 'files:Source/Lumos/**.mm'
			flags  { 'NoPCH' }

	filter "system:ios"
		cppdialect "C++17"
		systemversion "latest"
		kind "StaticLib"

		defines
		{
			-- Tracy still listens, but its UDP "here I am" broadcast is what makes iOS ask for local network access
			"TRACY_NO_BROADCAST",
			"LUMOS_PLATFORM_IOS",
			"LUMOS_PLATFORM_MOBILE",
			"LUMOS_PLATFORM_UNIX",
			"LUMOS_RENDER_API_VULKAN",
			"VK_USE_PLATFORM_IOS_MVK",
			"LUMOS_IMGUI",
            "LUMOS_OPENAL",
		}

		if _OPTIONS["shaderc"] then
			defines { "LUMOS_SHADERC" }
		end

		files
		{
			"Source/Lumos/Platform/iOS/*.mm",
			"Source/Lumos/Platform/iOS/*.h",
			"Source/Lumos/Platform/iOS/*.cpp",
			"Source/Lumos/Platform/MacOS/MacOSNowPlaying.mm", -- lock screen / Control Center (shared with macOS)
			"Source/Lumos/Platform/MacOS/MacOSHttp.mm",       -- Foundation only, shared with macOS
			"Source/Lumos/Platform/MacOS/MacOSHttpStream.mm",
			"Source/Lumos/Platform/MacOS/MacOSBookmarks.mm",
			"Source/Lumos/Platform/MacOS/MacOSRoutePicker.mm",
			"Source/Lumos/Platform/MacOS/MacOSMusicStream.cpp", -- RemoteIO on iOS
			"Source/Lumos/Platform/Apple/*.swift",

			"Source/Lumos/Platform/OpenAL/*.h",
			"Source/Lumos/Platform/OpenAL/*.cpp",

			"Source/Lumos/Platform/Unix/*.h",
			"Source/Lumos/Platform/Unix/*.cpp",

			"Source/Lumos/Platform/Vulkan/*.h",
			"Source/Lumos/Platform/Vulkan/*.cpp"
		}

		removefiles
		{
			"Source/Lumos/Platform/Unix/UnixFileSystem.cpp"
		}

		links
		{
			"QuartzCore.framework",
			"Metal.framework",
        	"IOKit.framework",
        	"CoreFoundation.framework",
			"CoreVideo.framework",
			"OpenAL.framework",
			"AudioToolbox.framework",
			"AVFoundation.framework",
			"MediaPlayer.framework",
			"AVKit.framework",
			"StoreKit.framework"
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
			"LUMOS_RENDER_API_VULKAN",
			"VK_USE_PLATFORM_XCB_KHR",
			"LUMOS_IMGUI",
			"LUMOS_VOLK",
			"LUMOS_OPENAL",
			"USE_VMA_ALLOCATOR",
		}

		files
		{
			"Source/Lumos/Platform/Unix/*.h",
			"Source/Lumos/Platform/Unix/*.cpp",

			"Source/Lumos/Platform/GLFW/*.h",
			"Source/Lumos/Platform/GLFW/*.cpp",

			"Source/Lumos/Platform/OpenAL/*.h",
			"Source/Lumos/Platform/OpenAL/*.cpp",

			"Source/Lumos/Platform/Vulkan/*.h",
			"Source/Lumos/Platform/Vulkan/*.cpp"
		}

		links
		{
			"glfw",
			"libopenal"
		}

		linkoptions{ "-Wl,-rpath=\\$$ORIGIN" }

		libdirs
		{
			"../bin/**",
			"External/OpenAL/libs/linux",
		}

		buildoptions
		{
			"-fpermissive",
			"-fPIC",
			"-Wignored-attributes",
			"-Wno-psabi"
		}

		links { "X11", "pthread" }

		if _OPTIONS["shaderc"] then
			defines { "LUMOS_SHADERC" }
			links { "shaderc" }
		end

		pchheader "../Lumos/Source/Precompiled.h"
		pchsource "../Lumos/Source/Precompiled.cpp"

		filter 'files:External/**.cpp'
			flags  { 'NoPCH' }
		filter 'files:External/**.c'
			flags  { 'NoPCH' }
		filter 'files:Source/Lumos/**.c'
			flags  { 'NoPCH' }

		filter {'system:linux', 'architecture:x86_64'}
			buildoptions
			{
				"-msse4.1",
			}
        filter { 'system:linux', 'architecture:x86_64' }
		    defines { "LUMOS_SSE"  }

	filter "configurations:Debug"
        defines { "LUMOS_DEBUG", "_DEBUG","TRACY_ENABLE","LUMOS_PROFILE_ENABLED","TRACY_ON_DEMAND"  }
		symbols "On"
		runtime "Debug"
		optimize "Off"

	filter "configurations:Release"
        defines { "LUMOS_RELEASE", "NDEBUG", "TRACY_ENABLE","LUMOS_PROFILE_ENABLED", "TRACY_ON_DEMAND"}
		optimize "Speed"
		symbols "On"
		runtime "Release"

	filter "configurations:Production"
		defines { "LUMOS_PRODUCTION", "NDEBUG" }
		symbols "On"
		optimize "Full"
		runtime "Release"
