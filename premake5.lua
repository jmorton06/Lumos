require 'Scripts/premake-defines'
require 'Scripts/premake-common'
require 'Scripts/premake-triggers'
require 'Scripts/premake-vscode/vscode'

root_dir = os.getcwd()

Arch = ""

if _OPTIONS["arch"] then
	Arch = _OPTIONS["arch"]
else
	if _OPTIONS["os"] then
		_OPTIONS["arch"] = "arm"
		Arch = "arm"
	else
		_OPTIONS["arch"] = "x64"
		Arch = "x64"
	end
end

workspace "Lumos"
	location "build"
	flags { 'MultiProcessorCompile' }
	
	outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
	targetdir ("bin/%{outputdir}/")
	objdir ("bin-int/%{outputdir}/obj/")

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
		if not os.istarget(premake.IOS) and not os.istarget(premake.ANDROID) then
			require("Lumos/external/GLFWpremake5")
			SetRecommendedSettings()
		end
			
	filter {}
	group ""

	include "Lumos/premake5"
	include "Sandbox/premake5"

workspace("Lumos")
	startproject("Sandbox")
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
