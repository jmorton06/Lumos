require 'Scripts/premake-utilities/premake-defines'
require 'Scripts/premake-utilities/premake-common'
require 'Scripts/premake-utilities/premake-triggers'
require 'Scripts/premake-utilities/premake-settings'
require 'Scripts/premake-utilities/android_studio'
require 'Scripts/premake-utilities/premake-project-config'

newoption
{
	trigger = "shaderc",
	description = "Build with shaderc (enables runtime shader compilation)"
}

newoption
{
	trigger = "package",
	description = "Packaged build: bundle the game's Assets.lpak (+ .lmproj) instead of its loose project folder"
}

newoption
{
	trigger = "time-trace",
	description = "Build with -ftime-trace (clang only)"
}

include "premake-dependencies.lua"
--require 'Scripts/premake-utilities/premake-vscode/vscode'

root_dir = os.getcwd()

LoadGameProject()

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
	elseif Arch == "universal" then
		architecture "universal" -- macOS: Apple Silicon + Intel in one app
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

	-- workspace-wide so external libs get traced too; json lands next to each .o
	filter "options:time-trace"
		buildoptions { "-ftime-trace" }
	filter {}

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

		require("Lumos/External/ModelLoaders/meshoptimizer/premake5")
			SetRecommendedSettings()
		require("Lumos/External/ozz-animation/premake5")
			SetRecommendedSettings()
		require("Lumos/External/msdf-atlas-gen/premake5")
			SetRecommendedSettings()
		if not os.istarget(premake.IOS) and not os.istarget(premake.ANDROID) then
			require("Lumos/External/GLFWpremake5")
			SetRecommendedSettings()
		end
		if _OPTIONS["shaderc"] then
			require("Lumos/External/shadercpremake5")
			SetRecommendedSettings()
		end

	filter {}
	group ""

	include "Lumos/premake5"
	   SetRecommendedSettings()
	include "Runtime/premake5"
		   SetRecommendedSettings()
		   filter {"system:macosx or system:ios"}
				xcodebuildsettings { ['SKIP_INSTALL'] = 'NO', ['INSTALL_PATH'] = '$(LOCAL_APPS_DIR)' }
		   filter {}
	include "Editor/premake5"
		   SetRecommendedSettings()
		   filter {"system:macosx or system:ios"}
				xcodebuildsettings { ['SKIP_INSTALL'] = 'NO', ['INSTALL_PATH'] = '$(LOCAL_APPS_DIR)' }
		   filter {}
