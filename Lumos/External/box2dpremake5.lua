-- Box2D premake5 script.
-- https://premake.github.io/

project 'box2d'	
	kind "StaticLib"
	language "C"
	cppdialect "C++17"
	cdialect "C11"
	vectorextensions "SSE2"
	staticruntime "on"
	files { 'box2d/src/**' }
	includedirs { 'box2d/include' , 'box2d/src', 'box2d/extern/simde' }

	filter "system:windows"
		systemversion "latest"
		buildoptions {"/experimental:c11atomics"}
		flags{}
	filter "system:linux"
		buildoptions { "-fPIC" }

    filter "configurations:Debug"
		symbols "On"

	filter "configurations:Release"
		optimize "On"

	filter "configurations:Production"
		optimize "On"