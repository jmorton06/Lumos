project "ozz_base"	
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

	files
	{
		"include/ozz/base/**.h",
		"src/base/**.cc"
	}

	includedirs
	{
		"include",
		"src"
	}

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		runtime "Release"
		optimize "on"

	SetRecommendedSettings()


project "ozz_animation"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

	files
	{
	    "include/ozz/animation/runtime/**.h",
		"src/animation/runtime/**.h",
		"src/animation/runtime/**.cc"
	}

	includedirs
	{
		"include",
		"src"
	}

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		runtime "Release"
		optimize "on"

	SetRecommendedSettings()


-- ozz_geometry is for software skinning
-- we dont need this
--project "ozz_geometry"
--	kind "StaticLib"
--	language "C++"
--	cppdialect "C++11"
--	staticruntime "off"
--
--	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
--	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
--
--	files
--	{
--	    "include/ozz/geometry/runtime/**.h",
--		"src/geometry/runtime/**.h",
--		"src/geometry/runtime/**.cc"
--	}
--
--	includedirs
--	{
--		"include",
--		"src"
--	}
--
--	filter "system:windows"
--		systemversion "latest"
--
--	filter "configurations:Debug"
--		runtime "Debug"
--		symbols "on"
--
--	filter "configurations:Release"
--		runtime "Release"
--		optimize "on"
--
--	filter "configurations:Dist"
--		runtime "Release"
--		optimize "on"


-- ozz_animation_offline provides support for import of assets from digital content creators
-- (where we write the actual importers ourselves)
project "ozz_animation_offline"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

	-- wildcards are not recursive here, on purpose
	files
	{
		"include/ozz/animation/offline/*.h",
		"src/animation/offline/*.h",
		"src/animation/offline/*.cc",
		"src/animation/offline/tools/*.cc",
		"src/options/*.cc",
		"extern/jsoncpp/dist/jsoncpp.cpp"

	}

	includedirs
	{
		"include",
		"src"
	}

	externalincludedirs
	{
		"extern/jsoncpp/dist",
	}

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"

	SetRecommendedSettings()