-- Box2D premake5 script.
-- https://premake.github.io/

project 'Box2D'
	kind 'StaticLib'
	cppdialect "C++14"
	files { 'Box2D/**' }
	includedirs { '.' }

    filter "system:windows"
   		buildoptions { "/MP" }
	filter "system:linux"
		buildoptions { "-fPIC" }
    filter "configurations:Debug"
		symbols "On"

	filter "configurations:Release"
		optimize "On"

	filter "configurations:Dist"
		optimize "On"