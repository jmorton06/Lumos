--
-- Name:        _preload.lua
-- Purpose:     Define the cmake action.
-- Author:      Ryan Pusztai
-- Modified by: Andrea Zanellato
--              Andrew Gough
--              Manu Evans
--              Yehonatan Ballas
-- Created:     2013/05/06
-- Copyright:   (c) 2008-2020 Jason Perkins and the Premake project
--

local p = premake

newaction
{
	-- Metadata for the command line and help system

	trigger         = "vscode",
	shortname       = "VSCode",
	description     = "Generate Visual Studio Code workspace",
	toolset         = "clang",

	-- The capabilities of this action

	valid_kinds     = { "ConsoleApp", "WindowedApp", "Makefile", "SharedLib", "StaticLib", "Utility" },
	valid_languages = { "C", "C++" },
	valid_tools     = {
		cc = { "gcc", "clang", "msc" }
	},

	-- Workspace and project generation logic

	onWorkspace = function(wks)
		p.modules.vscode.generateWorkspace(wks)
	end,
	onProject = function(prj)
		p.modules.vscode.generateProject(prj)
	end,

	onCleanWorkspace = function(wks)
		p.modules.vscode.cleanWorkspace(wks)
	end,
	onCleanProject = function(prj)
		p.modules.vscode.cleanProject(prj)
	end,
}


--
-- Decide when the full module should be loaded.
--

return function(cfg)
	return (_ACTION == "vscode")
end
