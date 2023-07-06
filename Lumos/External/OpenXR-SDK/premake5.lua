project "OpenXR-SDK"	
	kind "StaticLib"	
	language "C++"
	staticruntime "off"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "src/external/jsoncpp/include/json/**.h",
        "src/external/jsoncpp/src/lib_json/**.h",
        "src/external/jsoncpp/src/lib_json/**.cpp",

        "src/common/**.h",
        "src/common/**.hpp",
        "src/common/**.c",
        "src/common/**.cpp",
        
        "include/**.h",
        
        "src/*.h",
        "src/*.hpp",
        "src/*.c",
        "src/*.cpp",

        "src/loader/**.h",
        "src/loader/**.hpp",
        "src/loader/**.c",
        "src/loader/**.cpp"
    }
    includedirs
    {
        "include",
        "include/openxr",
        "src",
        "src/common",
        "src/external/jsoncpp/include",
        "%{IncludeDir.VulkanSDK}"
    }

    links
    {
        "%{Library.Vulkan}",
		"%{Library.VulkanUtils}"
    }

	filter "system:windows"
		systemversion "latest"
		cppdialect "C++17"
		staticruntime "Off"
        
        defines
        {
            "XR_OS_WINDOWS",
            "XR_USE_PLATFORM_WIN32",
            "XR_USE_GRAPHICS_API_VULKAN",
            "_WINDOWS"
        }

    filter "system:macosx"
		systemversion "latest"
		cppdialect "C++17"
		staticruntime "Off"
        
        defines
        {
            "XR_OS_APPLE",
            --"XR_USE_PLATFORM_XLIB",
            "XR_USE_GRAPHICS_API_VULKAN",
        }

	filter "system:linux"
		pic "On"
		systemversion "latest"
		cppdialect "C++17"
		staticruntime "Off"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"


	filter "configurations:Release"
		runtime "Release"
		optimize "on"
