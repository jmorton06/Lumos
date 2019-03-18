require 'Scripts/ios'
require 'Scripts/premakeDefines'

workspace "Lumos"
	architecture "x64"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

	startproject "Sandbox"

	location "build"

	targetdir ("bin/%{cfg.longname}/")
	objdir ("bin-int/%{cfg.longname}/obj/")

	group "Dependencies"
		require("Dependencies/Box2D/premake5")
		require("Dependencies/lua/premake5")
		require("Dependencies/volk/premake5")
		filter "system:not ios"
			require("Dependencies/GLFW/premake5")
			require("Dependencies/glad/premake5")
		filter()
	group ""

	require("Lumos/premake5")
	require("Sandbox/premake5")
	--require("Examples/premake5")

	filter()

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