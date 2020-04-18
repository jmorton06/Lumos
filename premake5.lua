require 'Scripts/premake-ios'
require 'Scripts/premake-defines'
require 'Scripts/premake-common'

workspace "Lumos"
	
	Arch = ""

	if _OPTIONS["arch"] then
		Arch = _OPTIONS["arch"]
	else
		Arch = "x64"
	end

	if Arch == "arm" then 
		architecture "ARM"
	elseif Arch == "x64" then 
		architecture "x86_64"
	elseif Arch == "x86" then
		architecture "x86"
	end

	print("Arch = ", Arch)

	configurations
	{
		"Debug",
		"Release",
		"Production"
	}

	startproject "Sandbox"

	location "build"

	targetdir ("bin/%{cfg.longname}/")
	objdir ("bin-int/%{cfg.longname}/obj/")

	group "Dependencies"
		require("Dependencies/Box2D/premake5")
			SetRecommendedSettings()
		require("Dependencies/lua/premake5")
			SetRecommendedSettings()
		require("Dependencies/imgui/premake5")
			SetRecommendedSettings()
		filter "system:not ios"
			require("Dependencies/glfw/premake5")
				SetRecommendedSettings()
		filter()
	group ""

	require("Lumos/premake5")
	require("Sandbox/premake5")

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