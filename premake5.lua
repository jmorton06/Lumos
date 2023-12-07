require 'Scripts/premake-utilities/premake-defines'
require 'Scripts/premake-utilities/premake-common'
require 'Scripts/premake-utilities/premake-triggers'
require 'Scripts/premake-utilities/premake-settings'
require 'Scripts/premake-utilities/android_studio'

include "premake-dependencies.lua"
--require 'Scripts/premake-utilities/premake-vscode/vscode'

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

workspace( settings.workspace_name )
	 startproject "LumosEditor"
	flags 'MultiProcessorCompile'
	outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
	targetdir ("bin/%{outputdir}/")
	objdir ("bin-int/%{outputdir}/obj/")

	gradleversion "com.android.tools.build:gradle:7.0.0"

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

	assetpacks
	{
		["pack"] = "install-time",
	}
	
	group "External"
		require("Lumos/External/box2dpremake5")
			SetRecommendedSettings()
		require("Lumos/External/lua/premake5")
			SetRecommendedSettings()
		require("Lumos/External/imguipremake5")
			SetRecommendedSettings()
		require("Lumos/External/freetype/premake5")
			SetRecommendedSettings()
		require("Lumos/External/SPIRVCrosspremake5")
			SetRecommendedSettings()
		require("Lumos/External/spdlog/premake5")
			SetRecommendedSettings()
		require("Lumos/External/ModelLoaders/meshoptimizer/premake5")
			SetRecommendedSettings()
		-- require("Lumos/External/msdf-atlas-gen/msdfgen/premake5")
		-- 	SetRecommendedSettings()
		require("Lumos/External/msdf-atlas-gen/premake5")
			SetRecommendedSettings()
		if not os.istarget(premake.IOS) and not os.istarget(premake.ANDROID) then
			require("Lumos/External/GLFWpremake5")
			SetRecommendedSettings()
		end
			
	filter {}
	group ""

	include "Lumos/premake5"
	include "Runtime/premake5"
	include "Editor/premake5"
