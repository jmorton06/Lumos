workspace "LumosEngine"
	architecture "x64"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

	startproject "Sandbox"

	location "build"

	outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

	targetdir ("bin/" .. outputdir)
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	group "Dependencies"
		require("Dependencies/Box2D/premake5")
		require("Dependencies/GLFW/premake5")
		require("Dependencies/lua/premake5")
		require("Dependencies/glad/premake5")
	group ""

	require("Lumos/premake5")
	require("Sandbox/premake5")
	--require("Examples/premake5")

newaction 
{
	trigger     = "clean",
	description = "clean the software",
	execute     = function ()
		print("clean the build...")
		os.rmdir("./build")
		os.rmdir("./bin")
		os.rmdir("./bin-int")
		print("done.")
	end
}