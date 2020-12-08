require 'Scripts/premake-ios'
require 'Scripts/premake-defines'
require 'Scripts/premake-common'
require 'Scripts/premake-vscode/vscode'

root_dir = os.getcwd()

workspace "Lumos"
	startproject "Sandbox"
	location "build"
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

	flags
	{
		"MultiProcessorCompile"
	}

    outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

	targetdir ("bin/%{outputdir}/")
	objdir ("bin-int/%{outputdir}/obj/")

	group "External"
		require("Lumos/external/box2d/premake5")
			SetRecommendedSettings()
		require("Lumos/external/lua/premake5")
			SetRecommendedSettings()
		require("Lumos/external/imgui/premake5")
			SetRecommendedSettings()
		require("Lumos/external/freetype/premake5")
			SetRecommendedSettings()
		require("Lumos/external/SPIRVCrosspremake5")
			SetRecommendedSettings()
		filter "system:not ios"
			require("Lumos/external/GLFWpremake5")
				SetRecommendedSettings()
		filter()
	group ""

	include "Lumos/premake5"
	include "Sandbox/premake5"

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
