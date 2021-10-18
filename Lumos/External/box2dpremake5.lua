-- Box2D premake5 script.
-- https://premake.github.io/

project 'box2d'
	kind 'StaticLib'
	cppdialect "C++14"
	staticruntime "On"
	files { 'box2d/src/**' }
	includedirs { 'box2d/include' , 'box2d/src' }

	filter "system:windows"
		systemversion "latest"
   		buildoptions { "/MP" }
	filter "system:linux"
		buildoptions { "-fPIC" }

    filter "configurations:Debug"
		symbols "On"

	filter "configurations:Release"
		optimize "On"

	filter "configurations:Production"
		optimize "On"