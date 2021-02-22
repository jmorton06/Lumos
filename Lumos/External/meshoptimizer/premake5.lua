project( "meshoptimizer" )
    kind "StaticLib"
	staticruntime "On"
    files { "src/*.h", "src/*.cpp" }

	
  filter "configurations:Debug"
	symbols "On"
    runtime "Debug"

  filter "configurations:Release"
    runtime "Release"
	optimize "On"

  filter "configurations:Production"
    runtime "Release"
    optimize "On"